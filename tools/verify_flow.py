"""Decode control flow from reviewed entry candidates, validating recorded branch edges."""
import hashlib
import json
from pathlib import Path
import sys
import struct

sys.path.insert(0, str(Path(__file__).resolve().parent / 'python-deps'))
import capstone as cs
from capstone.x86 import X86_OP_IMM
import pefile

ROOT = Path(__file__).resolve().parents[1]
ENTRIES = {
    0x4328b1: 'frame_dispatch_candidate',
    0x432a56: 'view_setup_candidate',
    0x44bfe9: 'advance_game_candidate',
    0x44bda2: 'single_tick_candidate',
    0x44c631: 'command_source_dispatch_candidate',
    0x45f7d7: 'update_replay_candidate',
    0x445da0: 'consume_commands_candidate',
    0x44c2f9: 'out_of_sync_reporter_candidate',
    0x408b6a: 'command_header_conversion_candidate',
    0x408b16: 'queue_commit_candidate',
    0x4b90e8: 'render_entry_candidate',
    0x46bf56: 'set_primary_seed_candidate',
    0x46bf6a: 'primary_random16_candidate',
    0x46bf86: 'secondary_random16_candidate',
    0x446764: 'movement_builder_candidate',
    0x43b6b4: 'actor_slots_to_stable_ids_candidate',
    0x408af4: 'pending_command_enqueue_candidate',
    0x408d71: 'command_ring_write_candidate',
    0x408b82: 'append_fixed_payload_candidate',
    0x408bba: 'append_counted_payload_candidate',
    0x4670c2: 'selection_order_dispatch_candidate',
    0x408dd0: 'command_ring_read_candidate',
    0x4c7aa0: 'scene_collection_candidate',
    0x4c7ca0: 'render_list_initialize_candidate',
    0x4c7cb0: 'render_list_append_candidate',
    0x4c7820: 'render_list_finalize_candidate',
    0x44e229: 'view_parameters_candidate',
    0x43524e: 'visible_cell_effect_update_candidate',
    0x452e95: 'registered_scene_callback_candidate',
    0x434d7a: 'mesh_header_read_candidate',
    0x434834: 'mesh_body_read_candidate',
    0x434903: 'mesh_load_candidate',
    0x43451e: 'marker_convert_candidate',
    0x434574: 'palette_convert_candidate',
    0x49cce3: 'map_action_load_candidate',
    0x434461: 'marker_instantiate_dispatch_candidate',
    0x43447c: 'map_markers_initialize_candidate',
    0x439d80: 'unit_palette_initialize_candidate',
    0x43a5c0: 'unit_marker_instantiate_candidate',
    0x434332: 'marker_iterator_initialize_candidate',
    0x43435f: 'marker_iterator_next_candidate',
    0x4343d2: 'palette_iterator_initialize_candidate',
    0x4343f9: 'palette_iterator_next_candidate',
    0x44b6cf: 'game_start_candidate',
}
EXPECTED_EDGES = [
    (0x4328fa, 'call', 0x44bfe9), (0x4329b0, 'call', 0x432a56),
    (0x432ac5, 'call', 0x4b90e8), (0x44c1e3, 'call', 0x44c631),
    (0x44c23b, 'call', 0x44bda2), (0x44c693, 'jmp', 0x45f7d7),
    (0x44bdf3, 'call', 0x445da0), (0x45f922, 'call', 0x408b6a),
    (0x45f94e, 'call', 0x408b16), (0x445dca, 'call', 0x44c2f9),
    (0x4467a3, 'call', 0x43b6b4), (0x4467ea, 'call', 0x408b82),
    (0x446800, 'call', 0x408bba), (0x446811, 'call', 0x408bba),
    (0x446828, 'call', 0x447d72), (0x446836, 'call', 0x408af4),
    (0x408afb, 'call', 0x408d71),
    (0x4671b2, 'call', 0x446764), (0x467273, 'call', 0x446764),
    (0x432a85, 'call', 0x44e229), (0x4b9118, 'call', 0x4c7ca0),
    (0x4b9127, 'call', 0x4c7aa0), (0x4b9135, 'call', 0x4c7820),
    (0x44e50c, 'call', 0x46bf86), (0x44e532, 'call', 0x46bf86),
    (0x44e558, 'call', 0x46bf86),
    (0x4c7bc8, 'call', 0x43524e),
]


def main():
    path = ROOT / 'research/extracted/update-1.8.1/Myth II.exe'
    data = path.read_bytes()
    pe = pefile.PE(data=data)
    base = pe.OPTIONAL_HEADER.ImageBase
    md = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    md.detail = True
    # Reviewed guard at 0x445e11: sign-extend verb, subtract 1, unsigned compare 20.
    # Only this bounded absolute jump table is resolved; no generic table guessing.
    command_table = struct.unpack('<21I', pe.get_data(0x445f16-base, 21*4))

    def decode(va):
        rva = va - base
        section = pe.get_section_by_rva(rva)
        if section is None or not section.Characteristics & 0x20000000:
            raise ValueError(f'Not executable: {va:x}')
        off = pe.get_offset_from_rva(rva)
        ins = next(md.disasm(data[off:off+15], va, count=1), None)
        if ins is None:
            raise ValueError(f'Cannot decode: {va:x}')
        return ins

    all_nodes = set()
    results = []
    for entry, label in ENTRIES.items():
        pending = [entry]
        visited = {}
        edges = []
        unresolved = []
        while pending:
            va = pending.pop()
            if va in visited:
                continue
            if va != entry and va in ENTRIES:
                edges.append(dict(site=va, kind='entry_boundary', target=va))
                continue
            if abs(va - entry) > 0x10000 or len(visited) > 20000:
                raise ValueError('Traversal budget exceeded')
            ins = decode(va)
            visited[va] = dict(va=va, bytes=ins.bytes.hex(), text=f'{ins.mnemonic} {ins.op_str}')
            following = va + ins.size
            if ins.group(cs.CS_GRP_RET) or ins.mnemonic in ('int3', 'ud2', 'hlt'):
                continue
            if ins.group(cs.CS_GRP_CALL) or ins.group(cs.CS_GRP_JUMP):
                target = ins.operands[0].imm & 0xffffffff if ins.operands[0].type == X86_OP_IMM else None
                edges.append(dict(site=va, kind=ins.mnemonic, target=target, operand=ins.op_str))
                if target is None:
                    if va == 0x445e1f and ins.mnemonic == 'jmp':
                        for verb, dest in enumerate(command_table, start=1):
                            edges.append(dict(site=va, kind='reviewed_switch', target=dest, verb=verb))
                            pending.append(dest)
                    else:
                        unresolved.append(va)
                elif ins.group(cs.CS_GRP_JUMP):
                    pending.append(target)
                if ins.mnemonic == 'jmp':
                    continue
            pending.append(following)
        # Conflicting instruction boundaries indicate a bad seed or unsupported control flow.
        occupied = set()
        for va, row in visited.items():
            span = set(range(va, va + len(bytes.fromhex(row['bytes']))))
            if occupied & span:
                raise ValueError(f'Overlapping instructions at {va:x}')
            occupied |= span
        all_nodes.update(visited)
        results.append(dict(entry=entry, label=label, instructions=list(sorted(visited.values(), key=lambda r:r['va'])),
                            edges=edges, unresolved_indirect_transfers=unresolved))
        print(f'{entry:08x} {label}: {len(visited)} reachable instructions, {len(unresolved)} unresolved indirect transfers')
    for va, mnemonic, target in EXPECTED_EDGES:
        ins = decode(va)
        assert ins.mnemonic == mnemonic and ins.operands[0].imm == target, hex(va)
        assert va in all_nodes, f'Edge not reached: {va:x}'
    evidence = dict(sha256=hashlib.sha256(data).hexdigest(), image_base=base,
                    command_switch_table_va=0x445f16, command_switch_targets=command_table,
                    labels_are_inferences=True, expected_edges=EXPECTED_EDGES, routines=results,
                    limitations='Static reachability only. Indirect jumps terminate traversal; indirect calls have unknown callees. Names and semantic roles are hypotheses, not recovered symbols.')
    out = ROOT / 'research/reports/disassembly-1.8.1/reviewed-flow.json'
    out.write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    print(f'Validated {len(EXPECTED_EDGES)} direct edges against binary bytes and entry-based traversal.')


if __name__ == '__main__':
    main()

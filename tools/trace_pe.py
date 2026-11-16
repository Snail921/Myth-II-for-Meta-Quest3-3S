"""Static x86 reference survey. Linear decoding is evidence, not a recovered call graph."""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent / 'python-deps'))
import capstone as cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
import pefile


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('binary', type=Path)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    raw = args.binary.read_bytes()
    pe = pefile.PE(data=raw)
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('This survey supports x86 PE32 only')
    base = pe.OPTIONAL_HEADER.ImageBase
    anchors = {}
    pattern = re.compile(r'(?i)(commands\.c|recording\.c|update_replay|read command data|read block header|render_scene\.c|render_maps\.c|render\.c|motion_interpolation\.c|map_actions\.c|myth\.c|random seed|Finished playback|Starting mesh)')
    for match in re.finditer(rb'[\x20-\x7e]{6,}', raw):
        text = match.group().decode('ascii')
        if pattern.search(text):
            anchors[base + pe.get_rva_from_offset(match.start())] = text
    md = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    md.detail = True
    md.skipdata = True
    instructions = []
    refs = defaultdict(list)
    calls = defaultdict(list)
    imports = {}
    for dll in getattr(pe, 'DIRECTORY_ENTRY_IMPORT', []):
        for imp in dll.imports:
            imports[imp.address] = dll.dll.decode() + '!' + (imp.name.decode() if imp.name else f'ordinal_{imp.ordinal}')
    for section in pe.sections:
        if not section.Characteristics & 0x20000000:
            continue
        for ins in md.disasm(section.get_data(), base + section.VirtualAddress):
            row = dict(va=ins.address, rva=ins.address-base, bytes=ins.bytes.hex(),
                       mnemonic=ins.mnemonic, operands=ins.op_str)
            targets = set()
            if ins.id:
                for op in ins.operands:
                    if op.type == X86_OP_IMM:
                        targets.add(op.imm & 0xffffffff)
                    elif op.type == X86_OP_MEM and not op.mem.base and not op.mem.index:
                        targets.add(op.mem.disp & 0xffffffff)
                if ins.mnemonic == 'call' and ins.operands[0].type == X86_OP_IMM:
                    dest = ins.operands[0].imm & 0xffffffff
                    row['direct_call_target'] = dest
                    calls[dest].append(ins.address)
                for target in targets:
                    if target in imports:
                        row['import_reference'] = imports[target]
                    if target in anchors:
                        refs[target].append(len(instructions))
            instructions.append(row)
    index = {row['va']: i for i, row in enumerate(instructions)}
    # Prologues are only candidates; optimization, thunks and embedded data defeat this heuristic.
    prologues = [row['va'] for row in instructions if row['bytes'] == '55' and
                 index.get(row['va'] + 1) is not None and
                 instructions[index[row['va'] + 1]]['bytes'] in ('8bec', '89e5')]
    results = []
    for va, text in anchors.items():
        found = []
        for i in refs[va]:
            row = instructions[i]
            prior = [p for p in prologues if p <= row['va']]
            prologue = prior[-1] if prior else None
            found.append(dict(instruction_va=row['va'], candidate_preceding_prologue=prologue,
                              candidate_prologue_callers=calls.get(prologue, []),
                              context=instructions[max(0, i-10):i+14]))
        results.append(dict(string_va=va, text=text, references=found))
    report = dict(binary=str(args.binary), sha256=hashlib.sha256(raw).hexdigest(),
                  capstone=cs.__version__, pefile=pefile.__version__, image_base=base,
                  entry_va=base+pe.OPTIONAL_HEADER.AddressOfEntryPoint,
                  decoded_rows=len(instructions), anchors=results,
                  limitations='Linear section decoding; references and preceding prologues are candidates. No function extent, reachability, indirect-call resolution or runtime behavior established.')
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out/'references.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    with (args.out/'disassembly.tsv').open('w', encoding='utf-8') as out:
        out.write('VA\tRVA\tbytes\tmnemonic\toperands\timport\n')
        for row in instructions:
            out.write(f"{row['va']:08x}\t{row['rva']:08x}\t{row['bytes']}\t{row['mnemonic']}\t{row['operands']}\t{row.get('import_reference', '')}\n")
    for anchor in results:
        print(f"{anchor['string_va']:08x}: {anchor['text']}")
        for ref in anchor['references']:
            p = ref['candidate_preceding_prologue']
            print(f"  ref {ref['instruction_va']:08x}; preceding prologue {hex(p) if p else None}; {len(ref['candidate_prologue_callers'])} callers to candidate")


if __name__ == '__main__':
    main()

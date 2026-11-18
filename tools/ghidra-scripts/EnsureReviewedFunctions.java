// Define only an explicitly machine-code-reviewed callback missed by auto-analysis.
// @category MythResearch
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class EnsureReviewedFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        // Pushed as callback at 0x452af1, registered by 0x4c7c70.
        // verify_flow.py reaches 359 instructions from this exact entry.
        Address entry = toAddr(0x452e95L);
        if (getFunctionAt(entry) != null) return;
        Function owner = getFunctionContaining(entry);
        if (owner != null) throw new IllegalStateException("Entry overlaps existing function " + owner.getEntryPoint());
        disassemble(entry);
        Function created = createFunction(entry, "candidate_scene_callback_00452e95");
        if (created == null) throw new IllegalStateException("Could not define reviewed callback entry");
        created.setComment("Candidate entry validated by Capstone traversal and callback registration reference. Name and types inferred; not original symbols.");
    }
}

// Map-loading string references and statically visible tick dependencies.
// @category MythResearch
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.Reference;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class ExportMapEvidence extends GhidraScript {
    public void run() throws Exception {
        Path output = Path.of(getScriptArgs()[0]);
        Files.createDirectories(output);
        long[] anchors = {0x51a050L,0x51a1b0L,0x51a1d8L,0x51a1e4L,0x51a208L,0x51a21cL,0x51a948L,0x51a964L};
        Set<Function> roots = new LinkedHashSet<>();
        StringBuilder refs = new StringBuilder("anchor\tsite\tfunction\n");
        for (long anchor : anchors) {
            for (Reference ref : getReferencesTo(toAddr(anchor))) {
                Function owner = getFunctionContaining(ref.getFromAddress());
                refs.append(Long.toHexString(anchor)).append('\t').append(ref.getFromAddress()).append('\t')
                    .append(owner == null ? "" : owner.getEntryPoint()).append('\n');
                if (owner != null) roots.add(owner);
            }
        }
        Files.writeString(output.resolve("map-roots.tsv"),refs.toString(),StandardCharsets.UTF_8);
        DecompInterface decompiler = new DecompInterface();
        try {
            if (!decompiler.openProgram(currentProgram)) throw new IllegalStateException(decompiler.getLastMessage());
            for (Function function : roots) {
                DecompileResults result = decompiler.decompileFunction(function,60,monitor);
                if (!result.decompileCompleted()) throw new IllegalStateException(result.getErrorMessage());
                Files.writeString(output.resolve(function.getEntryPoint()+".c"),
                    "/* Unverified decompiler hypothesis; not recovered original source. */\n"+result.getDecompiledFunction().getC(),StandardCharsets.UTF_8);
            }
        } finally { decompiler.dispose(); }
        Function tick = getFunctionAt(toAddr(0x44bda2L));
        if (tick == null) throw new IllegalStateException("Missing tick entry");
        Set<Function> visited = new LinkedHashSet<>();
        ArrayDeque<Function> pending = new ArrayDeque<>();
        pending.add(tick);
        StringBuilder graph = new StringBuilder("caller\tcallee\n");
        StringBuilder unknown = new StringBuilder("function\tsite\tinstruction\n");
        while (!pending.isEmpty()) {
            monitor.checkCancelled();
            Function fn = pending.removeFirst();
            if (!visited.add(fn)) continue;
            if (visited.size()>20000) throw new IllegalStateException("Graph bound exceeded");
            for (Function callee : fn.getCalledFunctions(monitor)) {
                graph.append(fn.getEntryPoint()).append('\t').append(callee.getEntryPoint()).append('\n');
                if (!callee.isExternal()) pending.addLast(callee);
            }
            InstructionIterator instructions = currentProgram.getListing().getInstructions(fn.getBody(),true);
            while (instructions.hasNext()) {
                Instruction ins = instructions.next();
                if (ins.getFlowType().isComputed() && (ins.getFlowType().isCall() || ins.getFlowType().isJump()))
                    unknown.append(fn.getEntryPoint()).append('\t').append(ins.getAddress()).append('\t').append(ins.toString()).append('\n');
            }
        }
        Files.writeString(output.resolve("tick-callgraph.tsv"),graph.toString(),StandardCharsets.UTF_8);
        Files.writeString(output.resolve("tick-indirect-transfers.tsv"),unknown.toString(),StandardCharsets.UTF_8);
        Files.writeString(output.resolve("scope.txt"),"Statically visible tick closure: "+visited.size()+
            " functions. Ghidra inferred boundaries; indirect dispatch, callback registration and tail calls may be incomplete. Not a verified complete dependency graph.\n",StandardCharsets.UTF_8);
    }
}

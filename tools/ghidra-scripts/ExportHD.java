// Export decompiler hypotheses for reviewed entry points; never execute the target.
// @category MythResearch
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class ExportHD extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) throw new IllegalArgumentException("output directory, comma-separated hex entry addresses required");
        Path output = Path.of(args[0]);
        Files.createDirectories(output);
        DecompInterface decompiler = new DecompInterface();
        try {
            if (!decompiler.openProgram(currentProgram)) throw new IllegalStateException(decompiler.getLastMessage());
            StringBuilder index = new StringBuilder("entry\tfunction\tstatus\n");
            String entries=args[1];
            if(entries.contains("-")){String[] range=entries.split("-");long lo=Long.parseLong(range[0],16),hi=Long.parseLong(range[1],16);StringBuilder list=new StringBuilder();for(Function f:currentProgram.getFunctionManager().getFunctions(toAddr(lo),true)){if(f.getEntryPoint().getOffset()>hi)break;list.append(String.format("%08x,",f.getEntryPoint().getOffset()));}entries=list.toString().replaceAll(",$", "");}
            for (String raw : entries.split(",")) {
                monitor.checkCancelled();
                Address address = toAddr(Long.parseUnsignedLong(raw, 16));
                Function function = getFunctionContaining(address);
                if (function == null) {
                    index.append(raw).append("\t\tno function at exact entry\n");
                    continue;
                }
                DecompileResults result = decompiler.decompileFunction(function, 60, monitor);
                String status = result.decompileCompleted() ? "decompiled" : result.getErrorMessage().replace('\n', ' ');
                index.append(raw).append('\t').append(function.getName()).append('\t').append(status).append('\n');
                if (result.decompileCompleted()) {
                    String header = "/* ANALYSIS HYPOTHESIS ONLY: not original source or a validated port.\n" +
                        " * Program: " + currentProgram.getName() + "; entry: " + address + "\n" +
                        " * Function bounds, calling conventions, types and names require machine-code review. */\n\n";
                    Files.writeString(output.resolve(raw + ".c"), header + result.getDecompiledFunction().getC(), StandardCharsets.UTF_8);
                }
            }
            Files.writeString(output.resolve("index.tsv"), index.toString(), StandardCharsets.UTF_8);
            StringBuilder references = new StringBuilder("target\tsite\ttype\tcontaining_function\n");
            String globals = "0066d6e0,0066d6e4,0066d6e8,0066d698,0066d6a4,0067f2f4,0067f2f8,0067563c,00675634,006613f8,00553fd8,00553fdc,005543ea,005543ec";
            for (String raw : (entries + "," + globals).split(",")) {
                Address address = toAddr(Long.parseUnsignedLong(raw, 16));
                for (Reference reference : getReferencesTo(address)) {
                    Function owner = getFunctionContaining(reference.getFromAddress());
                    references.append(raw).append('\t').append(reference.getFromAddress()).append('\t')
                        .append(reference.getReferenceType()).append('\t')
                        .append(owner == null ? "" : owner.getEntryPoint()).append('\n');
                }
            }
            Files.writeString(output.resolve("references.tsv"), references.toString(), StandardCharsets.UTF_8);
        } finally {
            decompiler.dispose();
        }
    }
}

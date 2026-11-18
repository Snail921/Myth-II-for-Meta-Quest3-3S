// @category MythResearch
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.Reference;
import java.nio.file.*;
import java.util.*;
public class ExportBridgeEvidence extends GhidraScript {
 public void run() throws Exception {
  Path out=Path.of(getScriptArgs()[0]);Files.createDirectories(out);
  Set<Function> funcs=new LinkedHashSet<>();
  StringBuilder refs=new StringBuilder("global\tsite\tfunction\n");
  for(long a:new long[]{0x52a9f4L,0x52824cL,0x528f9cL}) {
   for(Reference r:getReferencesTo(toAddr(a))) {
    Function f=getFunctionContaining(r.getFromAddress());
    refs.append(Long.toHexString(a)).append('\t').append(r.getFromAddress()).append('\t').append(f==null?"":f.getEntryPoint()).append('\n');
    if(f!=null && ((f.getEntryPoint().getOffset()>=0x44c000L && f.getEntryPoint().getOffset()<0x44e000L) || a==0x52824cL)) funcs.add(f);
   }
  }
  Files.writeString(out.resolve("references.tsv"),refs.toString());
  DecompInterface d=new DecompInterface();
  try {d.openProgram(currentProgram);for(Function f:funcs){DecompileResults r=d.decompileFunction(f,30,monitor);if(r.decompileCompleted())Files.writeString(out.resolve(f.getEntryPoint()+".c"),"/* Decompiled hypothesis; verify against original instructions. */\n"+r.getDecompiledFunction().getC());}}finally{d.dispose();}
 }
}

import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.nio.file.*;
public class ExportVisibility extends GhidraScript {
 public void run() throws Exception {DecompInterface d=new DecompInterface();d.openProgram(currentProgram);
  for(long a:new long[]{0x41b29a,0x427ee2,0x42811c,0x468708,0x43695a,0x4be170,0x44b0bb}){Function f=getFunctionContaining(toAddr(a));if(f==null){println("MISSING "+Long.toHexString(a));continue;}DecompileResults r=d.decompileFunction(f,30,monitor);if(r.decompileCompleted())Files.writeString(Path.of(getScriptArgs()[0],"visibility-"+f.getEntryPoint()+".c"),r.getDecompiledFunction().getC());println("VIS "+Long.toHexString(a)+" "+f.getEntryPoint());}d.dispose();
 }
}

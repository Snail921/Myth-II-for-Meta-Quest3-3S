import java.io.File;
import java.nio.file.*;
import brut.androlib.src.SmaliBuilder;
import com.android.tools.smali.dexlib2.Opcodes;
import com.android.tools.smali.dexlib2.writer.builder.DexBuilder;
import com.android.tools.smali.dexlib2.writer.io.FileDataStore;
public class AssembleRuntime {
 public static void main(String[] args)throws Exception{
  Path root=Paths.get(args[0]);SmaliBuilder assembler=new SmaliBuilder(root.toFile(),29);DexBuilder dex=new DexBuilder(new Opcodes(29,-1));
  try(java.util.stream.Stream<Path> paths=Files.walk(root)){
   paths.filter(p->p.toString().endsWith(".smali")).sorted().forEach(p->assembler.buildFile(root.relativize(p).toString().replace('\\','/'),dex));
  }
  FileDataStore output=new FileDataStore(new File(args[1]));try{dex.writeTo(output);}finally{output.raf.close();}
  System.out.println("Runtime DEX assembled");
 }
}

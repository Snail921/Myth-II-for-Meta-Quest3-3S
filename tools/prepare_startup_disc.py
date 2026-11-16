from pathlib import Path
import shutil

def prepare(root):
 out=root/'build/standalone-runtime'
 path=out/'smali/com/winlator/core/PreloaderDialog.smali'
 text=path.read_text()
 anchor='    invoke-virtual {v0}, Landroid/app/Dialog;->show()V'
 hook='    invoke-static {v0, p1}, Lorg/mythvr/standalone/StartupDisc;->prepare(Landroid/app/Dialog;I)V\n\n'
 if 'Lorg/mythvr/standalone/StartupDisc;->prepare' not in text:
  assert text.count(anchor)==1
  text=text.replace(anchor,hook+anchor)
  path.write_text(text)
 target=out/'assets/standalone/CD_Face.png';target.parent.mkdir(parents=True,exist_ok=True)
 shutil.copyfile(root/'Assets/CD_Face.png',target)

if __name__=='__main__':prepare(Path(__file__).resolve().parents[1])

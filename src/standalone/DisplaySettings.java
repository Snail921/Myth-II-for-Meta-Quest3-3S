package org.mythvr.standalone;
import android.app.Activity;
import android.content.SharedPreferences;
import android.view.View;
import android.widget.*;
import java.io.*;
import java.util.*;

final class DisplaySettings {
 static final int[] WIDTH={0,1680,1832,1920,2064},HEIGHT={0,1760,1920,2048,2208};
 static void add(Activity a,LinearLayout group){
  boolean en=UiLanguage.english(a);SharedPreferences prefs=a.getSharedPreferences("vr_graphics",0);
  TextView help=new TextView(a);help.setText(en?"Applied the next time VR starts. Resolution is per eye.":"次回VR開始時に反映。解像度は片目あたりです。");group.addView(help);
  String[] sizes={en?"Automatic (recommended)":"自動（推奨値）","1680 × 1760","1832 × 1920","1920 × 2048","2064 × 2208"};
  Spinner resolution=choice(a,group,en?"Render resolution":"描画解像度",sizes);
  resolution.setSelection(Math.max(0,Math.min(4,prefs.getInt("display_resolution",0))));
  ArrayList<Integer> rates=new ArrayList<>();
  // Capabilities come from this device's OpenXR session, never a model-name guess.
  try(Scanner in=new Scanner(new File(a.getFilesDir(),"display-rates"))){while(in.hasNextInt()){int r=in.nextInt();if((r==72||r==80||r==90||r==120)&&!rates.contains(r))rates.add(r);}}catch(IOException ignored){}
  if(rates.isEmpty())rates.addAll(Arrays.asList(72,80,90,120));Collections.sort(rates);
  String[] labels=new String[rates.size()];for(int i=0;i<labels.length;i++)labels[i]=rates.get(i)+" Hz";
  Spinner refresh=choice(a,group,en?"Refresh rate":"リフレッシュレート",labels);
  int selected=rates.indexOf(prefs.getInt("display_hz",72));refresh.setSelection(selected<0?0:selected);
  Switch msaa=new Switch(a);msaa.setText(en?"MSAA (2× antialiasing)":"MSAA（2倍アンチエイリアス）");msaa.setTextSize(20);msaa.setChecked(prefs.getBoolean("display_msaa",false));group.addView(msaa);
  TextView applied=new TextView(a);try{String actual=new String(java.nio.file.Files.readAllBytes(new File(a.getFilesDir(),"display-applied").toPath()),java.nio.charset.StandardCharsets.UTF_8).trim();applied.setText((en?"Last applied: ":"前回の適用値：")+actual);}catch(IOException ignored){applied.setText(en?"Supported refresh rates are checked when VR starts.":"対応リフレッシュレートはVR開始時に確認します。");}group.addView(applied);
  Runnable save=()->{
   int index=resolution.getSelectedItemPosition(),hz=rates.get(refresh.getSelectedItemPosition());
   android.util.AtomicFile file=new android.util.AtomicFile(new File(a.getFilesDir(),"display.settings"));FileOutputStream out=null;
   try{out=file.startWrite();String data=WIDTH[index]+" "+HEIGHT[index]+" "+hz+" "+(msaa.isChecked()?2:1)+"\n";out.write(data.getBytes(java.nio.charset.StandardCharsets.US_ASCII));file.finishWrite(out);prefs.edit().putInt("display_resolution",index).putInt("display_hz",hz).putBoolean("display_msaa",msaa.isChecked()).apply();}
   catch(IOException error){if(out!=null)file.failWrite(out);Toast.makeText(a,en?"Could not save display settings":"描画設定を保存できませんでした",Toast.LENGTH_LONG).show();android.util.Log.e("MythStandalone","Display settings",error);}
  };
  AdapterView.OnItemSelectedListener changed=new AdapterView.OnItemSelectedListener(){public void onNothingSelected(AdapterView<?> p){}public void onItemSelected(AdapterView<?> p,View v,int pos,long id){save.run();}};
  resolution.setOnItemSelectedListener(changed);refresh.setOnItemSelectedListener(changed);msaa.setOnCheckedChangeListener((v,b)->save.run());save.run();
 }
 private static Spinner choice(Activity a,LinearLayout parent,String name,String[] values){TextView label=new TextView(a);label.setText(name);label.setTextSize(20);parent.addView(label);Spinner spinner=new Spinner(a);spinner.setAdapter(new ArrayAdapter<String>(a,android.R.layout.simple_spinner_dropdown_item,values));spinner.setContentDescription(name);parent.addView(spinner);return spinner;}
}

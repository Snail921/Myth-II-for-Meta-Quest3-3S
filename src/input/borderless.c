#include "win_min.h"
typedef struct {I left,top,right,bottom;} Rect;
typedef struct {I x,y;} Point;
API I WIN GetWindowLongA(H,I);API I WIN SetWindowLongA(H,I,I);
API I WIN GetClientRect(H,Rect*);API I WIN ClientToScreen(H,Point*);
API I WIN SetWindowPos(H,H,I,I,I,I,U);API void WIN SetLastError(U);
API H WIN OpenProcess(U,I,U);API U WIN WaitForSingleObject(H,U);
static I make_borderless(H w){
 U style=(U)GetWindowLongA(w,-16),ex=(U)GetWindowLongA(w,-20);
 const U frame=0x00cf0000,edge=0x00020301; /* caption, resize, sysmenu/min/max; extended edges */
 if(style&0x40000000)return 0; /* never modify child windows */
 if(!(style&frame)&&!(ex&edge))return 0;
 Rect r;Point p={0,0};if(!GetClientRect(w,&r)||!ClientToScreen(w,&p)||r.right<=0||r.bottom<=0)return -1;
 SetLastError(0);if(!SetWindowLongA(w,-16,(I)(style&~frame))&&GetLastError())return -2;
 SetLastError(0);if(!SetWindowLongA(w,-20,(I)(ex&~edge))&&GetLastError()){SetWindowLongA(w,-16,style);return -3;}
 /* Retain client resolution and screen origin; do not steal focus or reorder windows. */
 if(!SetWindowPos(w,0,p.x,p.y,r.right-r.left,r.bottom-r.top,0x34)){
  SetWindowLongA(w,-16,style);SetWindowLongA(w,-20,ex);SetWindowPos(w,0,0,0,0,0,0x37);return -4;
 }
 return 1;
}
#ifndef BORDERLESS_TEST
static U targetPid,changed;static H process;static I status;
static I WIN visit(H w,I unused){
 (void)unused;char title[32];U pid=0;
 if(GetWindowTextA(w,title,32)!=4||title[0]!='M'||title[1]!='Y'||title[2]!='T'||title[3]!='H')return 1;
 GetWindowThreadProcessId(w,&pid);if(targetPid&&pid!=targetPid)return 1;
 if(!targetPid){targetPid=pid;process=OpenProcess(0x00100000,0,pid);}
 I result=make_borderless(w);if(result){status=result;if(result>0)changed++;Rect r={0};Point p={0,0};GetClientRect(w,&r);ClientToScreen(w,&p);
  U report[10]={0x31445242,targetPid,changed,(U)status,(U)GetWindowLongA(w,-16),(U)GetWindowLongA(w,-20),(U)r.right,(U)r.bottom,(U)p.x,(U)p.y};write_packet("C:\\mythvr\\borderless.bin",report,sizeof(report));}
 return 1;
}
void entry(void){
 for(U attempt=0;!targetPid&&attempt<120;attempt++){EnumWindows(visit,0);if(!targetPid)Sleep(250);}
 if(!targetPid||!process)ExitProcess(1);
 while(WaitForSingleObject(process,250)==258)EnumWindows(visit,0);
 CloseHandle(process);ExitProcess(0);
}
#endif

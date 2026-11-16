#include "../win_min.h"
#include "test_paths.h"
static H window;static U thread;
static I WIN find_game(H w,I param){(void)param;U pid=0;U t=GetWindowThreadProcessId(w,&pid);char title[128];I n=GetWindowTextA(w,title,128);if(pid==TEST185_PID&&n==4&&title[0]=='M'&&title[1]=='Y'&&title[2]=='T'&&title[3]=='H'){window=w;thread=t;return 0;}return 1;}
void entry(void){
 EnumWindows(find_game,0);U report[4]={0x35545349,thread,0,0};if(!thread){report[3]=1;write_packet(TEST185_INSTALLER,report,16);ExitProcess(1);}
 H dll=LoadLibraryA(TEST185_DLL);if(!dll){report[3]=GetLastError();write_packet(TEST185_INSTALLER,report,16);ExitProcess(2);}
 void*proc=GetProcAddress(dll,"Myth185TestHook");H hook=SetWindowsHookExA(4,proc,dll,thread);report[2]=(U)hook;report[3]=GetLastError();write_packet(TEST185_INSTALLER,report,16);if(!hook)ExitProcess(3);
 U last=0;U initial[8];if(read_packet(TEST185_COMMAND,initial,32))last=initial[1];
 /* Bounded test harness lifetime: at most five minutes. */
 for(U loops=0;loops<15000&&IsWindow(window);loops++){U cmd[8];if(read_packet(TEST185_COMMAND,cmd,32)&&cmd[0]==0x35444d43&&cmd[1]!=last){U result=0;if(SendMessageTimeoutA(window,0x843a,0x313835,0,2,1000,&result))last=cmd[1];}Sleep(20);}
 UnhookWindowsHookEx(hook);ExitProcess(0);
}

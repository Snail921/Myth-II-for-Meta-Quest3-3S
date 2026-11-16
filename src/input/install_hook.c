#include "win_min.h"
static H window;static U thread;
static I WIN find_game(H w,I param){(void)param;char title[128];I n=GetWindowTextA(w,title,128);if(n==4&&title[0]=='M'&&title[1]=='Y'&&title[2]=='T'&&title[3]=='H'){window=w;thread=GetWindowThreadProcessId(w,0);return 0;}return 1;}
void entry(void){
 EnumWindows(find_game,0);U report[4]={0x31545349,thread,0,0};if(!thread){report[3]=1;write_packet("C:\\mythvr\\installer.bin",report,16);ExitProcess(1);}
 H dll=LoadLibraryA("C:\\mythvr\\mythinput-v7.dll");if(!dll){report[3]=GetLastError();write_packet("C:\\mythvr\\installer.bin",report,16);ExitProcess(2);}
 void*proc=GetProcAddress(dll,"MythInputHook");H hook=SetWindowsHookExA(4,proc,dll,thread);report[2]=(U)hook;report[3]=GetLastError();write_packet("C:\\mythvr\\installer.bin",report,16);if(!hook)ExitProcess(3);
 U last=0;U initial[8];if(read_packet("C:\\mythvr\\command.bin",initial,32)&&initial[0]==0x31444d43)last=initial[1];while(IsWindow(window)){U cmd[8];if(read_packet("C:\\mythvr\\command.bin",cmd,32)&&cmd[0]==0x31444d43&&cmd[1]!=last){U result=0;if(SendMessageTimeoutA(window,0x8439,0x4d595448,0,2,1000,&result))last=cmd[1];}Sleep(20);}UnhookWindowsHookEx(hook);ExitProcess(0);
}

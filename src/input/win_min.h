#ifndef MYTH_WIN_MIN_H
#define MYTH_WIN_MIN_H
#define API __declspec(dllimport)
#define WIN __stdcall
typedef unsigned int U;typedef int I;typedef void* H;typedef unsigned short US;typedef unsigned char B;
API H WIN CreateFileA(const char*,U,U,H,U,U,H);API I WIN ReadFile(H,void*,U,U*,H);API I WIN WriteFile(H,const void*,U,U*,H);API I WIN CloseHandle(H);API void WIN Sleep(U);API void WIN ExitProcess(U);API U WIN GetLastError(void);API H WIN LoadLibraryA(const char*);API void* WIN GetProcAddress(H,const char*);
API I WIN EnumWindows(I(WIN*)(H,I),I);API I WIN GetWindowTextA(H,char*,I);API U WIN GetWindowThreadProcessId(H,U*);API H WIN SetWindowsHookExA(I,void*,H,U);API I WIN UnhookWindowsHookEx(H);API I WIN CallNextHookEx(H,I,U,I);API I WIN IsWindow(H);API I WIN SendMessageTimeoutA(H,U,U,I,U,U,U*);
API I WIN VirtualProtect(void*,U,U,U*);API I WIN FlushInstructionCache(H,const void*,U);
static int read_packet(const char*path,void*b,U size){H f=CreateFileA(path,0x80000000,7,0,3,0,0);if(f==(H)-1)return 0;U n=0;I ok=ReadFile(f,b,size,&n,0);CloseHandle(f);return ok&&n==size;}
static void write_packet(const char*path,const void*b,U size){H f=CreateFileA(path,0x40000000,7,0,2,0,0);if(f==(H)-1)return;U n;WriteFile(f,b,size,&n,0);CloseHandle(f);}
#endif

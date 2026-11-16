#define BORDERLESS_TEST
#include "../src/input/borderless.c"
API H WIN CreateWindowExA(U,const char*,const char*,U,I,I,I,I,H,H,H,H);
API I WIN DestroyWindow(H);
void entry(void){
 H w=CreateWindowExA(0x200,"STATIC","Borderless fixture",0x00cf0000,100,100,640,480,0,0,0,0);
 if(!w)ExitProcess(10);Rect before,after;Point p={0,0},q={0,0};GetClientRect(w,&before);ClientToScreen(w,&p);
 if(make_borderless(w)!=1)ExitProcess(11);GetClientRect(w,&after);ClientToScreen(w,&q);
 if(before.right!=after.right||before.bottom!=after.bottom||p.x!=q.x||p.y!=q.y)ExitProcess(12);
 if((GetWindowLongA(w,-16)&0x00cf0000)||(GetWindowLongA(w,-20)&0x00020301))ExitProcess(13);
 if(make_borderless(w)!=0)ExitProcess(14);
 SetWindowLongA(w,-16,GetWindowLongA(w,-16)|0x00cf0000);SetWindowPos(w,0,0,0,0,0,0x37);
 if(make_borderless(w)!=1)ExitProcess(15);
 DestroyWindow(w);ExitProcess(0);
}

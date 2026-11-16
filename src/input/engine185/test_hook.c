#include "../win_min.h"
#include "game_calls.h"
#include "test_paths.h"
struct Command185 {U magic,seq,op,id,mesh; I x,y; U reserved;};
struct Ack185 {U magic,seq,status,count,tick;};
struct CW185 {I lparam;U wparam,message;H hwnd;};
static U lastSeq;
extern I can_control185(I slot);
static U execute185(const struct Command185*c){
 /* Host verifies the complete executable SHA256; these are in-process guards. */
 if(*(U*)0x47b31c!=0xb8ec8b55u||*(US*)0x532170!=220||*(US*)0x532e54!=80)return 10;
 if(*(US*)0x6bee40!=3||*(U*)0x531618!=c->mesh)return 20;
 if(c->op!=3||c->reserved)return 16;
 US n=*(US*)0x67ddb0,ac=*(US*)0x532182,oc=*(US*)0x532e66;
 if(n!=1||ac>400||oc>8192||(*(US*)0x67ddb2&1))return 18;
 US slot=*(US*)0x67ddb4;
 B*actors=*(B**)0x532188;B*objects=*(B**)0x532e6c;
 if(!actors||!objects||slot>=ac)return 11;
 B*a=actors+slot*220;
 if(*(US*)a!=c->id||!can_control185(slot))return 13;
 I oi=*(short*)(a+12);if(oi<0||oi>=oc)return 11;
 I x=*(I*)(objects+oi*80+8),y=*(I*)(objects+oi*80+12);
 I dx=c->x-x,dy=c->y-y;
 if(dx < -2048||dx>2048||dy < -2048||dy>2048)return 16;
 B*mesh=(B*)c->mesh;
 if(c->x<0||c->y<0||c->x>=(*(US*)(mesh+8)*32-1)*512||c->y>=(*(US*)(mesh+10)*32-1)*512)return 16;
 Myth185Word options[4]={1,0,0,0};Myth185Coord point[2]={c->x,c->y};
 movement185(options,1,&slot,1,point);return 1;
}
__declspec(dllexport) I WIN Myth185TestHook(I code,U wp,I lp){
 if(code>=0){struct CW185*m=(struct CW185*)lp;
  if(m->message==0x843a&&m->wparam==0x313835){
   struct Command185 c;
   if(read_packet(TEST185_COMMAND,&c,sizeof(c))&&c.magic==0x35444d43&&c.seq&&c.seq!=lastSeq){
    lastSeq=c.seq;U status=execute185(&c);struct Ack185 a={0x354b4341,c.seq,status,*(US*)0x67ddb0,*(U*)0x6bee64};write_packet(TEST185_ACK,&a,sizeof(a));
   }
   return 0;
  }
 }
 return CallNextHookEx(0,code,wp,lp);
}
I WIN DllMain(H module,U reason,H reserved){(void)module;(void)reason;(void)reserved;return 1;}

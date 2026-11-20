
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
static unsigned u32(const void*p){unsigned v;memcpy(&v,p,4);return v;}
#include "../src/vr/chat_state.h"
static void put(unsigned char*p,unsigned v){memcpy(p,&v,4);}
int main(){
 unsigned char wire[ChatState::bytes]={};put(wire,1);put(wire+4,2);put(wire+16,0x31544843);put(wire+20,99);
 assert(ChatState::valid(wire));chat.accept(wire,1,10);chat.show(true);
 assert(chat.open&&chat.mode&&chat.canSend(2));
 for(int i=0;i<2;i++){auto*r=wire+32+i*240;put(r,i+1);strcpy((char*)r+16,"gen");strcpy((char*)r+48,"same");}
 put(wire+24,2);chat.accept(wire,2,11);assert(chat.messages.size()==2&&chat.open);
 chat.accept(wire,3,12);assert(chat.messages.size()==2);
 chat.edit('a');chat.edit('b');chat.edit(258);chat.edit('x');assert(chat.draft=="axb");
 chat.edit(257);assert(chat.draft=="ab");chat.edit(259);assert(chat.cursor==2);
 for(int i=0;i<300;i++)chat.edit('x');assert(chat.draft.size()==191);
 for(bool shift:{false,true}){auto keys=chatKeys(shift);bool enter=false,close=false;for(auto&k:keys){assert(k.x>=0&&k.x+k.w<=14&&k.y<5);if(k.code==260)enter=true;if(k.code==261)close=true;}assert(enter&&close);}
 put(wire,0);chat.accept(wire,4,13);assert(!chat.open&&!chat.canSend(4));
 put(wire,1);chat.accept(wire,5,1);assert(chat.messages.empty()); // seek resets history
 put(wire+24,3);auto*r=wire+32+2*240;put(r,3);strcpy((char*)r+48,"new");chat.accept(wire,6,2);assert(chat.messages.size()==1);
 put(wire+28,1);assert(!ChatState::valid(wire));
 puts("PASS chat state: first event, duplicate messages, seek, replay, editing, limit and keyboard layout");
}

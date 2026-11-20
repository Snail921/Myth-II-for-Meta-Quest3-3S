#pragma once
#include <deque>
#include <string>
#include <vector>
#include <algorithm>
struct ChatState {
 static constexpr size_t bytes=3872;
 struct Message {std::string name,text;unsigned char color[3];double began;};
 std::deque<Message> messages;
 unsigned mesh=0,serial=0,lastTick=0,ownRow=0;bool baseline=false,live=false,open=false,shift=false;
 double updated=0;std::string draft;size_t cursor=0;
 static bool valid(const unsigned char*p){
  if(u32(p)>1||u32(p+4)>64||u32(p+8)||u32(p+12))return false;
  if(!u32(p+16))return true;
  if(u32(p+16)!=0x31544843||(u32(p+28)&1))return false;
  for(unsigned i=0;i<16;i++){auto r=p+32+i*240;if(u32(r+8)>1||r[47]||r[239])return false;}
  return true;
 }
 void accept(const unsigned char*p,double now,unsigned tick){
  if(!p){live=false;open=false;baseline=false;messages.clear();return;}
  live=u32(p)!=0;ownRow=u32(p+4);updated=now;if(!live)open=false;
  if(!u32(p+16))return;
  unsigned nextMesh=u32(p+20),next=u32(p+24);
  if(!baseline||mesh!=nextMesh||tick<lastTick||next<serial){mesh=nextMesh;serial=next;baseline=true;messages.clear();open=false;}
  else {
   unsigned first=serial+1;if(next-serial>16)first=next-15;
   for(unsigned seq=first;seq<=next&&seq;seq++){
    const auto*r=p+32+(seq-1)%16*240;
    if(u32(r)!=seq)continue;
    Message m;m.name=(const char*)r+16;m.text=(const char*)r+48;
    for(int c=0;c<3;c++)m.color[c]=r[12+c];
    m.began=now;messages.push_back(m);if(messages.size()>16)messages.pop_front();
   }
   serial=next;
  }
  lastTick=tick;
 }
 bool canSend(double now)const{return live&&now-updated<3;}
 void show(bool team){open=true;mode=team;draft.clear();cursor=0;shift=false;}
 bool mode=false;
 void edit(int code){
  if(code==256){shift=!shift;return;}
  if(code==257){if(cursor){draft.erase(--cursor,1);}return;}
  if(code==258){if(cursor)--cursor;return;}
  if(code==259){if(cursor<draft.size())++cursor;return;}
  if(code>=32&&code<=126&&draft.size()<191){draft.insert(cursor++,1,char(code));}
 }
};
static ChatState chat;
struct ChatKey {float x,y,w;std::string label;int code;};
static std::vector<ChatKey> chatKeys(bool shifted){
 std::vector<ChatKey> keys;
 auto row=[&](int y,const char*normal,const char*upper){for(int i=0;normal[i];i++){char c=shifted?upper[i]:normal[i];keys.push_back({float(i),float(y),1,std::string(1,c),c});}};
 row(0,"1234567890-=","!@#$%^&*()_+");keys.push_back({12,0,2,"Bksp",257});
 row(1,"qwertyuiop[]\\","QWERTYUIOP{}|");
 row(2,"asdfghjkl;'","ASDFGHJKL:\"");keys.push_back({11,2,3,"Enter",260});
 keys.push_back({0,3,2,"Shift",256});
 const char*a=shifted?"ZXCVBNM<>?":"zxcvbnm,./";for(int i=0;a[i];i++)keys.push_back({float(i+2),3,1,std::string(1,a[i]),a[i]});
 keys.push_back({0,4,1,shifted?"~":"`",shifted?'~':'`'});
 keys.push_back({1,4,7,"Space",32});keys.push_back({8,4,2,"<",258});keys.push_back({10,4,2,">",259});keys.push_back({12,4,2,"Close",261});
 return keys;
}

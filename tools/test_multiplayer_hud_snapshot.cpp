#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 std::vector<unsigned char>b(64,0);memcpy(b.data(),"MYTHBR01",8);
 auto set=[&](size_t at,unsigned v){memcpy(b.data()+at,&v,4);};
 auto word=[&](unsigned v){size_t at=b.size();b.resize(at+4);set(at,v);};
 set(8,23);set(12,7);set(16,7);set(32,216);set(36,80);
 b.resize(b.size()+272);word(0);word(0xffffffff);word(0);word(0);word(0);word(4);word(0);word(0);word(0);
 b.resize(b.size()+1828+40+44);word(0);word(0);word(0);word(5);
 size_t at=b.size();b.resize(at+MultiplayerHud::bytes);set(at,3);set(at+4,1);set(at+8,9);strcpy((char*)b.data()+at+16,"King of the Hill");strcpy((char*)b.data()+at+208,"3:00");strcpy((char*)b.data()+at+272,"Disconnected");strcpy((char*)b.data()+at+404,"Player");
 diagnosticEnabled=false;assert(snapshot(b.data(),b.size()));assert(multiplayerHud.scores(seconds())&&multiplayerHud.notice(seconds()));assert(!multiplayerHud.scores(seconds()+4));
 MultiplayerHud animated;animated.accept(b.data()+at,10.);
 assert(animated.disconnected(10.)&&animated.noticeAlpha(10.)==0);
 assert(fabs(animated.noticeAlpha(10.5)-.25f)<1e-6);
 animated.accept(b.data()+at,10.5);assert(fabs(animated.noticeAlpha(11.5)-.75f)<1e-6);
 assert(animated.noticeAlpha(12.)==0); // 2-second rising sawtooth, not reset by snapshots.
 assert(!animated.disconnected(14.));
 animated.accept(b.data()+at,14.);assert(animated.noticeAlpha(14.)==0);
 assert(!snapshot(b.data(),b.size()-1));set(at+4,65);assert(!snapshot(b.data(),b.size()));set(at+4,1);b[at+467]=1;assert(!snapshot(b.data(),b.size()));b[at+467]=0;
 size_t penAt=b.size();b.resize(penAt+MapPen::bytes);set(8,24);set(penAt,1);set(penAt+4,2);set(penAt+8,1);
 float range=128;memcpy(b.data()+penAt+16,&range,4);memcpy(b.data()+penAt+20,&range,4);
 assert(snapshot(b.data(),b.size())&&mapPen.available(seconds()));assert(!snapshot(b.data(),b.size()-1));
 set(penAt+4,201);assert(!snapshot(b.data(),b.size()));set(penAt+4,2);set(penAt+16,0x7fc00000);assert(!snapshot(b.data(),b.size()));
 puts("PASS schema24 map annotations, counts, truncation and NaN rejection");
 b.resize(at);set(8,22);assert(snapshot(b.data(),b.size()));assert(!multiplayerHud.scores(seconds())&&!multiplayerHud.notice(seconds()));
 puts("PASS production schema 23: HUD decode, stale expiry, truncation/count/string rejection, schema 22 compatibility and reset");
}

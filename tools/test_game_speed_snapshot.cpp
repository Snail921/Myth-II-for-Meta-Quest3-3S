#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 std::vector<unsigned char>b(64,0);memcpy(b.data(),"MYTHBR01",8);
 auto set=[&](size_t at,unsigned v){memcpy(b.data()+at,&v,4);};
 auto word=[&](unsigned v){size_t at=b.size();b.resize(at+4);set(at,v);};
 set(8,21);set(12,7);set(16,7);set(32,216);set(36,80);
 b.resize(b.size()+272);word(0);word(0xffffffff);word(0);word(0);word(0);word(4);word(0);word(0);word(0);
 b.resize(b.size()+1828+40+44);word(0);word(0);word(0);word(5);
 diagnosticEnabled=false;assert(snapshot(b.data(),b.size()));assert(gameSpeed.value==5&&gameSpeed.until==0);
 set(b.size()-4,0);assert(snapshot(b.data(),b.size()));assert(gameSpeed.value==0&&gameSpeed.until>seconds());
 assert(!snapshot(b.data(),b.size()-1));set(b.size()-4,12);assert(!snapshot(b.data(),b.size()));
 b.resize(b.size()-4);set(8,20);assert(snapshot(b.data(),b.size()));
 puts("PASS production snapshot schema 21, stopped tick, truncated/invalid speed rejection, schema 20 compatibility");
}

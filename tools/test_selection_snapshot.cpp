#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 FILE*f=fopen("/data/local/tmp/npc-schema11.bin","rb");assert(f);fseek(f,0,SEEK_END);size_t n=ftell(f);rewind(f);std::vector<unsigned char>b(n);assert(fread(b.data(),1,n,f)==n);fclose(f);
 assert(snapshot(b.data(),n));assert(!snapshot(b.data(),n-1));b.back()=2;assert(!snapshot(b.data(),n));b.back()=1;assert(snapshot(b.data(),n));
 puts("PASS schema11 selection trailer, truncation and malformed eligibility rejection");
}

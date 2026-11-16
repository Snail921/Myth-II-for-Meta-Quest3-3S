#include "../src/vr/hd_codec.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
int main(int argc,char**argv){if(argc!=3)return 2;std::ifstream f(argv[1],std::ios::binary);std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});FILE*out=fopen(argv[2],"wb");try{
 if(b.size()>=8&&!memcmp(b.data(),"HUECASE1",8)){for(size_t i=8;i+12<=b.size();i+=12){uint32_t v;memcpy(&v,b.data()+i,4);v=hd::hueRGB(v,b.data()+i+4);fwrite(&v,4,1,out);}}
 else if(b.size()>72&&!memcmp(b.data(),"HUEIMG01",8)){std::array<std::array<uint8_t,8>,8> controls{};memcpy(controls.data(),b.data()+8,64);std::vector<uint8_t> raw(b.begin()+72,b.end());auto rgba=hd::rgba(hd::decode(raw),controls);fwrite(rgba.data(),1,rgba.size(),out);}
 else {auto im=hd::decode(b);std::array<std::array<uint8_t,8>,8> controls{};auto rgba=hd::rgba(im,controls);fwrite(rgba.data(),1,rgba.size(),out);}
 fclose(out);return 0;}catch(std::exception&e){fprintf(stderr,"%s\n",e.what());fclose(out);return 1;}}

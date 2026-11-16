#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
using LiveParticle=std::array<uint32_t,16>;
static bool parseParticles(const uint8_t*b,size_t n,std::vector<LiveParticle>&out){
 if(n<4)return false;uint32_t count;memcpy(&count,b,4);if(count>2048||n!=4+size_t(count)*64)return false;std::vector<LiveParticle> next(count);
 for(unsigned i=0;i<count;i++){auto&v=next[i];memcpy(v.data(),b+4+i*64,64);if(v[2]>255||v[3]>255||!v[8]||v[8]>32767||v[13]>=255||v[15]>4)return false;if(v[15]==4){if(v[7]>65535||v[9]>32767||v[14]>=128)return false;}else if(v[15]==3){if(v[7]>65535||!v[9]||v[9]>32767||v[10]<3||v[10]>128||v[11]||v[12]||v[14]>=128)return false;}else if(v[15]==2){if(!v[9]||v[9]>512||(v[7]&65535)>=(v[7]>>16))return false;}else if(v[7]>65535||!v[9]||v[9]>32767||v[10]>3||v[12]>65535||v[14]>65535)return false;}
 out.swap(next);return true;
}

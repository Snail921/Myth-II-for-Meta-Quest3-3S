#include "../src/vr/ui_contrast.h"
#include <vector>
#include <cassert>
#include <sys/stat.h>
#include <unistd.h>
static bool test(unsigned r,unsigned g,unsigned b,unsigned percent,bool on=true){std::vector<unsigned char>ix(100),shade(100),lut(256*256*4);lut[0]=40;lut[1]=80;lut[2]=30;lut[3]=255;lut[4]=r;lut[5]=g;lut[6]=b;lut[7]=255;for(unsigned i=0;i<percent;i++)ix[i]=1;ui_contrast::enabled=on;ui_contrast::analyze(10,10,ix.data(),shade.data(),lut.data());assert(!on||ui_contrast::samples==100);return ui_contrast::active;}
int main(){assert(test(230,235,240,80));assert(test(210,205,170,60));assert(test(220,200,70,50));assert(!test(40,100,30,100));assert(!test(110,100,85,100));assert(!test(230,235,240,20));assert(!test(230,235,240,100,false));assert(ui_contrast::red()==1&&ui_contrast::green()==1&&ui_contrast::blue()==0);assert(test(240,240,240,30));assert(ui_contrast::green()==182/255.f);ui_contrast::color=0x123456;assert(ui_contrast::red()==18/255.f&&ui_contrast::green()==52/255.f&&ui_contrast::blue()==86/255.f);assert(!test(80,100,50,100));ui_contrast::load("/nonexistent-ui-test");assert(!ui_contrast::enabled&&!ui_contrast::active&&ui_contrast::color==0xF5B642);const char*dir="/data/local/tmp/myth-ui-color-test";mkdir(dir,0700);std::string path=std::string(dir)+"/auto-ui-contrast.color";
 for(const char*value:{"1193046","0","16777215","16777216","-1","garbage","1193046 extra"}){FILE*f=fopen(path.c_str(),"wb");assert(f);fputs(value,f);fclose(f);ui_contrast::load(dir);unsigned expected=std::string(value)=="1193046"?0x123456:std::string(value)=="0"?0:std::string(value)=="16777215"?0xFFFFFF:0xF5B642;assert(ui_contrast::color==expected);}
 remove(path.c_str());rmdir(dir);
 puts("PASS persisted color, black/white endpoints and invalid-file fallback");puts("PASS snow, pale terrain, yellow terrain, grass, dark terrain, small bright regions, OFF/default and level reset");}

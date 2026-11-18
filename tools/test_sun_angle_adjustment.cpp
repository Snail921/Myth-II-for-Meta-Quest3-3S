#include "../src/vr/sun_angle_adjustment.h"
#include <cassert>
int main(){
 SunAngleAdjustment a;float v[3]={.631582f,.398889f,-.664825f};a.init(v,"","levels/test");float out[3];a.direction(out);for(int i=0;i<3;i++)assert(std::fabs(out[i]-v[i])<.00001f);
 float start=a.azimuth;a.step(1,1,1,0);assert(a.azimuth==start&&!a.dirty);
 a.toggle(0);a.step(.1f,-.1f,.05f,.1);assert(a.azimuth==start&&!a.dirty);
 a.azimuth=359.9f;a.step(1,1,1,.2);assert(a.azimuth<1&&a.moving&&a.dirty);
 a.step(0,0,.05f,.3);assert(!a.moving&&a.lastLog==.3);
 for(int i=0;i<1000;i++)a.step(-1,-1,.05f,1);assert(a.elevation==2&&a.azimuth>=0&&a.azimuth<360);
 for(int i=0;i<1000;i++)a.step(0,1,.05f,1);assert(a.elevation==85);
 a.toggle(2);assert(!a.active&&!a.moving);puts("PASS sun angles: roundtrip, toggle, deadzone, dt clamp, wrap, limits, stop logging");
}

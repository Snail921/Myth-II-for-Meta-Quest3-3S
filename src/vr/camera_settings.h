#pragma once
#include <cstdio>
#include <algorithm>
struct CameraSpeeds {float move=1,stick=1,drag=1,mapSize=1,mapPitch=-30*.01745329252f,orbitDistance=15,radialSize=1,tapDistance=1;bool reuseFollowDistance=false,aimOnJump=false,tapGroups=true,selectionHaptics=true,buttonGroups=true;};
static CameraSpeeds cameraSpeeds;
static void loadCameraSpeeds(const char*root){
 cameraSpeeds={};char path[1024];snprintf(path,sizeof(path),"%s/camera.speeds",root);
 FILE*f=fopen(path,"rb");if(!f)return;
 int m=100,s=100,d=100,z=100,p=-30,o=15,r=100,reuse=0,aim=0,tap=1,haptic=1,tapDistance=100,buttons=1;int n=fscanf(f,"%d %d %d %d %d %d %d %d %d %d %d %d %d",&m,&s,&d,&z,&p,&o,&r,&reuse,&aim,&tap,&haptic,&tapDistance,&buttons);fclose(f);
 if(n>=3){cameraSpeeds.move=std::clamp(m,100,300)/100.f;cameraSpeeds.stick=std::clamp(s,100,300)/100.f;cameraSpeeds.drag=std::clamp(d,100,300)/100.f;}
 if(n>=4)cameraSpeeds.mapSize=std::clamp(z,50,200)/100.f;
 if(n>=13)cameraSpeeds.buttonGroups=buttons==1;
 if(n>=12)cameraSpeeds.tapDistance=std::clamp(tapDistance,10,100)/100.f;
 if(n>=10)cameraSpeeds.tapGroups=tap==1;
 if(n>=11)cameraSpeeds.selectionHaptics=haptic==1;
 if(n>=9)cameraSpeeds.aimOnJump=aim==1;
 if(n>=8)cameraSpeeds.reuseFollowDistance=reuse==1;
 if(n>=7)cameraSpeeds.radialSize=std::clamp(r,50,100)/100.f;
 if(n>=6)cameraSpeeds.orbitDistance=std::clamp(o,0,100);
 if(n>=5)cameraSpeeds.mapPitch=std::clamp(p,-75,15)*.01745329252f;
}

#pragma once
#include <cstdio>
#include <algorithm>
struct CameraSpeeds {bool swapHands=false,swapTriggers=false,swapGrips=false,swapAX=false,swapYB=false,swapSticks=false;bool chatFixed=true;float chatSmooth=.493f;float chatX=0,chatY=-.40f,chatDistance=1.75f;float move=1,stick=1,drag=1,mapSize=1,mapPitch=-30*.01745329252f,orbitDistance=15,radialSize=1,tapDistance=1;bool mapRotation=false,reuseFollowDistance=false,aimOnJump=false,tapGroups=true,selectionHaptics=true,buttonGroups=true;};
static CameraSpeeds cameraSpeeds;
static void loadCameraSpeeds(const char*root){
 cameraSpeeds={};char path[1024];
 const char* swapNames[]={"swap-hands","swap-triggers","swap-grips","swap-ax","swap-yb","swap-sticks"};
 bool* swapValues[]={&cameraSpeeds.swapHands,&cameraSpeeds.swapTriggers,&cameraSpeeds.swapGrips,&cameraSpeeds.swapAX,&cameraSpeeds.swapYB,&cameraSpeeds.swapSticks};
 for(int i=0;i<6;i++){snprintf(path,sizeof(path),"%s/%s.enabled",root,swapNames[i]);if(FILE*f=fopen(path,"rb")){int v=0;if(fscanf(f,"%d",&v)==1)*swapValues[i]=v==1;fclose(f);}}
 snprintf(path,sizeof(path),"%s/chat-keyboard.offset",root);
 if(FILE*f=fopen(path,"rb")){int x=0,y=-40,d=175,fixed=1,smooth=493;int fields=fscanf(f,"%d %d %d %d %d",&x,&y,&d,&fixed,&smooth);cameraSpeeds.chatFixed=fields<4||fixed==1;cameraSpeeds.chatSmooth=fields>=5?std::clamp(smooth,0,1500)/1000.f:.493f;if(fields>=3){cameraSpeeds.chatX=std::clamp(x,-60,60)/100.f;cameraSpeeds.chatY=std::clamp(y,-100,20)/100.f;cameraSpeeds.chatDistance=std::clamp(d,70,280)/100.f;}fclose(f);}
 snprintf(path,sizeof(path),"%s/minimap-rotation.enabled",root);
 if(FILE*option=fopen(path,"rb")){int value=0;if(fscanf(option,"%d",&value)==1)cameraSpeeds.mapRotation=value==1;fclose(option);}
 snprintf(path,sizeof(path),"%s/camera.speeds",root);
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

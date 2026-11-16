#include "../src/vr/main.cpp"
#include <cassert>
static void put(std::vector<unsigned char>&b,size_t o,unsigned v){memcpy(b.data()+o,&v,4);}
int main(){
 int pair[2];assert(!socketpair(AF_UNIX,SOCK_STREAM|SOCK_NONBLOCK,0,pair));listener=0;peer=pair[1];token.fill('a');objects.clear();liveSceneReady=false;authenticated=false;pending.clear();
 std::vector<unsigned char> packet(144);memcpy(packet.data(),"MYTHBR01",8);put(packet,8,1);put(packet,12,90);put(packet,16,91);put(packet,28,1);put(packet,32,216);put(packet,36,80);packet[64]=1;packet[67]=1;put(packet,72,512*17);put(packet,76,512*23);put(packet,80,512*2);
 assert(write(pair[0],token.data(),32)==32);pollIPC();assert(authenticated&&!liveSceneReady&&objects.empty());
 assert(write(pair[0],packet.data(),packet.size())==144);pollIPC();assert(!liveSceneReady&&objects.empty());
 put(packet,16,90);assert(write(pair[0],packet.data(),48)==48);pollIPC();assert(!liveSceneReady);
 assert(write(pair[0],packet.data()+48,96)==96);pollIPC();assert(liveSceneReady&&objects.size()==1&&tick==90);assert(objects[0].x==17&&objects[0].y==23);
 std::vector<unsigned char> actorPacket(360);memcpy(actorPacket.data(),packet.data(),64);put(actorPacket,24,1);actorPacket[64]=123;put(actorPacket,68,0x120034);actorPacket[84]=7;actorPacket[86]=2;actorPacket[87]=3;memcpy(actorPacket.data()+280,packet.data()+64,80);
 assert(snapshot(actorPacket.data(),actorPacket.size()));assert(objects[0].unitID==123&&objects[0].actorFlags==0x120034&&objects[0].family==7&&objects[0].team==2&&objects[0].owner==3);
 // Health comes from this snapshot, including damage and subsequent healing.
 for(unsigned hp:{256u,128u,192u,0u}){put(actorPacket,280+66,hp|(256u<<16));assert(snapshot(actorPacket.data(),actorPacket.size()));assert(objects[0].health==hp/256.f);}
 put(actorPacket,280+66,0);assert(snapshot(actorPacket.data(),actorPacket.size()));assert(objects[0].health==-1);
 puts("PASS live health snapshot: damage, healing, zero vitality and invalid maximum");
 // Real map descriptor reload and runtime collection remapping, including unknown slots.
 auto config=[&](const char*key,unsigned w,unsigned h,const char*name,unsigned canonical){std::vector<unsigned char>b(616);memcpy(b.data(),"MYTHLV01",8);memcpy(b.data()+8,key,8);put(b,16,w);put(b,20,h);float xyz[]={68,196,0,0};memcpy(b.data()+24,xyz,16);memcpy(b.data()+40,name,strlen(name));for(unsigned i=0;i<256;i++){unsigned short n=i==7?canonical:255;memcpy(b.data()+104+i*2,&n,2);}FILE*f=fopen("/data/local/tmp/active-level.bin","wb");assert(f);assert(fwrite(b.data(),1,b.size(),f)==b.size());fclose(f);};
 config("63723031",160,224,"Salvation",18);loadLevelConfig("/data/local/tmp");assert(levelWidth==160&&levelHeight==224&&levelName=="Salvation"&&mappedCollection(7)==18&&mappedCollection(0)==255);
 packet[64+42]=7;assert(snapshot(packet.data(),packet.size())&&objects.size()==1&&objects[0].col==18);
 packet[64+42]=6;assert(snapshot(packet.data(),packet.size())&&objects.empty());
 config("6c312020",224,192,"Willow Creek",23);loadLevelConfig("/data/local/tmp");assert(levelWidth==224&&levelHeight==192&&levelName=="Willow Creek"&&mappedCollection(7)==23);
 config("63723031",160,225,"Salvation",18);bool rejected=false;try{loadLevelConfig("/data/local/tmp");}catch(...){rejected=true;}assert(rejected&&levelName=="Willow Creek");
 config("6c653365",192,192,"Down a Broken Path",23);
 {FILE*f=fopen("/data/local/tmp/active-level.bin","ab");float initial[]={110,65,3,1.25f};assert(fwrite(initial,1,16,f)==16);fclose(f);}
 loadLevelConfig("/data/local/tmp");assert(levelInitialCamera&&levelCamera[0]==110&&levelCamera[1]==65&&levelCamera[3]==1.25f);
 {FILE*f=fopen("/data/local/tmp/active-level.bin","r+b");fseek(f,616,SEEK_SET);float invalid=999;fwrite(&invalid,4,1,f);fclose(f);}
 rejected=false;try{loadLevelConfig("/data/local/tmp");}catch(...){rejected=true;}assert(rejected);
 unlink("/data/local/tmp/active-level.bin");
 puts("PASS descriptor switch Salvation/Willow, canonical runtime binding, unknown exclusion, invalid dimensions rejected");
 close(pair[0]);close(pair[1]);puts("PASS startup gate: authentication, invalid and partial snapshots stay hidden; complete live snapshot activates scene");
}

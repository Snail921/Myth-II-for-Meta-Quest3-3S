// Build 434: script camera OBMO setup and legacy camera SNIF call sites.
static U cameraHeartbeat, cameraHeartbeatMap, cameraEventSerial;
static I cameraHooksInstalled;
static U cameraInputHeld;
static I script_camera_active(void){return (*(US*)0x544768&2)&&*(short*)0x657c28>0&&*(short*)0x657c2a<=*(short*)0x657c28;}
extern void script_camera_setup_hook(void);
extern void legacy_camera_snif_hook(void);
extern void legacy_camera_move_hook(void);
static I vr_camera_recent(void){return cameraHeartbeatMap==*(U*)0x529bd4 && (U)(*(U*)0x67ec24-cameraHeartbeat)<60;}
I allow_legacy_camera_move(void){return !(vr_camera_recent()&&(script_camera_active()||cameraInputHeld));}
I allow_legacy_camera_snif(void){
 U mesh=*(U*)0x529bd4;US lesson=*(US*)0x561872;
 return !(vr_camera_recent()&&mesh&&*(U*)mesh==0x3030746d&&lesson>=51&&lesson<=55);
}
void publish_script_camera(void){
 if(!vr_camera_recent())return;
 U mesh=*(U*)0x529bd4;if(!mesh)return;
 cameraInputHeld=1;
 U serial=++cameraEventSerial;
 U packet[9]={0x314d4143,serial,*(U*)mesh,*(U*)0x67ec24,*(U*)0x657c14,*(U*)0x657c18,*(US*)0x657c1e,mesh,serial};
 write_packet("C:\\mythvr\\camera.bin",packet,sizeof(packet));
}
static U install_camera_hooks(void){
 if(cameraHooksInstalled)return 1;
 const U sites[4]={0x4a03c7,0x4a042c,0x4321f1,0x4321f6};
 const U originals[4]={0x4a04e5,0x4a04e5,0x46a80a,0x450f61};
 const U replacements[4]={(U)script_camera_setup_hook,(U)script_camera_setup_hook,(U)legacy_camera_snif_hook,(U)legacy_camera_move_hook};
 for(U i=0;i<4;i++)if(*(B*)sites[i]!=0xe8||*(U*)(sites[i]+1)!=originals[i]-sites[i]-5)return 21;
 // Acquire all pages before mutation; no partially installed hook on protection failure.
 U old[4],ignored;for(U i=0;i<4;i++)if(!VirtualProtect((void*)sites[i],5,0x40,&old[i])){while(i){--i;VirtualProtect((void*)sites[i],5,old[i],&ignored);}return 22;}
 for(U i=0;i<4;i++){*(U*)(sites[i]+1)=replacements[i]-sites[i]-5;FlushInstructionCache((H)-1,(void*)sites[i],5);}
 for(U i=4;i;){--i;VirtualProtect((void*)sites[i],5,old[i],&ignored);}
 cameraHooksInstalled=1;return 1;
}

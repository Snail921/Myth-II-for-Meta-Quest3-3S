#define CHAT_BYTES (16+16+16*240)
static void collect_chat(unsigned char*out){
 memset(out,0,CHAT_BYTES);
#ifdef MYTH_COOP_471
 uint32_t mesh=word(MVR_MESH),live=((word(0x6bee46)&65535)==1)&&(int32_t)word(0x5332dc)>=0&&(int16_t)word(0x540a58)<0;
 memcpy(out,&live,4);memcpy(out+4,&chatOwnRow,4);
 uint32_t locator[4];FILE*f=fopen("files/rootfs/home/xuser-1/.wine/drive_c/mythvr/chat-address.bin","rb");
 if(!f)return;int ok=fread(locator,1,16,f)==16;fclose(f);
 if(!ok||locator[0]!=0x31544843||locator[2]!=3856||word(0x47df23)!=locator[3]-0x47df22-5)return;
 uint32_t before,after;if(!optional_read(locator[1]+12,&before,4))return;
 if(before&1||!optional_read(locator[1],out+16,3856)||!optional_read(locator[1]+12,&after,4)||after!=before||u32(out+28)!=before||
 u32(out+16)!=0x31544843||u32(out+20)!=mesh){memset(out+16,0,3856);return;}
#endif
}

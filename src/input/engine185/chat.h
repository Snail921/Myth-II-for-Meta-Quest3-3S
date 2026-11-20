// Build 471b only. UI thread; original accepted-message call and network queue.
struct ChatEvent {U seq,tick,mode;B color[4];char name[32],text[192];};
struct ChatEvents {U magic,mesh,serial;volatile U guard;struct ChatEvent rows[16];};
static struct ChatEvents chatEvents;
static I chatInstalled;
static char chatDraft[192];static U chatLength,chatExpected,chatMode,chatMesh;
static void chat_received(I player,const char*text,I mode){
 if(player>=0&&player<*(US*)0x533296&&mode>=0&&mode<=1){
  U mesh=*(U*)0x531618;
  __asm__ __volatile__("" ::: "memory");chatEvents.guard++;__asm__ __volatile__("" ::: "memory");
  if(chatEvents.mesh!=mesh){chatEvents.mesh=mesh;chatEvents.serial=0;}
  U seq=++chatEvents.serial;struct ChatEvent*r=&chatEvents.rows[(seq-1)%16];
  B*p=*(B**)0x53329c+player*104;
  r->seq=seq;r->tick=*(U*)0x6bee64;r->mode=mode;
  r->color[0]=p[89];r->color[1]=p[91];r->color[2]=p[93];r->color[3]=255;
  U i;for(i=0;i<31&&p[24+i];i++)r->name[i]=p[24+i];for(;i<32;i++)r->name[i]=0;
  for(i=0;i<191&&text[i];i++)r->text[i]=text[i];for(;i<192;i++)r->text[i]=0;
  __asm__ __volatile__("" ::: "memory");chatEvents.guard++;__asm__ __volatile__("" ::: "memory");
 }
 ((void(*)(I,const char*,I))0x435a84)(player,text,mode);
}
static U install_chat_hook(void){
 if(chatEvents.mesh!=*(U*)0x531618){
  __asm__ __volatile__("" ::: "memory");chatEvents.guard++;__asm__ __volatile__("" ::: "memory");chatEvents.mesh=*(U*)0x531618;chatEvents.serial=0;chatExpected=0;chatEvents.guard++;__asm__ __volatile__("" ::: "memory");
 }
 if(chatInstalled)return 1;
 U site=0x47df22;
 if(*(B*)site!=0xe8||*(U*)(site+1)!=0x435a84-site-5)return 21;
 U old,ignored;if(!VirtualProtect((void*)site,5,0x40,&old))return 22;
 chatEvents.magic=0x31544843;
 *(U*)(site+1)=(U)chat_received-site-5;FlushInstructionCache((H)-1,(void*)site,5);
 VirtualProtect((void*)site,5,old,&ignored);
 U locator[4]={0x31544843,(U)&chatEvents,sizeof(chatEvents),(U)chat_received};
 write_packet("C:\\mythvr\\chat-address.bin",locator,sizeof(locator));
 chatInstalled=1;return 1;
}
static U chat_command(const struct Command*c){
 I player=*(I*)0x5332dc;
 if(*(short*)0x6bee46!=1||player<0||player>=*(US*)0x533296||*(short*)0x540a58>=0){chatExpected=0;return 20;}
 if(c->op==26){
  chatExpected=chatLength=0;
  if(c->id>1||c->flags<1||c->flags>191||c->x||c->y||c->reserved)return 16;
  chatMode=c->id;chatExpected=c->flags;chatMesh=*(U*)0x531618;return 1;
 }
 if(!chatExpected||chatMesh!=*(U*)0x531618)return 19;
 if(c->op==27){
  const B*p=(const B*)&c->id;U n=chatExpected-chatLength;if(n>20)n=20;
  for(U i=0;i<n;i++)if(p[i]<32||p[i]>126){chatExpected=0;return 16;}
  for(U i=n;i<20;i++)if(p[i]){chatExpected=0;return 16;}
  for(U i=0;i<n;i++)chatDraft[chatLength++]=p[i];
  return 1;
 }
 if(c->op!=28||c->id||c->flags||c->x||c->y||c->reserved||chatLength!=chatExpected){chatExpected=0;return 16;}
 chatExpected=0;chatDraft[chatLength]=0;US flags=(US)chatMode;
 ((void(*)(const US*,const char*))0x47b94a)(&flags,chatDraft);
 return 1;
}

#define main capture_main
#include "../src/bridge/capture.c"
#undef main
#include <assert.h>
int main(void){
 mailbox="/data/local/tmp/coop-command-test.bin";temporary="/data/local/tmp/coop-command-test.tmp";
 int pair[2],original=dup(STDOUT_FILENO);assert(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0);assert(dup2(pair[1],STDOUT_FILENO)>=0);
 for(unsigned c=0;c<5;c++){
  unlink(mailbox);uint32_t packet[8]={0x31444d43,10+c,23,1,0x08898001,0,0,0xc8765432};
  if(c==1)packet[3]=32;if(c==2)packet[5]=1;if(c==3)packet[6]=1;if(c==4)packet[2]=24;
  assert(write(pair[0],packet,32)==32);receive_command();FILE*f=fopen(mailbox,"rb");
  if(c==0){assert(f);uint32_t actual[8];assert(fread(actual,1,32,f)==32&&!memcmp(actual,packet,32));fclose(f);}else assert(!f);
 }
 unlink(mailbox);unlink(temporary);dup2(original,STDOUT_FILENO);close(original);close(pair[0]);close(pair[1]);puts("PASS DETACH transport accepts identity/hash and rejects invalid target/coordinates/opcode");
}

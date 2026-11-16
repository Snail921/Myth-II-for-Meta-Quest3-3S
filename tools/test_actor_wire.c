#include <assert.h>
#include <stdio.h>
#include "../src/engine/actor_wire.h"
int main(void){
 unsigned char source[440],out[434];memset(source,0,sizeof(source));memset(out,0xa5,sizeof(out));
 for(unsigned i=0;i<2;i++){
  unsigned char*a=source+i*220;a[0]=11+i;a[2]=2+i;a[12]=62+i;a[20]=4+i;a[22]=1;a[23]=2;
  a[0x7a]=70+i;a[0x8c]=5-i;a[0x90]=9+i;a[0x94]=14+i;a[0xb0]=2+i;
  memset(a+0xc6,0xff,4);memcpy(a+0xca,i?"Second actor name!":"First actor name!!",18);
 }
 assert(myth_actor_wire(out+1,432,source,sizeof(source),2,471));
 assert(out[0]==0xa5&&out[433]==0xa5);
 for(unsigned i=0;i<2;i++){
  unsigned char*a=out+1+i*216;assert(a[0]==11+i&&a[12]==62+i&&a[22]==1&&a[23]==2);
  assert(a[0x7a]==70+i&&a[0x8c]==5-i&&a[0xb0]==2+i);
  assert(!memcmp(a+0xc6,source+i*220+0xca,18));assert(a[40]==0);
 }
 unsigned char before[434];memcpy(before,out,434);
 assert(!myth_actor_wire(out+1,431,source,440,2,471));assert(!memcmp(out,before,434));
 assert(!myth_actor_wire(out+1,432,source,439,2,471));assert(!memcmp(out,before,434));
 assert(!myth_actor_wire(out+1,432,source,440,2,999));assert(!memcmp(out,before,434));
 assert(myth_actor_wire(out+1,432,source,432,2,434));assert(!memcmp(out+1,source,432));
 assert(out[0]==0xa5&&out[433]==0xa5);
 puts("PASS: 471b actor field/name relocation, adjacent record boundaries, reject-before-write and unchanged 434 copy");return 0;
}

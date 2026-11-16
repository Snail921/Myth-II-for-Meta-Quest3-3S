#ifndef MYTH_ACTOR_WIRE_H
#define MYTH_ACTOR_WIRE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* MYTHBR01 contains 216-byte records. Only fields consumed by the renderer
 * and selection-info producer are normalized for 471b. This is NOT a complete
 * serialization of the newer engine's actor structure.
 * Verified: identity/definition/flags/object/family/team/owner, mana +7a,
 * inventory +8c, name index +90, appearance +94, original definition +b0.
 * 471b custom name is +ca (485aa1), versus 434 +c6.
 */
static int myth_actor_wire(void*destination,size_t capacity,const void*source,size_t bytes,unsigned count,unsigned build){
 size_t stride=build==434?216u:build==471?220u:0u;
 if(!stride||count>400||capacity<(size_t)count*216||bytes<(size_t)count*stride||(!destination&&count)||(!source&&count))return 0;
 unsigned char*out=(unsigned char*)destination;const unsigned char*in=(const unsigned char*)source;
 if(build==434){if(count)memcpy(out,in,(size_t)count*216);return 1;}
 for(unsigned i=0;i<count;i++,out+=216,in+=220){
  memset(out,0,216);memcpy(out,in,24);
  memcpy(out+0x7a,in+0x7a,2);out[0x8c]=in[0x8c];out[0x90]=in[0x90];
  memcpy(out+0x94,in+0x94,2);memcpy(out+0xb0,in+0xb0,2);
  memcpy(out+0xc6,in+0xca,18);
 }
 return 1;
}
#endif

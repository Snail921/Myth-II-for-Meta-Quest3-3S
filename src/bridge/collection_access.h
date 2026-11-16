/* Read-only collection access. Build 471b keeps tag-relative references;
 * Build 434 keeps resolved pointers. Output bitmap pixel pointer is normalized.
 * Requires optional_read(), u16(), u32() and memcpy() from the producer.
 */
#ifndef MVR_COLLECTION_RELATIVE
#define MVR_COLLECTION_RELATIVE 0
#endif
static int collection_header(uint32_t base,unsigned index,unsigned char*out,size_t bytes){
 if(index>=255||bytes>320||base<65536)return 0;
#if MVR_COLLECTION_RELATIVE
 unsigned char raw[320];if(!optional_read(base+index*320,raw,320))return 0;
 uint32_t blob=u32(raw+8),bitmap=u32(raw+100),sequence=u32(raw+132);
 if(blob<65536||bitmap>0x10000000u||sequence>0x10000000u||blob>UINT32_MAX-bitmap||blob>UINT32_MAX-sequence)return 0;
 bitmap+=blob;sequence+=blob;memcpy(raw+108,&bitmap,4);memcpy(raw+140,&sequence,4);
 memcpy(raw+192,raw+24,4);memcpy(raw+196,raw+16,4);
 memcpy(out,raw,bytes);return 1;
#else
 return optional_read(base+index*320,out,bytes);
#endif
}
static int collection_sequence(const unsigned char*head,unsigned index,unsigned char*out,uint32_t*address){
 unsigned char ref[80];if(index>=u32(head+128)||u32(head+128)>1024||!optional_read(u32(head+140)+index*128,ref,80))return 0;
#if MVR_COLLECTION_RELATIVE
 uint32_t base=u32(head+8),offset=u32(ref+64);if(offset>0x10000000u||base>UINT32_MAX-offset)return 0;*address=base+offset;
#else
 *address=u32(ref+72);
#endif
 return optional_read(*address,out,12);
}
static int collection_bitmap(const unsigned char*head,unsigned index,unsigned char*out){
 unsigned char ref[84];if(index>=u32(head+96)||u32(head+96)>20000||!optional_read(u32(head+108)+index*128,ref,sizeof(ref)))return 0;
#if MVR_COLLECTION_RELATIVE
 if(!optional_read(u32(ref+80),out,52))return 0;
 memcpy(out+48,out+44,4);
#else
 if(!optional_read(u32(ref+72),out,52))return 0;
#endif
 return 1;
}
static uint32_t collection_palette_offset(void){return MVR_COLLECTION_RELATIVE?156u:148u;}

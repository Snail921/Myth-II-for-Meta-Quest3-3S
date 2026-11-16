#include "mesh.h"

typedef struct { uint16_t offset; uint8_t width; } swap_span;
#include "mesh_layout.inc"

static uint32_t be32(const uint8_t *p)
{
    return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];
}
static uint32_t be16(const uint8_t *p)
{
    return (uint32_t)p[0]<<8 | p[1];
}
static void apply(uint8_t *p, const swap_span *plan, uint32_t count)
{
    for (uint32_t i=0; i<count; ++i) {
        uint8_t *field=p+plan[i].offset;
        uint32_t width=plan[i].width;
        for (uint32_t j=0; j<width/2; ++j) {
            uint8_t tmp=field[j];field[j]=field[width-1-j];field[width-1-j]=tmp;
        }
    }
}
#define APPLY(p,name) apply((p),name##_swaps,sizeof(name##_swaps)/sizeof(name##_swaps[0]))

int myth181_mesh_decode(const uint8_t *source, uint32_t size,
                       uint8_t *destination, uint32_t capacity)
{
    static const uint16_t offset_fields[7]={0xc,0x28,0x38,0x84,0xc0,0xcc,0x120};
    static const uint16_t size_fields[7]={0x10,0x2c,0x3c,0x88,0xc4,0xd0,0x124};
    uint32_t offsets[7], sizes[7];
    if (size<1024 || capacity<size || be32(source+0x18)!=1024)
        return 0;
    uint32_t body_size=be32(source+0x1c);
    if (body_size>size-1024)
        return 0;
    for (unsigned i=0; i<7; ++i) {
        offsets[i]=be32(source+offset_fields[i]);
        sizes[i]=be32(source+size_fields[i]);
        if (offsets[i]>body_size || sizes[i]>body_size-offsets[i])
            return 0;
        for (unsigned j=0; j<i; ++j)
            if (sizes[i] && sizes[j] && offsets[i]<offsets[j]+sizes[j] && offsets[j]<offsets[i]+sizes[i])
                return 0;
    }
    uint32_t width=be16(source+8)*32, height=be16(source+10)*32;
    uint32_t palette_count=be32(source+0x24), marker_count=be32(source+0x34);
    uint32_t connector_count=be32(source+0x11c), action_count=be32(source+0x80);
    if (!width || !height || width>32767 || height>32767 ||
        (uint64_t)width*height*12!=sizes[0] || (uint64_t)palette_count*32!=sizes[1] ||
        (uint64_t)marker_count*64!=sizes[2] || (uint64_t)connector_count*64!=sizes[6] ||
        (uint64_t)action_count*64>sizes[3] || (sizes[4]&1) || (sizes[5]&1))
        return 0;
    for (uint32_t i=0; i<marker_count; ++i)
        if (be16(source+1024+offsets[2]+i*64+4)>=12)
            return 0;
    uint32_t action_base=1024+offsets[3], parameter_begin=action_count*64;
    for (uint32_t i=0; i<action_count; ++i) {
        const uint8_t *entry=source+action_base+i*64;
        uint32_t relative=be32(entry+24), length=be16(entry+22);
        if (relative>sizes[3]-parameter_begin || length>sizes[3]-parameter_begin-relative)
            return 0;
    }
    uint32_t parameter_cursor=parameter_begin;
    while (parameter_cursor<sizes[3]) {
        if (sizes[3]-parameter_cursor<8)
            return 0;
        const uint8_t *entry=source+action_base+parameter_cursor;
        uint32_t kind=be16(entry), count=be16(entry+2);
        if (kind>=21)
            return 0;
        uint32_t bytes=(count*parameter_sizes[kind]+3)&~UINT32_C(3);
        if (bytes>sizes[3]-parameter_cursor-8)
            return 0;
        parameter_cursor+=8+bytes;
    }
    for (uint32_t i=0; i<size; ++i)
        destination[i]=source[i];
    APPLY(destination,header);
    for (uint32_t i=0; i<width*height; ++i)
        APPLY(destination+1024+offsets[0]+i*12,cell);
    for (uint32_t i=0; i<palette_count; ++i)
        APPLY(destination+1024+offsets[1]+i*32,palette);
    for (uint32_t i=0; i<marker_count; ++i) {
        uint32_t at=1024+offsets[2]+i*64;
        uint32_t kind=be16(source+at+4);
        APPLY(destination+at,marker);
        if (kind==5 || kind==6)
            APPLY(destination+at+36,marker_extra);
    }
    for (unsigned section=4; section<=5; ++section) {
        for (uint32_t i=0; i<sizes[section]; i+=2) {
            uint8_t *p=destination+1024+offsets[section]+i;
            uint8_t tmp=p[0];p[0]=p[1];p[1]=tmp;
        }
    }
    for (uint32_t i=0; i<connector_count; ++i)
        APPLY(destination+1024+offsets[6]+i*64,connector);
    for (uint32_t i=0; i<action_count; ++i)
        APPLY(destination+action_base+i*64,action);
    parameter_cursor=parameter_begin;
    while (parameter_cursor<sizes[3]) {
        const uint8_t *encoded=source+action_base+parameter_cursor;
        uint8_t *entry=destination+action_base+parameter_cursor;
        uint32_t kind=be16(encoded), count=be16(encoded+2);
        uint32_t bytes=count*parameter_sizes[kind], width=parameter_widths[kind];
        APPLY(entry,parameter);
        if (width) {
            for (uint32_t i=0; i<bytes; i+=width) {
                for (uint32_t j=0; j<width/2; ++j) {
                    uint8_t *value=entry+8+i;
                    uint8_t tmp=value[j];value[j]=value[width-1-j];value[width-1-j]=tmp;
                }
            }
        }
        parameter_cursor+=8+((bytes+3)&~UINT32_C(3));
    }
    return 1;
}

static int16_t native16(const uint8_t *p)
{
    return (int16_t)((uint16_t)p[0] | (uint16_t)p[1]<<8);
}
static uint32_t native32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
const uint8_t *myth181_marker_next(const uint8_t *markers, uint32_t count, myth181_marker_iterator *state)
{
    state->current_index=-1;
    if (count>32766 || state->next_index<0)
        return (const uint8_t *)0;
    while ((uint32_t)state->next_index<count) {
        int16_t index=state->next_index++;
        const uint8_t *entry=markers+(uint32_t)index*64;
        if ((state->type==-1 || state->type==native16(entry+4)) &&
            (state->palette==-1 || state->palette==native16(entry+6)) &&
            (state->datum==-1 || state->datum==native16(entry+60))) {
            state->current_index=index;
            return entry;
        }
    }
    return (const uint8_t *)0;
}
const uint8_t *myth181_palette_next(const uint8_t *palette, uint32_t count, myth181_palette_iterator *state)
{
    state->current_index=-1;
    if (count>32766 || state->next_index<0)
        return (const uint8_t *)0;
    while ((uint32_t)state->next_index<count) {
        int16_t index=state->next_index++;
        const uint8_t *entry=palette+(uint32_t)index*32;
        if (state->type==-1 || state->type==native16(entry)) {
            state->type_index++;
            if (state->tag==UINT32_MAX || state->tag==native32(entry+4)) {
                state->current_index=index;
                return entry;
            }
        }
    }
    return (const uint8_t *)0;
}

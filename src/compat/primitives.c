#include "primitives.h"

void myth181_seed_primary(uint32_t *state, uint32_t seed)
{
    *state = seed == 0 ? UINT32_C(0xfded) : seed;
}

uint16_t myth181_random16(uint32_t *state)
{
    *state = *state * UINT32_C(1664525) + UINT32_C(1013904223);
    return (uint16_t)(*state >> 16);
}

static void store16le(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

static void store32le(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
    out[2] = (uint8_t)(value >> 16);
    out[3] = (uint8_t)(value >> 24);
}

void myth181_pack_view_fields(const float values[9], uint8_t output[24])
{
    /* Binary64 holds the exact product of two binary32 operands here. Avoid an
     * intermediate binary32 rounding: original x87 multiplies before truncation.
     * Angular scale is the exact binary32 constant at 0x522f54, not recomputed pi.
     */
    for (unsigned i = 0; i < 3; ++i)
        store32le(output + 4 * i, (uint32_t)(int32_t)((double)values[i] * 512.0));
    for (unsigned i = 3; i < 9; ++i) {
        double scale = i < 7 ? 512.0 : 10430.3779296875;
        store16le(output + 12 + 2 * (i - 3), (uint16_t)(int64_t)((double)values[i] * scale));
    }
}

static uint16_t load16le(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | (uint16_t)bytes[1] << 8);
}

static uint32_t load32le(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
           (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

int myth181_ring_write(myth181_command_ring *ring, const uint8_t *command)
{
    int32_t tick = (int32_t)load32le(command + 4);
    int32_t size = (int16_t)load16le(command);
    if (tick > ring->max_tick)
        ring->max_tick = tick;
    if (ring->write_offset >= ring->read_offset &&
        ring->write_offset + size > ring->capacity) {
        ring->wrap_end = ring->write_offset;
        ring->write_offset = 0;
    }
    int32_t free_bytes = ring->read_offset > ring->write_offset
        ? ring->read_offset - ring->write_offset
        : ring->capacity - ring->write_offset + ring->read_offset;
    if (free_bytes <= size)
        return 0;
    for (int32_t i = 0; i < size; ++i)
        ring->bytes[ring->write_offset + i] = command[i];
    ring->write_offset += size;
    return 1;
}

const uint8_t *myth181_ring_read(myth181_command_ring *ring, int32_t max_tick)
{
    if (ring->read_offset > ring->write_offset && ring->read_offset >= ring->wrap_end)
        ring->read_offset = 0;
    if (ring->read_offset == ring->write_offset)
        return (const uint8_t *)0;
    const uint8_t *command = ring->bytes + ring->read_offset;
    if ((int32_t)load32le(command + 4) > max_tick)
        return (const uint8_t *)0;
    ring->read_offset += (int16_t)load16le(command);
    return command;
}

static void store16be(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value >> 8);
    out[1] = (uint8_t)value;
}

static void store32be(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)value;
}

uint32_t myth181_encode_movement(uint8_t *output, uint32_t capacity,
    const uint8_t options[8], const int16_t *stable_ids, uint32_t unit_count,
    const int32_t *point_xy, uint32_t point_count, int32_t gameplay_version)
{
    uint32_t limit = gameplay_version > 1300 ? 126 : 30;
    if (unit_count > limit)
        unit_count = limit;
    if (point_count > 4093)
        return 0;
    uint32_t size = 20 + 2 * unit_count + 8 * point_count;
    if (size > 32767 || size > capacity)
        return 0;
    store16le(output, (uint16_t)size);
    output[2] = 3;
    output[3] = 255;
    store32le(output + 4, 0);
    for (unsigned i = 0; i < 6; i += 2)
        store16be(output + 8 + i, load16le(options + i));
    output[14] = options[6];
    output[15] = options[7];
    store16be(output + 16, (uint16_t)unit_count);
    uint32_t offset = 18;
    for (uint32_t i = 0; i < unit_count; ++i, offset += 2)
        store16be(output + offset, (uint16_t)stable_ids[i]);
    store16be(output + offset, (uint16_t)point_count);
    offset += 2;
    for (uint32_t i = 0; i < point_count; ++i, offset += 8) {
        store32be(output + offset, (uint32_t)point_xy[2 * i]);
        store32be(output + offset + 4, (uint32_t)point_xy[2 * i + 1]);
    }
    return size;
}

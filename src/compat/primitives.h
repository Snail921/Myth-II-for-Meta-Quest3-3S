#ifndef MYTH181_COMPAT_PRIMITIVES_H
#define MYTH181_COMPAT_PRIMITIVES_H

#include <stdint.h>

/* Recovered Build 434 behavior, not the original source or a complete engine.
 * Explicit state pointers replace the two original globals; callers must preserve
 * stream identity and call order. Only the primary initializer normalizes zero.
 */
void myth181_seed_primary(uint32_t *state, uint32_t seed);
uint16_t myth181_random16(uint32_t *state);

/* 0x44ffb3: nine binary32 fields to the original 24-byte native view record.
 * Preconditions: valid nonoverlapping buffers; finite inputs; scaled first three
 * fields fit int32_t, remaining fields fit int64_t before low-word extraction.
 * Names/semantics of individual fields are not yet fully recovered.
 */
void myth181_pack_view_fields(const float values[9], uint8_t output[24]);

/* Offsets remain signed 32-bit as in the original. The buffer pointer is native
 * width and must never be serialized as part of a Film or asset structure.
 * Preconditions: valid original ring invariants, nonoverlapping command bytes,
 * capacity in 8..32767, offsets in 0..capacity, command size in 8..capacity,
 * nonnegative ticks <=INT32_MAX.
 * No locking; engine-side ordering/ownership must be preserved by the caller.
 */
typedef struct {
    uint32_t reserved;
    int32_t read_offset;
    int32_t write_offset;
    int32_t wrap_end;
    int32_t max_tick;
    int32_t capacity;
    uint8_t *bytes;
} myth181_command_ring;

int myth181_ring_write(myth181_command_ring *ring, const uint8_t *command);
const uint8_t *myth181_ring_read(myth181_command_ring *ring, int32_t max_tick);

/* Serialize the movement builder's command after slot-to-stable-ID mapping.
 * Options are the original 8 native little-endian bytes. Points are pairs of
 * native signed int32 coordinates. Output is native header + big-endian payload.
 * Return 0 without writing on invalid sizes/capacity (a boundary guard added here,
 * not behavior claimed for malformed input in the original executable).
 */
uint32_t myth181_encode_movement(uint8_t *output, uint32_t capacity,
    const uint8_t options[8], const int16_t *stable_ids, uint32_t unit_count,
    const int32_t *point_xy, uint32_t point_count, int32_t gameplay_version);

#endif

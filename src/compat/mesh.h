#ifndef MYTH181_COMPAT_MESH_H
#define MYTH181_COMPAT_MESH_H
#include <stdint.h>

/* Decode original mesh storage into a caller-owned buffer using Build 434's
 * conversion plans. Source/destination must be valid and nonoverlapping.
 * Returns 0 without writing for rejected sizes, ranges or marker types.
 * Runtime pointers remain unbound; actions are decoded but not executed.
 * A successful conversion is not world initialization or mission execution.
 */
int myth181_mesh_decode(const uint8_t *source, uint32_t size,
                       uint8_t *destination, uint32_t capacity);
/* Iteration states preserve original 10-byte / 12-byte layouts. Counts must be
 * <=32766; next_index starts at zero. Native record storage comes from decode.
 * Marker current_index starts at zero; palette current_index/type_index at -1.
 * Return pointers into the supplied record array; no actor is instantiated.
 */
typedef struct {
    int16_t type, palette, datum, current_index, next_index;
} myth181_marker_iterator;
typedef struct {
    uint32_t tag;
    int16_t type, current_index, type_index, next_index;
} myth181_palette_iterator;
const uint8_t *myth181_marker_next(const uint8_t *markers, uint32_t count, myth181_marker_iterator *state);
const uint8_t *myth181_palette_next(const uint8_t *palette, uint32_t count, myth181_palette_iterator *state);
#endif

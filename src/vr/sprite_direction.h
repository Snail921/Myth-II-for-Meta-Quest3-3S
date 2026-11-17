#ifndef MYTH_VR_SPRITE_DIRECTION_H
#define MYTH_VR_SPRITE_DIRECTION_H
#include <math.h>
// Original images: 0 front, 2 left-facing, 4 back, 6 right-facing (8 views).
// Original x/y -> VR -x/z. Convert the camera direction back to original
// coordinates; relative angle follows the preserved-handedness sprite order.
// Both eyes share head-center orientation.
static inline unsigned myth_sprite_direction(float camera_dx, float camera_dz,
                                             float unit_yaw_turns, unsigned views) {
    if (views <= 1) return 0;
    float turns = atan2f(camera_dz, -camera_dx) / 6.2831853071795864769f - unit_yaw_turns;
    turns -= floorf(turns);
    return (unsigned)floorf(turns * views + 0.5f) % views;
}
// Mirrored object variants reverse the angular lookup AND mirror the resulting
// image. Keep emitter-particle flips independent of this object convention.
static inline unsigned myth_object_sprite_direction(float dx, float dz, float yaw,
                                                    unsigned views, unsigned flips) {
    unsigned direction = myth_sprite_direction(dx, dz, yaw, views);
    return (flips & 1) && views > 1 ? (views - direction) % views : direction;
}
#endif

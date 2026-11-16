#pragma once
// Original z-up -> OpenXR y-up, preserving handedness: (-x, z, y).
static inline float mythWorldX(float x,float origin,float scale){return (origin-x)*scale;}
static inline float mythGameX(float x,float origin,float scale){return origin-x/scale;}

/////////////////////////////////////////////////////
//
//   TT RealShadowMapping - pure geometry helpers
//
//   Mirrors A_GetFloors (TT_Gravity_RT.dll 0x100039A0) and the old-floor
//   snapshot in RealShadowMappingPreRenderCallBack (0x10003620).
//
/////////////////////////////////////////////////////
#ifndef TT_REALSHADOWMAPPINGGEOMETRY_H
#define TT_REALSHADOWMAPPINGGEOMETRY_H

#include "VxVector.h"
#include "Vx2dVector.h"

// Light source used for the projection: the object position plus the
// Light-Vector input (0x10003A4D..0x10003A87).
inline VxVector RealShadowLightPosition(const VxVector &objPos, const VxVector &lightVector)
{
    return VxVector(objPos.x + lightVector.x, objPos.y + lightVector.y, objPos.z + lightVector.z);
}

// Throws the point objPos + (offsetX, 0, offsetZ) from the light onto the
// horizontal plane at 'height'. Evaluated in the DLL's order:
// (p - l + off) * (l.y - height) / (l.y - p.y) + l.
inline VxVector RealShadowProject(const VxVector &light, const VxVector &objPos,
                                  float offsetX, float offsetZ, float height)
{
    const float num = light.y - height;
    const float den = light.y - objPos.y;
    VxVector result;
    result.x = (objPos.x - light.x + offsetX) * num / den + light.x;
    result.y = height;
    result.z = (objPos.z - light.z + offsetZ) * num / den + light.z;
    return result;
}

// Corner offsets around the object: (+-radius*Size.x, +-radius*Size.y) in
// x/z, in the DLL's order (+,+), (-,+), (-,-), (+,-) for Eckpunkt1..4.
inline void RealShadowCornerOffsets(float radius, const Vx2DVector &size, float offsetX[4], float offsetZ[4])
{
    const float sx = size.x * radius;
    const float sz = radius * size.y;
    offsetX[0] = sx;          offsetZ[0] = sz;
    offsetX[1] = sx * -1.0f;  offsetZ[1] = sz;
    offsetX[2] = sx * -1.0f;  offsetZ[2] = sz * -1.0f;
    offsetX[3] = sx;          offsetZ[3] = sz * -1.0f;
}

// The four shadow corners, thrown from the light onto the centre hit's height.
inline void RealShadowCorners(const VxVector &light, const VxVector &objPos, float radius,
                              const Vx2DVector &size, float height, VxVector corners[4])
{
    float ox[4], oz[4];
    RealShadowCornerOffsets(radius, size, ox, oz);
    for (int i = 0; i < 4; ++i)
        corners[i] = RealShadowProject(light, objPos, ox[i], oz[i], height);
}

// DLL quirk (rep movsd at 0x100036AD): the previous floor list is snapshotted with
// memcpy(pool, tss->floor, 4 * old_count), i.e. 4 bytes per floor, but is then
// walked with the 28-byte Win32 floor record stride (id, min, max). Slot 'a'
// therefore only holds a real id when its first 4 bytes fall inside the copied
// range; every other slot is uninitialised pool memory. Returns whether slot
// 'a' was copied; callers treat an uncopied slot as "no floor", so the channel
// stays on that mesh. With at most 16 floors this means slot 0 always, slot 1
// only for 8+ old floors and slot 2 only for 15+.
static const int REALSHADOW_WIN32_FLOOR_STRIDE = 28;

inline bool RealShadowOldFloorSlotCopied(int slot, int oldCount)
{
    return slot * REALSHADOW_WIN32_FLOOR_STRIDE + 4 <= oldCount * 4;
}

#endif // TT_REALSHADOWMAPPINGGEOMETRY_H

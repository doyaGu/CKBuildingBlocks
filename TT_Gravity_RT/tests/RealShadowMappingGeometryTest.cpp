// Checks the TT RealShadowMapping geometry against TT_Gravity_RT.dll
// (A_GetFloors_0 at 0x100039A0, old-floor snapshot at 0x100036AD).
#include "../Behaviors/RealShadowMappingGeometry.h"

#include <cmath>
#include <cstdio>

static int g_failures = 0;

static void Check(bool value, const char *message)
{
    if (!value)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++g_failures;
    }
}

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static bool NearVec(const VxVector &v, float x, float y, float z)
{
    return Near(v.x, x) && Near(v.y, y) && Near(v.z, z);
}

int main()
{
    // Light = object position + Light-Vector.
    const VxVector objPos(2.0f, 5.0f, -1.0f);
    const VxVector light = RealShadowLightPosition(objPos, VxVector(1.0f, 5.0f, 2.0f));
    Check(NearVec(light, 3.0f, 10.0f, 1.0f), "light is position + Light-Vector");

    // Centre miss case: line light -> object hits the floor height 0.
    // t = (10 - 0) / (10 - 5) = 2: light + (obj - light) * 2.
    const VxVector centre = RealShadowProject(light, objPos, 0.0f, 0.0f, 0.0f);
    Check(NearVec(centre, 1.0f, 0.0f, -3.0f), "centre is thrown from the light through the object");

    // Corners: object position +- (radius*Size.x, radius*Size.y), order (+,+),(-,+),(-,-),(+,-).
    float ox[4], oz[4];
    RealShadowCornerOffsets(2.0f, Vx2DVector(0.5f, 1.5f), ox, oz);
    Check(Near(ox[0], 1.0f) && Near(oz[0], 3.0f), "corner 1 offset (+x,+z)");
    Check(Near(ox[1], -1.0f) && Near(oz[1], 3.0f), "corner 2 offset (-x,+z)");
    Check(Near(ox[2], -1.0f) && Near(oz[2], -3.0f), "corner 3 offset (-x,-z)");
    Check(Near(ox[3], 1.0f) && Near(oz[3], -3.0f), "corner 4 offset (+x,-z)");

    // Corners are thrown onto the centre hit's height (here 1.0, not the floor origin).
    VxVector corners[4];
    RealShadowCorners(light, objPos, 2.0f, Vx2DVector(0.5f, 1.5f), 1.0f, corners);
    // t = (10 - 1) / (10 - 5) = 1.8
    // corner 1: x = (2 - 3 + 1) * 1.8 + 3 = 3, z = (-1 - 1 + 3) * 1.8 + 1 = 2.8
    Check(NearVec(corners[0], 3.0f, 1.0f, 2.8f), "corner 1 projection");
    // corner 3: x = (2 - 3 - 1) * 1.8 + 3 = -0.6, z = (-1 - 1 - 3) * 1.8 + 1 = -8
    Check(NearVec(corners[2], -0.6f, 1.0f, -8.0f), "corner 3 projection");
    for (int i = 0; i < 4; ++i)
        Check(Near(corners[i].y, 1.0f), "corner height is the centre height");

    // Old floor snapshot quirk: 4 bytes per floor, 28-byte stride.
    Check(!RealShadowOldFloorSlotCopied(0, 0), "no slot with no old floors");
    Check(RealShadowOldFloorSlotCopied(0, 1), "slot 0 copied for 1 floor");
    Check(!RealShadowOldFloorSlotCopied(1, 2), "slot 1 not copied for 2 floors");
    Check(!RealShadowOldFloorSlotCopied(1, 7), "slot 1 not copied for 7 floors");
    Check(RealShadowOldFloorSlotCopied(1, 8), "slot 1 copied for 8 floors");
    Check(!RealShadowOldFloorSlotCopied(2, 14), "slot 2 not copied for 14 floors");
    Check(RealShadowOldFloorSlotCopied(2, 15), "slot 2 copied for 15 floors");
    Check(!RealShadowOldFloorSlotCopied(3, 16), "slot 3 never copied");

    if (g_failures)
        return 1;
    std::printf("RealShadowMappingGeometryTest passed\n");
    return 0;
}

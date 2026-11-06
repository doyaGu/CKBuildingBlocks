#ifndef TT_TOOLBOX_RT_DYNAMICPOSITIONARITHMETIC_H
#define TT_TOOLBOX_RT_DYNAMICPOSITIONARITHMETIC_H

#include "VxFloat80.h"
#include "VxFPUControlWord.h"

inline void TTScaleDynamicForce(float deltaTime, float force, float *result)
{
#if VXMATH_ENABLE_X87_EMULATION
    VxFloatContext arithmetic;
    VxFloatInitContext(arithmetic, VxGetFPUControlWord());
    const VxFloat80 factor = VxFloat80Multiply(arithmetic, VxFloat80Load(arithmetic, deltaTime), VxFloat80Load(arithmetic, 0.001f));
    *result = VxFloat80StoreFloat(arithmetic, VxFloat80Multiply(arithmetic, factor, VxFloat80Load(arithmetic, force)));
#else
    const float factor = deltaTime * 0.001f;
    *result = force * factor;
#endif
}

inline void TTAdvanceDynamicPosition(float target, float distance, float offset,
    float force, float delta, float damping, float *result)
{
#if VXMATH_ENABLE_X87_EMULATION
    VxFloatContext arithmetic;
    VxFloatInitContext(arithmetic, VxGetFPUControlWord());
    const VxFloat80 correction = VxFloat80Multiply(arithmetic, VxFloat80Subtract(arithmetic, VxFloat80Load(arithmetic, distance), VxFloat80Load(arithmetic, offset)), VxFloat80Load(arithmetic, force));
    const VxFloat80 drag = VxFloat80Multiply(arithmetic, VxFloat80Load(arithmetic, delta), VxFloat80Load(arithmetic, damping));
    *result = VxFloat80StoreFloat(arithmetic, VxFloat80Add(arithmetic, VxFloat80Add(arithmetic, correction, drag), VxFloat80Load(arithmetic, target)));
#else
    *result = target + ((distance - offset) * force + delta * damping);
#endif
}

#endif // TT_TOOLBOX_RT_DYNAMICPOSITIONARITHMETIC_H

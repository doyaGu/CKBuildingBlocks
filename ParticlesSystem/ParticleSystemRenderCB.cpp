
#include "CKAll.h"

#include "ParticleEmitter.h"
#include "ParticleSystemRenderCallbacks.h"

// VxDrawPrimitiveData layout differs across Virtools SDK versions.
#if (CKVERSION <= 0x05082002) || (CKVERSION == 0x13022002)
#define PS_STRIDED_UVS(data) XPtrStrided<VxUV> uvs((data)->TexCoordPtr, (data)->TexCoordStride)
#define PS_STRIDED_POSITIONS(data) XPtrStrided<VxVector4> positions((data)->PositionPtr, (data)->PositionStride)
#define PS_STRIDED_COLORS(data) XPtrStrided<CKDWORD> colors((data)->ColorPtr, (data)->ColorStride)

#define PS_STRIDED_SET_UVS(data) uvs = XPtrStrided<VxUV>((data)->TexCoordPtr, (data)->TexCoordStride)
#define PS_STRIDED_SET_POSITIONS(data) positions = XPtrStrided<VxVector4>((data)->PositionPtr, (data)->PositionStride)
#define PS_STRIDED_SET_COLORS(data) colors = XPtrStrided<CKDWORD>((data)->ColorPtr, (data)->ColorStride)
#else
#define PS_STRIDED_UVS(data) XPtrStrided<VxUV> uvs((data)->TexCoord.Ptr, (data)->TexCoord.Stride)
#define PS_STRIDED_POSITIONS(data) XPtrStrided<VxVector4> positions((data)->Positions.Ptr, (data)->Positions.Stride)
#define PS_STRIDED_COLORS(data) XPtrStrided<CKDWORD> colors((data)->Colors.Ptr, (data)->Colors.Stride)

#define PS_STRIDED_SET_UVS(data) uvs = XPtrStrided<VxUV>((data)->TexCoord.Ptr, (data)->TexCoord.Stride)
#define PS_STRIDED_SET_POSITIONS(data) positions = XPtrStrided<VxVector4>((data)->Positions.Ptr, (data)->Positions.Stride)
#define PS_STRIDED_SET_COLORS(data) colors = XPtrStrided<CKDWORD>((data)->Colors.Ptr, (data)->Colors.Stride)
#endif

int (*renderParticles)(CKRenderContext *dev, CKRenderObject *obj, void *arg);

// ACC - July 10,2002
// Need to add code into each RenderCB to test if shutdown
// Then we need to wait on PE to be finished
//
extern VxMutex logguard;
extern FILE *ACCLOG;

#ifdef USE_THR
void ThreadWaitForCompletion(ParticleEmitter *em, const char *RenderingMethod)
{
#ifdef MT_VERB
    // ACC, July 10, 2002
    {
        VxMutexLock lock(logguard);
        fprintf(ACCLOG, "About to render a %s\n", RenderingMethod);
        fflush(ACCLOG);
    }
#endif

    // Wait
    if (!em->hasBeenRendered && em->hasBeenEnqueud)
    {
        WaitForSingleObject(em->hasBeenComputedEvent, INFINITE);
        em->hasBeenRendered = TRUE;
    }
}
#endif

bool IsRenderingNeeded(ParticleEmitter *em)
{
    // This section of code can go into the render call back, and return premature
    // if particle is shutdown
    // if there is no more particles and the behavior is inactive
    // We get the activity
    CKBehavior *beh = em->m_Behavior;

    int activity = 0;
    beh->GetLocalParameterValue(1, &activity);

    if (!activity && !(em->getParticles()))
    {
        ShowParticles(beh, FALSE);
        beh->ActivateOutput(1);
        return false;
    }

    if (!em->particleCount)
        return false;

    return true;
}

int RenderParticles_P(CKRenderContext *dev, CKRenderObject *obj, void *arg)
{
    CK3dEntity *mov = (CK3dEntity *)obj;
    ParticleEmitter *em = (ParticleEmitter *)arg;

#ifdef USE_THR
    ThreadWaitForCompletion(em, "Point");
#endif
    if (!IsRenderingNeeded(em))
        return CKBR_OK;

    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure(CKRST_DP_TR_CL_VC, em->particleCount);

    VxMatrix oldmatrix = dev->GetWorldTransformationMatrix();
    dev->SetWorldTransformationMatrix(oldmatrix * mov->GetInverseWorldMatrix());

    // we don't let write to the ZBuffer
    dev->SetTexture(NULL);
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_SRCBLEND, em->m_SrcBlend);
    dev->SetState(VXRENDERSTATE_DESTBLEND, em->m_DestBlend);
    if (em->m_DestBlend != VXBLEND_ZERO)
        dev->SetState(VXRENDERSTATE_ALPHABLENDENABLE, TRUE);
    else
        dev->SetState(VXRENDERSTATE_ALPHABLENDENABLE, FALSE);
    dev->SetTextureStageState(CKRST_TSS_STAGEBLEND, 0, 1);

    Particle *p = em->getParticles();

    PS_STRIDED_POSITIONS(data);
    PS_STRIDED_COLORS(data);

    while (p)
    {
        *positions = p->pos;
        *colors = RGBAFTOCOLOR(&(p->m_Color));

        // next point
        p = p->next;

        ++colors;
        ++positions;
    }

    // The drawing
    dev->DrawPrimitive(VX_POINTLIST, NULL, em->particleCount, data);

    // we let write to the ZBuffer
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, TRUE);

    dev->SetWorldTransformationMatrix(oldmatrix);
    return 0;
}

int RenderParticles_L(CKRenderContext *dev, CKRenderObject *obj, void *arg)
{
    CK3dEntity *mov = (CK3dEntity *)obj;
    ParticleEmitter *em = (ParticleEmitter *)arg;

#ifdef USE_THR
    ThreadWaitForCompletion(em, "Line");
#endif
    if (!IsRenderingNeeded(em))
        return CKBR_OK;

    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure(CKRST_DP_TR_CL_VC, 2 * em->particleCount);

    VxMatrix oldmatrix = dev->GetWorldTransformationMatrix();
    dev->SetWorldTransformationMatrix(oldmatrix * mov->GetInverseWorldMatrix());

    // we don't let write to the ZBuffer
    dev->SetTexture(NULL);
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_SRCBLEND, em->m_SrcBlend);
    dev->SetState(VXRENDERSTATE_DESTBLEND, em->m_DestBlend);
    if (em->m_DestBlend != VXBLEND_ZERO)
        dev->SetState(VXRENDERSTATE_ALPHABLENDENABLE, TRUE);
    else
        dev->SetState(VXRENDERSTATE_ALPHABLENDENABLE, FALSE);
    dev->SetTextureStageState(CKRST_TSS_STAGEBLEND, 0, 1);

    Particle *p = em->getParticles();
    int i = 0;
    VxColor oldColor;

    PS_STRIDED_POSITIONS(data);
    PS_STRIDED_COLORS(data);

    while (p)
    {
        // Colors
        oldColor.r = p->m_Color.r - p->deltaColor.r * p->m_DeltaTime;
        oldColor.g = p->m_Color.g - p->deltaColor.g * p->m_DeltaTime;
        oldColor.b = p->m_Color.b - p->deltaColor.b * p->m_DeltaTime;
        oldColor.a = p->m_Color.a - p->deltaColor.a * p->m_DeltaTime;

        // Start of line
        *positions = p->pos - p->dir * p->m_DeltaTime;
        ++positions;

        *colors = RGBAFTOCOLOR(&oldColor);
        ++colors;

        // End of line
        *positions = p->pos + p->dir * p->m_Angle;
        ++positions;

        *colors = RGBAFTOCOLOR(&(p->m_Color));
        ++colors;

        // Next particle
        p = p->next;
    }

    // the Drawing itself
    dev->DrawPrimitive(VX_LINELIST, NULL, em->particleCount, data);

    // we let write to the ZBuffer
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, TRUE);

    dev->SetWorldTransformationMatrix(oldmatrix);
    return 0;
}

int RenderParticles_LL(CKRenderContext *dev, CKRenderObject *obj, void *arg)
{
    CK3dEntity *mov = (CK3dEntity *)obj;
    ParticleEmitter *em = (ParticleEmitter *)arg;

#ifdef USE_THR
    ThreadWaitForCompletion(em, "LongLine");
#endif
    if (!IsRenderingNeeded(em))
        return CKBR_OK;

    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure(CKRST_DP_TR_CL_VC,
                                                              (em->m_TrailCount) * 2 * em->particleCount);

    VxMatrix oldmatrix = dev->GetWorldTransformationMatrix();
    dev->SetWorldTransformationMatrix(oldmatrix * mov->GetInverseWorldMatrix());

    // we don't let write to the ZBuffer
    dev->SetTexture(NULL);
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_SRCBLEND, em->m_SrcBlend);
    dev->SetState(VXRENDERSTATE_DESTBLEND, em->m_DestBlend);
    if (em->m_DestBlend != VXBLEND_ZERO)
        dev->SetState(VXRENDERSTATE_ALPHABLENDENABLE, TRUE);
    else
        dev->SetState(VXRENDERSTATE_ALPHABLENDENABLE, FALSE);
    dev->SetTextureStageState(CKRST_TSS_STAGEBLEND, 0, 1);

    Particle *p = em->getParticles();
    int i = 0;
    VxColor oldColor;

    PS_STRIDED_POSITIONS(data);
    PS_STRIDED_COLORS(data);

    CKDWORD num_prims = 0;
    while (p)
    {
#define for    \
    if (false) \
    {          \
    }          \
    else for
        ParticleEmitter::ParticleHistoric &ph = em->GetParticleHistoric(p);

        // L. Puts the particle in the VB
        // Assumes, that when count isn't to the max, start is always 0;
        unsigned int prev = 0;
        unsigned int px = 1;
        unsigned int l1end = 0;
        unsigned int l2end = 0;
        if (ph.start == 0)
        {
            l1end = ph.count;
            l2end = 0;
        }
        else
        {
            l1end = ph.start;
            l2end = ph.particles.Size();

            *positions = ph.particles[l2end - 1].pos;
            ++positions;

            *colors = RGBAFTOCOLOR(&(p->m_Color));
            ++colors;

            *positions = ph.particles[0].pos;
            ++positions;

            *colors = RGBAFTOCOLOR(&(p->m_Color));
            ++colors;
        }
        for (; px < l1end; prev = px++)
        {
            *positions = ph.particles[prev].pos;
            ++positions;

            *colors = RGBAFTOCOLOR(&(p->m_Color));
            ++colors;

            *positions = ph.particles[px].pos;
            ++positions;

            *colors = RGBAFTOCOLOR(&(p->m_Color));
            ++colors;
        }
        for (prev = px++; px < l2end; prev = px++)
        {
            *positions = ph.particles[prev].pos;
            ++positions;

            *colors = RGBAFTOCOLOR(&(p->m_Color));
            ++colors;

            *positions = ph.particles[px].pos;
            ++positions;

            *colors = RGBAFTOCOLOR(&(p->m_Color));
            ++colors;
        }
        num_prims += ph.count - 1;

        // Next particle
        p = p->next;
    }

    // the Drawing itself
    dev->DrawPrimitive(VX_LINELIST, NULL, num_prims, data);

    // we let write to the ZBuffer
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, TRUE);

    dev->SetWorldTransformationMatrix(oldmatrix);
    return 0;
}

int RenderParticles_S(CKRenderContext *dev, CKRenderObject *obj, void *arg)
{
    CK3dEntity *mov = (CK3dEntity *)obj;
    ParticleEmitter *em = (ParticleEmitter *)arg;

#ifdef USE_THR
    ThreadWaitForCompletion(em, "Sprite");
#endif
    if (!IsRenderingNeeded(em))
        return CKBR_OK;

    const int VBUFFERSIZE = 4000;

    int pc = em->particleCount;
#ifndef PSX2
    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT | CKRST_DP_VBUFFER), (4 * pc > VBUFFERSIZE) ? VBUFFERSIZE : 4 * pc);
#else
    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT), (4 * pc > VBUFFERSIZE) ? VBUFFERSIZE : 4 * pc);
#endif

    PS_STRIDED_UVS(data);
    PS_STRIDED_POSITIONS(data);
    PS_STRIDED_COLORS(data);

    ///
    // Indices
    if (pc * 6 > ParticleEmitter::m_GlobalIndicesCount)
    {
        delete[] ParticleEmitter::m_GlobalIndices;

        ParticleEmitter::m_GlobalIndicesCount = pc * 6;
        ParticleEmitter::m_GlobalIndices = new CKWORD[ParticleEmitter::m_GlobalIndicesCount];

        int ni = 0;
        int fi = 0;
        for (int i = 0; i < pc; ++i)
        {
            ParticleEmitter::m_GlobalIndices[fi] = ni;
            ParticleEmitter::m_GlobalIndices[fi + 1] = ni + 1;
            ParticleEmitter::m_GlobalIndices[fi + 2] = ni + 2;
            ParticleEmitter::m_GlobalIndices[fi + 3] = ni;
            ParticleEmitter::m_GlobalIndices[fi + 4] = ni + 2;
            ParticleEmitter::m_GlobalIndices[fi + 5] = ni + 3;
            fi += 6;
            ni += 4;
        }
    }

    // Render States
    em->SetState(dev);

    CK3dEntity *cam = dev->GetViewpoint();
    const VxMatrix &mat = cam->GetWorldMatrix();
    VxMatrix oldmatrix = dev->GetWorldTransformationMatrix();
    VxMatrix realmatrix = oldmatrix * mov->GetInverseWorldMatrix();
    dev->SetWorldTransformationMatrix(realmatrix);
    VxVector camright;
    Vx3DRotateVector(&camright, realmatrix, &mat[0]);
    VxVector camup;
    Vx3DRotateVector(&camup, realmatrix, &mat[1]);

    BOOL changeuv = FALSE;
    float step = 1.0f;
    int framehc = 0;
    if (em->m_TextureFrameCount > 1)
    {
        changeuv = TRUE;
        framehc = 1 + (int)(sqrtf((float)(em->m_TextureFrameCount - 1)));
        step = 1.0f / framehc;
    }

    Particle *p = em->getParticles();

    VxVector cr, cu;
    VxVector rcr, rcu;
    VxVector pos;

    float u = 0.0f;
    float v = 0.0f;

    int np = 0;
    int remaining = 4 * pc;

    while (p)
    {
        pos = p->pos;

        // Precalcul for positions
        float halfsize = p->m_Size * 0.5f;
        cr = camright * halfsize;
        cu = camup * halfsize;

        // Angle rotation, if necessary
        if (p->m_Angle)
        {
            float cosa = cosf(p->m_Angle);
            float sina = sinf(p->m_Angle);
            rcr = cr * cosa - cu * sina;
            rcu = cr * sina + cu * cosa;
            cr = rcr;
            cu = rcu;
        }

        // Precalcul for color
        // We must now swap the color according to the rasterizer
        CKDWORD col = RGBAFTOCOLOR(&(p->m_Color));

        // Precalcul for uvs
        if (changeuv)
        {
            v = 0.0f;
            int c = p->m_CurrentFrame;

            while (c >= framehc)
            {
                v += step;
                c -= framehc;
            }
            u = c * step;
        }

        // Filling vertices
        *positions = pos - cr + cu;
        ++positions;
        *colors = col;
        ++colors;
        uvs->u = u;
        uvs->v = v;
        ++uvs;

        *positions = pos + cr + cu;
        ++positions;
        *colors = col;
        ++colors;
        uvs->u = u + step;
        uvs->v = v;
        ++uvs;

        *positions = pos + cr - cu;
        ++positions;
        *colors = col;
        ++colors;
        uvs->u = u + step;
        uvs->v = v + step;
        ++uvs;

        *positions = pos - cr - cu;
        ++positions;
        *colors = col;
        ++colors;
        uvs->u = u;
        uvs->v = v + step;
        ++uvs;

        ++np;
        p = p->next;

        if (np * 4 == VBUFFERSIZE)
        { // need to flush the vertex buffer
            // The Primitive Drawing
            dev->DrawPrimitive(VX_TRIANGLELIST, ParticleEmitter::m_GlobalIndices, 6 * np, data);

            remaining -= VBUFFERSIZE;
            np = 0; // restart

            if (remaining > 0)
            {
#ifndef PSX2
                data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT | CKRST_DP_VBUFFER),
                                                    (remaining > VBUFFERSIZE) ? VBUFFERSIZE : remaining);
#else
                data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT),
                                                    (remaining > VBUFFERSIZE) ? VBUFFERSIZE : remaining);
#endif
                PS_STRIDED_SET_UVS(data);
                PS_STRIDED_SET_POSITIONS(data);
                PS_STRIDED_SET_COLORS(data);
            }
        }
    }

    // The Primitive Drawing
    if (remaining)
        dev->DrawPrimitive(VX_TRIANGLELIST, ParticleEmitter::m_GlobalIndices, 6 * np, data);

    // we let write to the ZBuffer
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, TRUE);

    dev->SetWorldTransformationMatrix(oldmatrix);
    return 0;
}

//-----------------------------------------------------------------------------
// Long sprite (with trail)
int RenderParticles_LS(CKRenderContext *dev, CKRenderObject *obj, void *arg)
{
    CK3dEntity *mov = (CK3dEntity *)obj;
    ParticleEmitter *em = (ParticleEmitter *)arg;

#ifdef USE_THR
    ThreadWaitForCompletion(em, "LongSprite");
#endif
    if (!IsRenderingNeeded(em))
        return CKBR_OK;

    const int VBUFFERSIZE = 4000;

    assert(em->m_TrailCount > 0);
    int pc = em->particleCount * em->m_TrailCount;
#ifndef PSX2
    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT | CKRST_DP_VBUFFER), (4 * pc > VBUFFERSIZE) ? VBUFFERSIZE : 4 * pc);
#else
    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT), (4 * pc > VBUFFERSIZE) ? VBUFFERSIZE : 4 * pc);
#endif

    PS_STRIDED_UVS(data);
    PS_STRIDED_POSITIONS(data);
    PS_STRIDED_COLORS(data);

    ///
    // Indices
    if (pc * 6 > ParticleEmitter::m_GlobalIndicesCount)
    {
        delete[] ParticleEmitter::m_GlobalIndices;

        ParticleEmitter::m_GlobalIndicesCount = pc * 6;
        ParticleEmitter::m_GlobalIndices = new CKWORD[ParticleEmitter::m_GlobalIndicesCount];

        int ni = 0;
        int fi = 0;
        for (int i = 0; i < pc; ++i)
        {
            ParticleEmitter::m_GlobalIndices[fi] = ni;
            ParticleEmitter::m_GlobalIndices[fi + 1] = ni + 1;
            ParticleEmitter::m_GlobalIndices[fi + 2] = ni + 2;
            ParticleEmitter::m_GlobalIndices[fi + 3] = ni;
            ParticleEmitter::m_GlobalIndices[fi + 4] = ni + 2;
            ParticleEmitter::m_GlobalIndices[fi + 5] = ni + 3;
            fi += 6;
            ni += 4;
        }
    }

    // Render States
    em->SetState(dev);

    CK3dEntity *cam = dev->GetViewpoint();
    const VxMatrix &mat = cam->GetWorldMatrix();
    VxMatrix oldmatrix = dev->GetWorldTransformationMatrix();
    VxMatrix realmatrix = oldmatrix * mov->GetInverseWorldMatrix();
    dev->SetWorldTransformationMatrix(realmatrix);
    VxVector camright;
    Vx3DRotateVector(&camright, realmatrix, &mat[0]);
    VxVector camup;
    Vx3DRotateVector(&camup, realmatrix, &mat[1]);

    CKBOOL changeuv = FALSE;
    float step = 1.0f;
    int framehc = 0;
    if (em->m_TextureFrameCount > 1)
    {
        changeuv = TRUE;
        framehc = 1 + (int)(sqrtf((float)(em->m_TextureFrameCount - 1)));
        step = 1.0f / framehc;
    }

    Particle *p = em->getParticles();

    int np = 0;
    int remaining = 4 * pc;

    while (p)
    {
        struct local
        {
            static inline void FillParticle(const Particle *p,
                                            VxDrawPrimitiveData *data, XPtrStrided<VxVector4> &positions, XPtrStrided<DWORD> &colors,
                                            XPtrStrided<VxUV> &uvs, const VxVector &camup, const VxVector &camright,
                                            CKBOOL changeuv, int framehc, float step)
            {
                VxVector pos = p->pos;

                // Precalcul for positions
                float halfsize = p->m_Size * 0.5f;
                VxVector cr = camright * halfsize;
                VxVector cu = camup * halfsize;
                VxVector rcr, rcu;

                // Angle rotation, if necessary
                if (p->m_Angle)
                {
                    float cosa = cosf(p->m_Angle);
                    float sina = sinf(p->m_Angle);
                    rcr = cr * cosa - cu * sina;
                    rcu = cr * sina + cu * cosa;
                    cr = rcr;
                    cu = rcu;
                }

                // Precalcul for color
                CKDWORD col = RGBAFTOCOLOR(&(p->m_Color));

                float u = 0.0f;
                float v = 0.0f;
                // Precalcul for uvs
                if (changeuv)
                {
                    v = 0.0f;
                    int c = p->m_CurrentFrame;

                    while (c >= framehc)
                    {
                        v += step;
                        c -= framehc;
                    }
                    u = c * step;
                }

                // Filling vertices
                *positions = pos - cr + cu;
                ++positions;
                *colors = col;
                ++colors;
                uvs->u = u;
                uvs->v = v;
                ++uvs;

                *positions = pos + cr + cu;
                ++positions;
                *colors = col;
                ++colors;
                uvs->u = u + step;
                uvs->v = v;
                ++uvs;

                *positions = pos + cr - cu;
                ++positions;
                *colors = col;
                ++colors;
                uvs->u = u + step;
                uvs->v = v + step;
                ++uvs;

                *positions = pos - cr - cu;
                ++positions;
                *colors = col;
                ++colors;
                uvs->u = u;
                uvs->v = v + step;
                ++uvs;
            }
        };

        // need to flush the vertex buffer
        if ((np + em->m_TrailCount) * 4 > VBUFFERSIZE)
        {
            // The Primitive Drawing
            dev->DrawPrimitive(VX_TRIANGLELIST, ParticleEmitter::m_GlobalIndices, 6 * np, data);

            remaining -= np * 4;
            np = 0; // restart

            if (remaining > 0)
            {
#ifndef PSX2
                data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT | CKRST_DP_VBUFFER), (remaining > VBUFFERSIZE) ? VBUFFERSIZE : remaining);
#else
                data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT), (remaining > VBUFFERSIZE) ? VBUFFERSIZE : remaining);
#endif
                PS_STRIDED_SET_UVS(data);
                PS_STRIDED_SET_POSITIONS(data);
                PS_STRIDED_SET_COLORS(data);
            }
        }

        if (em->m_TrailCount == 0)
        {
            local::FillParticle(p, data, positions, colors, uvs, camup, camright,
                                changeuv, framehc, step);
            ++np;
        }
        else
        {
            // Assumes that when start != 0, count == m_TrailCount
            // So the buffer is full, and order does not matter.
            ParticleEmitter::ParticleHistoric &ph = em->GetParticleHistoric(p);

            for (int px = 0; px < ph.count; ++px)
            {
                local::FillParticle(&ph.particles[px], data, positions, colors,
                                    uvs, camup, camright, changeuv, framehc, step);
                ++np;
            }
        }

        p = p->next;
    }

    // The Primitive Drawing
    if (remaining && np > 0)
        dev->DrawPrimitive(VX_TRIANGLELIST, ParticleEmitter::m_GlobalIndices, 6 * np, data);

    dev->ReleaseCurrentVB();

    // we let write to the ZBuffer
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, TRUE);

    dev->SetWorldTransformationMatrix(oldmatrix);
    return 1;
}

int RenderParticles_FS(CKRenderContext *dev, CKRenderObject *obj, void *arg)
{
    CK3dEntity *mov = (CK3dEntity *)obj;
    ParticleEmitter *em = (ParticleEmitter *)arg;

#ifdef USE_THR
    ThreadWaitForCompletion(em, "FastSprite");
#endif
    if (!IsRenderingNeeded(em))
        return CKBR_OK;

    int pc = em->particleCount;
    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT), 3 * pc);

    PS_STRIDED_UVS(data);
    PS_STRIDED_POSITIONS(data);
    PS_STRIDED_COLORS(data);

    // Render States
    em->SetState(dev);

    CK3dEntity *cam = dev->GetViewpoint();
    const VxMatrix &mat = cam->GetWorldMatrix();
    VxMatrix oldmatrix = dev->GetWorldTransformationMatrix();
    VxMatrix realmatrix = oldmatrix * mov->GetInverseWorldMatrix();
    dev->SetWorldTransformationMatrix(realmatrix);
    VxVector camright;
    Vx3DRotateVector(&camright, realmatrix, &mat[0]);
    VxVector camup;
    Vx3DRotateVector(&camup, realmatrix, &mat[1]);

    CKBOOL changeuv = FALSE;
    float step = 1.0f;
    int framehc = 0;
    if (em->m_TextureFrameCount > 1)
    {
        changeuv = TRUE;
        framehc = 1 + (int)(sqrtf((float)(em->m_TextureFrameCount - 1)));
        step = 1.0f / framehc;
    }

    Particle *p = em->getParticles();

    VxVector cr, cu;
    VxVector rcr, rcu;
    VxVector pos;

    float u = 0.0f;
    float v = 0.0f;

    int count = 0;
    while (p)
    {
        cr = camright * p->m_Size * 0.5f;
        cu = camup * p->m_Size * 0.5f;

        // Positions
        if (p->m_Angle)
        {
            float cosa = cosf(p->m_Angle);
            float sina = sinf(p->m_Angle);
            rcr = cr * cosa - cu * sina;
            rcu = cr * sina + cu * cosa;
            cr = rcr;
            cu = rcu;
        }
        pos = p->pos;

        // Colors
        CKDWORD col = RGBAFTOCOLOR(&(p->m_Color));

        // The texture coordinates
        if (changeuv)
        {
            v = 0.0f;
            int c = p->m_CurrentFrame;

            while (c >= framehc)
            {
                v += step;
                c -= framehc;
            }
            u = c * step;
        }

        *positions = pos + cu;
        ++positions;
        *colors = col;
        ++colors;
        uvs->u = u + step * 0.5f;
        uvs->v = v;
        ++uvs;

        *positions = pos + cr - cu;
        ++positions;
        *colors = col;
        ++colors;
        uvs->u = u + step;
        uvs->v = v + step;
        ++uvs;

        *positions = pos - cr - cu;
        ++positions;
        *colors = col;
        ++colors;
        uvs->u = u;
        uvs->v = v + step;
        ++uvs;

        p = p->next;
        ++count;
    }

    // The Primitive Drawing
    dev->DrawPrimitive(VX_TRIANGLELIST, NULL, 3 * pc, data);

    // we let write to the ZBuffer
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, TRUE);

    dev->SetWorldTransformationMatrix(oldmatrix);
    return 1;
}

// Oriented Sprite
int RenderParticles_OS(CKRenderContext *dev, CKRenderObject *obj, void *arg)
{
    CK3dEntity *mov = (CK3dEntity *)obj;
    ParticleEmitter *em = (ParticleEmitter *)arg;

#ifdef USE_THR
    ThreadWaitForCompletion(em, "OrientableSprite");
#endif
    if (!IsRenderingNeeded(em))
        return CKBR_OK;
    int pc = em->particleCount;
#ifndef PSX2
    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT /*|CKRST_DP_VBUFFER*/), 4 * pc);
#else
    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT), 4 * pc);
#endif

    PS_STRIDED_UVS(data);
    PS_STRIDED_POSITIONS(data);
    PS_STRIDED_COLORS(data);

    ///
    // Indices
    if (pc * 6 > ParticleEmitter::m_GlobalIndicesCount)
    {
        delete[] ParticleEmitter::m_GlobalIndices;

        ParticleEmitter::m_GlobalIndicesCount = pc * 6;
        ParticleEmitter::m_GlobalIndices = new CKWORD[ParticleEmitter::m_GlobalIndicesCount];

        int ni = 0;
        int fi = 0;
        for (int i = 0; i < pc; ++i)
        {
            ParticleEmitter::m_GlobalIndices[fi] = ni;
            ParticleEmitter::m_GlobalIndices[fi + 1] = ni + 1;
            ParticleEmitter::m_GlobalIndices[fi + 2] = ni + 2;
            ParticleEmitter::m_GlobalIndices[fi + 3] = ni;
            ParticleEmitter::m_GlobalIndices[fi + 4] = ni + 2;
            ParticleEmitter::m_GlobalIndices[fi + 5] = ni + 3;
            fi += 6;
            ni += 4;
        }
    }

    // Set States
    em->SetState(dev, TRUE);

    CK3dEntity *cam = dev->GetViewpoint();
    const VxMatrix &mat = cam->GetWorldMatrix();
    VxMatrix oldmatrix = dev->GetWorldTransformationMatrix();
    VxMatrix realmatrix = oldmatrix * mov->GetInverseWorldMatrix();
    dev->SetWorldTransformationMatrix(realmatrix);
    VxVector camright;
    Vx3DRotateVector(&camright, realmatrix, &mat[0]);
    VxVector camup;
    Vx3DRotateVector(&camup, realmatrix, &mat[1]);
    VxVector camdir;
    Vx3DRotateVector(&camdir, realmatrix, &mat[2]);

    CKBOOL changeuv = FALSE;
    float step = 1.0f;
    int framehc = 0;
    if (em->m_TextureFrameCount > 1)
    {
        changeuv = TRUE;
        framehc = 1 + (int)((float)(sqrtf((float)(em->m_TextureFrameCount - 1))));
        step = 1.0f / framehc;
    }

    Particle *p = em->getParticles();

    VxVector pos, dir, old, vv(0, 0, 0), ww(0, 0, 0);
    float oldsize;
    VxColor oldColor;

    float u = 0.0f;
    float v = 0.0f;

    int ni = 0;
    int fi = 0;
    while (p)
    {

        pos = p->pos;
        dir = p->dir;

        // the old pos
        old = pos - dir * p->m_DeltaTime;
        // the old size
        oldsize = p->m_Size - p->m_DeltaSize * p->m_DeltaTime;
        // we exagerate the current position according to the latenty
        pos += dir * p->m_Angle;

        vv = pos - old;
        vv.Normalize();
        ww = Normalize(CrossProduct(camdir, vv));

        // the colors
        oldColor.r = p->m_Color.r - p->deltaColor.r * p->m_DeltaTime;
        oldColor.g = p->m_Color.g - p->deltaColor.g * p->m_DeltaTime;
        oldColor.b = p->m_Color.b - p->deltaColor.b * p->m_DeltaTime;
        oldColor.a = p->m_Color.a - p->deltaColor.a * p->m_DeltaTime;

        CKDWORD col = RGBAFTOCOLOR(&(p->m_Color));
        CKDWORD oldcol = RGBAFTOCOLOR(&oldColor);

        // The texture coordinates
        if (changeuv)
        {
            v = 0.0f;
            int c = p->m_CurrentFrame;

            while (c >= framehc)
            {
                v += step;
                c -= framehc;
            }
            u = c * step;
        }

        // Filling vertices
        *positions = pos + vv * p->m_Size + ww * p->m_Size;
        ++positions;
        *colors = col;
        ++colors;
        uvs->u = u;
        uvs->v = v;
        ++uvs;

        *positions = pos + vv * p->m_Size - ww * p->m_Size;
        ++positions;
        *colors = col;
        ++colors;
        uvs->u = u + step;
        uvs->v = v;
        ++uvs;

        *positions = old - vv * oldsize - ww * oldsize;
        ++positions;
        *colors = oldcol;
        ++colors;
        uvs->u = u + step;
        uvs->v = v + step;
        ++uvs;

        *positions = old - vv * oldsize + ww * oldsize;
        ++positions;
        *colors = oldcol;
        ++colors;

        uvs->u = u;
        uvs->v = v + step;
        ++uvs;

        p = p->next;
        ni += 4;
    }

    // The Primitive Drawing
    dev->DrawPrimitive(VX_TRIANGLELIST, ParticleEmitter::m_GlobalIndices, 6 * pc, data);

    // we let write to the ZBuffer
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, TRUE);

    dev->SetWorldTransformationMatrix(oldmatrix);
    return 1;
}

//-----------------------------------------------------------------------------
// Long Oriented Sprite (with trail)
int RenderParticles_LOS(CKRenderContext *dev, CKRenderObject *obj, void *arg)
{
    CK3dEntity *mov = (CK3dEntity *)obj;
    ParticleEmitter *em = (ParticleEmitter *)arg;

#ifdef USE_THR
    ThreadWaitForCompletion(em, "LongOrientableSprite");
#endif
    if (!IsRenderingNeeded(em))
        return CKBR_OK;

    const int VBUFFERSIZE = 4000;

    assert(em->m_TrailCount > 1);
    int pc = em->particleCount;
    // Maximum number of vertices per particle.
    const int num_vtx_max = (em->m_TrailCount) * 2;
    // Maximum number of indices per particle.
    const int num_idx_max = (em->m_TrailCount - 1) * 2 * 3;
#ifndef PSX2
    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT | CKRST_DP_VBUFFER), (num_vtx_max * pc > VBUFFERSIZE) ? VBUFFERSIZE : num_vtx_max * pc);
#else
    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT), (num_vtx_max * pc > VBUFFERSIZE) ? VBUFFERSIZE : num_vtx_max * pc);
#endif

    PS_STRIDED_UVS(data);
    PS_STRIDED_POSITIONS(data);
    PS_STRIDED_COLORS(data);

    // Index buffer
    CKWORD *index_buffer = dev->GetDrawPrimitiveIndices(pc * num_idx_max);

    // Render States
    em->SetState(dev);

    CK3dEntity *cam = dev->GetViewpoint();
    const VxMatrix &mat = cam->GetWorldMatrix();
    VxMatrix oldmatrix = dev->GetWorldTransformationMatrix();
    VxMatrix realmatrix = oldmatrix * mov->GetInverseWorldMatrix();
    dev->SetWorldTransformationMatrix(realmatrix);
    VxVector camdir;
    Vx3DRotateVector(&camdir, realmatrix, &mat[2]);

    CKBOOL changeuv = FALSE;
    float step = 1.0f;
    int framehc = 0;
    if (em->m_TextureFrameCount > 1)
    {
        changeuv = TRUE;
        framehc = 1 + (int)(sqrtf((float)(em->m_TextureFrameCount - 1)));
        step = 1.0f / framehc;
    }

    Particle *p = em->getParticles();

    int remaining = num_vtx_max * pc;
    int fi = 0; // face index
    int ni = 0; // number of indices
    while (p)
    {
        // Precalcul for positions
        float halfsize = p->m_Size * 0.5f;
        VxVector cd = CrossProduct(camdir, p->dir);
        cd.Normalize();
        cd *= halfsize;

        // Precalcul for color
        CKDWORD col = RGBAFTOCOLOR(&(p->m_Color));

        float u = 0.0f;
        float v = 0.0f;
        // Precalcul for uvs
        if (changeuv)
        {
            v = 0.0f;
            int c = p->m_CurrentFrame;

            while (c >= framehc)
            {
                v += step;
                c -= framehc;
            }
            u = c * step;
        }

        // Assumes that when start != 0, count == m_TrailCount
        // So the buffer is full.
        // Along u texture coordinate.
        ParticleEmitter::ParticleHistoric &ph = em->GetParticleHistoric(p);
        // Number of vertices per particles.
        const int num_vtx = (ph.count) * 2;

        // need to flush the vertex buffer
        if (ni + num_vtx > VBUFFERSIZE)
        {
            // The Primitive Drawing
            dev->DrawPrimitive(VX_TRIANGLELIST, index_buffer, fi, data);

            remaining -= ni;
            fi = 0;
            ni = 0;

            if (remaining > 0)
            {
#ifndef PSX2
                data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT | CKRST_DP_VBUFFER), (remaining > VBUFFERSIZE) ? VBUFFERSIZE : remaining);
#else
                data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT), (remaining > VBUFFERSIZE) ? VBUFFERSIZE : remaining);
#endif
                PS_STRIDED_SET_UVS(data);
                PS_STRIDED_SET_POSITIONS(data);
                PS_STRIDED_SET_COLORS(data);

                index_buffer = dev->GetDrawPrimitiveIndices(pc * num_idx_max);
            }
        }

        float local_step = step * (1.0f / (ph.count + 1));
        int start = ph.start;
        int size = ph.count;
        while (start || size)
        {
            for (int px = start; px < size; ++px)
            {
                VxVector &pos = ph.particles[px].pos;
                CKDWORD col = RGBAFTOCOLOR(&ph.particles[px].m_Color);

                // Filling vertices
                *positions = pos + cd;
                ++positions;
                *colors = col;
                ++colors;
                uvs->u = u;
                uvs->v = v;
                ++uvs;

                *positions = pos - cd;
                ++positions;
                *colors = col;
                ++colors;
                uvs->u = u;
                uvs->v = v + step;
                ++uvs;

                u += local_step;

                if (px != ph.start)
                {
                    index_buffer[fi] = ni - 2;
                    index_buffer[fi + 1] = ni;
                    index_buffer[fi + 2] = ni + 1;
                    index_buffer[fi + 3] = ni - 2;
                    index_buffer[fi + 4] = ni + 1;
                    index_buffer[fi + 5] = ni - 1;
                    fi += 6;
                }
                ni += 2;
            }
            if (size == ph.count)
            {
                start = 0;
                size = ph.start;
            }
            else
            {
                start = size = 0;
            }
        }
        p = p->next;
    }

    // The Primitive Drawing
    if (remaining && fi > 0)
    {
        data->VertexCount = ni;
        dev->DrawPrimitive(VX_TRIANGLELIST, index_buffer, fi, data);
    }
    dev->ReleaseCurrentVB();
    // we let write to the ZBuffer
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, TRUE);

    dev->SetWorldTransformationMatrix(oldmatrix);
    return 1;
}

// Comet Sprite
int RenderParticles_CS(CKRenderContext *dev, CKRenderObject *obj, void *arg)
{
    CK3dEntity *mov = (CK3dEntity *)obj;
    ParticleEmitter *em = (ParticleEmitter *)arg;

#ifdef USE_THR
    ThreadWaitForCompletion(em, "Comet");
#endif
    if (!IsRenderingNeeded(em))
        return CKBR_OK;

    int pc = em->particleCount;
    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure(CKRST_DP_TR_CL_VCT, 4 * pc);

    PS_STRIDED_UVS(data);
    PS_STRIDED_POSITIONS(data);
    PS_STRIDED_COLORS(data);
    ///
    // Indices
    if (pc * 6 > ParticleEmitter::m_GlobalIndicesCount)
    {
        delete[] ParticleEmitter::m_GlobalIndices;

        ParticleEmitter::m_GlobalIndicesCount = pc * 6;
        ParticleEmitter::m_GlobalIndices = new CKWORD[ParticleEmitter::m_GlobalIndicesCount];

        int ni = 0;
        int fi = 0;
        for (int i = 0; i < pc; ++i)
        {
            ParticleEmitter::m_GlobalIndices[fi] = ni;
            ParticleEmitter::m_GlobalIndices[fi + 1] = ni + 1;
            ParticleEmitter::m_GlobalIndices[fi + 2] = ni + 2;
            ParticleEmitter::m_GlobalIndices[fi + 3] = ni;
            ParticleEmitter::m_GlobalIndices[fi + 4] = ni + 2;
            ParticleEmitter::m_GlobalIndices[fi + 5] = ni + 3;
            fi += 6;
            ni += 4;
        }
    }

    // Set States
    em->SetState(dev, TRUE);

    CK3dEntity *cam = dev->GetViewpoint();
    const VxMatrix &mat = cam->GetWorldMatrix();
    VxMatrix oldmatrix = dev->GetWorldTransformationMatrix();
    VxMatrix realmatrix = oldmatrix * mov->GetInverseWorldMatrix();
    dev->SetWorldTransformationMatrix(realmatrix);
    VxVector camright;
    Vx3DRotateVector(&camright, realmatrix, &mat[0]);
    VxVector camup;
    Vx3DRotateVector(&camup, realmatrix, &mat[1]);
    VxVector camdir;
    Vx3DRotateVector(&camdir, realmatrix, &mat[2]);

    CKBOOL changeuv = FALSE;
    float step = 1.0f;
    int framehc = 0;
    if (em->m_TextureFrameCount > 1)
    {
        changeuv = TRUE;
        framehc = 1 + (int)((float)(sqrtf((float)(em->m_TextureFrameCount - 1))));
        step = 1.0f / framehc;
    }

    Particle *p = em->getParticles();

    VxVector pos, dir, center, old, vv(0, 0, 0), ww(0, 0, 0);
    VxColor oldColor;

    float u = 0.0f;
    float v = 0.0f;

    int ni = 0;
    int fi = 0;
    while (p)
    {
        pos = p->pos;
        dir = p->dir;
        float halfsize = p->m_Size;
        float speed = Magnitude(dir);
        dir /= speed;

        // the old pos
        old = pos - dir * (2.0f * halfsize + speed * p->m_Angle * p->m_DeltaTime);

        vv = pos - old;

        float fu = -halfsize * DotProduct(dir, camright);
        float fv = -halfsize * DotProduct(dir, camup);
        ww = -fv * camright + fu * camup;
        center = pos - halfsize * dir;

        // w = Normalize(CrossProduct(camdir,v));

        // Colors
        oldColor.r = p->m_Color.r - p->deltaColor.r * p->m_DeltaTime;
        oldColor.g = p->m_Color.g - p->deltaColor.g * p->m_DeltaTime;
        oldColor.b = p->m_Color.b - p->deltaColor.b * p->m_DeltaTime;
        oldColor.a = p->m_Color.a - p->deltaColor.a * p->m_DeltaTime;

        CKDWORD col = RGBAFTOCOLOR(&(p->m_Color));
        CKDWORD oldcol = RGBAFTOCOLOR(&oldColor);

        // The texture coordinates
        if (changeuv)
        {
            v = 0.0f;
            int c = p->m_CurrentFrame;

            while (c >= framehc)
            {
                v += step;
                c -= framehc;
            }
            u = c * step;
        }

        // Filling vertices
        *positions = center + ww;
        ++positions;
        *colors = col;
        ++colors;

        uvs->u = u;
        uvs->v = v;
        ++uvs;

        *positions = pos;
        ++positions;
        *colors = col;
        ++colors;

        uvs->u = u + step;
        uvs->v = v;
        ++uvs;

        *positions = center - ww;
        ++positions;
        *colors = col;
        ++colors;

        uvs->u = u + step;
        uvs->v = v + step;
        ++uvs;

        *positions = old;
        ++positions;
        *colors = oldcol;
        ++colors;
        uvs->u = u;
        uvs->v = v + step;
        ++uvs;

        p = p->next;
        ni += 4;
    }

    // The Primitive Drawing
    dev->DrawPrimitive(VX_TRIANGLELIST, ParticleEmitter::m_GlobalIndices, 6 * pc, data);

    // we let write to the ZBuffer
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, TRUE);

    dev->SetWorldTransformationMatrix(oldmatrix);
    return 1;
}

int RenderParticles_RS(CKRenderContext *dev, CKRenderObject *obj, void *arg)
{
    CK3dEntity *mov = (CK3dEntity *)obj;
    ParticleEmitter *em = (ParticleEmitter *)arg;
    // ACC, July 10, 2002

#ifdef USE_THR
    ThreadWaitForCompletion(em, "Radial");
#endif
    if (!IsRenderingNeeded(em))
        return CKBR_OK;

    int pc = em->particleCount;
    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure(CKRST_DP_TR_CL_VCT, 4 * pc);

    ///
    // Indices
    if (pc * 6 > ParticleEmitter::m_GlobalIndicesCount)
    {
        delete[] ParticleEmitter::m_GlobalIndices;

        ParticleEmitter::m_GlobalIndicesCount = pc * 6;
        ParticleEmitter::m_GlobalIndices = new CKWORD[ParticleEmitter::m_GlobalIndicesCount];

        int ni = 0;
        int fi = 0;
        for (int i = 0; i < pc; ++i)
        {
            ParticleEmitter::m_GlobalIndices[fi] = ni;
            ParticleEmitter::m_GlobalIndices[fi + 1] = ni + 1;
            ParticleEmitter::m_GlobalIndices[fi + 2] = ni + 2;
            ParticleEmitter::m_GlobalIndices[fi + 3] = ni;
            ParticleEmitter::m_GlobalIndices[fi + 4] = ni + 2;
            ParticleEmitter::m_GlobalIndices[fi + 5] = ni + 3;
            fi += 6;
            ni += 4;
        }
    }

    PS_STRIDED_UVS(data);
    PS_STRIDED_POSITIONS(data);
    PS_STRIDED_COLORS(data);

    // Render States
    em->SetState(dev);

    CK3dEntity *cam = dev->GetViewpoint();
    VxMatrix oldmatrix = dev->GetWorldTransformationMatrix();
    dev->SetWorldTransformationMatrix(cam->GetWorldMatrix());

    CKBOOL changeuv = FALSE;
    float step = 1.0f;
    int framehc = 0;
    if (em->m_TextureFrameCount > 1)
    {
        changeuv = TRUE;
        framehc = 1 + (int)(sqrtf((float)(em->m_TextureFrameCount - 1)));
        step = 1.0f / framehc;
    }

    Particle *p = em->getParticles();

    VxVector pos, old(0, 0, 0), vv(0, 0, 0), ww(0, 0, 0);

    float u = 0.0f;
    float v = 0.0f;

    CK3dEntity *ent = (CK3dEntity *)dev->GetCKContext()->GetObject(em->m_Entity);
    cam->InverseTransform(&old, &vv, ent);
    const VxMatrix &worldCam = cam->GetWorldMatrix();

    old += Normalize(old) * 40.0f;

    CK_ID oldtex = -1;

    const VxMatrix &invCam = cam->GetInverseWorldMatrix();

    VxMatrix finalMat = /*ent->GetWorldMatrix()*/ invCam;

    int fi = 0;
    int ni = 0;
    while (p)
    {

        Vx3DMultiplyMatrixVector(&pos, finalMat, &p->pos);

        vv.x = pos.x - old.x;
        vv.y = pos.y - old.y;
        vv.Normalize();
        ww.x = -vv.y;
        ww.y = vv.x;

        // the colors
        CKDWORD col = RGBAFTOCOLOR(&(p->m_Color));

        // The texture coordinates
        if (changeuv)
        {
            v = 0.0f;
            int c = p->m_CurrentFrame;

            while (c >= framehc)
            {
                v += step;
                c -= framehc;
            }
            u = c * step;
        }

        // Filling vertices
        *positions = pos + vv * p->m_Size + ww * p->m_Size;
        ++positions;
        *colors = col;
        ++colors;
        uvs->u = u;
        uvs->v = v;
        ++uvs;

        *positions = pos + vv * p->m_Size - ww * p->m_Size;
        ++positions;
        *colors = col;
        ++colors;
        uvs->u = u + step;
        uvs->v = v;
        ++uvs;

        *positions = old - vv * p->m_Size - ww * p->m_Size;
        ++positions;
        *colors = col;
        ++colors;
        uvs->u = u + step;
        uvs->v = v + step;
        ++uvs;

        *positions = old - vv * p->m_Size + ww * p->m_Size;
        ++positions;
        *colors = col;
        ++colors;
        uvs->u = u;
        uvs->v = v + step;
        ++uvs;

        p = p->next;
        ni += 4;
    }

    dev->DrawPrimitive(VX_TRIANGLELIST, ParticleEmitter::m_GlobalIndices, pc * 6, data);

    // we let write to the ZBuffer
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, TRUE);

    dev->SetWorldTransformationMatrix(oldmatrix);
    return 1;
}

// TODO : change this callback : not very nice...
int RenderParticles_O(CKRenderContext *dev, CKRenderObject *obj, void *arg)
{
    //	int todo_manage_reflection_in_objects;
    CK3dEntity *mov = (CK3dEntity *)obj;
    ParticleEmitter *em = (ParticleEmitter *)arg;

#ifdef USE_THR
    ThreadWaitForCompletion(em, "Object");
#endif
    if (!IsRenderingNeeded(em))
        return CKBR_OK;

    CKGroup *group = (CKGroup *)dev->GetCKContext()->GetObject(em->m_Group);
    if (!group)
        return 1;
    CK3dEntity *ent;
    CKMaterial *mat;
    CKMesh *mesh;
    Particle *p = em->getParticles();
    VxVector dir, up, right;
    int oc = group->GetObjectCount();
    while (p)
    {
        if (p->m_GroupIndex < oc)
            ent = (CK3dEntity *)group->GetObject(p->m_GroupIndex);
        else
            ent = (CK3dEntity *)group->GetObject(0);
        if (CKIsChildClassOf(ent, CKCID_3DENTITY))
        {
            mesh = ent->GetCurrentMesh();
            if (mesh)
            {
                mat = mesh->GetFaceMaterial(0);
                if (mat)
                    mat->SetDiffuse((p->m_Color));

                ent->SetPosition(&(p->pos), NULL);
                dir = Normalize(p->dir);
                up.Set(0, 1, 0);
                right = CrossProduct(up, dir);
                up = CrossProduct(dir, right);
                ent->SetOrientation(&dir, &up, &right);
                if (p->m_Angle)
                {
                    ent->Rotate(&dir, p->m_Angle);
                }
                ent->SetScale(&VxVector(p->m_Size, p->m_Size, p->m_Size));
                ent->Render(dev, FALSE);
            }
        }
        p = p->next;
    }

    return 1;
}

//////////////////////////////////////////////////////////////////////////
// PointSprite (Hardware) Render
int RenderParticles_PS(CKRenderContext *dev, CKRenderObject *obj, void *arg)
{
    CK3dEntity *mov = (CK3dEntity *)obj;
    ParticleEmitter *em = (ParticleEmitter *)arg;

#ifdef USE_THR
    ThreadWaitForCompletion(em, "PointSprite");
#endif
    if (!IsRenderingNeeded(em))
        return CKBR_OK;

    const int VBUFFERSIZE = 4000;
    int pc = em->particleCount;
    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure(CKRST_DPFLAGS(CKRST_DP_TR_CL_VC | CKRST_DP_VBUFFER), (pc > VBUFFERSIZE) ? VBUFFERSIZE : pc);

    VxMatrix oldmatrix = dev->GetWorldTransformationMatrix();
    // To manage reflection
    dev->SetWorldTransformationMatrix(oldmatrix * mov->GetInverseWorldMatrix());

    // we don't let write to the ZBuffer
    em->SetState(dev);
    float averageSize = em->m_StartSize * 2.0f;
    float minSize = 0.0f;
    float maxSize = 4096.0f;
    dev->SetState(VXRENDERSTATE_POINTSPRITEENABLE, TRUE);
    dev->SetState(VXRENDERSTATE_POINTSIZE, *(CKDWORD *)&averageSize);
    dev->SetState(VXRENDERSTATE_POINTSIZE_MIN, *(CKDWORD *)&minSize);
    dev->SetState(VXRENDERSTATE_POINTSIZE_MAX, *(CKDWORD *)&maxSize);
    dev->SetState(VXRENDERSTATE_POINTSCALEENABLE, TRUE);

    float pointScaleA = 1.0f;
    float pointScaleB = 0.0f;
    float pointScaleC = 1.0f;
    dev->SetState(VXRENDERSTATE_POINTSCALE_A, *(CKDWORD *)&pointScaleA);
    dev->SetState(VXRENDERSTATE_POINTSCALE_B, *(CKDWORD *)&pointScaleB);
    dev->SetState(VXRENDERSTATE_POINTSCALE_C, *(CKDWORD *)&pointScaleC);

    Particle *p = em->getParticles();

    PS_STRIDED_POSITIONS(data);
    PS_STRIDED_COLORS(data);

    int np = 0;
    int remaining = pc;

    while (p)
    {
        *positions = p->pos;
        *colors = RGBAFTOCOLOR(&(p->m_Color));

        // next point
        p = p->next;

        ++colors;
        ++positions;
        ++np;

        if (np == VBUFFERSIZE)
        { // need to flush the vertex buffer
            // The Primitive Drawing
            dev->DrawPrimitive(VX_POINTLIST, 0, np, data);

            remaining -= VBUFFERSIZE;
            np = 0; // restart

            if (remaining > 0)
            {
                data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT | CKRST_DP_VBUFFER), (remaining > VBUFFERSIZE) ? VBUFFERSIZE : remaining);
                PS_STRIDED_SET_POSITIONS(data);
                PS_STRIDED_SET_COLORS(data);
            }
        }
    }

    // The Primitive Drawing
    if (remaining)
        dev->DrawPrimitive(VX_POINTLIST, NULL, remaining, data);

    // we let write to the ZBuffer
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, TRUE);
    dev->SetState(VXRENDERSTATE_POINTSPRITEENABLE, FALSE);

    dev->SetWorldTransformationMatrix(oldmatrix);
    return 0;
}

// FixedPointSprite (Hardware) Render
int RenderParticles_FPS(CKRenderContext *dev, CKRenderObject *obj, void *arg)
{
    CK3dEntity *mov = (CK3dEntity *)obj;
    ParticleEmitter *em = (ParticleEmitter *)arg;

#ifdef USE_THR
    ThreadWaitForCompletion(em, "PointSprite");
#endif
    if (!IsRenderingNeeded(em))
        return CKBR_OK;

    const int VBUFFERSIZE = 4000;
    int pc = em->particleCount;
    VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure(CKRST_DPFLAGS(CKRST_DP_TR_CL_VC | CKRST_DP_VBUFFER), (pc > VBUFFERSIZE) ? VBUFFERSIZE : pc);

    VxMatrix oldmatrix = dev->GetWorldTransformationMatrix();
    // To manage reflection
    dev->SetWorldTransformationMatrix(oldmatrix * mov->GetInverseWorldMatrix());

    // we don't let write to the ZBuffer
    em->SetState(dev);
    float averageSize = em->m_StartSize * 2.0f;
    float minSize = 0.0f;
    float maxSize = 4096.0f;
    dev->SetState(VXRENDERSTATE_POINTSPRITEENABLE, TRUE);
    dev->SetState(VXRENDERSTATE_POINTSIZE, *(CKDWORD *)&averageSize);
    dev->SetState(VXRENDERSTATE_POINTSIZE_MIN, *(CKDWORD *)&minSize);
    dev->SetState(VXRENDERSTATE_POINTSIZE_MAX, *(CKDWORD *)&maxSize);
    dev->SetState(VXRENDERSTATE_POINTSCALEENABLE, FALSE);

    Particle *p = em->getParticles();

    PS_STRIDED_POSITIONS(data);
    PS_STRIDED_COLORS(data);

    int np = 0;
    int remaining = pc;

    while (p)
    {
        *positions = p->pos;
        *colors = RGBAFTOCOLOR(&(p->m_Color));

        // next point
        p = p->next;

        ++colors;
        ++positions;
        ++np;

        if (np == VBUFFERSIZE)
        { // need to flush the vertex buffer
            // The Primitive Drawing
            dev->DrawPrimitive(VX_POINTLIST, 0, np, data);

            remaining -= VBUFFERSIZE;
            np = 0; // restart

            if (remaining > 0)
            {
                data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_TR_CL_VCT | CKRST_DP_VBUFFER), (remaining > VBUFFERSIZE) ? VBUFFERSIZE : remaining);
                PS_STRIDED_SET_POSITIONS(data);
                PS_STRIDED_SET_COLORS(data);
            }
        }
    }

    // The Primitive Drawing
    if (remaining)
        dev->DrawPrimitive(VX_POINTLIST, NULL, remaining, data);

    // we let write to the ZBuffer
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, TRUE);
    dev->SetState(VXRENDERSTATE_POINTSPRITEENABLE, FALSE);

    dev->SetWorldTransformationMatrix(oldmatrix);
    return 0;
}

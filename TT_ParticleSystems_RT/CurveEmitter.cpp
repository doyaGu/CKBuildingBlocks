#include "CKAll.h"

#include "CurveEmitter.h"

void CurveEmitter::InitiateParticle(Particle *p)
{
    m_Curve->GetPos(m_Context->Random() * INV_RAND, &(p->pos));
}

void CurveEmitter::InitiateDirection(Particle *p)
{
    VxVector newDir;
    CK3dEntity *entity = (CK3dEntity *)m_Context->GetObject(m_Entity);
    entity->GetOrientation(NULL, &newDir);
    VxMatrix mat;
    Vx3DMatrixFromEulerAngles(mat, m_YawVariation * ParticleRandomSigned(m_Context), 0.0, m_PitchVariation * ParticleRandomSigned(m_Context));
    Vx3DRotateVector(&(p->dir), mat, &newDir);
}

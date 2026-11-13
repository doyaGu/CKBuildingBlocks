#include "CKAll.h"

#include "CubicEmitter.h"

void CubicEmitter::InitiateParticle(Particle *p)
{
    VxVector randpos(ParticleRandomSigned(m_Context), ParticleRandomSigned(m_Context), ParticleRandomSigned(m_Context));
    CK3dEntity *entity = (CK3dEntity *)m_Context->GetObject(m_Entity);
    entity->Transform(&p->pos, &randpos);
}
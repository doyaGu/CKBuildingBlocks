#include "CKAll.h"

#include "PlanarEmitter.h"

void PlanarEmitter::InitiateParticle(Particle *p)
{
    VxVector randpos(ParticleRandomSigned(m_Context), ParticleRandomSigned(m_Context), 0.0);
    CK3dEntity *entity = (CK3dEntity *)m_Context->GetObject(m_Entity);
    entity->Transform(&p->pos, &randpos);
}
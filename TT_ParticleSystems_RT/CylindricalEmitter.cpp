#include "CKAll.h"

#include "CylindricalEmitter.h"

void CylindricalEmitter::InitiateParticle(Particle *p)
{
    VxVector randpos(ParticleRandomSigned(m_Context), ParticleRandomSigned(m_Context), 0.0);
    randpos.Normalize();
    CK3dEntity *entity = (CK3dEntity *)m_Context->GetObject(m_Entity);
    VxVector newdir;
    entity->TransformVector(&newdir, &randpos);
    p->dir = Normalize(newdir);
    randpos.z = ParticleRandomSigned(m_Context);
    entity->Transform(&p->pos, &randpos);
}

void CylindricalEmitter::InitiateDirection(Particle *p) {}

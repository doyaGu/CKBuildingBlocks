#ifndef PHYSICS_RT_RETAILCONTACTKERNELS_H
#define PHYSICS_RT_RETAILCONTACTKERNELS_H

class IVP_Event_Sim;

float native_friction(void *contact, const IVP_Event_Sim *event);
void native_pressure(void *contact, const IVP_Event_Sim *event);

#endif // PHYSICS_RT_RETAILCONTACTKERNELS_H

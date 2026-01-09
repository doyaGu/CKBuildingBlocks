# ParticlesSystem

add_library(ParticlesSystem SHARED
        ParticleSystems.cpp
        CubicEmitter.cpp
        CubicEmitter.h
        CubicParticleSystem.cpp
        CurveEmitter.cpp
        CurveEmitter.h
        CurveParticleSystem.cpp
        CylindricalEmitter.cpp
        CylindricalEmitter.h
        CylindricalParticleSystem.cpp
        DiscEmitter.cpp
        DiscEmitter.h
        DiscParticleSystem.cpp
        GeneralParticleSystem.cpp
        GeneralParticleSystem.h
        LinearParticleSystem.cpp
        LineEmitter.cpp
        LineEmitter.h
        ObjectEmitter.cpp
        ObjectEmitter.h
        ObjectParticleSystem.cpp
        Particle.cpp
        Particle.h
        ParticleEmitter.cpp
        ParticleEmitter.h
        ParticleGuids.h
        ParticleManager.cpp
        ParticleManager.h
        ParticleSystemDeflectors.cpp
        ParticleSystemInteractors.cpp
        ParticleSystemMeshes.cpp
        ParticleSystemRenderCallBacks.h
        ParticleSystemRenderCB.cpp
        ParticleTools.cpp
        ParticleTools.h
        PlanarEmitter.cpp
        PlanarEmitter.h
        PlanarParticleSystem.cpp
        PointEmitter.cpp
        PointEmitter.h
        PointParticleSystem.cpp
        SphericalEmitter.cpp
        SphericalEmitter.h
        SphericalParticleSystem.cpp
        BlockingQueue.h
        fvec.h
        ThreadSafeQueue.h)
target_include_directories(ParticlesSystem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ParticlesSystem PRIVATE CK2 VxMath)

install(TARGETS ParticlesSystem RUNTIME DESTINATION BuildingBlocks)

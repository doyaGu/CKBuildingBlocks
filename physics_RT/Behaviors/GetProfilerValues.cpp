/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		          Get Profiler Values
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

#include "CKIpionManager.h"

CKObjectDeclaration *FillBehaviorGetProfilerValuesDecl();
CKERROR CreateGetProfilerValuesProto(CKBehaviorPrototype **pproto);
int GetProfilerValues(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorGetProfilerValuesDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Get Profiler Values");
    od->SetDescription("Get Profiler Values");
    od->SetCategory("Physics");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x1c8e61d1, 0x32723c6f));
    od->SetAuthorGuid(TERRATOOLS_GUID);
    od->SetAuthorName("Terratools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateGetProfilerValuesProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateGetProfilerValuesProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Get Profiler Values");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("Start");
    proto->DeclareInput("Reset");

    proto->DeclareOutput("Out");
    proto->DeclareOutput("ResetOut");

    proto->DeclareOutParameter("HasPhysicsCalls", CKPGUID_INT);
    proto->DeclareOutParameter("PhysicalizeCalls", CKPGUID_INT);
    proto->DeclareOutParameter("DePhysicalizeCalls", CKPGUID_INT);
    proto->DeclareOutParameter("HasPhysicsTime", CKPGUID_FLOAT);
    proto->DeclareOutParameter("DePhysicalizeTime", CKPGUID_FLOAT);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(GetProfilerValues);

    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int GetProfilerValues(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    CKIpionManager *man = CKIpionManager::GetManager(context);

    if (beh->IsInputActive(1))
    {
        man->ResetProfiler();

        beh->ActivateInput(1, FALSE);
        beh->ActivateOutput(1, TRUE);
    }
    else
    {
        int hasPhysicsCalls = man->m_HasPhysicsCalls;
        int physicalizeCalls = man->m_PhysicalizeCalls;
        int dePhysicalizeCalls = man->m_DePhysicalizeCalls;

        if (man->m_ProfilerCounter.QuadPart == 0)
            QueryPerformanceFrequency(&man->m_ProfilerCounter);

        float hasPhysicsTime = 0.0f;
        float dePhysicalizeTime = 0.0f;
        if (man->m_ProfilerCounter.QuadPart > 0)
        {
            hasPhysicsTime = (float)(1000.0 * (double)man->m_HasPhysicsTime.QuadPart / (double)man->m_ProfilerCounter.QuadPart);
            dePhysicalizeTime = (float)(1000.0 * (double)man->m_DePhysicalizeTime.QuadPart / (double)man->m_ProfilerCounter.QuadPart);
        }

        beh->SetOutputParameterValue(0, &hasPhysicsCalls);
        beh->SetOutputParameterValue(1, &physicalizeCalls);
        beh->SetOutputParameterValue(2, &dePhysicalizeCalls);
        beh->SetOutputParameterValue(3, &hasPhysicsTime);
        beh->SetOutputParameterValue(4, &dePhysicalizeTime);
        beh->ActivateInput(0, FALSE);
        beh->ActivateOutput(0, TRUE);
    }

    return CKBR_OK;
}

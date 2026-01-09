/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            LODManagerOptions
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"
#include "LODManager.h"

CKObjectDeclaration *FillBehaviorLODManagerOptionsDecl();
CKERROR CreateLODManagerOptionsProto(CKBehaviorPrototype **pproto);
int LODManagerOptions(const CKBehaviorContext &behcontext);
CKERROR LODManagerOptionsCallBack(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorLODManagerOptionsDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("LOD Manager Options");
    od->SetDescription("Set and Get information about the LOD Manager.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Activate: </SPAN>if TRUE, the LOD manager is activated, otherwise it's deactivated.<BR>
    <SPAN CLASS=pin>Global LOD Factor: </SPAN>sets the LOD factor by which all faces count will be multiplied for LOD objects.<BR>
    <BR>
    <SPAN CLASS=pout>Active: </SPAN>if TRUE, the LOD manager is active, otherwise it's not active.<BR>
    <SPAN CLASS=pin>Global LOD Factor: </SPAN>gets the LOD factor by which all faces count will be multiplied for LOD objects.<BR>
    <BR>
    */
    od->SetCategory("Optimizations/Level Of Detail");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x2b557187, 0x2027baf));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateLODManagerOptionsProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateLODManagerOptionsProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("LOD Manager Options");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Activate", CKPGUID_BOOL, "TRUE");
    proto->DeclareInParameter("Global LOD Factor", CKPGUID_FLOAT, "1");
    proto->DeclareInParameter("Build Normals For Progressive Meshes", CKPGUID_BOOL, "FALSE");

    proto->DeclareOutParameter("Active", CKPGUID_BOOL);
    proto->DeclareOutParameter("Global LOD Factor", CKPGUID_FLOAT);
    proto->DeclareOutParameter("Build Normals For Progressive Meshes", CKPGUID_BOOL);

#define CKPGUID_LODMANAGEROPTIONS_SETTING CKDEFINEGUID(0x3c5302cc, 0x2a611067)
    proto->DeclareSetting("Modify Parameters", CKPGUID_LODMANAGEROPTIONS_SETTING, "Activate,Global LOD Factor,Build Normals For Progressive Meshes");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(LODManagerOptions);
    proto->SetBehaviorCallbackFct(LODManagerOptionsCallBack);

    *pproto = proto;
    return CK_OK;
}

int LODManagerOptions(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    LODManager *lm = (LODManager *)behcontext.Context->GetManagerByGuid(LOD_MANAGER_GUID);

    // Set IO states
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    //--- Inputs

    CKBOOL activate;
    float factor;
    CKBOOL allowPMBuildNorm;

    CKParameterIn *pin;
    int pindex = 0;

    // Get Activate
    pin = beh->GetInputParameter(pindex);
    if (pin && pin->IsEnabled())
    {
        activate = TRUE;
        pin->GetValue(&activate);
        lm->ActivateLODManager(activate);
    }
    ++pindex;

    // Get Global LOD Factor
    pin = beh->GetInputParameter(pindex);
    if (pin && pin->IsEnabled())
    {
        factor = 1.0f;
        pin->GetValue(&factor);
        lm->SetGlobalLODFactor(factor);
    }
    ++pindex;

    // Get PM Build Norm
    pin = beh->GetInputParameter(pindex);
    if (pin && pin->IsEnabled())
    {
        allowPMBuildNorm = FALSE;
        pin->GetValue(&allowPMBuildNorm);
        lm->AllowNormalBuildingForPM(allowPMBuildNorm);
    }
    ++pindex;

    //--- Outputs
    activate = lm->IsActiveLODManager();
    beh->SetOutputParameterValue(0, &activate);

    factor = lm->GetGlobalLODFactor();
    beh->SetOutputParameterValue(1, &factor);

    allowPMBuildNorm = lm->IsNormalBuildingForPMAllowed();
    beh->SetOutputParameterValue(2, &allowPMBuildNorm);

    return CKBR_OK;
}

/*******************************************************/
/*                     CALLBACK                        */
/*******************************************************/
CKERROR LODManagerOptionsCallBack(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORCREATE:
    case CKM_BEHAVIORSETTINGSEDITED:
    {
        CKDWORD flags = 0xFFFFFFFF; // Activate
        beh->GetLocalParameterValue(0, &flags);

        CKParameterIn *pin;

        int pcount = beh->GetInputParameterCount();
        for (int a = 0, p = 1; a < pcount; ++a, p <<= 1)
        {
            pin = beh->GetInputParameter(a);
            if (!pin)
                continue;
            pin->Enable((flags & p) ? TRUE : FALSE);
        }
    }
    break;
    }
    return CKBR_OK;
}
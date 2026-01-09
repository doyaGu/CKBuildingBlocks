/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            SetLODAttribute
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"
#include "LODManager.h"

CKObjectDeclaration *FillBehaviorSetLODAttributeDecl();
CKERROR CreateSetLODAttributeProto(CKBehaviorPrototype **pproto);
int SetLODAttribute(const CKBehaviorContext &behcontext);
CKERROR LODManagerOptionsCallBack(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetLODAttributeDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Set LOD Attribute");
    od->SetDescription("Set the LOD Attribute on an object.");
    od->SetCategory("Optimizations/Level Of Detail");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0xd360136, 0x63d0633b));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSetLODAttributeProto);
    od->SetCompatibleClassId(CKCID_3DENTITY);
    return od;
}

CKERROR CreateSetLODAttributeProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Set LOD Attribute");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Multiple Meshes", CKPGUID_BOOL, "FALSE");
    proto->DeclareInParameter("Character Animation", CKPGUID_BOOL, "FALSE");
    proto->DeclareInParameter("Patch Mesh", CKPGUID_BOOL, "FALSE");
    proto->DeclareInParameter("Alpha", CKPGUID_BOOL, "FALSE");
    proto->DeclareInParameter("Screen Mag", CKPGUID_PERCENTAGE, "40");
    proto->DeclareInParameter("Screen Min", CKPGUID_PERCENTAGE, "1");
    proto->DeclareInParameter("Faces Mag", CKPGUID_PERCENTAGE, "100");
    proto->DeclareInParameter("Faces Min", CKPGUID_PERCENTAGE, "3");
    proto->DeclareInParameter("Screen Alpha Mag", CKPGUID_PERCENTAGE, "10");
    proto->DeclareInParameter("Screen Alpha Min", CKPGUID_PERCENTAGE, "5");

#define CKPGUID_SETLODATTRIBUTE_SETTING CKDEFINEGUID(0x7e4417e1, 0x5d0d45b3)
    proto->DeclareSetting("Modify Parameters", CKPGUID_SETLODATTRIBUTE_SETTING, "Multiple Meshes,Character Animation,Patch Mesh,Alpha,Screen Mag,Screen Min,Face Mag,Face Min,Screen Alpha Mag,Screen Alpha Min");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(SetLODAttribute);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);
    proto->SetBehaviorCallbackFct(LODManagerOptionsCallBack);

    *pproto = proto;
    return CK_OK;
}

int SetLODAttribute(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
    if (!ent)
        return CKBR_OK;

    LODManager *lm = (LODManager *)behcontext.Context->GetManagerByGuid(LOD_MANAGER_GUID);
    if (!lm)
        return CKBR_OK;

    // Set IO states
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    LODOptions *lo = lm->GetLOD(ent);
    if (!lo)
    {
        ent->SetAttribute(lm->GetLODAttribute());
    }
    lo = lm->GetLOD(ent);
    if (!lo)
        return CKBR_OK;

    //--- Inputs
    CKBOOL multiMesh, anim, patchMesh, alpha;

    CKParameterIn *pin;
    int pindex = 0;

    // Multiple Meshes
    pin = beh->GetInputParameter(pindex);
    if (pin && pin->IsEnabled())
    {
        multiMesh = FALSE;
        pin->GetValue(&multiMesh);
        if (multiMesh)
            lo->flags |= LODMultiMesh;
        else
            lo->flags &= ~LODMultiMesh;
    }
    ++pindex;

    // Character Animation
    pin = beh->GetInputParameter(pindex);
    if (pin && pin->IsEnabled())
    {
        anim = FALSE;
        pin->GetValue(&anim);
        if (anim)
            lo->flags |= LODAnimation;
        else
            lo->flags &= ~LODAnimation;
    }
    ++pindex;

    // PatchMesh
    pin = beh->GetInputParameter(pindex);
    if (pin && pin->IsEnabled())
    {
        patchMesh = FALSE;
        pin->GetValue(&patchMesh);
        if (patchMesh)
            lo->flags |= LODPatchMesh;
        else
            lo->flags &= ~LODPatchMesh;
    }
    ++pindex;

    // Alpha
    pin = beh->GetInputParameter(pindex);
    if (pin && pin->IsEnabled())
    {
        alpha = FALSE;
        pin->GetValue(&alpha);
        if (alpha)
            lo->flags |= LODAlpha;
        else
            lo->flags &= ~LODAlpha;
    }
    ++pindex;

    // Screen Mag
    pin = beh->GetInputParameter(pindex);
    if (pin && pin->IsEnabled())
    {
        lo->screenMag = 0.4f;
        pin->GetValue(&lo->screenMag);
    }
    ++pindex;

    // Screen Mag
    pin = beh->GetInputParameter(pindex);
    if (pin && pin->IsEnabled())
    {
        lo->screenMin = 0.01f;
        pin->GetValue(&lo->screenMin);
    }
    ++pindex;

    // Faces Mag
    pin = beh->GetInputParameter(pindex);
    if (pin && pin->IsEnabled())
    {
        lo->facesMag = 1.0f;
        pin->GetValue(&lo->facesMag);
    }
    ++pindex;

    // Faces Min
    pin = beh->GetInputParameter(pindex);
    if (pin && pin->IsEnabled())
    {
        lo->facesMin = 0.03f;
        pin->GetValue(&lo->facesMin);
    }
    ++pindex;

    // Screen Alpha Mag
    pin = beh->GetInputParameter(pindex);
    if (pin && pin->IsEnabled())
    {
        lo->alphaScreenMag = 0.1f;
        pin->GetValue(&lo->alphaScreenMag);
    }
    ++pindex;

    // Screen Alpha Min
    pin = beh->GetInputParameter(pindex);
    if (pin && pin->IsEnabled())
    {
        lo->alphaScreenMin = 0.05f;
        pin->GetValue(&lo->alphaScreenMin);
    }
    ++pindex;

    return CKBR_OK;
}

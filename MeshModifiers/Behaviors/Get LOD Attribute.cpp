/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            GetLODAttribute
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"
#include "LODManager.h"

CKObjectDeclaration *FillBehaviorGetLODAttributeDecl();
CKERROR CreateGetLODAttributeProto(CKBehaviorPrototype **pproto);
int GetLODAttribute(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorGetLODAttributeDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Get LOD Attribute");
    od->SetDescription("Get the LOD Attribute parameters applied to an object.");
    od->SetCategory("Optimizations/Level Of Detail");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x62bf39fa, 0x53e264c6));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateGetLODAttributeProto);
    od->SetCompatibleClassId(CKCID_3DENTITY);
    return od;
}

CKERROR CreateGetLODAttributeProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Get LOD Attribute");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("LOD Attribute Found");
    proto->DeclareOutput("LOD Attribute Not Found");

    proto->DeclareOutParameter("Multiple Meshes", CKPGUID_BOOL);
    proto->DeclareOutParameter("Character Animation", CKPGUID_BOOL);
    proto->DeclareOutParameter("Patch Mesh", CKPGUID_BOOL);
    proto->DeclareOutParameter("Alpha", CKPGUID_BOOL);
    proto->DeclareOutParameter("Screen Mag", CKPGUID_PERCENTAGE);
    proto->DeclareOutParameter("Screen Min", CKPGUID_PERCENTAGE);
    proto->DeclareOutParameter("Faces Mag", CKPGUID_PERCENTAGE);
    proto->DeclareOutParameter("Faces Min", CKPGUID_PERCENTAGE);
    proto->DeclareOutParameter("Screen Alpha Mag", CKPGUID_PERCENTAGE);
    proto->DeclareOutParameter("Screen Alpha Min", CKPGUID_PERCENTAGE);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(GetLODAttribute);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int GetLODAttribute(const CKBehaviorContext &behcontext)
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

    LODOptions *lo = lm->GetLOD(ent);
    if (!lo)
    {
        beh->ActivateOutput(1); // Not Found
        return CKBR_OK;
    }
    beh->ActivateOutput(0); // Found

    //--- Output Parameters

    // booleans
    CKBOOL multiMesh = (lo->flags & LODMultiMesh) ? TRUE : FALSE;
    beh->SetOutputParameterValue(0, &multiMesh);

    CKBOOL anim = (lo->flags & LODAnimation) ? TRUE : FALSE;
    beh->SetOutputParameterValue(1, &anim);

    CKBOOL patchMesh = (lo->flags & LODPatchMesh) ? TRUE : FALSE;
    beh->SetOutputParameterValue(2, &patchMesh);

    CKBOOL alpha = (lo->flags & LODAlpha) ? TRUE : FALSE;
    beh->SetOutputParameterValue(3, &alpha);

    // floats
    beh->SetOutputParameterValue(4, &lo->screenMag);
    beh->SetOutputParameterValue(5, &lo->screenMin);
    beh->SetOutputParameterValue(6, &lo->facesMag);
    beh->SetOutputParameterValue(7, &lo->facesMin);
    beh->SetOutputParameterValue(8, &lo->alphaScreenMag);
    beh->SetOutputParameterValue(8, &lo->alphaScreenMin);

    return CKBR_OK;
}

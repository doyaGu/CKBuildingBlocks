/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            AnimationLOD
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorAnimationLODDecl();
CKERROR CreateAnimationLODProto(CKBehaviorPrototype **);
int AnimationLOD(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorAnimationLODDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("LOD Character Animation");
    od->SetDescription("Sets the level of detail for character animations.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Min Distance: </SPAN>minimum distance from which the most detailled mesh is displayed.<BR>
    <SPAN CLASS=pin>Max Distance: </SPAN>maximum distance after which the least detailled mesh is displayed.<BR>
    <SPAN CLASS=pin>Minimum: </SPAN>minimum percentage of animation processed, 0 means only the root and the floor
    ref (if available) get animated.<BR>
    <BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x200d6501, 0x4cf61e52));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateAnimationLODProto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    od->SetCategory("Optimizations/Level Of Detail");
    return od;
}

CKERROR CreateAnimationLODProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("LOD Character Animation");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Distance Min", CKPGUID_FLOAT, "100");
    proto->DeclareInParameter("Distance Max", CKPGUID_FLOAT, "1000");
    proto->DeclareInParameter("Minimum", CKPGUID_PERCENTAGE, "0");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(AnimationLOD);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int AnimationLOD(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // SET IO
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CKCharacter *carac = (CKCharacter *)beh->GetTarget();
    if (!carac)
        return CKBR_OWNERERROR;

    float distmin = 100.0f;
    beh->GetInputParameterValue(0, &distmin);

    float distmax = 1000.0f;
    beh->GetInputParameterValue(1, &distmax);

    float LOD = 1.0f;
    beh->GetInputParameterValue(2, &LOD);

    // we get the viewer position
    CK3dEntity *cam = behcontext.CurrentRenderContext->GetViewpoint();

    VxVector campos;
    carac->GetPosition(&campos, cam);

    float distance = Magnitude(campos);

    float index = 1.0f - (distance - distmin) / (distmax - distmin);
    XThreshold(index, LOD, 1.0f);

    carac->SetAnimationLevelOfDetail(index);

    return CKBR_OK;
}

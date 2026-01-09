/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		       SetBlendedAnimationFactor
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorSetMergedAnimationFactorDecl();
CKERROR CreateSetMergedAnimationFactorProto(CKBehaviorPrototype **pproto);
int SetMergedAnimationFactor(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetMergedAnimationFactorDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Set Blended Animation Factor");
    od->SetDescription("Sets the coefficient of interpolation of a blended animation");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Animation: </SPAN>specify the blended animation to change.<BR>
    <SPAN CLASS=pin>Factor: </SPAN>interpolation factor.<BR>
    <BR>
    A blended animation is an interpolation of two animations. The factor passed as input parameter
    specifies the influence each animation.<BR>
    A factor of 0 sets this animation to be equal to the first animation.<BR>
    A factor of 50 sets this animations to be a fair blending of the two animations.<BR>
    <BR>
    See Also: "Create Blended Animation".<BR>
    */
    /* warning:
    - also Remember: PERCENTAGE values are compatible with FLOAT values, and ANGLE values.<BR>
    (for the 'Progression' output parameter)<BR>
    */
    od->SetCategory("Characters/Animation");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x7afc20a3, 0x6d367a99));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSetMergedAnimationFactorProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);

    return od;
}

CKERROR CreateSetMergedAnimationFactorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Set Blended Animation Factor");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Animation", CKPGUID_ANIMATION);
    proto->DeclareInParameter("Factor", CKPGUID_PERCENTAGE, "50");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(SetMergedAnimationFactor);

    *pproto = proto;
    return CK_OK;
}

int SetMergedAnimationFactor(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CKKeyedAnimation *anim = (CKKeyedAnimation *)beh->GetInputParameterObject(0);
    float factor = 0.5f;
    beh->GetInputParameterValue(1, &factor);

    if (anim)
        anim->SetMergeFactor(factor);

    beh->ActivateOutput(0);
    return CKBR_OK;
}
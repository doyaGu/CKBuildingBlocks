/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            CreateBlendedAnimation2
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorCreateBlendedAnimation2Decl();
CKERROR CreateCreateBlendedAnimation2Proto(CKBehaviorPrototype **pproto);
int CreateBlendedAnimation2(const CKBehaviorContext &context);

CKObjectDeclaration *FillBehaviorCreateBlendedAnimation2Decl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Create Blended Animation 2");
    od->SetDescription("Creates a new animation by merging two exsiting animations.");
    /* rem:
    <BR>
    <SPAN CLASS=pin>Animation 0: </SPAN>specifies the first animation to use.<BR>
    <SPAN CLASS=pin>Animation 1: </SPAN>specifies the second animation to use.<BR>
    <BR>
    <SPAN CLASS=pout>Blended Animation: </SPAN>animation created.<BR>
    <BR>
    The new animation is created the first time the behavior is edited. Its settings will
    only change if the input parameters are edited. Once created the animation can be controlled
    with the behavior "Set Blended Animation Factor" which will specify the influence of each of the two
    animations for the final result.<BR>
    <BR>
    See Also: "Set Blended Animation Factor".<BR>
    */
    /* warning:
    - Once a blended animation is created, it is attached to the owner character.
    The created animation is stored as any other animation on the character.<BR>
    - Once triggered one time the building block can be safely removed.
    */
    od->SetCategory("Characters/Animation");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x56864d6d, 0x159b5d1e));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateCreateBlendedAnimation2Proto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    return od;
}

CKERROR CreateCreateBlendedAnimation2Proto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Create Blended Animation 2");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("Create");
    proto->DeclareOutput("Exit Created");

    proto->DeclareOutParameter("Blended Animation", CKPGUID_ANIMATION);

    proto->DeclareInParameter("Animation 0", CKPGUID_ANIMATION);
    proto->DeclareInParameter("Animation 1", CKPGUID_ANIMATION);

    proto->DeclareSetting("Dynamic", CKPGUID_BOOL, "TRUE");
    proto->DeclareSetting("Add to character", CKPGUID_BOOL, "TRUE");

    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);
    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(CreateBlendedAnimation2);

    *pproto = proto;
    return CK_OK;
}

int CreateBlendedAnimation2(const CKBehaviorContext &context)
{
    CKBehavior *beh = context.Behavior;
    CKCharacter *owner = (CKCharacter *)beh->GetTarget();

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CKKeyedAnimation *anim1 = (CKKeyedAnimation *)beh->GetInputParameterObject(0);
    CKKeyedAnimation *anim2 = (CKKeyedAnimation *)beh->GetInputParameterObject(1);

    CKBOOL Dynamic = TRUE;
    beh->GetLocalParameterValue(0, &Dynamic);

    CKBOOL AddAnimation = TRUE;
    beh->GetLocalParameterValue(1, &AddAnimation);

    CKKeyedAnimation *res = (CKKeyedAnimation *)beh->GetOutputParameterObject(0);
    if (anim1 && anim2)
    {
        res = (CKKeyedAnimation *)anim1->CreateMergedAnimation(anim2, Dynamic);
    }

    if (res && owner)
    {
        if (AddAnimation)
        {
            owner->AddAnimation(res);
        }
    }

    beh->SetOutputParameterObject(0, res);

    return CKBR_OK;
}

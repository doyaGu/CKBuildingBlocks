/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            CreateBlendedAnimation
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorCreateMergedAnimationDecl();
CKERROR CreateCreateMergedAnimationProto(CKBehaviorPrototype **pproto);
CKERROR CreateMergedAnimationCB(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorCreateMergedAnimationDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Create Blended Animation");
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
    od->SetGuid(CKGUID(0x248a56ed, 0x2c232967));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateCreateMergedAnimationProto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    return od;
}

CKERROR CreateCreateMergedAnimationProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Create Blended Animation");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareOutParameter("Blended Animation", CKPGUID_ANIMATION);

    proto->DeclareInParameter("Animation 0", CKPGUID_ANIMATION);
    proto->DeclareInParameter("Animation 1", CKPGUID_ANIMATION);

    proto->SetBehaviorCallbackFct(CreateMergedAnimationCB);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_OBSOLETE);
    //	proto->SetFunction(NULL);

    *pproto = proto;
    return CK_OK;
}

CKERROR CreateMergedAnimationCB(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIOREDITED:
    {
        CKKeyedAnimation *anim1 = (CKKeyedAnimation *)beh->GetInputParameterObject(0);
        CKKeyedAnimation *anim2 = (CKKeyedAnimation *)beh->GetInputParameterObject(1);
        CKKeyedAnimation *res = (CKKeyedAnimation *)beh->GetOutputParameterObject(0);
        CKCharacter *owner = (CKCharacter *)beh->GetOwner();
        if (!res)
            if (anim1 && anim2)
            {
                res = (CKKeyedAnimation *)anim1->CreateMergedAnimation(anim2);
                // res->GetBodyPartList();
                if (res)
                    owner->AddAnimation(res);

                beh->SetOutputParameterObject(0, res);
            }
    }
    break;
    }
    return CKBR_OK;
}

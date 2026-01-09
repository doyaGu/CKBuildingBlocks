/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            SetAnimationStep
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"
CKObjectDeclaration *FillBehaviorSetAnimationStepDecl();
CKERROR CreateSetAnimationStepProto(CKBehaviorPrototype **pproto);
int SetAnimationStep(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetAnimationStepDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Set Animation Step");
    od->SetDescription("Places the character in a specified frame of an animation cycle.");
    /* rem:
    <SPAN CLASS=in>In : </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out : </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Step : </SPAN>advancement in percentage of the animation (between 0 and 100).<BR>
    <SPAN CLASS=pin>Animation : </SPAN>specify which animation should be used.<BR>
    <BR>
    You should use this behavior to have an accurate control on the character's animation.
    */
    /* warning:
    - as there are no 'Hierarchy' input parameter, all the object's children will stay parented during the motion.
    Therefore you can't ask a object to move without moving its children.<BR>
    */
    od->SetCategory("Characters/Animation");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x120127aa, 0x960added));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSetAnimationStepProto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    return od;
}

CKERROR CreateSetAnimationStepProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Set Animation Step");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Step", CKPGUID_PERCENTAGE, "0");
    proto->DeclareInParameter("Animation", CKPGUID_ANIMATION);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(SetAnimationStep);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int SetAnimationStep(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CKCharacter *character = (CKCharacter *)beh->GetTarget();
    if (!character)
        return CKBR_OWNERERROR;

    // Set IO states
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    // Get step
    float step = 0.0f;
    beh->GetInputParameterValue(0, &step);

    // Get animation
    CKAnimation *anim = (CKAnimation *)beh->GetInputParameterObject(1);

    if (anim)
        anim->SetStep(step);

    return CKBR_OK;
}

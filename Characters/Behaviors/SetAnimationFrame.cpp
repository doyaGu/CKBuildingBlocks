/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            SetAnimationFrame
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorSetAnimationFrameDecl();
CKERROR CreateSetAnimationFrameProto(CKBehaviorPrototype **pproto);
int SetAnimationFrame(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetAnimationFrameDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Set Animation Frame");
    od->SetDescription("Places the character in a specified frame of an animation cycle.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Frame: </SPAN>frame of the animation cycle to be selected.<BR>
    <SPAN CLASS=pin>Animation: </SPAN>specify which animation should be used.<BR>
    <BR>
    If <SPAN CLASS=pin>Frame</SPAN> is a float value, for example 3.5, the result will be an interpolation between the existing frames of the animation (i.e. frames 3 and 4).<BR>
    This behavior can be used for more accurate control of an character's animation.<BR>
    */
    /* warning:
    - as there are no 'Hierarchy' input parameter, all the object's children will stay parented during the motion.
    Therefore you can't ask a object to move without moving its children.<BR>
    */
    od->SetCategory("Characters/Animation");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x0e579636, 0xa3b6e7f0));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSetAnimationFrameProto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    return od;
}

CKERROR CreateSetAnimationFrameProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Set Animation Frame");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Frame", CKPGUID_FLOAT);
    proto->DeclareInParameter("Animation", CKPGUID_ANIMATION);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(SetAnimationFrame);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int SetAnimationFrame(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CKCharacter *character = (CKCharacter *)beh->GetTarget();
    if (!character)
        return CKBR_OWNERERROR;

    // Set IO states
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    // Get frame number
    float frame = 0;
    beh->GetInputParameterValue(0, &frame);

    // Get animation
    CKAnimation *anim = (CKAnimation *)beh->GetInputParameterObject(1);
    if (anim)
        anim->SetFrame(frame);

    return CKBR_OK;
}

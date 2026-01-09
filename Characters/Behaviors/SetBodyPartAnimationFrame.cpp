/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            SetBodyPartAnimationFrame
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorSetBodypartAnimationFrameDecl();
CKERROR CreateSetBodypartAnimationFrameProto(CKBehaviorPrototype **pproto);
int SetBodypartAnimationFrame(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetBodypartAnimationFrameDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Set Bodypart Animation Frame");
    od->SetDescription("Places a character's body part in a specified frame of an animation cycle.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Frame: </SPAN>frame of the animation cycle to be selected.<BR>
    <SPAN CLASS=pin>Animation: </SPAN>specifies which animation should be used.<BR>
    <BR>
    If <SPAN CLASS=pin>Frame</SPAN> is a float value, for example 3.5, the result will be an interpolation between the existing frames of the animation (i.e. frames 3 and 4).<BR>
    This behavior can be used for more accurate control of an animation performed by a character's body
    part. When setting the frame position for a body part animation, other animations may modify the body
    part's position or orientation. To disable a possible conflict from other animations, the ExludeFromAnimation
    behavior should be used before this one. The ExludeFromAnimation behavior will not prevent this building block
    (Set Bodypart Animation Frame) from setting a current animation frame.<BR>
    */
    /* warning:
    - as there are no 'Hierarchy' input parameter, all the object's children will stay parented during the motion.
    Therefore you can't ask a object to move without moving its children.<BR>
    */
    od->SetCategory("Characters/Animation");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x55f045c7, 0x6cb3417b));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSetBodypartAnimationFrameProto);
    od->SetCompatibleClassId(CKCID_BODYPART);
    return od;
}

CKERROR CreateSetBodypartAnimationFrameProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Set Bodypart Animation Frame");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Frame", CKPGUID_FLOAT);
    proto->DeclareInParameter("Animation", CKPGUID_ANIMATION);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(SetBodypartAnimationFrame);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int SetBodypartAnimationFrame(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CKBodyPart *body = (CKBodyPart *)beh->GetTarget();

    // Set IO states
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    // Get frame number
    float frame = 0;
    beh->GetInputParameterValue(0, &frame);

    // Get animation
    CKAnimation *anim = (CKAnimation *)beh->GetInputParameterObject(1);
    if (!anim)
        return CKBR_PARAMETERERROR;

    if (anim->GetClassID() == CKCID_KEYEDANIMATION)
    {
        CKObjectAnimation *oanim = ((CKKeyedAnimation *)anim)->GetAnimation(body);
        if (oanim)
            oanim->SetFrame(frame, (CKKeyedAnimation *)CKANIMATION_FORCESETSTEP);
    }
    else
        return CKBR_PARAMETERERROR;

    return CKBR_OK;
}

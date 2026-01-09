/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            CharacterController
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateCharacterControllerBehaviorProto(CKBehaviorPrototype **pproto);
int DoCharacterController(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorCharacterControllerDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Character Controller");
    od->SetDescription("Simple control of a character with 4 animations by joystick or keyboard.");
    /* rem:
    <SPAN CLASS=in>On: </SPAN>activates the process.<BR>
    <SPAN CLASS=in>Off: </SPAN>deactivates the process.<BR>
    <BR>
    <SPAN CLASS=pin>Stand Animation: </SPAN>default animation, played when no other animation is in play.<BR>
    <SPAN CLASS=pin>Walk Animation: </SPAN>walk animation played when 'Joy_Up' message is received.<BR>
    <SPAN CLASS=pin>Walk Backward Animation: </SPAN>backwards walk animation played when 'Joy_Down' message is received.<BR>
    <SPAN CLASS=pin>Run Animation: </SPAN>running animation played when 'Joy_Button1' and 'Joy_Up' messages are received.<BR>
    <BR>
    This building block controls a character by using animations provided in response to messages from a joystick or keyboard.<BR>
    <BR>
    See Also: "Enhanced Character Controller".<BR>
    */
    /* warning:
    - This building block is intended to be used in conjunction with the "Keyboard Controller" BB or "Joystick Controller" BB, as they send
    appropriate messages that makes the "Character Controller" BB react (but in fact any correct message sending will make the "Character Controller" BB react).<BR>
    */
    od->SetCategory("Characters/Movement");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x12874356, 0x12874356));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateCharacterControllerBehaviorProto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    return od;
}

CKERROR DaCharacterCB(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKMessageManager *mm = behcontext.MessageManager;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORREADSTATE:
    case CKM_BEHAVIORLOAD:
    case CKM_BEHAVIORCREATE:
    {
        int tab[7];
        tab[0] = mm->AddMessageType("Joy_Left");
        tab[1] = mm->AddMessageType("Joy_Right");
        tab[2] = mm->AddMessageType("Joy_Up");
        tab[3] = mm->AddMessageType("Joy_Down");
        tab[4] = mm->AddMessageType("Joy_Button1");
        tab[5] = mm->AddMessageType("Joy_Button2");
        tab[6] = 0;

        beh->SetLocalParameterValue(0, (void*)tab, 7 * sizeof(int));

        CKCharacter *carac = (CKCharacter *)beh->GetOwner();
        if (!carac)
            return CKBR_OWNERERROR;
        carac->SetAsWaitingForMessages();

        return CKBR_OK;
    }
    break;

    case CKM_BEHAVIORATTACH:
    {
        CKCharacter *carac = (CKCharacter *)beh->GetOwner();
        if (!carac)
            return CKBR_OWNERERROR;
        carac->SetAsWaitingForMessages();
    }
    break;

    case CKM_BEHAVIORDETACH:
    {
        CKCharacter *carac = (CKCharacter *)beh->GetOwner();
        if (!carac)
            return CKBR_OWNERERROR;

        carac->SetAsWaitingForMessages(FALSE);
        carac->SetAutomaticProcess(FALSE);
    }
    break;

    case CKM_BEHAVIORDELETE:
    {
        CKDWORD d = 0;
        beh->SetLocalParameterValue(0, &d, sizeof(CKDWORD));
        return CKBR_OK;
    }
    break;
    }
    return CKBR_OK;
}

CKERROR CreateCharacterControllerBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Character Controller");
    if (!proto)
        return CKERR_OUTOFMEMORY;
    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS)(CKBEHAVIOR_ACTIVE | CKBEHAVIOR_MESSAGERECEIVER));

    proto->DeclareInput("On");
    proto->DeclareInput("Off");

    proto->DeclareInParameter("Stand Animation", CKPGUID_ANIMATION);
    proto->DeclareInParameter("Walk Animation", CKPGUID_ANIMATION);
    proto->DeclareInParameter("Walk Backward Animation", CKPGUID_ANIMATION);
    proto->DeclareInParameter("Run Animation", CKPGUID_ANIMATION);

    proto->DeclareLocalParameter(NULL, CKPGUID_VOIDBUF); //"Messages"

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);

    proto->SetFunction(DoCharacterController);
    proto->SetBehaviorCallbackFct(DaCharacterCB);

    *pproto = proto;
    return CK_OK;
}

int DoCharacterController(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CKCharacter *carac = (CKCharacter *)beh->GetOwner();
    if (beh->IsInputActive(1))
    { // enter by OFF
        beh->ActivateInput(1, FALSE);
        carac->SetAutomaticProcess(FALSE);
        return CKBR_OK;
    }
    else
    { // enter by ON
        beh->ActivateInput(0, FALSE);
    }

    carac->SetAutomaticProcess(TRUE);

    // Get animations
    CKAnimation *animfixe = (CKAnimation *)beh->GetInputParameterObject(0);
    CKAnimation *animwalk = (CKAnimation *)beh->GetInputParameterObject(1);
    CKAnimation *animwalkback = (CKAnimation *)beh->GetInputParameterObject(2);
    CKAnimation *animrun = (CKAnimation *)beh->GetInputParameterObject(3);

    if (animrun)
        animrun->SetCanBeInterrupt(FALSE);

    CKParameterLocal *varray = NULL;
    int *tab = (int *)beh->GetLocalParameterReadDataPtr(0);

    if (!tab)
        return CKBR_PARAMETERERROR;
    CKBOOL joy1 = FALSE, joy2 = FALSE, joy3 = FALSE, joy4 = FALSE, joyleft = FALSE, joyright = FALSE, joyup = FALSE, joydown = FALSE;

    for (int i = 0; i < carac->GetLastFrameMessageCount(); i++)
    {
        CKMessage *msg = carac->GetLastFrameMessage(i);
        if (msg)
        {
            int type = msg->GetMsgType();
            if (type == tab[0])
                joyleft = TRUE;
            else if (type == tab[1])
                joyright = TRUE;
            else if (type == tab[2])
                joyup = TRUE;
            else if (type == tab[3])
                joydown = TRUE;
            else if (type == tab[4])
                joy1 = TRUE;
            else if (type == tab[5])
                joy2 = TRUE;
        }
    }

    if (joy1)
        carac->SetNextActiveAnimation(animrun, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);
    else if (joyup)
        carac->SetNextActiveAnimation(animwalk, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);
    else if (joydown)
        carac->SetNextActiveAnimation(animwalkback, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);
    else
        carac->SetNextActiveAnimation(animfixe, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);

    float m_FrameRate = carac->GetActiveAnimation() ? carac->GetActiveAnimation()->GetLinkedFrameRate() : 30.0f;
    VxVector up(0.0f, 1.0f, 0.0f);
    if (joyright)
        carac->Rotate(&up, -0.06f * (behcontext.DeltaTime * m_FrameRate) / 1000.0f);
    else if (joyleft)
        carac->Rotate(&up, +0.06f * (behcontext.DeltaTime * m_FrameRate) / 1000.0f);

    return CKBR_ACTIVATENEXTFRAME;
}

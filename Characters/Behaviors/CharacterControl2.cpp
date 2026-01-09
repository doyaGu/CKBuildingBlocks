/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            CharacterController
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateEnhancedCharacterControllerBehaviorProto(CKBehaviorPrototype **pproto);
int DoEnhancedCharacterController(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorEnhancedCharacterControllerDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Enhanced Character Controller");
    od->SetDescription("Controls a character by using 7 animations reacting to a joystick or keyboard.");
    /* rem:
    <SPAN CLASS=in>On: </SPAN>activates the behavior.<BR>
    <SPAN CLASS=in>Off: </SPAN>deactivates the behavior.<BR>
    <BR>
    <SPAN CLASS=pin>Stand Animation: </SPAN>default animation when no other is in play.<BR>
    <SPAN CLASS=pin>Walk Animation: </SPAN>walk animation played when 'Joy_Up' message is received.<BR>
    <SPAN CLASS=pin>Walk Backward Animation: </SPAN>backwards walk animation played when 'Joy_Down' message is received.<BR>
    <SPAN CLASS=pin>Run Animation: </SPAN>run animation played when 'Joy_Button1' and 'Joy_Up' messages are received.<BR>
    <SPAN CLASS=pin>Creep Animation: </SPAN>creep animation played when 'Joy_Button2' and 'Joy_Up' messages are received.<BR>
    <SPAN CLASS=pin>Jump Animation: </SPAN>jump animation played when 'Joy_Button3' and 'Joy_Up' messages are received.<BR>
    <SPAN CLASS=pin>Dance Animation: </SPAN>dance animation played when 'Joy_Button4' and 'Joy_Up' messages are received.<BR>
    <BR>
    This behavior controls a character by using animations provided in response to messages from a joystick or keyboard.<BR>
    <BR>
    See Also: "Character Controller".<BR>
    */
    /* warning:
    - This building block is intended to be used in conjunction with the "Keyboard Controller" BB or "Joystick Controller" BB, as they send
    appropriate messages that makes the "Character Controller" BB react (but in fact any correct message sending will make the "Character Controller" BB react).<BR>
    */
    od->SetCategory("Characters/Movement");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x44433224, 0x12874356));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateEnhancedCharacterControllerBehaviorProto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    return od;
}

CKERROR DaEnhancedCharacterCB(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORREADSTATE:
    case CKM_BEHAVIORLOAD:
    case CKM_BEHAVIORCREATE:
    {
        CKMessageManager *mm = behcontext.MessageManager;

        int tab[11];
        tab[0] = mm->AddMessageType("Joy_Left");
        tab[1] = mm->AddMessageType("Joy_Right");
        tab[2] = mm->AddMessageType("Joy_Up");
        tab[3] = mm->AddMessageType("Joy_Down");
        tab[4] = mm->AddMessageType("Joy_Button1");
        tab[5] = mm->AddMessageType("Joy_Button2");
        tab[6] = mm->AddMessageType("Joy_Button3");
        tab[7] = mm->AddMessageType("Joy_Button4");
        tab[8] = mm->AddMessageType("Joy_Button5");
        tab[9] = mm->AddMessageType("Joy_Button6");
        tab[10] = 0;

        beh->SetLocalParameterValue(0, (void *)tab, 11 * sizeof(int));

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
        return CKBR_OK;
    }
    break;

    case CKM_BEHAVIORDETACH:
    {
        CKCharacter *carac = (CKCharacter *)beh->GetOwner();
        if (!carac)
            return CKBR_OWNERERROR;

        carac->SetAsWaitingForMessages(FALSE);
        carac->SetAutomaticProcess(FALSE);

        return CKBR_OK;
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

CKERROR CreateEnhancedCharacterControllerBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Enhanced Character Controller");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("On");
    proto->DeclareInput("Off");

    proto->DeclareInParameter("Stand Animation", CKPGUID_ANIMATION);
    proto->DeclareInParameter("Walk Animation (Up)", CKPGUID_ANIMATION);
    proto->DeclareInParameter("Walk Backward Animation (Down)", CKPGUID_ANIMATION);
    proto->DeclareInParameter("Run Animation (B1)", CKPGUID_ANIMATION);
    proto->DeclareInParameter("Creep Animation (B2)", CKPGUID_ANIMATION);
    proto->DeclareInParameter("Jump Animation (B3)", CKPGUID_ANIMATION);
    proto->DeclareInParameter("Dance Animation (B4)", CKPGUID_ANIMATION);

    proto->DeclareLocalParameter(NULL, CKPGUID_VOIDBUF); //"Messages"

    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS)(CKBEHAVIOR_ACTIVE | CKBEHAVIOR_MESSAGERECEIVER));
    proto->SetFlags(CK_BEHAVIORPROTOTYPE_OBSOLETE);

    proto->SetFunction(DoEnhancedCharacterController);
    proto->SetBehaviorCallbackFct(DaEnhancedCharacterCB);

    *pproto = proto;
    return CK_OK;
}

int DoEnhancedCharacterController(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // Set IO states
    CKCharacter *carac = (CKCharacter *)beh->GetOwner();
    if (beh->IsInputActive(1))
    { // enter by Off
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
    CKAnimation *animcreep = (CKAnimation *)beh->GetInputParameterObject(4);
    CKAnimation *animjump = (CKAnimation *)beh->GetInputParameterObject(5);
    if (animjump)
        animjump->SetCanBeInterrupt(FALSE);
    CKAnimation *animdance = (CKAnimation *)beh->GetInputParameterObject(6);
    if (animdance)
        animdance->SetCanBeInterrupt(FALSE);

    int *tab = (int *)beh->GetLocalParameterReadDataPtr(0);

    CKBOOL joy1 = FALSE, joy2 = FALSE, joy3 = FALSE, joy4 = FALSE, joy5 = FALSE, joy6 = FALSE, joyleft = FALSE, joyright = FALSE, joyup = FALSE, joydown = FALSE;

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
            else if (type == tab[6])
                joy3 = TRUE;
            else if (type == tab[7])
                joy4 = TRUE;
            else if (type == tab[8])
                joy5 = TRUE;
            else if (type == tab[9])
                joy6 = TRUE;
        }
    }

    if (joy1)
        carac->SetNextActiveAnimation(animrun, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);
    else if (joy2)
        carac->SetNextActiveAnimation(animcreep, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);
    else if (joy3)
        carac->SetNextActiveAnimation(animjump, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);
    else if (joyup)
        carac->SetNextActiveAnimation(animwalk, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);
    else if (joydown)
        carac->SetNextActiveAnimation(animwalkback, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);
    else if (joy4)
        carac->SetNextActiveAnimation(animdance, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);
    else
        carac->SetNextActiveAnimation(animfixe, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);

    float m_FrameRate = carac->GetActiveAnimation() ? carac->GetActiveAnimation()->GetLinkedFrameRate() : 30.0f;
    VxVector up = VxVector::axisY();
    if (joyright)
        carac->Rotate(&up, -0.06f * (behcontext.DeltaTime * m_FrameRate) / 1000.0f);
    else if (joyleft)
        carac->Rotate(&up, +0.06f * (behcontext.DeltaTime * m_FrameRate) / 1000.0f);

    return CKBR_ACTIVATENEXTFRAME;
}

#include "CKAll.h"
#include "BallanceMessages.h"

CKObjectDeclaration *FillBehaviorBallanceEventRouterDecl();
CKERROR CreateBallanceEventRouterProto(CKBehaviorPrototype **pproto);
int BallanceEventRouter(const CKBehaviorContext &behcontext);
CKERROR BallanceEventRouterCallback(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorBallanceEventRouterDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Ballance Event Router");
    od->SetDescription("Routes the global Ballance runtime messages into named gameplay outputs.");
    od->SetCategory("Ballance/Runtime");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x42414c05, 0x10000005));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Ballance");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateBallanceEventRouterProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateBallanceEventRouterProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Ballance Event Router");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("On");
    proto->DeclareInput("Off");

    proto->DeclareOutput("Load Menu Level");
    proto->DeclareOutput("Exit To System");
    proto->DeclareOutput("Load Level");
    proto->DeclareOutput("Start Level");
    proto->DeclareOutput("Reset Level");
    proto->DeclareOutput("Pause Level");
    proto->DeclareOutput("Unpause Level");
    proto->DeclareOutput("Exit Level");
    proto->DeclareOutput("Next Level");
    proto->DeclareOutput("Show Cursor");
    proto->DeclareOutput("Highscore");

    proto->DeclareInParameter("Use base.cmo Message IDs", CKPGUID_BOOL, "TRUE");
    proto->DeclareInParameter("Load Menu Level Message", CKPGUID_MESSAGE, "Ballance_LoadMenuLevel");
    proto->DeclareInParameter("Exit To System Message", CKPGUID_MESSAGE, "Ballance_ExitToSystem");
    proto->DeclareInParameter("Load Level Message", CKPGUID_MESSAGE, "Ballance_LoadLevel");
    proto->DeclareInParameter("Start Level Message", CKPGUID_MESSAGE, "Ballance_StartLevel");
    proto->DeclareInParameter("Reset Level Message", CKPGUID_MESSAGE, "Ballance_ResetLevel");
    proto->DeclareInParameter("Pause Level Message", CKPGUID_MESSAGE, "Ballance_PauseLevel");
    proto->DeclareInParameter("Unpause Level Message", CKPGUID_MESSAGE, "Ballance_UnpauseLevel");
    proto->DeclareInParameter("Exit Level Message", CKPGUID_MESSAGE, "Ballance_ExitLevel");
    proto->DeclareInParameter("Next Level Message", CKPGUID_MESSAGE, "Ballance_NextLevel");
    proto->DeclareInParameter("Show Cursor Message", CKPGUID_MESSAGE, "Ballance_ShowCursor");
    proto->DeclareInParameter("Highscore Message", CKPGUID_MESSAGE, "Ballance_Highscore");

    proto->DeclareOutParameter("Received Message", CKPGUID_INT);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetBehaviorFlags(CKBEHAVIOR_MESSAGERECEIVER);
    proto->SetFunction(BallanceEventRouter);
    proto->SetBehaviorCallbackFct(BallanceEventRouterCallback);

    *pproto = proto;
    return CK_OK;
}

int BallanceEventRouter(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKBeObject *owner = (CKBeObject *)beh->GetOwner();

    if (beh->IsInputActive(1))
    {
        beh->ActivateInput(1, FALSE);
        return CKBR_OK;
    }

    if (beh->IsInputActive(0))
        beh->ActivateInput(0, FALSE);

    if (!owner)
        return CKBR_OWNERERROR;

    const int messageCount = owner->GetLastFrameMessageCount();
    CKBOOL useBaseIds = TRUE;
    beh->GetInputParameterValue(0, &useBaseIds);

    const int outputCount = beh->GetOutputCount();
    for (int output = 0; output < outputCount; ++output)
    {
        int expected = -1;
        if (useBaseIds)
        {
            const Ballance::BaseEventMessage *event = Ballance::GetBaseEventMessage(output);
            if (event)
                expected = event->messageId;
        }
        else
        {
            beh->GetInputParameterValue(output + 1, &expected);
        }

        if (expected < 0)
            continue;

        for (int i = 0; i < messageCount; ++i)
        {
            CKMessage *message = owner->GetLastFrameMessage(i);
            if (message && message->GetMsgType() == expected)
            {
                beh->SetOutputParameterValue(0, &expected);
                beh->ActivateOutput(output);
            }
        }
    }

    return CKBR_ACTIVATENEXTFRAME;
}

CKERROR BallanceEventRouterCallback(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORATTACH:
    case CKM_BEHAVIORLOAD:
    {
        CKBeObject *owner = beh->GetOwner();
        if (owner)
            owner->SetAsWaitingForMessages(TRUE);
    }
    break;
    case CKM_BEHAVIORDETACH:
    {
        CKBeObject *owner = beh->GetOwner();
        if (owner)
            owner->SetAsWaitingForMessages(FALSE);
    }
    break;
    }
    return CKBR_OK;
}

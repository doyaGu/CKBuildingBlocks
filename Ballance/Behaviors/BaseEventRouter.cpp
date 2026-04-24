#include "CKAll.h"
#include "BallanceMessages.h"

CKObjectDeclaration *FillBehaviorBallanceBaseEventRouterDecl();
CKERROR CreateBallanceBaseEventRouterProto(CKBehaviorPrototype **pproto);
int BallanceBaseEventRouter(const CKBehaviorContext &behcontext);
CKERROR BallanceBaseEventRouterCallback(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorBallanceBaseEventRouterDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Ballance Base Event Router");
    od->SetDescription("Routes the base.cmo global event messages while preserving the original Switch On Message node signature.");
    od->SetCategory("Ballance/Runtime");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x42414c07, 0x10000007));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Ballance");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateBallanceBaseEventRouterProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateBallanceBaseEventRouterProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Ballance Base Event Router");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("On");
    proto->DeclareInput("Off");

    const int eventCount = Ballance::GetBaseEventMessageCount();
    for (int i = 0; i < eventCount; ++i)
    {
        const Ballance::BaseEventMessage *event = Ballance::GetBaseEventMessage(i);
        proto->DeclareOutput(const_cast<CKSTRING>(event->name));
        proto->DeclareInParameter(const_cast<CKSTRING>(event->parameterName), CKPGUID_MESSAGE);
    }

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS)(CKBEHAVIOR_MESSAGERECEIVER | CKBEHAVIOR_VARIABLEOUTPUTS | CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS));
    proto->SetFunction(BallanceBaseEventRouter);
    proto->SetBehaviorCallbackFct(BallanceBaseEventRouterCallback);

    *pproto = proto;
    return CK_OK;
}

int BallanceBaseEventRouter(const CKBehaviorContext &behcontext)
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
    const int outputCount = beh->GetOutputCount();
    const int parameterCount = beh->GetInputParameterCount();
    const int eventCount = Ballance::GetBaseEventMessageCount();
    const int count = outputCount < parameterCount ? (outputCount < eventCount ? outputCount : eventCount) : (parameterCount < eventCount ? parameterCount : eventCount);

    for (int output = 0; output < count; ++output)
    {
        int expected = -1;
        beh->GetInputParameterValue(output, &expected);
        if (expected < 0)
            continue;

        for (int i = 0; i < messageCount; ++i)
        {
            CKMessage *message = owner->GetLastFrameMessage(i);
            if (message && message->GetMsgType() == expected)
                beh->ActivateOutput(output);
        }
    }

    return CKBR_ACTIVATENEXTFRAME;
}

CKERROR BallanceBaseEventRouterCallback(const CKBehaviorContext &behcontext)
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

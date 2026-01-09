/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            IKPosition
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateIKBehaviorProto(CKBehaviorPrototype **pproto);
int DoIKController(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillIKControllerDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("IK Position");
    od->SetDescription("Sets a body part's position using Inverse Kinematics.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Starting Body Part: </SPAN>root of the IK. For an arm IK animation for example,
    the Starting Body Part would be the shoulder.<BR>
    <SPAN CLASS=pin>Ending Body Part: </SPAN>for an arm IK animation for example, the Ending Body Part would be the hand.<BR>
    <SPAN CLASS=pin>Entity To Follow: </SPAN>3D Entity to which the Ending Body Part will sets its position.<BR>
    <SPAN CLASS=pin>Latency: </SPAN>indicates the number of frames taken by the body part to go to the entity's position.<BR>
    <BR>
    This building block sets the position of the ending bodypart to the entity to follow.<BR>
    */
    /* warning:
    - This building block should only be used with non-animated bodyparts.<BR>
    - If an animated character moves its arm with IK, all of an arm's body parts
    should have the building block "Exclude from Animation".<BR>
    */
    od->SetCategory("Characters/IK");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0xbefc4a32, 0x893ecad4));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateIKBehaviorProto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    return od;
}

CKERROR DaIKCB(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORCREATE:
    case CKM_BEHAVIORLOAD:
    {
        CKKinematicChain *chain = (CKKinematicChain *)behcontext.Context->CreateObject(CKCID_KINEMATICCHAIN, "IKPosChain");
        beh->SetLocalParameterObject(0, (CKObject *)chain);
        return CKBR_OK;
    }
    break;

    case CKM_BEHAVIORDELETE:
    {
        CKKinematicChain *chain = (CKKinematicChain *)beh->GetLocalParameterObject(0);
        if (chain)
            CKDestroyObject((CKObject *)chain);
        return CKBR_OK;
    }
    break;
    }
    return CKBR_OK;
}

CKERROR CreateIKBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("IK Position");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Starting Body Part", CKPGUID_BODYPART);
    proto->DeclareInParameter("Ending Body Part", CKPGUID_BODYPART);
    proto->DeclareInParameter("Entity To Follow", CKPGUID_3DENTITY);
    proto->DeclareInParameter("Latency", CKPGUID_INT, "5");

    proto->DeclareLocalParameter(NULL, CKPGUID_KINEMATICCHAIN); //"Chain"

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);

    proto->SetFunction(DoIKController);
    proto->SetBehaviorCallbackFct(DaIKCB);

    *pproto = proto;
    return CK_OK;
}

int DoIKController(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // Set IO states
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CKCharacter *carac = (CKCharacter *)beh->GetOwner();

    CKKinematicChain *chain = (CKKinematicChain *)beh->GetLocalParameterObject(0);

    if (!chain)
        return CKBR_PARAMETERERROR;

    CKBodyPart *start = (CKBodyPart *)beh->GetInputParameterObject(0);
    CKBodyPart *end = (CKBodyPart *)beh->GetInputParameterObject(1);
    CK3dEntity *ent = (CK3dEntity *)beh->GetInputParameterObject(2);
    int latency;
    beh->GetInputParameterValue(3, &latency);

    if (latency == 0)
        latency = 1;

    chain->SetStartEffector(start);
    CKIkJoint joint;
    start->GetRotationJoint(&joint);
    //	joint.m_Active[0]=FALSE;
    //	joint.m_Active[1]=FALSE;
    //	joint.m_Active[2]=FALSE;
    start->SetRotationJoint(&joint);
    chain->SetEndEffector(end);

    if (ent && end)
    {
        VxVector pos, pos2;
        ent->GetPosition(&pos);
        end->GetPosition(&pos2);
        pos = pos2 + (pos - pos2) / (float)latency;
        chain->IKSetEffectorPos(&pos);
        return CKBR_OK;
    }
    return CKBR_PARAMETERERROR;
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            CharacterPathFollow
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateCharacterPathFollowBehaviorProto(CKBehaviorPrototype **pproto);
int DoCharacterPathFollow(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorCharacterPathFollowDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Character Curve Follow");
    od->SetDescription("Makes a character follow a 3D Curved.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=in>Loop In: </SPAN>triggers the next step in a process loop.<BR>
    <BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <SPAN CLASS=out>Loop Out: </SPAN>is activated when the process is not yet completed.<BR>
    <BR>
    <SPAN CLASS=pin>Curve To Follow: </SPAN>name of the curve the character will follow.<BR>
    <SPAN CLASS=pin>Start Percentage: </SPAN>where the character should start on the curve, with 0 (zero) being the beginning, 50% being midway etc.<BR>
    <SPAN CLASS=pin>Character Direction: </SPAN>defines the character's frontal direction.<BR>
    <BR>
    <SPAN CLASS=pout>Progression: </SPAN>percentage between 0% and 100% which defines the progression of the process. start=0%, middle time=50%, end=100%.
    Useful if you need to interpolates at the same time (in the same loop) a color vlaue, a vector, a orientation or something like this.<BR>
    <BR>
    This behavior sends the message "Joy_Up" to the character so that it will move along the path.<BR>
    <BR>
    See Also: "Character Path Follow 2", "Character Go To".<BR>
    */
    /* warning:
    - The required animation, walk for example, must be linked to the message "Joy_Up" using another
    building block such as Character Controller or Unlimited Controller for this behavior to work.<BR>
    - also Remember: PERCENTAGE values are compatible with FLOAT values, and ANGLE values.<BR>
    */
    od->SetCategory("Characters/Movement");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x4212cabd, 0xeafbcda6));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateCharacterPathFollowBehaviorProto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    return od;
}

CKERROR DaCharacterPFCB(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    switch (behcontext.CallbackMessage)
    {

    case CKM_BEHAVIORREADSTATE:
    case CKM_BEHAVIORCREATE:
    case CKM_BEHAVIORLOAD:
    case CKM_BEHAVIORRESET:
    {
        int tab;
        tab = behcontext.MessageManager->AddMessageType("Joy_Up");
        beh->SetLocalParameterValue(0, (void *)&tab, sizeof(int));

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

CKERROR CreateCharacterPathFollowBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Character Curve Follow");
    if (!proto)
        return CKERR_OUTOFMEMORY;
    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS)(CKBEHAVIOR_ACTIVE | CKBEHAVIOR_MESSAGESENDER));

    proto->DeclareInput("In");
    proto->DeclareInput("Loop In");

    proto->DeclareOutput("Out");
    proto->DeclareOutput("Loop Out");

    proto->DeclareInParameter("Curve To Follow", CKPGUID_CURVE);
    proto->DeclareInParameter("Start Percentage", CKPGUID_PERCENTAGE);
    proto->DeclareInParameter("Character Direction", CKPGUID_VECTOR, "1,0,0");

    proto->DeclareOutParameter("Current Percentage", CKPGUID_PERCENTAGE, "0");

    proto->DeclareLocalParameter(NULL, CKPGUID_VOIDBUF); //"Msg"
    proto->DeclareLocalParameter(NULL, CKPGUID_VECTOR);  //"Pos"
    proto->DeclareLocalParameter(NULL, CKPGUID_FLOAT);   //"Length"

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS)(CKBEHAVIOR_MESSAGESENDER | CKBEHAVIOR_TARGETABLE));

    proto->SetFunction(DoCharacterPathFollow);
    proto->SetBehaviorCallbackFct(DaCharacterPFCB);

    *pproto = proto;
    return CK_OK;
}

int DoCharacterPathFollow(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CKCharacter *carac = (CKCharacter *)beh->GetTarget();
    if (!carac)
        return CKBR_OWNERERROR;

    int msg = 0;
    beh->GetLocalParameterValue(0, &msg);

    VxVector CharacterDirection;
    beh->GetInputParameterValue(2, &CharacterDirection);

    CKMessageManager *mm = behcontext.MessageManager;

    //___________________________ In
    if (beh->IsInputActive(0))
    {
        // Initializations
        beh->ActivateInput(0, FALSE);
        CKCurve *curve = (CKCurve *)beh->GetInputParameterObject(0);
        if (!curve)
        {
            beh->ActivateOutput(0, TRUE);
            return CKBR_OK;
        }

        float start = 0;
        beh->GetInputParameterValue(1, &start);
        float length = curve->GetLength();
        length *= start;
        beh->SetLocalParameterValue(2, &length);
        beh->SetOutputParameterValue(0, &start);

        VxVector pos;
        VxVector up(0, 1.0f, 0), dir;
        carac->AlignCharacterWithRootPosition();
        carac->GetPosition(&pos);
        float caracheight = pos.y;
        curve->GetPos(start, &pos);
        curve->GetPos(start + 0.01f, &dir);
        dir -= pos;

        pos.y = caracheight;
        beh->SetLocalParameterValue(1, &pos);
        carac->SetPosition(&pos);
        dir.y = 0;
        dir.Normalize();

        if (CharacterDirection.z == 1)
            carac->SetOrientation(&dir, &up);

        if (CharacterDirection.z == -1)
        {
            dir *= -1;
            carac->SetOrientation(&dir, &up);
        }

        if (CharacterDirection.x == 1)
        {
            VxVector r = CrossProduct(dir, up);
            carac->SetOrientation(&r, &up);
        }

        if (CharacterDirection.x == -1)
        {
            VxVector r = CrossProduct(up, dir);
            carac->SetOrientation(&r, &up);
        }

        mm->SendMessageSingle(msg, carac, carac);

        beh->ActivateOutput(1, TRUE);
        return CKBR_OK;
    }

    //___________________________ Loop In
    if (beh->IsInputActive(1))
    {
        beh->ActivateInput(1, FALSE);

        VxVector pos, pos2, rdir;
        VxVector up(0, 1.0f, 0), dir;
        CKCurve *curve = (CKCurve *)beh->GetInputParameterObject(0);
        if (!curve)
            return CKBR_OK;
        float curstep, curl, l = curve->GetLength();

        carac->AlignCharacterWithRootPosition();
        carac->GetPosition(&pos);
        float caracheight = pos.y;

        beh->GetLocalParameterValue(2, &curl);
        beh->GetLocalParameterValue(1, &pos2);
        pos -= pos2;
        pos.y = 0;
        curl += Magnitude(pos);
        beh->SetLocalParameterValue(2, &curl);
        rdir = Normalize(pos);

        /*else*/
        {
            curstep = curl / l;
            beh->SetOutputParameterValue(0, &curstep);

            curve->GetPos(curstep, &pos);
            curve->GetPos(curstep + 0.01f, &dir);
            dir -= pos;
            beh->SetLocalParameterValue(1, &pos);

            dir.y = 0;
            dir.Normalize();

            carac->AlignCharacterWithRootPosition();
            pos.y = caracheight;
            if (curl < l)
            {
                carac->SetPosition(&pos);

                if (CharacterDirection.z == 1)
                    carac->SetOrientation(&dir, &up);

                if (CharacterDirection.z == -1)
                {
                    dir *= -1;
                    carac->SetOrientation(&dir, &up);
                }

                if (CharacterDirection.x == 1)
                {
                    VxVector r = CrossProduct(dir, up);
                    carac->SetOrientation(&r, &up);
                }

                if (CharacterDirection.x == -1)
                {
                    VxVector r = CrossProduct(up, dir);
                    carac->SetOrientation(&r, &up);
                }
            }

            mm->SendMessageSingle(msg, carac, carac);
            if (curl >= l)
            {
                beh->ActivateOutput(0, TRUE);
                curstep = 1;
                beh->SetOutputParameterValue(0, &curstep);
            }
            else
                beh->ActivateOutput(1, TRUE);
        }
    }
    return CKBR_OK;
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            CharacterGoTo
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateCharacterGoToBehaviorProto(CKBehaviorPrototype **);
int DoCharacterGoTo(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorGoToDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Character Go To");
    od->SetDescription("Make the character walk to a given object.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=in>Loop In: </SPAN>triggers the next step in a process loop.<BR>
    <BR>
    <SPAN CLASS=out>Arrived:  </SPAN>is activated when the distance between the character and the Target Object is the same as the <SPAN CLASS=pin>Distance</SPAN> input parameter.<BR>
    <SPAN CLASS=out>No Object: </SPAN>is activated if the <SPAN CLASS=pin>Target Object</SPAN> does not exist.<BR>
    <SPAN CLASS=out>Loop Out:  </SPAN>is activated when the process needs to loop.<BR>
    <BR>
    <SPAN CLASS=pin>Target Object: </SPAN>destination object.<BR>
    <SPAN CLASS=pin>Distance: </SPAN>distance just before destination object where the character should stop.<BR>
    <SPAN CLASS=pin>Character Direction: </SPAN>character's frontal direction.<BR>
    <SPAN CLASS=pin>Limit Angle: </SPAN>angle limit between the character's direction and the destination object.<BR>
    <BR>
    See Also: "Character Path Follow 2", "Character Path Follow".<BR>
    */
    od->SetCategory("Characters/Movement");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x11223344, 0xaabbccdd));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateCharacterGoToBehaviorProto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    return od;
}

CKERROR DaCharacterGoToCB(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORCREATE:
    case CKM_BEHAVIORLOAD:
    case CKM_BEHAVIORREADSTATE:
    {
        int tab[4];
        CKMessageManager *mm = behcontext.MessageManager;
        tab[0] = mm->AddMessageType("Joy_Up");
        tab[1] = mm->AddMessageType("Joy_Down");
        tab[2] = mm->AddMessageType("Joy_Left");
        tab[3] = mm->AddMessageType("Joy_Right");

        beh->SetLocalParameterValue(0, (void *)tab, 4 * sizeof(int));

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

CKERROR CreateCharacterGoToBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Character Go To");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareInput("Loop In");

    proto->DeclareOutput("Arrived");
    proto->DeclareOutput("No Object");
    proto->DeclareOutput("Loop Out");

    proto->DeclareInParameter("Target Object", CKPGUID_3DENTITY);
    proto->DeclareInParameter("Distance", CKPGUID_FLOAT, "12");
    proto->DeclareInParameter("Character Direction", CKPGUID_DIRECTION, "X");
    proto->DeclareInParameter("Limit Angle", CKPGUID_ANGLE, "0:70");
    proto->DeclareInParameter("Reverse", CKPGUID_BOOL, "FALSE");

    proto->DeclareLocalParameter("Messages", CKPGUID_VOIDBUF);

    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS)(CKBEHAVIOR_TARGETABLE | CKBEHAVIOR_MESSAGESENDER));
    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);

    proto->SetFunction(DoCharacterGoTo);
    proto->SetBehaviorCallbackFct(DaCharacterGoToCB);
    *pproto = proto;
    return CK_OK;
}

int DoCharacterGoTo(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // Set Input State
    if (beh->IsInputActive(0))
        beh->ActivateInput(0, FALSE);

    if (beh->IsInputActive(1))
        beh->ActivateInput(1, FALSE);

    CKCharacter *carac = (CKCharacter *)beh->GetTarget();
    if (!carac)
        return CKBR_OWNERERROR;

    CK3dEntity *root = (CK3dEntity *)carac->GetRootBodyPart();

    // Get target
    CK3dEntity *target = (CK3dEntity *)beh->GetInputParameterObject(0);

    // Get minimum distance
    float distmin;
    beh->GetInputParameterValue(1, &distmin);

    CKBOOL arrived = FALSE;

    if (target)
    {
        int *tab = (int *)beh->GetLocalParameterReadDataPtr(0);

        VxVector dist, up, dirx, dir, pos1(0, 0, 0), pos2(0, 0, 0);
        float angle1, angle2;

        target->GetPosition(&pos2);

        if (root)
        {
            int CharacterDirection = 1;
            beh->GetInputParameterValue(2, &CharacterDirection);

            root->GetPosition(&pos1);
            dist = pos2 - pos1;
            dist.y = 0.0f;
            float distance = (float)sqrt(dist.x * dist.x + dist.z * dist.z);

            switch (CharacterDirection)
            {
            case 1:
                carac->GetOrientation(&dirx, &up, &dir);
                dirx = -dirx;
                break;
            case 2:
            {
                carac->GetOrientation(&dirx, &up, &dir);
                dir = -dir;
            }
            break;
            case 3:
                carac->GetOrientation(&up, &dir, &dirx);
                break;
            case 4:
            {
                carac->GetOrientation(&up, &dir, &dirx);
                dir = -dir;
                dirx = -dirx;
            }
            break;
            case 5:
                carac->GetOrientation(&dir, &up, &dirx);
                break;
            case 6:
            {
                carac->GetOrientation(&dir, &up, &dirx);
                dir = -dir;
                dirx = -dirx;
            }
            break;
            }

            dirx.y = 0;
            dir.y = 0;
            dist /= distance;

            CKBOOL reverse = FALSE;
            beh->GetInputParameterValue(4, &reverse);
            if (reverse)
                dir = -dir;

            angle1 = DotProduct(dir, dist);
            angle2 = DotProduct(dirx, dist);

            float anglelimit = 1.22f;
            beh->GetInputParameterValue(3, &anglelimit);
            float anglelimitcos = cosf(anglelimit);
            if (anglelimitcos >= 0.98f)
                anglelimitcos = 0.98f;

            CKMessageManager *mm = behcontext.MessageManager;
            if (distance > distmin)
            {
                if (angle1 > anglelimitcos)
                {
                    if (reverse)
                        mm->SendMessageSingle(tab[1], carac, carac);
                    else
                        mm->SendMessageSingle(tab[0], carac, carac);
                }
            }

            if (angle1 <= 0.98f)
            {
                if (angle2 < 0.0f)
                    mm->SendMessageSingle(tab[2], carac, carac);
                else
                    mm->SendMessageSingle(tab[3], carac, carac);
            }

            if (distance <= distmin)
                arrived = TRUE;

            if (arrived)
            {
                beh->ActivateOutput(0);
                return CKBR_OK;
            }
            else
            {
                beh->ActivateOutput(2);
                return CKBR_OK;
            }
        }
    }
    beh->ActivateOutput(1);
    return CKBR_OK;
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            AnimationSynchro
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateAnimationSynchroBehaviorProto(CKBehaviorPrototype **pproto);
int DoAnimationSynchro(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorAnimationSynchroDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Animation Synchronizer");
    od->SetDescription("Provides a way to send messages at a specific moment during a character animation.");
    /* rem:
    <SPAN CLASS=in>On: </SPAN>activates the process.<BR>
    <SPAN CLASS=in>Off: </SPAN>deactivates the process.<BR>
    <BR>
    In the custom dialog box, the first thing to specify is the type of animation in the top left section,
    clicking "-Add-" to validate this choice. "Message..." will appear in the left hand column of the
    bottom half of the screen. Double clicking on this will reveal a drop down menu allowing selection
    of the appropriate message (OnClick, OnCursorOn etc). The frame when this message should be sent is
    selected by using the frame slider. To validate the choice of frame when the message should be sent,
    it is necessary to click once on the row to the right of the message. A point will appear on the
    line corresponding to the desired frame. To deselect the choice of frame, it is just necessary to
    click once more on the point.<BR>
    <BR>
    */
    od->SetCategory("Characters/Animation");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x2c1835eb, 0x68083d54));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateAnimationSynchroBehaviorProto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    return od;
}

CKERROR DaSyncCharacterCB(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORCREATE:
    {
        CKBYTE *Info = (CKBYTE *)beh->GetLocalParameterReadDataPtr(1);
        CKDWORD *val = (CKDWORD *)Info;
        if (*val == 0)
        {
            CKDWORD Params[2];
            Params[0] = 4;
            Params[1] = 0;

            beh->SetLocalParameterValue(1, (void *)Params, 8);
        }
        return CKBR_OK;
    }

    break;

    case CKM_BEHAVIORREADSTATE:
    case CKM_BEHAVIORLOAD:
    {
        CKCharacter *carac = (CKCharacter *)beh->GetOwner();
        if (!carac)
            return CKBR_OWNERERROR;

        CKBYTE *Info = (CKBYTE *)beh->GetLocalParameterWriteDataPtr(1);
        int index = 0;
        int i, j, nbanim = 0;
        int NbMessage;
        int size;

        // Block Data:
        // 0-3 : Block Size
        memcpy(&size, &Info[index], 4);
        if (CKM_BEHAVIORLOAD == behcontext.CallbackMessage)
        {
            ENDIANSWAP32(size);
            memcpy(&Info[index], &size, 4);
        }
        index += 4;

        // 4-7 : Number of Anim
        memcpy(&nbanim, &Info[index], 4);
        if (CKM_BEHAVIORLOAD == behcontext.CallbackMessage)
        {
            ENDIANSWAP32(nbanim);
            memcpy(&Info[index], &nbanim, 4);
        }
        index += 4;

        CKMessageManager *mm = behcontext.MessageManager;

        for (i = 0; i < nbanim; i++)
        {
            if (index >= size)
                break;
            // Start Anim
            // 8-11 : Sub Block size
            int subsize;
            memcpy(&subsize, &Info[index], 4);
            if (CKM_BEHAVIORLOAD == behcontext.CallbackMessage)
            {
                ENDIANSWAP32(subsize);
                memcpy(&Info[index], &subsize, 4);
            }
            index += 4;
            // 12-15: Index of animation in the object array
            int animindex;
            memcpy(&animindex, &Info[index], 4);
            if (CKM_BEHAVIORLOAD == behcontext.CallbackMessage)
            {
                ENDIANSWAP32(animindex);
                memcpy(&Info[index], &animindex, 4);
            }
            index += 4;
            // 16-19 : Number of Messages
            memcpy(&NbMessage, &Info[index], 4);
            if (CKM_BEHAVIORLOAD == behcontext.CallbackMessage)
            {
                ENDIANSWAP32(NbMessage);
                memcpy(&Info[index], &NbMessage, 4);
            }
            index += 4;

            for (j = 0; j < NbMessage; j++)
            {
                // 20-83 : Message Name
                char Message[64];
                memcpy(Message, &Info[index], 64);
                index += 64;

                // 84-87 : Message Type (integer)
                int MsgType = mm->AddMessageType(Message);
                memcpy(&Info[index], &MsgType, 4);
                index += 4;

                // 88-91 : Number of Keys
                int nbkeys;
                memcpy(&nbkeys, &Info[index], 4);
                if (CKM_BEHAVIORLOAD == behcontext.CallbackMessage)
                {
                    ENDIANSWAP32(nbkeys);
                    memcpy(&Info[index], &nbkeys, 4);
                }
                // 92-... : Keys (char)
                index += 4 + nbkeys;
            }
        }
    }
        return CKBR_OK;
        break;

    case CKM_BEHAVIOREDITED:
    {
        CKInterfaceManager *UIMan = (CKInterfaceManager *)behcontext.Context->GetManagerByGuid(INTERFACE_MANAGER_GUID);
        if (UIMan)
            UIMan->CallBehaviorEditionFunction(beh, 0);
    }
    break;

    case CKM_BEHAVIORDETACH:
    {
        CKCharacter *carac = (CKCharacter *)beh->GetOwner();
        if (!carac)
            return CKBR_OWNERERROR;
        return CKBR_OK;
    }
    break;

    case CKM_BEHAVIORPAUSE:
    case CKM_BEHAVIORRESUME:
        break;
    }
    return CKBR_OK;
}

CKERROR CreateAnimationSynchroBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Animation Synchronizer");
    if (!proto)
        return CKERR_OUTOFMEMORY;
    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS)(CKBEHAVIOR_ACTIVE | CKBEHAVIOR_MESSAGESENDER | CKBEHAVIOR_CUSTOMEDITDIALOG));

    proto->DeclareInput("On");
    proto->DeclareInput("Off");

    proto->DeclareLocalParameter(NULL, CKPGUID_OBJECTARRAY); //"Anims"
    proto->DeclareLocalParameter(NULL, CKPGUID_VOIDBUF);	 //"Data"

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);

    proto->SetFunction(DoAnimationSynchro);
    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS)(CKBEHAVIOR_MESSAGESENDER | CKBEHAVIOR_CUSTOMEDITDIALOG));
    proto->SetBehaviorCallbackFct(DaSyncCharacterCB);

    *pproto = proto;
    return CK_OK;
}

int DoAnimationSynchro(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    int i;

    if (beh->IsInputActive(1)) // enter by OFF
    {
        beh->ActivateInput(1, FALSE);
        return CKBR_OK;
    }
    else
    { // enter by ON
        beh->ActivateInput(0, FALSE);
    }

    CKCharacter *carac = (CKCharacter *)beh->GetOwner();
    CKMessageManager *mmanager = behcontext.MessageManager;
    if (!carac)
        return CKBR_OK;

    CKAnimation *CurrentAnim = carac->GetActiveAnimation();
    //  if (!CurrAnim)  return CKBR_ACTIVATENEXTFRAME;

    //-----------------------------------------------
    // Use a marker to temporary mark all the animation
    // that can be tested
    // this will be removed at the end of the behavior
    if (CurrentAnim)
        CurrentAnim->ModifyObjectFlags(CK_OBJECT_TEMPMARKER, 0);
    for (i = 0; i < carac->GetSecondaryAnimationsCount(); ++i)
    {
        CKAnimation *anim = carac->GetSecondaryAnimation(i);
        if (anim)
        {
            anim->ModifyObjectFlags(CK_OBJECT_TEMPMARKER, 0);
        }
    }

    XObjectArray *TheArray = NULL;
    beh->GetLocalParameterValue(0, &TheArray);

    //----- Synchro Data
    CKBYTE *Info = (CKBYTE *)beh->GetLocalParameterReadDataPtr(1);
    int size, index = 0;
    int nbanim = 0;
    memcpy(&size, &Info[index], 4);
    index += 4;
    // ENDIANSWAP32(size);
    memcpy(&nbanim, &Info[index], 4);
    index += 4;
    // ENDIANSWAP32(nbanim);

    // Block Data:
    // 0-3 : Block Size
    // 4-7 : Number of Anim
    // Start Anim 0
    // 8-11 : Sub Block size
    // 12-15: Index of animation in the object array
    // 16-19 : Number of Messages
    // 20-83 : Message Name
    // 84-87 : Message Type (integer)
    // 88-91 : Number of Keys
    // 92-... : Keys (char)
    // Start Anim 1, etc...

    for (i = 0; i < nbanim; i++)
    {

        // Too far : stop
        if (index >= size)
            break;
        int IndexAnimation;
        int subsize;
        memcpy(&subsize, &Info[index], 4);
        memcpy(&IndexAnimation, &Info[index + 4], 4);

        CKAnimation *anim = (CKAnimation *)TheArray->GetObject(behcontext.Context, IndexAnimation);

        //-----------------------------------------
        // Not the relevant animation (one of the actives) if not marked with
        // CK_OBJECT_TEMPMARKER
        if (!anim || !(anim->GetObjectFlags() & CK_OBJECT_TEMPMARKER))
        {
            index += subsize;
        }
        else
        {
            float currframe = anim->GetFrame();
            if (currframe == anim->GetLength())
                currframe += 0.01f;

            float prevframe, nextframe2;
            if (anim->IsLinkedToFrameRate())
                prevframe = currframe - (behcontext.DeltaTime * anim->GetLinkedFrameRate()) / 1000.0f;
            else
                prevframe = currframe - 1.0f;
            nextframe2 = anim->GetLength() + prevframe;

            int NbMessage;
            memcpy(&NbMessage, &Info[index + 8], 4);
            index += 12;

            //				BOOL animend=FALSE;
            // Last Frame of animation was 	at end of the animation
            //				if (prevframe<0) animend=TRUE;

            int start = XMax(0, (int)prevframe - 2), end;
            //				int start2;
            //				if (animend) start2=max(0,(int)nextframe2-2);
            for (int j = 0; j < NbMessage; j++)
            {
                int Msg;
                index += 64;
                memcpy(&Msg, &Info[index], 4);
                index += 4;
                int nbkeys;
                memcpy(&nbkeys, &Info[index], 4);
                end = XMin(nbkeys, (int)currframe + 2);
                index += 4;
                float key;
                for (int k = start; k < end; k++)
                    if (Info[index + k])
                    {
                        key = (float)k;
                        if (key >= prevframe && key <= currframe)
                        {
                            mmanager->SendMessageSingle(Msg, carac, carac);
                        }
                    }
                /*					if (animend)
                for (k=start2;k<nbkeys;k++)
                if (Info[index+k])
                {
                key=(float)k;
                if (key>=nextframe2)
                CKSendMessageSingle(Msg,carac,carac);

                  }
                */
                index += nbkeys;
            }
        }
    }

    //-----------------------------------------------
    // Remove the marker we may have set
    if (CurrentAnim)
        CurrentAnim->ModifyObjectFlags(0, CK_OBJECT_TEMPMARKER);
    for (i = 0; i < carac->GetSecondaryAnimationsCount(); ++i)
    {
        CKAnimation *anim = carac->GetSecondaryAnimation(i);
        if (anim)
        {
            anim->ModifyObjectFlags(0, CK_OBJECT_TEMPMARKER);
        }
    }
    return CKBR_ACTIVATENEXTFRAME;
}

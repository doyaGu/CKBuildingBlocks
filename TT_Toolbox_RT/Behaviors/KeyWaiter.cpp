/////////////////////////////////
/////////////////////////////////
//
//        TT_Key Waiter
//
/////////////////////////////////
/////////////////////////////////
#include "CKAll.h"
#include "ToolboxGuids.h"

CKObjectDeclaration *FillBehaviorTTKeyWaiterDecl();
CKERROR CreateTTKeyWaiterProto(CKBehaviorPrototype **pproto);
int TTKeyWaiter(const CKBehaviorContext &behcontext);
int TTReadKey(const CKBehaviorContext &behcontext);
CKERROR TTKeyWaiterCallBack(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorTTKeyWaiterDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("TT_Key Waiter");
    od->SetDescription("Waits for a key to be pressed.");
    od->SetCategory("TT Toolbox/Controllers");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x2ffb3ef0, 0x21807ae3));
    od->SetAuthorGuid(TERRATOOLS_GUID);
    od->SetAuthorName("Terratools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateTTKeyWaiterProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    od->NeedManager(INPUT_MANAGER_GUID);
    return od;
}

CKERROR CreateTTKeyWaiterProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("TT_Key Waiter");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareInput("Off");

    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Key", CKPGUID_KEY);

    proto->DeclareSetting("Wait For Any Key", CKPGUID_BOOL, "FALSE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(TTKeyWaiter);

    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS)(CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS | CKBEHAVIOR_INTERNALLYCREATEDOUTPUTPARAMS));
    proto->SetBehaviorCallbackFct(TTKeyWaiterCallBack);

    *pproto = proto;
    return CK_OK;
}

/*******************************************************/
/*                     CALLBACK                        */
/*******************************************************/
CKERROR TTKeyWaiterCallBack(const CKBehaviorContext &behcontext)
{

    CKBehavior *beh = behcontext.Behavior;

    switch (behcontext.CallbackMessage)
    {
        case CKM_BEHAVIORLOAD:
        case CKM_BEHAVIORSETTINGSEDITED:
        {
            CKBOOL anyKey = FALSE;
            beh->GetLocalParameterValue(0, &anyKey);

            CKParameterIn *pin = beh->GetInputParameter(0);
            if (anyKey) // wait any Key
            {
                beh->SetFunction(TTReadKey);
                if (pin)
                {
                    CKDestroyObject(beh->RemoveInputParameter(0));
                    beh->CreateOutputParameter("Key", CKPGUID_KEY);
                }
            }
            else // wait specific Key
            {
                beh->SetFunction(TTKeyWaiter);
                if (!pin)
                {
                    CKDestroyObject(beh->RemoveOutputParameter(0));
                    beh->CreateInputParameter("Key", CKPGUID_KEY);
                }
            }
        }
            break;
    }

    return CKBR_OK;
}

/*******************************************************/
/*               Main Function 1                       */
/*******************************************************/
int TTKeyWaiter(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CKInputManager *input = (CKInputManager *)behcontext.Context->GetManagerByGuid(INPUT_MANAGER_GUID);
    if (!input)
    {
        behcontext.Context->OutputToConsoleEx("Can't get the Input Manager");
        return CKBR_GENERICERROR;
    }

    int key = 0;
    beh->GetInputParameterValue(0, &key);
    if (input->IsKeyDown(key))
    {
        // Set IO states
        beh->ActivateInput(0, FALSE);
        beh->ActivateOutput(0, TRUE);
        return CKBR_OK;
    }

    return CKBR_ACTIVATENEXTFRAME;
}

/*******************************************************/
/*               Main Function 2                       */
/*******************************************************/
int TTReadKey(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CKInputManager *input = (CKInputManager *)behcontext.Context->GetManagerByGuid(INPUT_MANAGER_GUID);
    if (!input)
    {
        behcontext.Context->OutputToConsoleEx("Can't get the Input Manager");
        return CKBR_GENERICERROR;
    }

    CKDWORD key = 0;
    if (input->GetKeyFromBuffer(0, key) == KEY_PRESSED)
    {
        // Set IO states
        beh->ActivateInput(0, FALSE);
        beh->ActivateOutput(0, TRUE);

        // write the key
        beh->SetOutputParameterValue(0, &key);

        return CKBR_OK;
    }

    return CKBR_ACTIVATENEXTFRAME;
}

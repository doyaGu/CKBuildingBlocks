//////////////////////////////////////
//////////////////////////////////////
//
//        TT Activate Object
//
//////////////////////////////////////
//////////////////////////////////////
#include "CKAll.h"
#include "ToolboxGuids.h"

CKObjectDeclaration *FillBehaviorTTActivateObjectDecl();
CKERROR CreateTTActivateObjectProto(CKBehaviorPrototype **pproto);
int TTActivateObject(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorTTActivateObjectDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("TT Activate Object");
    od->SetDescription("Activates all the scripts of an object, either in the state they were before deactivation or once reset.");
    od->SetCategory("TT Toolbox/Narratives");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x59245fae, 0x24a123f3));
    od->SetAuthorGuid(TERRATOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateTTActivateObjectProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateTTActivateObjectProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("TT Activate Object");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");

    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Object", CKPGUID_BEOBJECT);
    proto->DeclareInParameter("Reset Scripts?", CKPGUID_BOOL, "FALSE");
    proto->DeclareInParameter("Activate All Scripts", CKPGUID_BOOL, "FALSE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(TTActivateObject);

    *pproto = proto;
    return CK_OK;
}

int TTActivateObject(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKScene *currentScene = behcontext.Context ? behcontext.Context->GetCurrentScene() : NULL;

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0, TRUE);

    CKBeObject *object = (CKBeObject *)beh->GetInputParameterObject(0);
    if (!object)
        return CKBR_PARAMETERERROR;

    CKBOOL resetScript = FALSE;
    beh->GetInputParameterValue(1, &resetScript);

    if (currentScene)
        currentScene->Activate(object, FALSE);

    CKBOOL activateAllScript = FALSE;
    beh->GetInputParameterValue(2, &activateAllScript);

    if (activateAllScript)
    {
        for (int i = 0; i < object->GetScriptCount(); ++i)
        {
            CKBehavior *script = object->GetScript(i);
            if (currentScene)
                currentScene->Activate(script, resetScript);
        }
    }

    return CKBR_OK;
}

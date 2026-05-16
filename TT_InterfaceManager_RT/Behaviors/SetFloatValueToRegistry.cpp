/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		   TT Set Float Value To Registry
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"
#include "InterfaceManager.h"
#include "RegistryUtils.h"

CKObjectDeclaration *FillBehaviorSetFloatValueToRegistryDecl();
CKERROR CreateSetFloatValueToRegistryProto(CKBehaviorPrototype **pproto);
int SetFloatValueToRegistry(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetFloatValueToRegistryDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("TT Set Float Value To Registry");
    od->SetDescription("Writes a Float value to the registry");
    od->SetCategory("TT InterfaceManager/Registry");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x60522f, 0x41d4c1f));
    od->SetAuthorGuid(TERRATOOLS_GUID);
    od->SetAuthorName("Terratools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSetFloatValueToRegistryProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateSetFloatValueToRegistryProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("TT Set Float Value To Registry");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");

    proto->DeclareOutput("OK");
    proto->DeclareOutput("FAILED");

    proto->DeclareInParameter("Float Value ", CKPGUID_FLOAT);
    proto->DeclareInParameter("RegKey  ...\\..\\", CKPGUID_STRING);
    proto->DeclareInParameter("Value Name", CKPGUID_STRING);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(SetFloatValueToRegistry);

    *pproto = proto;
    return CK_OK;
}

int SetFloatValueToRegistry(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    float value;
    char regKey[512] = {0};
    char valueName[128] = {0};
    beh->GetInputParameterValue(0, &value);
    beh->GetInputParameterValue(1, regKey);
    beh->GetInputParameterValue(2, valueName);

    InterfaceManager *man = InterfaceManager::GetManager(context);
    if (!man)
    {
        context->OutputToConsoleExBeep("TT_SetFloatValueToRegistry: im==NULL.");
        beh->ActivateOutput(1);
        return CKBR_OK;
    }

    CGameInfo *gameInfo = man->GetGameInfo();
    if (!gameInfo)
    {
        context->OutputToConsoleExBeep("TT_SetFloatValueToRegistry: System was not sent by the TT player");
        beh->ActivateOutput(1);
        return CKBR_OK;
    }

    char buffer[512];
    if (strlen(gameInfo->regSubkey) + strlen(regKey) >= sizeof(buffer))
    {
        context->OutputToConsoleExBeep("TT_SetFloatValueToRegistry: registry path too long");
        beh->ActivateOutput(1);
        return CKBR_OK;
    }
    strcpy(buffer, gameInfo->regSubkey);
    strcat(buffer, regKey);

    if (!TTWriteRegistryFloat(gameInfo->registryRoot, buffer, valueName, value))
    {
        context->OutputToConsoleExBeep("TT_SetFloatValueToRegistry: Failed to set %s %f", valueName, value);
        beh->ActivateOutput(1);
        return CKBR_OK;
    }

    beh->ActivateOutput(0);
    return CKBR_OK;
}

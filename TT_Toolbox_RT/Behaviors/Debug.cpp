////////////////////////////
////////////////////////////
//
//        TT_Debug
//
////////////////////////////
////////////////////////////
#include "CKAll.h"
#include "ToolboxGuids.h"
#include "DebugManager.h"

#include <string.h>

CKObjectDeclaration *FillBehaviorDebugDecl();
CKERROR CreateDebugProto(CKBehaviorPrototype **pproto);
int Debug(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorDebugDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("TT_Debug");
    od->SetDescription("Debug");
    od->SetCategory("TT Toolbox/Debug");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x4a446c43, 0x66fa2375));
    od->SetAuthorGuid(TERRATOOLS_GUID);
    od->SetAuthorName("Terratools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateDebugProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateDebugProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("TT_Debug");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("IN");

    proto->DeclareOutput("Exit");

    proto->DeclareInParameter("DebugText", CKPGUID_STRING);
    proto->DeclareInParameter("Console", CKPGUID_BOOL, "TRUE");
    proto->DeclareInParameter("Log-File", CKPGUID_BOOL, "FALSE");
    proto->DeclareInParameter("Enable", CKPGUID_BOOL, "TRUE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(Debug);

    *pproto = proto;
    return CK_OK;
}

void BuildBehaviorLocationPath(char *buf, int bufSize, CKBehavior *beh, CKBehavior *ownerScript)
{
    if (!buf || bufSize <= 0)
        return;

    buf[0] = '\0';

    if (!beh)
        return;

    CKBehavior *parent = beh->GetParent();
    if (!parent)
        return;

    XString path = parent->GetName() ? parent->GetName() : "";
    while (parent && parent != ownerScript)
    {
        parent = parent->GetParent();
        if (!parent)
            break;

        XString parentName = parent->GetName() ? parent->GetName() : "";
        XString fullPath = parentName;
        fullPath += "/";
        fullPath += path.CStr();
        path = fullPath;
    }

    const char *pathText = path.CStr();
    if (!pathText)
        return;

    size_t copyLen = strlen(pathText);
    if (copyLen >= (size_t)bufSize)
        copyLen = (size_t)bufSize - 1;

    memcpy(buf, pathText, copyLen);
    buf[copyLen] = '\0';
}

int Debug(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    CKBOOL enable;
    beh->GetInputParameterValue(3, &enable);

    if (enable)
    {
        DebugManager *dm = DebugManager::GetManager(context);
        if (dm && dm->IsInDebugMode())
        {
            CKSTRING text = (CKSTRING)beh->GetInputParameterReadDataPtr(0);
            if (!text)
                text = "";

            CKBOOL console;
            beh->GetInputParameterValue(1, &console);
            CKBOOL logfile;
            beh->GetInputParameterValue(2, &logfile);

            CKBehavior *ownerScript = beh->GetOwnerScript();
            CKBeObject *owner = beh->GetOwner();
            char buf[256];
            BuildBehaviorLocationPath(buf, sizeof(buf), beh, ownerScript);
            CKSTRING ownerName = (owner && owner->GetName()) ? owner->GetName() : "";

            if (console)
            {
                context->OutputToConsoleEx("Debug Output Behavior \n Location \t: %s \n Owner \t\t: %s \n Comment \t: %s",
                                           buf, ownerName, text);
            }

            if (logfile)
            {
                dm->WriteToLogFile("Debug.log", "Debug Output Behavior \n Location \t: %s \n Owner \t\t: %s \n Comment \t: %s\n",
                                   buf, ownerName, text);
            }

            beh->ActivateInput(0, FALSE);
        }
    }

    beh->ActivateOutput(0, TRUE);
    return CKBR_OK;
}

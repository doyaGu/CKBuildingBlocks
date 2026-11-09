/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		          TT Read Registry
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"
#include "InterfaceManager.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "RegistryUtils.h"

CKObjectDeclaration *FillBehaviorReadRegistryDecl();
CKERROR CreateReadRegistryProto(CKBehaviorPrototype **pproto);
int ReadRegistry(const CKBehaviorContext &behcontext);
CKERROR ReadRegistryCallBack(const CKBehaviorContext &behcontext);

// Retail reads every value from HKEY_LOCAL_MACHINE (TT_WriteRegistry writes HKEY_CURRENT_USER).
static const VxConfigRoot ReadRegistryRoot = VXCONFIG_ROOT_LOCAL_MACHINE;
// Retail queries strings into a 256-byte buffer and sets the string output with that size.
static const int ReadRegistryStringSize = 256;

static void *ReadDwordFromRegistry(CKBehavior *beh, CKContext *context, const char *subKey, const char *valueName, CKDWORD *data);
static void *ReadStringFromRegistry(CKBehavior *beh, CKContext *context, const char *subKey, const char *valueName, char *buffer);

CKObjectDeclaration *FillBehaviorReadRegistryDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("TT_ReadRegistry");
    od->SetDescription("Reads a value from the registry");
    od->SetCategory("TT InterfaceManager/Registry");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x460044b5, 0x6e927b66));
    od->SetAuthorGuid(TERRATOOLS_GUID);
    od->SetAuthorName("Terratools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateReadRegistryProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateReadRegistryProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("TT_ReadRegistry");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");

    proto->DeclareOutput("OK");
    proto->DeclareOutput("FAILED");

    proto->DeclareInParameter("Reg-Section  ...\\..\\", CKPGUID_STRING);
    proto->DeclareInParameter("Reg-Entry", CKPGUID_STRING);
    proto->DeclareInParameter("Destination-Array", CKPGUID_DATAARRAY);
    proto->DeclareInParameter("Array to load", CKPGUID_STRING);

    proto->DeclareOutParameter("Data", CKPGUID_INT);

    proto->DeclareSetting("SaveArray-Mode", CKPGUID_BOOL);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(ReadRegistry);

    proto->SetBehaviorFlags(CKBEHAVIOR_VARIABLEPARAMETEROUTPUTS);
    proto->SetBehaviorCallbackFct(ReadRegistryCallBack);

    *pproto = proto;
    return CK_OK;
}

int ReadRegistry(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    CKBOOL saveArrayMode = FALSE;
    beh->GetLocalParameterValue(0, &saveArrayMode);

    const char *regSection = (const char *)beh->GetInputParameterReadDataPtr(0);
    if (!regSection)
        regSection = "";

    CKDWORD data = 0;
    char str[ReadRegistryStringSize];

    if (!saveArrayMode)
    {
        // Retail TT_ReadRegistry (0x10007A00), single value: Reg-Section is the key as given.
        const char *regEntry = (const char *)beh->GetInputParameterReadDataPtr(1);
        if (!regEntry)
            regEntry = "";

        CKParameterOut *out = beh->GetOutputParameter(0);
        if (!out)
            return CKBR_OK;

        void *value;
        int size = 0;
        CKGUID guid = out->GetGUID();
        if (guid == CKPGUID_INT || guid == CKPGUID_FLOAT || guid == CKPGUID_BOOL)
        {
            // Float and bool outputs take the raw REG_DWORD bits as well.
            value = ReadDwordFromRegistry(beh, context, regSection, regEntry, &data);
        }
        else if (guid == CKPGUID_STRING)
        {
            value = ReadStringFromRegistry(beh, context, regSection, regEntry, str);
            size = ReadRegistryStringSize;
        }
        else
        {
            return CKBR_OK;
        }

        // A failed read has already activated FAILED; retail still sets the output from a
        // NULL buffer and activates OK. Size 0 keeps that a no-op instead of resizing the
        // string output to an uninitialised 256-byte buffer.
        beh->SetOutputParameterValue(0, value, value ? size : 0);
        beh->ActivateOutput(0);
        return CKBR_OK;
    }

    // Array mode: one key per column, "<section>\<Array to load>\<column>\", one value per row.
    CKDataArray *array = (CKDataArray *)beh->GetInputParameterObject(2);
    if (!array)
    {
        context->OutputToConsoleExBeep("TT_ReadRegistry: Destination array is NULL");
        beh->ActivateOutput(1);
        return CKBR_OK;
    }

    const char *arrayToLoad = (const char *)beh->GetInputParameterReadDataPtr(3);
    XString base(regSection);
    base << "\\" << (arrayToLoad ? arrayToLoad : "") << "\\";

    const int cols = array->GetColumnCount();
    const int rows = array->GetRowCount();
    for (int c = 0; c < cols; ++c)
    {
        CK_ARRAYTYPE type = array->GetColumnType(c);
        XString subKey(base);
        subKey << c << "\\";
        for (int i = 0; i < rows; ++i)
        {
            XString regEntry;
            regEntry << i;

            void *value;
            switch (type)
            {
            case CKARRAYTYPE_INT:
            case CKARRAYTYPE_FLOAT:
                value = ReadDwordFromRegistry(beh, context, subKey.CStr(), regEntry.CStr(), &data);
                break;
            case CKARRAYTYPE_PARAMETER:
            {
                CKParameter *parameter = *(CKParameter **)array->GetElement(i, c);
                if (!parameter || parameter->GetGUID() != CKPGUID_BOOL)
                {
                    context->OutputToConsoleExBeep("TT_ReadRegistry: ArrayCollumnType invalid(use string/bool/int/float)");
                    continue;
                }
                value = ReadDwordFromRegistry(beh, context, subKey.CStr(), regEntry.CStr(), &data);
                break;
            }
            case CKARRAYTYPE_STRING:
                value = ReadStringFromRegistry(beh, context, subKey.CStr(), regEntry.CStr(), str);
                break;
            default:
                continue;
            }
            // Retail keeps going after a failed read and stores the NULL result, which
            // clears an int/float/string cell and leaves a bool parameter unchanged.
            array->SetElementValue(i, c, value, 0);
        }
    }

    // Retail never activates OK in array mode.
    return CKBR_OK;
}

CKERROR ReadRegistryCallBack(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORATTACH:
    case CKM_BEHAVIORLOAD:
        break;
    case CKM_BEHAVIOREDITED:
    case CKM_BEHAVIORSETTINGSEDITED:
    {
        CKParameterOut *data = beh->GetOutputParameter(0);
        if (!data)
        {
            beh->CreateOutputParameter("Data", CKPGUID_INT);
            break;
        }

        CKGUID guid = data->GetGUID();
        if (guid != CKPGUID_INT &&
            guid != CKPGUID_FLOAT &&
            guid != CKPGUID_BOOL &&
            guid != CKPGUID_STRING)
        {
            context->DestroyObject(data);
            data = beh->CreateOutputParameter("Data", CKPGUID_INT);
            if (data)
            {
                int zero = 0;
                data->SetValue(&zero, sizeof(zero));
            }
            context->OutputToConsoleExBeep("TT_ReadRegistry: OutputParameter-Type invalid(use string/bool/int/float)");
        }
        break;
    }
    default:
        return CKBR_OK;
    }

    CKBOOL saveArrayMode = FALSE;
    beh->GetLocalParameterValue(0, &saveArrayMode);
    if (saveArrayMode)
    {
        beh->EnableInputParameter(1, FALSE);
        beh->EnableInputParameter(2, TRUE);
        beh->EnableInputParameter(3, TRUE);
        beh->EnableOutputParameter(0, FALSE);
    }
    else
    {
        beh->EnableInputParameter(1, TRUE);
        beh->EnableInputParameter(2, FALSE);
        beh->EnableInputParameter(3, FALSE);
        beh->EnableOutputParameter(0, TRUE);
    }

    return CKBR_OK;
}

// Retail TT_ReadRegistry_0/_2 (0x10007ED0/0x10007FA0): REG_DWORD query, FAILED on error.
static void *ReadDwordFromRegistry(CKBehavior *beh, CKContext *context, const char *subKey, const char *valueName, CKDWORD *data)
{
    *data = 0;
    if (!TTQueryRegistryDword(ReadRegistryRoot, subKey, valueName, data))
    {
        beh->ActivateOutput(1);
        context->OutputToConsoleExBeep("TT_ReadRegistry: ReadError: %s %d.", valueName, 0);
        return NULL;
    }
    return data;
}

// Retail TT_ReadRegistry_1 (0x10008070): REG_SZ query into 256 bytes, FAILED on error.
static void *ReadStringFromRegistry(CKBehavior *beh, CKContext *context, const char *subKey, const char *valueName, char *buffer)
{
    memset(buffer, 0, ReadRegistryStringSize);
    if (!TTQueryRegistryString(ReadRegistryRoot, subKey, valueName, buffer, ReadRegistryStringSize))
    {
        beh->ActivateOutput(1);
        context->OutputToConsoleExBeep("TT_ReadRegistry: ReadError: %s %d.", valueName, 0);
        return NULL;
    }
    return buffer;
}

#include "CKAll.h"
#include "BallanceHelpers.h"

CKObjectDeclaration *FillBehaviorBallanceLoadNMORangeDecl();
CKERROR CreateBallanceLoadNMORangeProto(CKBehaviorPrototype **pproto);
int BallanceLoadNMORange(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorBallanceLoadNMORangeDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Ballance Load NMO Range");
    od->SetDescription("Loads a range from Ballance's NMO table and registers the loaded All_* groups.");
    od->SetCategory("Ballance/Loading");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x42414c02, 0x10000002));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Ballance");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateBallanceLoadNMORangeProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateBallanceLoadNMORangeProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Ballance Load NMO Range");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareInput("Loop In");
    proto->DeclareOutput("Loaded");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetBehaviorFlags(CKBEHAVIOR_MESSAGESENDER);
    proto->SetFunction(BallanceLoadNMORange);

    *pproto = proto;
    return CK_OK;
}

int BallanceLoadNMORange(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *ctx = behcontext.Context;
    CKScene *scene = behcontext.CurrentScene;
    CKLevel *level = behcontext.CurrentLevel;

    const int inputCount = beh->GetInputCount();
    const int inputParamCount = beh->GetInputParameterCount();
    const int outputParamCount = beh->GetOutputParameterCount();
    const int outputCount = beh->GetOutputCount();
    for (int i = 0; i < inputCount && i < 2; ++i)
        beh->ActivateInput(i, FALSE);

    CKDataArray *table = inputParamCount > 0 ? (CKDataArray *)beh->GetInputParameterObject(0) : NULL;
    int start = 0;
    int end = 0;
    int loadId = 2;
    if (inputParamCount > 1)
        beh->GetInputParameterValue(1, &start);
    if (inputParamCount > 2)
        beh->GetInputParameterValue(2, &end);
    if (inputParamCount > 3)
        beh->GetInputParameterValue(3, &loadId);
    CKDataArray *registerArray = inputParamCount > 4 ? (CKDataArray *)beh->GetInputParameterObject(4) : NULL;
    if (!registerArray)
    {
        registerArray = Ballance::FindArray(ctx, "RegisterNMO");
        Ballance::DebugLog(ctx, "LoadNMORange '%s': RegisterNMO fallback -> %s",
                           beh->GetName(), registerArray ? registerArray->GetName() : "<null>");
    }

    if (!table)
    {
        CKDataArray *loading = Ballance::FindArray(ctx, "Loading_Parameter");
        if (loading && loading->GetRowCount() > 0 && loading->GetColumnCount() >= 4)
        {
            CKObject *tableObject = loading->GetElementObject(0, 0);
            if (!tableObject)
            {
                CK_ID tableParameterId = 0;
                if (loading->GetElementValue(0, 0, &tableParameterId) && tableParameterId)
                    tableObject = ctx ? ctx->GetObject(tableParameterId) : NULL;
            }
            CKParameter *tableParameter = CKParameter::Cast(tableObject);
            if (tableParameter)
                tableObject = tableParameter->GetValueObject();
            table = CKIsChildClassOf(tableObject, CKCID_DATAARRAY) ? (CKDataArray *)tableObject : NULL;
            Ballance::DebugLog(ctx, "LoadNMORange '%s': Loading_Parameter table object=%s class=%d",
                               beh->GetName(),
                               tableObject ? tableObject->GetName() : "<null>",
                               tableObject ? tableObject->GetClassID() : -1);
            Ballance::GetIntCell(loading, 0, 1, start);
            Ballance::GetIntCell(loading, 0, 2, end);
            Ballance::GetIntCell(loading, 0, 3, loadId);
        }
        Ballance::DebugLog(ctx, "LoadNMORange '%s': Loading_Parameter fallback loading=%s table=%s start=%d end=%d loadId=%d",
                           beh->GetName(), loading ? loading->GetName() : "<null>",
                           table ? table->GetName() : "<null>", start, end, loadId);
    }

    int loadedMessage = 2;
    if (inputParamCount > 5)
        beh->GetInputParameterValue(5, &loadedMessage);
    CKBOOL activateInitScripts = TRUE;
    CKBOOL addToScene = TRUE;
    CKBOOL reuseMeshes = TRUE;
    CKBOOL reuseMaterials = TRUE;
    if (inputParamCount > 6)
        beh->GetInputParameterValue(6, &activateInitScripts);
    if (inputParamCount > 7)
        beh->GetInputParameterValue(7, &addToScene);
    if (inputParamCount > 8)
        beh->GetInputParameterValue(8, &reuseMeshes);
    if (inputParamCount > 9)
        beh->GetInputParameterValue(9, &reuseMaterials);

    Ballance::DebugLog(ctx,
                       "LoadNMORange '%s': table=%s rows=%d cols=%d start=%d end=%d loadId=%d register=%s regRows=%d msg=%d init=%d addScene=%d reuseMesh=%d reuseMat=%d level=%s scene=%s",
                       beh->GetName(),
                       table ? table->GetName() : "<null>",
                       table ? table->GetRowCount() : -1,
                       table ? table->GetColumnCount() : -1,
                       start, end, loadId,
                       registerArray ? registerArray->GetName() : "<null>",
                       registerArray ? registerArray->GetRowCount() : -1,
                       loadedMessage, activateInitScripts, addToScene, reuseMeshes, reuseMaterials,
                       level ? level->GetName() : "<null>",
                       scene ? scene->GetName() : "<null>");

    int loadedCount = 0;
    int failedRow = -1;
    std::string failedFile;

    if (!table || !level || start < 0 || end < start || table->GetColumnCount() < 3)
    {
        Ballance::DebugLog(ctx, "LoadNMORange '%s': parameter error table=%p level=%p start=%d end=%d cols=%d",
                           beh->GetName(), table, level, start, end, table ? table->GetColumnCount() : -1);
        if (outputParamCount > 0)
            beh->SetOutputParameterValue(0, &loadedCount);
        if (outputParamCount > 1)
            beh->SetOutputParameterValue(1, &failedRow);
        if (outputParamCount > 2)
            beh->SetOutputParameterValue(2, "");
        if (outputCount > 1)
            beh->ActivateOutput(1);
        return CKBR_PARAMETERERROR;
    }

    if (end >= table->GetRowCount())
        end = table->GetRowCount() - 1;
    Ballance::DebugLog(ctx, "LoadNMORange '%s': clamped range start=%d end=%d", beh->GetName(), start, end);

    for (int row = start; row <= end; ++row)
    {
        std::string relPath;
        std::string nmoName;
        CKBOOL rowActivateInit = TRUE;
        Ballance::GetStringCell(table, row, 0, relPath);
        Ballance::GetStringCell(table, row, 1, nmoName);
        Ballance::GetBoolCell(table, row, 2, rowActivateInit);

        const std::string file = Ballance::JoinPath(relPath, nmoName);
        Ballance::DebugLog(ctx, "LoadNMORange '%s': row=%d rel='%s' nmo='%s' file='%s' rowInit=%d",
                           beh->GetName(), row, relPath.c_str(), nmoName.c_str(), file.c_str(), rowActivateInit);
        CKObjectArray *loadedObjects = NULL;
        if (!Ballance::LoadFile(ctx, level, scene, file.c_str(), addToScene, reuseMeshes, reuseMaterials, &loadedObjects))
        {
            failedRow = row;
            failedFile = file;
            Ballance::DebugLog(ctx, "LoadNMORange '%s': load failed row=%d file='%s'", beh->GetName(), row, file.c_str());
            if (loadedObjects)
                DeleteCKObjectArray(loadedObjects);
            break;
        }

        const std::string groupName = Ballance::MakeLoadedGroupName(nmoName);
        CKGroup *group = Ballance::FindOrCreateLoadedGroup(ctx, level, groupName.c_str(), loadedObjects);
        const int registered = Ballance::AddRegisterRow(registerArray, group, loadId);
        Ballance::DebugLog(ctx, "LoadNMORange '%s': loaded row=%d objects=%d group=%s registered=%d regRows=%d",
                           beh->GetName(), row, loadedObjects ? loadedObjects->GetCount() : -1,
                           group ? group->GetName() : "<null>", registered,
                           registerArray ? registerArray->GetRowCount() : -1);

        if (activateInitScripts && rowActivateInit)
        {
            const std::string scriptName = Ballance::MakeInitScriptName(nmoName);
            CKBehavior *script = Ballance::FindScript(ctx, scriptName.c_str());
            Ballance::DebugLog(ctx, "LoadNMORange '%s': activate init '%s' -> %s",
                               beh->GetName(), scriptName.c_str(), script ? script->GetName() : "<null>");
            Ballance::ActivateScript(scene, script, TRUE);
        }

        if (loadedObjects)
            DeleteCKObjectArray(loadedObjects);
        ++loadedCount;
    }

    if (outputParamCount > 0)
        beh->SetOutputParameterValue(0, &loadedCount);
    if (outputParamCount > 1)
        beh->SetOutputParameterValue(1, &failedRow);
    if (outputParamCount > 2)
        beh->SetOutputParameterValue(2, failedFile.c_str());

    CKBeObject *sender = (CKBeObject *)beh->GetOwner();
    if (loadedMessage >= 0)
        Ballance::SendBroadcast(behcontext.MessageManager, loadedMessage, sender);
    Ballance::DebugLog(ctx, "LoadNMORange '%s': done loaded=%d failedRow=%d failedFile='%s' sender=%s output=%d",
                       beh->GetName(), loadedCount, failedRow, failedFile.c_str(),
                       sender ? sender->GetName() : "<null>", failedRow >= 0 ? 1 : 0);

    if (failedRow >= 0)
    {
        if (outputCount > 1)
            beh->ActivateOutput(1);
    }
    else if (outputCount > 0)
    {
        beh->ActivateOutput(0);
    }
    return CKBR_OK;
}

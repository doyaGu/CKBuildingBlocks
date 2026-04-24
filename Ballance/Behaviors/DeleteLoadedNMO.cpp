#include "CKAll.h"
#include "BallanceHelpers.h"
#include <cstring>

CKObjectDeclaration *FillBehaviorBallanceDeleteLoadedNMODecl();
CKERROR CreateBallanceDeleteLoadedNMOProto(CKBehaviorPrototype **pproto);
int BallanceDeleteLoadedNMO(const CKBehaviorContext &behcontext);

enum DeleteMode
{
    DELETE_BY_GROUP,
    DELETE_BY_LOAD_ID,
    DELETE_ALL
};

static int DeleteMatching(CKContext *ctx, CKDataArray *registerArray, CKGroup *targetGroup, int targetLoadId, DeleteMode mode)
{
    if (!ctx || !registerArray || registerArray->GetColumnCount() < 2)
    {
        Ballance::DebugLog(ctx, "DeleteLoadedNMO: DeleteMatching parameter error register=%p cols=%d",
                           registerArray, registerArray ? registerArray->GetColumnCount() : -1);
        return 0;
    }

    int deleted = 0;
    for (int row = registerArray->GetRowCount() - 1; row >= 0; --row)
    {
        CKGroup *group = (CKGroup *)registerArray->GetElementObject(row, 0);
        int loadId = 0;
        registerArray->GetElementValue(row, 1, &loadId);

        bool matches = false;
        if (mode == DELETE_ALL)
            matches = true;
        else if (mode == DELETE_BY_GROUP)
            matches = targetGroup && group == targetGroup;
        else if (mode == DELETE_BY_LOAD_ID)
            matches = loadId == targetLoadId;

        if (!matches)
            continue;

        const int deletedInGroup = Ballance::DeleteGroupObjects(ctx, group);
        Ballance::DebugLog(ctx, "DeleteLoadedNMO: row=%d mode=%d group=%s loadId=%d deleted=%d",
                           row, mode, group ? group->GetName() : "<null>", loadId, deletedInGroup);
        deleted += deletedInGroup;
        registerArray->RemoveRow(row);
    }
    return deleted;
}

CKObjectDeclaration *FillBehaviorBallanceDeleteLoadedNMODecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Ballance Delete Loaded NMO");
    od->SetDescription("Deletes loaded Ballance NMO groups from RegisterNMO by group, load id, or all rows.");
    od->SetCategory("Ballance/Loading");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x42414c03, 0x10000003));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Ballance");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateBallanceDeleteLoadedNMOProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateBallanceDeleteLoadedNMOProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Ballance Delete Loaded NMO");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("Start");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetBehaviorFlags(CKBEHAVIOR_MESSAGESENDER);
    proto->SetFunction(BallanceDeleteLoadedNMO);

    *pproto = proto;
    return CK_OK;
}

int BallanceDeleteLoadedNMO(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *ctx = behcontext.Context;

    const int inputCount = beh->GetInputCount();
    const int inputParamCount = beh->GetInputParameterCount();
    const int outputParamCount = beh->GetOutputParameterCount();

    bool deleteGroup = inputCount > 0 && beh->IsInputActive(0) != FALSE;
    bool deleteLoadId = inputCount > 1 && beh->IsInputActive(1) != FALSE;
    bool deleteAll = inputCount > 2 && beh->IsInputActive(2) != FALSE;
    for (int i = 0; i < inputCount && i < 3; ++i)
        beh->ActivateInput(i, FALSE);

    CKDataArray *registerArray = inputParamCount > 0 ? (CKDataArray *)beh->GetInputParameterObject(0) : NULL;
    if (!registerArray)
    {
        registerArray = Ballance::FindArray(ctx, "RegisterNMO");
        Ballance::DebugLog(ctx, "DeleteLoadedNMO '%s': RegisterNMO fallback -> %s",
                           beh->GetName(), registerArray ? registerArray->GetName() : "<null>");
    }

    CKGroup *group = (deleteGroup && inputParamCount > 1) ? (CKGroup *)beh->GetInputParameterObject(1) : NULL;
    int loadId = 0;
    int deletedMessage = -1;
    if (inputParamCount > 2)
        beh->GetInputParameterValue(2, &loadId);
    if (inputParamCount > 3)
        beh->GetInputParameterValue(3, &deletedMessage);

    Ballance::DebugLog(ctx, "DeleteLoadedNMO '%s': active group=%d loadIdInput=%d all=%d register=%s regRows=%d group=%s loadId=%d msg=%d",
                       beh->GetName(), deleteGroup, deleteLoadId, deleteAll,
                       registerArray ? registerArray->GetName() : "<null>",
                       registerArray ? registerArray->GetRowCount() : -1,
                       group ? group->GetName() : "<null>", loadId, deletedMessage);

    const char *name = beh->GetName();
    if (name && std::strstr(name, "All") && deleteGroup && !group)
    {
        deleteGroup = false;
        deleteAll = true;
        Ballance::DebugLog(ctx, "DeleteLoadedNMO '%s': name fallback selected Delete All", beh->GetName());
    }

    if (!deleteGroup && !deleteLoadId && !deleteAll)
    {
        deleteGroup = true;
        Ballance::DebugLog(ctx, "DeleteLoadedNMO '%s': no active input fallback selected Delete Group", beh->GetName());
    }

    if ((deleteGroup && !group) || deleteLoadId)
    {
        CKDataArray *deleteParams = Ballance::FindArray(ctx, "Loading_Del_Parameter");
        if (deleteParams && deleteParams->GetRowCount() > 0)
        {
            if (!group && deleteParams->GetColumnCount() > 0)
                group = (CKGroup *)deleteParams->GetElementObject(0, 0);
            if (deleteParams->GetColumnCount() > 1)
                Ballance::GetIntCell(deleteParams, 0, 1, loadId);
        }
        Ballance::DebugLog(ctx, "DeleteLoadedNMO '%s': Loading_Del_Parameter fallback params=%s group=%s loadId=%d",
                           beh->GetName(), deleteParams ? deleteParams->GetName() : "<null>",
                           group ? group->GetName() : "<null>", loadId);
    }

    int deleted = 0;
    if (deleteAll)
        deleted = DeleteMatching(ctx, registerArray, NULL, loadId, DELETE_ALL);
    else if (deleteGroup)
        deleted = DeleteMatching(ctx, registerArray, group, loadId, DELETE_BY_GROUP);
    else if (deleteLoadId)
        deleted = DeleteMatching(ctx, registerArray, NULL, loadId, DELETE_BY_LOAD_ID);

    if (outputParamCount > 0)
        beh->SetOutputParameterValue(0, &deleted);

    CKBeObject *sender = (CKBeObject *)beh->GetOwner();
    if (deletedMessage >= 0)
        Ballance::SendBroadcast(behcontext.MessageManager, deletedMessage, sender);
    Ballance::DebugLog(ctx, "DeleteLoadedNMO '%s': done deleted=%d sender=%s output=%d",
                       beh->GetName(), deleted, sender ? sender->GetName() : "<null>", deleted > 0 ? 0 : 1);

    if (beh->GetOutputCount() > (deleted > 0 ? 0 : 1))
        beh->ActivateOutput(deleted > 0 ? 0 : 1);
    return CKBR_OK;
}

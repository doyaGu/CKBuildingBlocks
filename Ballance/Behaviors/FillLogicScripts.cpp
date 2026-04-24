#include "CKAll.h"
#include "BallanceHelpers.h"

CKObjectDeclaration *FillBehaviorBallanceFillLogicScriptsDecl();
CKERROR CreateBallanceFillLogicScriptsProto(CKBehaviorPrototype **pproto);
int BallanceFillLogicScripts(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorBallanceFillLogicScriptsDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Ballance Fill Logic Scripts");
    od->SetDescription("Builds the Ballance Logic_Scripts array from gameplay, camera, ball, and animation groups.");
    od->SetCategory("Ballance/Runtime");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x42414c04, 0x10000004));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Ballance");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateBallanceFillLogicScriptsProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateBallanceFillLogicScriptsProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Ballance Fill Logic Scripts");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");
    proto->DeclareOutput("Failed");

    proto->DeclareInParameter("Logic Scripts Array", CKPGUID_DATAARRAY);
    proto->DeclareInParameter("All_Gameplay", CKPGUID_GROUP);
    proto->DeclareInParameter("All_Camera", CKPGUID_GROUP);
    proto->DeclareInParameter("All_Balls", CKPGUID_GROUP);
    proto->DeclareInParameter("All_AnimTrafo", CKPGUID_GROUP);
    proto->DeclareInParameter("Include Ball Reset Scripts", CKPGUID_BOOL, "TRUE");

    proto->DeclareOutParameter("Scripts Added", CKPGUID_INT);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(BallanceFillLogicScripts);

    *pproto = proto;
    return CK_OK;
}

int BallanceFillLogicScripts(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *ctx = behcontext.Context;

    beh->ActivateInput(0, FALSE);

    CKDataArray *scriptArray = (CKDataArray *)beh->GetInputParameterObject(0);
    if (!scriptArray)
    {
        scriptArray = Ballance::FindArray(ctx, "Logic_Scripts");
        Ballance::DebugLog(ctx, "FillLogicScripts '%s': Logic_Scripts fallback -> %s",
                           beh->GetName(), scriptArray ? scriptArray->GetName() : "<null>");
    }
    if (!scriptArray || scriptArray->GetColumnCount() < 1)
    {
        int added = 0;
        Ballance::DebugLog(ctx, "FillLogicScripts '%s': parameter error scriptArray=%p cols=%d",
                           beh->GetName(), scriptArray, scriptArray ? scriptArray->GetColumnCount() : -1);
        beh->SetOutputParameterValue(0, &added);
        beh->ActivateOutput(1);
        return CKBR_PARAMETERERROR;
    }

    Ballance::DebugLog(ctx, "FillLogicScripts '%s': clear array=%s rows=%d cols=%d",
                       beh->GetName(), scriptArray->GetName(), scriptArray->GetRowCount(), scriptArray->GetColumnCount());

    while (scriptArray->GetRowCount() > 0)
        scriptArray->RemoveRow(scriptArray->GetRowCount() - 1);

    int added = 0;
    const char *fallbackGroups[] = {"All_Gameplay", "All_Camera", "All_Balls", "All_AnimTrafo"};
    for (int i = 1; i <= 4; ++i)
    {
        CKGroup *group = (CKGroup *)beh->GetInputParameterObject(i);
        if (!group)
            group = Ballance::FindGroup(ctx, fallbackGroups[i - 1]);
        const int groupAdded = Ballance::FillScriptsFromGroup(scriptArray, group);
        Ballance::DebugLog(ctx, "FillLogicScripts '%s': group '%s' -> %s objects=%d added=%d",
                           beh->GetName(), fallbackGroups[i - 1],
                           group ? group->GetName() : "<null>",
                           group ? group->GetObjectCount() : -1, groupAdded);
        added += groupAdded;
    }

    CKBOOL includeBallResetScripts = TRUE;
    beh->GetInputParameterValue(5, &includeBallResetScripts);
    Ballance::DebugLog(ctx, "FillLogicScripts '%s': include reset scripts=%d", beh->GetName(), includeBallResetScripts);
    if (includeBallResetScripts)
    {
        CKBehavior *paper = Ballance::FindScript(ctx, "Ball_ResetPieces_Paper");
        CKBehavior *wood = Ballance::FindScript(ctx, "Ball_ResetPieces_Wood");
        CKBehavior *stone = Ballance::FindScript(ctx, "Ball_ResetPieces_Stone");
        const int paperAdded = Ballance::AddScriptRow(scriptArray, paper);
        const int woodAdded = Ballance::AddScriptRow(scriptArray, wood);
        const int stoneAdded = Ballance::AddScriptRow(scriptArray, stone);
        Ballance::DebugLog(ctx, "FillLogicScripts '%s': reset paper=%s/%d wood=%s/%d stone=%s/%d",
                           beh->GetName(),
                           paper ? paper->GetName() : "<null>", paperAdded,
                           wood ? wood->GetName() : "<null>", woodAdded,
                           stone ? stone->GetName() : "<null>", stoneAdded);
        added += paperAdded + woodAdded + stoneAdded;
    }

    beh->SetOutputParameterValue(0, &added);
    Ballance::DebugLog(ctx, "FillLogicScripts '%s': done added=%d rows=%d",
                       beh->GetName(), added, scriptArray->GetRowCount());
    beh->ActivateOutput(0);
    return CKBR_OK;
}

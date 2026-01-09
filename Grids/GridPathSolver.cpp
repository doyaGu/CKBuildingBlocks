/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            Grid Path Solver
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

#include "CKAll.h"
#include "GridManager.h"
#include "GridPathManager.h"

#define CKPGUID_HEURISTIC CKGUID(0x72604f23, 0x5fe7f0f)
#define CKPGUID_PATHTYPE CKGUID(0x3e7f7035, 0x7835401f)

// input
enum
{
    INPUT_ON,
    INPUT_OFF
};

// outpout
enum
{
    OUTPUT_FIND,
    OUTPUT_NOTFIND
};

// pinput
enum
{
    INPUT_PARAM_TARGETPOS,
    INPUT_PARAM_GOALPOS,
    INPUT_PARAM_GOALREF,
    INPUT_PARAM_HEURISTIC,
    INPUT_PARAM_HEURISTICCOEF,
    INPUT_PARAM_DIAGONAL,
    INPUT_PARAM_OBSTACLELAYER,
    INPUT_PARAM_THRESHOLD,
    INPUT_PARAM_SLOWING,
    INPUT_PARAM_LINKER,
    INPUT_PARAM_LINKEROBS,
    INPUT_PARAM_TIME
};

// poutpout
enum
{
    OUTPUT_PARAM_PATH,
    OUTPUT_PARAM_LENGTH
};

// local parameter/setting
enum
{
    LOCAL_SETTING_PATHTYPE,
    LOCAL_SETTING_OPTIMIZE
};

CKObjectDeclaration *FillBehaviorGridPathSolverDecl();
CKERROR CreateGridPathSolverProto(CKBehaviorPrototype **);
CKERROR GridPathSolverCallback(const CKBehaviorContext &behcontext);
int GridPathSolver(const CKBehaviorContext &behcontext);

//-------------------------------------------------
// Declaration
//-------------------------------------------------

CKObjectDeclaration *FillBehaviorGridPathSolverDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Grid Path Solver");

    od->SetDescription("Solve a new path finding problem.");
    /* rem:
    <SPAN CLASS=in>On: </SPAN>Activate the search.<BR>
    <SPAN CLASS=in>Reset: </SPAN>Abord the search<BR>
    <BR>
    <SPAN CLASS=out>Find: </SPAN>Is activate when the path is find.<BR>
    <SPAN CLASS=out>Not Find: </SPAN>Is activate when the path is not find.<BR>
    <BR>
    <SPAN CLASS=pin>Target Position: </SPAN>Position in target referencial<BR>
    <SPAN CLASS=pin>Goal Position Ref: </SPAN>Goal referential.<BR>
    <SPAN CLASS=pin>Goal Position: </SPAN>Position in goal referencial<BR>
    <SPAN CLASS=pin>Heuristic Method: </SPAN>Method to calculate distance<BR>
    <SPAN CLASS=pin>Heuristic Factor: </SPAN>Determine importance of heuristic in calculs.<BR>
    <SPAN CLASS=pin>Diagonal: </SPAN> Enable diagonal moves.<BR>
    <SPAN CLASS=pin>Obstacle Layer: </SPAN>Layer wich stand for obstacle.<BR>
    <SPAN CLASS=pin>Obstacle Threshold: </SPAN>If obstacle layer value is higher than obstacle threshold then it is an obstacle.<BR>
    <SPAN CLASS=pin>Slowing Factor: </SPAN>Increase or decrease the value of the layer obstacle.<BR>
    <SPAN CLASS=pin>Use Linkers: </SPAN>Allow the target to use linkers<BR>
    <SPAN CLASS=pin>Teleporter As Obstacles: </SPAN>Do the target must consider teleporters as obstacles<BR>
    <SPAN CLASS=pin>Ms/Frame: </SPAN>Max time to spend for path calculation for one frame.<BR>
    <BR>
    <SPAN CLASS=pout>Path ID: </SPAN>Path representation<BR>
    <BR>or<BR>
    <SPAN CLASS=pout>List of point: </SPAN>Path representation<BR>
    <BR>or<BR>
    <SPAN CLASS=pout>Curve: </SPAN>Path representation<BR>
    <SPAN CLASS=pout>Path's Length: </SPAN>Path's Length<BR>
    <BR>
    <SPAN CLASS=setting>Path Type</SPAN>Choose the representation of the path.<BR>
    <BR>
    <BR>
    */
    od->SetCategory("Grids/Path Finding");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x17e75566, 0xc5625a6));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00020000);
    od->SetCreationFunction(CreateGridPathSolverProto);
    od->SetCompatibleClassId(CKCID_3DENTITY);
    return od;
}

//-------------------------------------------------
// Prototype
//-------------------------------------------------

CKERROR CreateGridPathSolverProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Grid Path Solver");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("On");
    proto->DeclareInput("Reset");

    proto->DeclareOutput("Found");
    proto->DeclareOutput("Not Found");

    proto->DeclareInParameter("Position In Owner Ref", CKPGUID_VECTOR);
    proto->DeclareInParameter("Goal Position", CKPGUID_VECTOR);
    proto->DeclareInParameter("Goal Position Ref", CKPGUID_3DENTITY);
    proto->DeclareInParameter("Heuristic Method", CKPGUID_HEURISTIC, "1");
    proto->DeclareInParameter("Heuristic Factor", CKPGUID_FLOAT, "2");
    proto->DeclareInParameter("Diagonal", CKPGUID_BOOL);
    proto->DeclareInParameter("Obstacle Layer", CKPGUID_LAYERTYPE);
    proto->DeclareInParameter("Obstacle Threshold", CKPGUID_INT);
    proto->DeclareInParameter("Slowing Factor", CKPGUID_FLOAT, "1");
    proto->DeclareInParameter("Use Linkers", CKPGUID_BOOL, "TRUE");
    proto->DeclareInParameter("Linkers As Obstacles", CKPGUID_BOOL, "FALSE");
    proto->DeclareInParameter("Ms/Frame", CKPGUID_FLOAT, "1");

    proto->DeclareOutParameter("Path ID", CKPGUID_INT);
    proto->DeclareOutParameter("Path's Length", CKPGUID_FLOAT);

    proto->DeclareSetting("Path Type", CKPGUID_PATHTYPE, "1");
    proto->DeclareSetting("Optimize Path", CKPGUID_BOOL, "TRUE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(GridPathSolver);
    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS)(CKBEHAVIOR_TARGETABLE | CKBEHAVIOR_INTERNALLYCREATEDOUTPUTPARAMS));
    proto->SetBehaviorCallbackFct(GridPathSolverCallback);

    *pproto = proto;
    return CK_OK;
}

//-------------------------------------------------
// Fonction
//-------------------------------------------------

int GridPathSolver(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    GridManager *gm = (GridManager *)behcontext.Context->GetManagerByGuid(GRID_MANAGER_GUID);
    GridPathManager *gridPathManager = gm->GetGridPathManager();
    CK3dEntity *target;
    CK3dEntity *goalRef;
    VxVector targetPos;
    VxVector goalPos;
    int obstacleLayer;
    int obstacleThreshold;
    float slowingFactor;
    CKBOOL linker;
    CKBOOL linkerObs;
    float heuristicCoef;
    float timeFrame;
    int state;
    int heuristic;
    CKBOOL diagonal;
    CKBOOL optimize;

    // Get the 3d entity target.
    target = (CK3dEntity *)beh->GetTarget();
    if (!target)
    {
        beh->ActivateOutput(OUTPUT_NOTFIND, TRUE);
        return CKBR_OWNERERROR;
    }

    // ON
    if (beh->IsInputActive(INPUT_ON))
    {
        beh->ActivateInput(INPUT_ON, FALSE);
        state = gridPathManager->GetTargetStatus(target);
        if (!state)
        {
            // Register a new path problem.
            beh->GetInputParameterValue(INPUT_PARAM_TARGETPOS, &targetPos);

            // In version 2 pos and ref are inversed.
            if (beh->GetVersion() == 0x00020000)
            {
                goalRef = (CK3dEntity *)beh->GetInputParameterObject(INPUT_PARAM_GOALREF);
                beh->GetInputParameterValue(INPUT_PARAM_GOALPOS, &goalPos);
            }
            else
            {
                goalRef = (CK3dEntity *)beh->GetInputParameterObject(INPUT_PARAM_GOALPOS);
                beh->GetInputParameterValue(INPUT_PARAM_GOALREF, &goalPos);
            }

            beh->GetInputParameterValue(INPUT_PARAM_HEURISTIC, &heuristic);
            beh->GetInputParameterValue(INPUT_PARAM_DIAGONAL, &diagonal);
            beh->GetInputParameterValue(INPUT_PARAM_OBSTACLELAYER, &obstacleLayer);
            beh->GetInputParameterValue(INPUT_PARAM_THRESHOLD, &obstacleThreshold);
            beh->GetInputParameterValue(INPUT_PARAM_SLOWING, &slowingFactor);
            beh->GetInputParameterValue(INPUT_PARAM_LINKER, &linker);
            beh->GetInputParameterValue(INPUT_PARAM_LINKEROBS, &linkerObs);
            beh->GetInputParameterValue(INPUT_PARAM_HEURISTICCOEF, &heuristicCoef);
            beh->GetInputParameterValue(INPUT_PARAM_TIME, &timeFrame);
            beh->GetLocalParameterValue(LOCAL_SETTING_OPTIMIZE, &optimize);

            // Register the path problem...
            if (!obstacleLayer || !gridPathManager->RegisterPathProblem(target, &targetPos, goalRef, &goalPos, obstacleLayer, obstacleThreshold, slowingFactor, linker, linkerObs, heuristicCoef, timeFrame, heuristic, diagonal, optimize))
            {
                beh->ActivateOutput(OUTPUT_NOTFIND, TRUE);
                return CKBR_OWNERERROR;
            }
        }
        return CKBR_ACTIVATENEXTFRAME;
    }

    // OFF
    if (beh->IsInputActive(INPUT_OFF))
    {
        beh->ActivateInput(INPUT_OFF, FALSE);
        beh->ActivateOutput(OUTPUT_NOTFIND, TRUE);
        gridPathManager->UnregisterPathProblem(target, FALSE);
        return CKBR_OK;
    }

    // INTERNE
    state = gridPathManager->GetTargetStatus(target);
    if (!state)
    {
        // Path Problem was not found
        beh->ActivateOutput(OUTPUT_NOTFIND, TRUE);
        return CKBR_OK;
    }

    if (state == 2)
    {
        // Path was found.
        int pathType;
        float coast;
        beh->GetLocalParameterValue(LOCAL_SETTING_PATHTYPE, &pathType);
        if (pathType == 1)
        {
            // Path ID.
            int followPathID;

            // Get the path and unregister the problem.
            gridPathManager->GetPath(target, coast, followPathID);
            gridPathManager->UnregisterPathProblem(target, TRUE);

            // Send the path id to the param output.
            beh->SetOutputParameterValue(OUTPUT_PARAM_PATH, &followPathID);
        }
        else if (pathType == 2)
        {
            // Array vector.
            CKDataArray *dataArray = (CKDataArray *)beh->GetOutputParameterObject(OUTPUT_PARAM_PATH);
            if (!dataArray)
            {
                dataArray = (CKDataArray *)behcontext.Context->CreateObject(CKCID_DATAARRAY, "PathArray", CK_OBJECTCREATION_DYNAMIC);
                dataArray->InsertColumn(-1, CKARRAYTYPE_PARAMETER, "path node", CKPGUID_VECTOR);
                behcontext.CurrentLevel->AddObject(dataArray);
            }

            // Get the path and unregister the problem.
            gridPathManager->GetPath(target, coast, dataArray);
            gridPathManager->UnregisterPathProblem(target, FALSE);

            // Send the curve to the param output.
            beh->SetOutputParameterObject(OUTPUT_PARAM_PATH, dataArray);
        }
        else if (pathType == 3)
        {
            // Curve.
            CKCurve *curve = (CKCurve *)beh->GetOutputParameterObject(OUTPUT_PARAM_PATH);
            if (!curve)
            {
                curve = (CKCurve *)(behcontext.Context->CreateObject(CKCID_CURVE, "Result Path", CK_OBJECTCREATION_DYNAMIC));
                behcontext.CurrentLevel->AddObject(curve);
                beh->SetOutputParameterObject(OUTPUT_PARAM_PATH, curve);
            }

            // Get the path and unregister the problem.
            curve->SetColor(VxColor(255, 255, 255));
            gridPathManager->GetPath(target, coast, curve);
            gridPathManager->UnregisterPathProblem(target, FALSE);

            // Send the curve to the param output.
            beh->SetOutputParameterObject(OUTPUT_PARAM_PATH, curve);
        }

        // Send the coast to the param output.
        coast--;
        beh->SetOutputParameterValue(OUTPUT_PARAM_LENGTH, &coast);
        beh->ActivateOutput(OUTPUT_FIND, TRUE);
        return CKBR_OK;
    }
    else if (state == 3)
    {
        // Path was not found.
        gridPathManager->UnregisterPathProblem(target, FALSE);
        beh->ActivateOutput(OUTPUT_NOTFIND, TRUE);
        return CKBR_OK;
    }
    return CKBR_ACTIVATENEXTFRAME;
}

//-------------------------------------------------
// Callback
//-------------------------------------------------

CKERROR GridPathSolverCallback(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORLOAD:
    case CKM_BEHAVIORCREATE:
    {
#pragma NOTE("nicoh: ca sert a rien ca ???")
        void *path = (CKCurve *)beh->GetOutputParameterObject(OUTPUT_PARAM_PATH);
        path = 0;
    }

    case CKM_BEHAVIORSETTINGSEDITED:
    {
        int pathType;
        CKParameterOut *pout;
        beh->GetLocalParameterValue(LOCAL_SETTING_PATHTYPE, &pathType);
        pout = beh->GetOutputParameter(OUTPUT_PARAM_PATH);

        if (pathType == 1)
        {
            pout->SetGUID(CKPGUID_INT);
            pout->SetName("Path ID");
        }
        else if (pathType == 2)
        {
            pout->SetGUID(CKPGUID_DATAARRAY);
            pout->SetName("List of point");
        }
        else if (pathType == 3)
        {
            pout->SetGUID(CKPGUID_CURVE);
            pout->SetName("Curve");
        }
    }
    break;
    }
    return CKBR_OK;
}

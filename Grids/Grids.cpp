//
// Grids.cpp : Defines the initialization routines for the DLL.
//

/// 2DO NEXT TIME :
//
//  Structures de graphe pour permettre la recherche de chemin sur plusieurs grilles.
//  => Mise a jour et/ou precomp

#include "CKAll.h"
#include "GridManager.h"

#ifdef CK_LIB
#define RegisterBehaviorDeclarations    Register_Grids_BehaviorDeclarations
#define InitInstance                    _Grids_InitInstance
#define ExitInstance                    _Grids_ExitInstance
#define CKGetPluginInfoCount            CKGet_Grids_PluginInfoCount
#define CKGetPluginInfo                 CKGet_Grids_PluginInfo
#define g_PluginInfo                    g_Grids_PluginInfo
#else
#define RegisterBehaviorDeclarations    RegisterBehaviorDeclarations
#define InitInstance                    InitInstance
#define ExitInstance                    ExitInstance
#define CKGetPluginInfoCount            CKGetPluginInfoCount
#define CKGetPluginInfo                 CKGetPluginInfo
#define g_PluginInfo                    g_PluginInfo
#endif

#define CKPGUID_HEURISTIC CKGUID(0x72604f23, 0x5fe7f0f)
#define CKPGUID_PATHTYPE CKGUID(0x3e7f7035, 0x7835401f)
#define CKPGUID_FOLLOWMODE CKGUID(0x83e512e, 0x351c7933)

#define GRIDS_BEHAVIOR CKGUID(0x2d854713, 0x52771d50)

CKPluginInfo g_PluginInfo[2];

PLUGIN_EXPORT int CKGetPluginInfoCount() { return 2; }

///////////////////////
///   Param Op      ///
///////////////////////
void LayerTypeGetLayerByNameString(CKContext *context, CKParameterOut *res, CKParameterIn *p1, CKParameterIn *p2)
{
    CKGridManager *gm = (CKGridManager *)context->GetManagerByGuid(GRID_MANAGER_GUID);
    int layerType = gm->GetTypeFromName((char *)p1->GetReadDataPtr());
    res->SetValue(&layerType);
}

CKERROR InitInstance(CKContext *context)
{
    // Register Manager
    GridManager *gridManager = new GridManager(context);
    CKParameterManager *pm = context->GetParameterManager();

#define CKOGUID_GETLAYERBYNAME CKDEFINEGUID(0x470c4e6c, 0x144748b6)
    //--- register a new Operation Type
    pm->RegisterOperationType(CKOGUID_GETLAYERBYNAME, "Get Layer By Name");

    //--- register a new Parameter Operation

    pm->RegisterOperationFunction(CKOGUID_GETLAYERBYNAME, CKPGUID_LAYERTYPE, CKPGUID_STRING, CKPGUID_NONE, LayerTypeGetLayerByNameString);

    // gridfindpath START
    pm->RegisterNewEnum(CKPGUID_HEURISTIC, "Heuristic (distance calculation method)", "Euclidian Distance=1,Manhattan Distance=2,Squared Euclidian Distance=3,Optimized Euclidian Distance=4");
    pm->RegisterNewEnum(CKPGUID_PATHTYPE, "Path Type", "Path ID = 1 ,List of Point= 2 ,Curve = 3");
    pm->RegisterNewEnum(CKPGUID_FOLLOWMODE, "Follow Mode", "Step Follow=1,Simple Follow=2,Delayed Follow=3,Smooth Follow=4");

    // We hide the parameters (structures and flags)
    CKParameterTypeDesc *heur_param_type = pm->GetParameterTypeDescription(CKPGUID_HEURISTIC);
    if (heur_param_type)
        heur_param_type->dwFlags |= CKPARAMETERTYPE_HIDDEN;
    // gridfindpath END

    // Register Types
    pm->RegisterNewEnum(CKPGUID_LAYERSQUARETYPE_ENUM, "Layer Square Type", "Integer=1,Linker=2");
    pm->RegisterNewEnum(CKPGUID_LINKERGRAPH_ENUM, "Linker", "None=0,Start=1,End=2,Start/End=3,Door=4");

    return (CK_OK);
}

CKERROR ExitInstance(CKContext *context)
{
    CKParameterManager *pm = context->GetParameterManager();

    pm->UnRegisterParameterType(CKPGUID_HEURISTIC);
    pm->UnRegisterParameterType(CKPGUID_FOLLOWMODE);

    return CK_OK;
}

PLUGIN_EXPORT CKPluginInfo *CKGetPluginInfo(int Index)
{
    g_PluginInfo[0].m_Author = "Virtools";
    g_PluginInfo[0].m_Description = "Grids Building Blocks";
    g_PluginInfo[0].m_Extension = "";
    g_PluginInfo[0].m_Type = CKPLUGIN_BEHAVIOR_DLL;
    g_PluginInfo[0].m_Version = 0x000001;
    g_PluginInfo[0].m_InitInstanceFct = NULL;
    g_PluginInfo[0].m_ExitInstanceFct = NULL;
    g_PluginInfo[0].m_GUID = GRIDS_BEHAVIOR;
    g_PluginInfo[0].m_Summary = "Grids Building Blocks";

    g_PluginInfo[1].m_Author = "Virtools";
    g_PluginInfo[1].m_Description = "Grids/PathFinding Manager";
    g_PluginInfo[1].m_Extension = "";
    g_PluginInfo[1].m_Type = CKPLUGIN_MANAGER_DLL;
    g_PluginInfo[1].m_Version = 0x000001;
    g_PluginInfo[1].m_InitInstanceFct = InitInstance;
    g_PluginInfo[1].m_ExitInstanceFct = ExitInstance;
    g_PluginInfo[1].m_GUID = GRID_MANAGER_GUID;
    g_PluginInfo[1].m_Summary = GridManagerName;

    return &g_PluginInfo[Index];
}

PLUGIN_EXPORT void RegisterBehaviorDeclarations(XObjectDeclarationArray *reg);
void RegisterBehaviorDeclarations(XObjectDeclarationArray *reg)
{
    // Grid/Basic
    RegisterBehavior(reg, FillBehaviorFillLayerDecl);
    RegisterBehavior(reg, FillBehaviorGetLayerSquareDecl);
    RegisterBehavior(reg, FillBehaviorGetPosFromValueDecl);
    RegisterBehavior(reg, FillBehaviorLayerSliderDecl);
    RegisterBehavior(reg, FillBehaviorSetGridPriorityDecl);
    RegisterBehavior(reg, FillBehaviorSetLayerSquareDecl);
    RegisterBehavior(reg, FillBehaviorSwitchIfSquareDecl);
    RegisterBehavior(reg, FillBehaviorFillGridWithShapeDecl);

    // Grid/Path Finding
    RegisterBehavior(reg, FillBehaviorGridPathInitDecl);
    RegisterBehavior(reg, FillBehaviorGridPathSolverDecl);
    RegisterBehavior(reg, FillBehaviorCharacterGridPathFollowDecl);
}
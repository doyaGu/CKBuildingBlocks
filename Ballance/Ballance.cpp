#include "CKAll.h"

#ifdef CK_LIB
#define RegisterBehaviorDeclarations    Register_Ballance_BehaviorDeclarations
#define CKGetPluginInfoCount            CKGet_Ballance_PluginInfoCount
#define CKGetPluginInfo                 CKGet_Ballance_PluginInfo
#define g_PluginInfo                    g_Ballance_PluginInfo
#else
#define RegisterBehaviorDeclarations    RegisterBehaviorDeclarations
#define CKGetPluginInfoCount            CKGetPluginInfoCount
#define CKGetPluginInfo                 CKGetPluginInfo
#define g_PluginInfo                    g_PluginInfo
#endif

CKObjectDeclaration *FillBehaviorBallanceSetLoadingRangeDecl();
CKObjectDeclaration *FillBehaviorBallanceLoadNMORangeDecl();
CKObjectDeclaration *FillBehaviorBallanceDeleteLoadedNMODecl();
CKObjectDeclaration *FillBehaviorBallanceResolveLevelReferencesDecl();
CKObjectDeclaration *FillBehaviorBallanceFillLogicScriptsDecl();
CKObjectDeclaration *FillBehaviorBallanceEventRouterDecl();
CKObjectDeclaration *FillBehaviorBallanceBaseEventRouterDecl();

CKPluginInfo g_PluginInfo[1];

PLUGIN_EXPORT int CKGetPluginInfoCount() { return 1; }

PLUGIN_EXPORT CKPluginInfo *CKGetPluginInfo(int Index)
{
    g_PluginInfo[0].m_Author = "Ballance";
    g_PluginInfo[0].m_Description = "Ballance runtime orchestration building blocks";
    g_PluginInfo[0].m_Extension = "";
    g_PluginInfo[0].m_Type = CKPLUGIN_BEHAVIOR_DLL;
    g_PluginInfo[0].m_Version = 0x000001;
    g_PluginInfo[0].m_InitInstanceFct = NULL;
    g_PluginInfo[0].m_ExitInstanceFct = NULL;
    g_PluginInfo[0].m_GUID = CKGUID(0x4b414c4c, 0x414e4345);
    g_PluginInfo[0].m_Summary = "Ballance";

    return &g_PluginInfo[Index];
}

PLUGIN_EXPORT void RegisterBehaviorDeclarations(XObjectDeclarationArray *reg);

void RegisterBehaviorDeclarations(XObjectDeclarationArray *reg)
{
    RegisterBehavior(reg, FillBehaviorBallanceSetLoadingRangeDecl);
    RegisterBehavior(reg, FillBehaviorBallanceLoadNMORangeDecl);
    RegisterBehavior(reg, FillBehaviorBallanceDeleteLoadedNMODecl);
    RegisterBehavior(reg, FillBehaviorBallanceResolveLevelReferencesDecl);
    RegisterBehavior(reg, FillBehaviorBallanceFillLogicScriptsDecl);
    RegisterBehavior(reg, FillBehaviorBallanceEventRouterDecl);
    RegisterBehavior(reg, FillBehaviorBallanceBaseEventRouterDecl);
}

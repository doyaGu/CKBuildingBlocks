/*************************************************************************/
/*	File : CKGridManager.h												 */
/*	Author :  Cabrita Francisco											 */
/*																		 */
/*	Virtools SDK 														 */
/*	Copyright (c) Virtools 2001, All Rights Reserved.					 */
/*************************************************************************/
#ifndef GridManager_H
// {secret}
#define GridManager_H "$Id:$"

extern char *GridManagerName;

//{secret}
#define GRID_FLAG_SCENE 0
//{secret}
#define GRID_FLAG_LEVEL 1

#include "CKGrid.h"
#include "CKBaseManager.h"
#include "GridPathManager.h"

#define CKPGUID_LINKERGRAPH_ENUM CKGUID(0x16a90c1b, 0x740d0f12)
#define CKPGUID_PATHFINDINGCOLLISION CKDEFINEGUID(0x638737d6, 0xf783eae)

//{secret}
#define DEFAULT_LAYER_NAME "- default -"

/*************************************************
Summary:
The Grid Manager, manage all grids and is the gateway between a point in 3D Space and
the correspondant grid.
The Grid Manager, also manage the Layer Types (eg: fire, danger, grass)

Remarks:
- What we mean by the "Type" is the "Layer Type"

{Group:Managers classes}
See also:


*************************************************/
class GridManager : public CKGridManager
{
public:
    //____________________________________________________
    //      TYPE
    DLL_EXPORT int GetTypeFromName(CKSTRING TypeName);
    DLL_EXPORT CKSTRING GetTypeName(int type);
    DLL_EXPORT CKERROR SetTypeName(int type, CKSTRING Name);
    DLL_EXPORT int RegisterType(CKSTRING TypeName);
    DLL_EXPORT int UnRegisterType(CKSTRING TypeName);
    DLL_EXPORT CKERROR SetAssociatedParam(int type, CKGUID param);
    DLL_EXPORT CKGUID GetAssociatedParam(int type);
    DLL_EXPORT CKERROR SetAssociatedColor(int type, VxColor *col);
    DLL_EXPORT CKERROR GetAssociatedColor(int type, VxColor *col);
    DLL_EXPORT int GetLayerTypeCount();

    //____________________________________________________
    //      CLASSIFICATION
    DLL_EXPORT int GetClassificationFromName(CKSTRING ClassificationName);
    DLL_EXPORT CKSTRING GetClassificationName(int Classification);
    DLL_EXPORT int RegisterClassification(CKSTRING ClassificationName);
    DLL_EXPORT int GetGridClassificationCategory();

    //____________________________________________________
    //      GRID FINDING
    DLL_EXPORT const XObjectPointerArray &GetGridArray(int flag = GRID_FLAG_SCENE);
    DLL_EXPORT CKGrid *GetNearestGrid(VxVector *pos, CK3dEntity *ref = NULL);
    DLL_EXPORT CKGrid *GetPreferredGrid(VxVector *pos, CK3dEntity *ref = NULL); // Considering Priority
    DLL_EXPORT CKBOOL IsInGrid(CKGrid *grid, VxVector *pos, CK3dEntity *ref = NULL);

    //____________________________________________________
    //      GRID PATH FINDING
    DLL_EXPORT GridPathManager *GetGridPathManager();

    //____________________________________________________
    //      ADDING / REMOVING GRID OBJECTS
    DLL_EXPORT int GetGridObjectCount(int flag = GRID_FLAG_SCENE);
    DLL_EXPORT CKGrid *GetGridObject(int pos, int flag = GRID_FLAG_SCENE);

    //____________________________________________________
    //      OTHER ...
    DLL_EXPORT void FillGridWithObjectShape(CK3dEntity *ent, int type, void *fillVal);
    DLL_EXPORT void FillGridWithObjectShape(CK3dEntity *ent, int solidLayerType, int shapeLayerType, void *fillVal);

    void FillBoundingBox(const VxBbox &box, int solidLayerType, void *fillVal);

    void Init() {}
    CKERROR OnCKInit();
    CKERROR PostClearAll();
    CKERROR PostLoad() { return CK_OK; }
    CKERROR PreSave();
    CKERROR OnCKReset();
    CKERROR PreProcess();
    CKERROR SequenceToBeDeleted(CK_ID *objids, int count);

    CKDWORD GetValidFunctionsMask()
    {
        return CKMANAGER_FUNC_OnSequenceToBeDeleted |
               CKMANAGER_FUNC_PostClearAll |
               CKMANAGER_FUNC_OnCKInit |
               CKMANAGER_FUNC_PreSave |
               CKMANAGER_FUNC_PostLoad |
               CKMANAGER_FUNC_OnCKReset |
               CKMANAGER_FUNC_PreProcess;
    }
    CKERROR LoadData(CKStateChunk *chunk, CKFile *LoadedFile);
    CKStateChunk *SaveData(CKFile *SavedFile);

    ////////////////////////////////////////////////////////
    ////               Private Part                     ////
    ////////////////////////////////////////////////////////

    //{secret}
    GridManager(CKContext *Context);
    //{secret}
    ~GridManager();
    //{secret}
    void ClearData();
    //{secret}
    void InitData();
    //{secret}

protected:
    GridPathManager *m_GridPathManager;
    void InternalClearData(CKBOOL unregister = TRUE);

    int m_GridAttribute;            // id for the grid attribut given by the Attribute Manager
    int m_GridClassificationCatego; // id of the 'Grid Classification" Catego

    int m_LayerTypeCount;
    CKSTRING *m_LayerTypeName;  // Name of layer types (eg: "collision", "rugosity", ...)
    VxColor *m_LayerTypeColor;  // Associated Color with the Layer Type
    CKGUID *m_AssociatedParam;  // Associated Parameter Type (eg: FLOAT, INT, SOUND, ...)
    int *m_LayerAttributeIndex; // Associated Attribute Index

    //{secret}
    // fill intersected grids with the object shape
    void FillPartWithShape(CK3dEntity *ent, CKGrid *grid, Vx2DVector min, Vx2DVector max, int solidLayerType, int shapeLayerType, void *fillVal);
    void FillPartWithRect(CKGrid *grid, const CKRECT &r, int LayerType, void *fillVal);
};

// GRIDS FILES VERSION ...
#endif
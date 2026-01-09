#ifndef GRIDPATHMANAGER
#define GRIDPATHMANAGER

//----------------------------------------------------------------------------------------------------------
#include "XList.h"
#include "XSHashTable.h"
#include "PFUtils.h"
#include "NodeLinker.h"
#include "NodeGrid.h"
#include "GridInfo.h"
#include "PathProblem.h"
#include "FollowProblem.h"
//----------------------------------------------------------------------------------------------------------
// The minimun DEFAUL_MAX_PATH_CONTEXT must be 1 because we need 1 context to perform precalculs.
#define DEFAUL_MAX_PATH_CONTEXT             1 
#define DEFAUL_MAX_FOLLOW_CONTEXT           0 

#define PRECALC_OPENLIST_BUFFER_FACTOR      0.5	
#define REALTIME_OPENLIST_BUFFER_FACTOR     0.5
#define DISTANCE_CASE_JUXTAPOSE             1	
#define DISTANCE_CASE_DIAGONAL              1.414f

#define STATE_ISNTCOMPUTED                  0
#define STATE_GETSTARTENDGRIDSID            GetStartEndGridID
#define STATE_BEFORESTARTLINKER             BeforeStartLinker
#define STATE_GETLINKERFROMSTART            GetLinkerFromStart
#define STATE_GETLINKERFROMEND              GetLinkerFromEnd
#define STATE_AFTERENDLINKER                AfterEndLinker
#define STATE_DIRECTPATH                    DirectPath
#define STATE_GETGRID2DRIDPATH              GetGrid2GridPath
#define STATE_GETALLCASE2CASEPATH           GetAllCase2CasePath
#define STATE_PATHFOUND                     PathFound
#define STATE_PATHNOTFOUND                  PathNotFound
#define STATE_FINISH                        Finish
//----------------------------------------------------------------------------------------------------------
class GridPathManager
{
protected:
    CKBOOL m_Reseted;
    /************************************/
    /* Path finding protected attributs */
    /************************************/
    CKAttributeManager *m_AttributManager;
    XHashTable<int, int> m_Target2PathContext;

    // Num Layer to consider.
    int m_NumLayer;

    // number of case of the bigger grid.
    int m_MaxCase;

    // Array of path problem.
    XArray<PathProblem *> m_ArrayPathProblem;

    // List of free path context index.
    XList<int> m_StackFreePathIndex;
    int m_MaxPathProblem;

    // Does the graphs have been calculated ?
    CKBOOL m_GraphDone;

    // Buffer of path which stay to be followed
    XSHashTable<Path *, int> m_PathID2Path;
    int m_FollowPathID;

    /*************************************/
    /* Path follower protected attributs */
    /*************************************/
    XSHashTable<int, int> m_PathID2FollowContext;

    // Array of Follow Problem.
    XArray<FollowProblem *> m_ArrayFollowProblem;

    // List of free follow context index.
    XList<int> m_StackFreeFollowIndex;
    int m_MaxFollowProblem;

    // Number of problem to search.
    int m_NumFollowProblem;

    // PF Collision attribut type
    CKAttributeType m_PFCAttrType;

public:
    /*********************************/
    /* Path finding public attributs */
    /*********************************/

    // Generale data.
    CKGridManager *m_GridManager;
    CKMessageManager *m_MessageManager;

    // Graph representation of all linkers.
    XList<NodeLinker *> m_ListNodeLinker;

    // Array of grid's graph representation.
    XSHashTable<GridInfo *, int> m_ListGridInfo;

    // Number of problem to search.
    int m_NumPathProblem;

    // Tools for a* search.
    static const float m_SuccesorIndexCost[12];
    static const float m_SuccesorIndexCost2[6];

    int m_IndexSuccStart;
    int m_IndexSuccEnd;
    CKLayer *m_Layer;

    CKBOOL m_SuccesorID[12];
    float m_SuccesorCost[12];
    int m_SuccesorLayerCost[12];

    int m_NumSucc, m_Count, m_GridW, m_GridL;
    int m_CaseX, m_CaseY, m_LayerVal;
    int m_TempX, m_TempY;

    // Convert layer id in layer index.
    int *m_LayerID2Index;

    // Data to compute A* on all grid.
    NodeGrid *m_ArrayNodeGrid;

    /**************************/
    /* Constructor/destructor */
    /**************************/

    GridPathManager(CKGridManager *, CKMessageManager *, CKAttributeManager *);
    ~GridPathManager();

    /*******************/
    /* Reset functions */
    /*******************/
    void Reset();

    /*********************************/
    /* Graphs construction functions */
    /*********************************/

    // Construct a graph only with linker (a linker link two grids).
    CKBOOL ConstructListNodeLinker(XList<int> *);
    // Call the two above function...
    CKBOOL ConstructGraph(XList<int> *);

    /**********************************/
    /* Regiser path problem functions */
    /**********************************/

    // Now the status of a target.
    int GetTargetStatus(CK3dEntity *);
    // Register a new path problem.
    CKBOOL RegisterPathProblem(CK3dEntity *, VxVector *, CK3dEntity *, VxVector *, int, int, float, CKBOOL, CKBOOL, float, float, int, CKBOOL, CKBOOL);
    void ResizePathContextArray(int);
    // Unregister a path problem.
    CKBOOL UnregisterPathProblem(CK3dEntity *, CKBOOL);

    /*********************/
    /* Path recuperation */
    /*********************/

    void GetPath(CK3dEntity *, float &, int &);
    void GetPath(CK3dEntity *, float &, CKCurve *);
    void GetPath(CK3dEntity *, float &, CKDataArray *);

    /**********************/
    /* Follower functions */
    /**********************/

    CKBOOL RegisterFollowProblem(FollowStruct *);
    CKBOOL UnregisterFollowProblem(int);
    void ResizeFollowContextArray(int);

    CKBOOL SetStateFollowProblem(int, FollowState);
    FollowState GetStateFollowProblem(int);

    void ConstructFollowData();

    /************************/
    /* Management functions */
    /************************/

    // Path finding management.
    void ManagePathFinding();
    // Path follow management.
    void ManagePathFollow();
    // Manage.
    void Manage();
    // Character destruction management
    CKERROR SequenceToBeDeleted(CK_ID *objids, int count);

    /*********/
    /* Tools */
    /*********/

    CKBOOL GridContainOneLayer(CKGrid *, XList<int> *) const;
    inline void FillSuccessor(CKGrid *, int, int);
    inline void FillSuccessor(PathProblem &, int);
    inline CKBOOL CaseIsLinker(PathProblem &, CKGrid *, int);
    inline CKBOOL CaseIsLinker(PathProblem &, int);
    GridContext *CasesDistance(CKGrid *, int, int, int, int, int, int &);
    void ResetArrayContextInfo(int, int) const;
    void ResetArrayLinkerContextInfo(int) const;
    int GetGridNum(CKGrid *);
    void OptimizePath(PathProblem &);
    CKBOOL ObsBetweenPoint(PathProblem &, int, int, int);
};
//----------------------------------------------------------------------------------------------------------
/********************/
/* Search functions */
/********************/
// These function are the differente state than a path problem can be.

// Give the start grid id and the end grid id.
int GetStartEndGridID(PathProblem &, int);

int BeforeStartLinker(PathProblem &, int);

// Give list of Nodelink accesible from start.
int GetLinkerFromStart(PathProblem &, int);

// Give list of Nodelink accesible from end.
int GetLinkerFromEnd(PathProblem &, int);

// Just search a path between start and and without to pass in linker.
int DirectPath(PathProblem &, int);

// Give a list of NodeLink to go to the start grid to the end grid.
int GetGrid2GridPath(PathProblem &, int);
void ManageNodeLinkerSuccesor(PathProblem &, int, NodeLinker *, NodeLinker *, float);
void AnalyseGrid2GridPath(PathProblem &, int);
void FindHypPath(PathProblem &, int, NodeLinker *, NodeLinker *, SubPath **, SubPath **);

// Give a list of NodeGrid to go to the start case to the en d case of a grid.
int GetCase2CasePath(PathProblem &, int);

// Give the result path on a list of NodeGrid.
int GetAllCase2CasePath(PathProblem &, int);

// Cool...
int PathFound(PathProblem &, int);

// Not cool...
int PathNotFound(PathProblem &, int);

// Nothing
int Finish(PathProblem &, int);
//----------------------------------------------------------------------------------------------------------
#endif

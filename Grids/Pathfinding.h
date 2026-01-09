#ifndef _GRID_PATHFINDER_
#define _GRID_PATHFINDER_

#include "CKAll.h"

/*-----------------------------------------------------------------------------
    Class : PathNode

    Comment : Path's logical component

    Members :
        - m_X, m_Y : Position on grid
        - m_GridIndex : Index of Grid's descriptor (CKGrid, PathNode Matrix)
        - m_Flags : State Flags (Side, Open/Close, Direction <= not sure)
        - m_Cost, m_G : Node's total cost and partial
                        (from the start to this node) cost
        - m_NextOpen, m_NextClose
-----------------------------------------------------------------------------*/

#define NODE_FAILURE 64
#define NODE_GOALSIDE 32
#define NODE_CLOSE 16
/*
  Flags :

       ...   8 4 2 1
    |_|_|_|_|_|_|_|_|
       | | | \--+--/
 Fail -+ | |    |
         | |    +--  Direction
 Side ---+ |

         Close
*/

class GridPathFinder;

class PathNode
{
    int m_X, m_Y;		   // Node's position
    int m_GridIndex;	   // Grid's Descriptor
    unsigned char m_Flags; // Flags
    float m_Cost, m_G;	   // Node's total and partial cost.
                           // Partial cost is used for square checking and Total cost (ie. partial + heuristic)
                           // is used for Open List's sort ...
    PathNode *m_NextOpen;  // Next in Open List
    PathNode *m_NextClose; // Next in Close List

public:
    // Constructor / Destructor
    PathNode() : m_NextOpen(NULL), m_NextClose(NULL), m_GridIndex(-1) {}
    PathNode(int x, int y, unsigned char flags, int gridIndex, PathNode *father, float g, float h);
    PathNode(VxVector position, unsigned char flags, GridPathFinder *pathfinder, PathNode *father, float g, float h);

    ~PathNode(){};

    // Get / Set
    void GetPosition(int &x, int &y)
    {
        x = m_X;
        y = m_Y;
    }

    void SetGrid(int descriptor) { m_GridIndex = descriptor; }
    int GetGrid() { return (m_GridIndex); }

    void SetScores(float g, float total)
    {
        m_Cost = total;
        m_G = g;
    }
    float GetScore() { return (m_Cost); }
    float GetCost() { return (m_G); }

    void SetNextOpen(PathNode *next) { m_NextOpen = next; }
    void SetNextClose(PathNode *next) { m_NextClose = next; }
    PathNode *GetNextOpen() { return (m_NextOpen); }
    PathNode *GetNextClose() { return (m_NextClose); }

    void SetClosed() { m_Flags |= NODE_CLOSE; }

    unsigned char GetFlags() { return (m_Flags); }

    // Test
    int IsClosed() { return (m_Flags & NODE_CLOSE); }
    int IsOpposite(PathNode *node) { return (this->m_Flags ^ node->m_Flags) & NODE_GOALSIDE; }
    int IsGoalSide() { return (m_Flags & NODE_GOALSIDE); }
    int IsCorrupted() { return (m_Flags & NODE_FAILURE); }

    // Other
    unsigned char GenerateFlag(int direction) { return ((m_Flags & ~15) | (((unsigned char)direction) & 15)); }

    // List Managment
    void Insert(PathNode *toInsert);
};

/*-----------------------------------------------------------------------------
    Class : GridDescriptor

    Comment : Grid's temporary data for path-finding

    Members :
        - m_Grid : Reference to the grid object in the real scene
        - m_Matrix : Nodes Array (logical representation of m_Grid)
        - m_ObstacleLayer, m_ObstacleLayerCount : Obstacle Layers (and number of them) used by the actor
        - m_LinkerLayer, m_LinkerLayerCount: Same thing but with the linker layer ...
-----------------------------------------------------------------------------*/

class GridDescriptor
{
    CKGrid *m_Grid; // Useful to retrieve grid's dimensions and other datas

    PathNode **m_Matrix; // Direct link to Open/Close list

    int m_ObstacleLayerCount; // Informations about obstacle layers
    CKLayer **m_ObstacleLayer;
    // Add obstacle's limit value
    int *m_ObstacleLimit;

    int m_LinkerLayerCount; // Informations about linker layers
    CKLayer **m_LinkerLayer;
    // Add linker's use cost
    float *m_LinkerCost;

public:
    // Constructor / Destructor
    GridDescriptor() : m_Grid(NULL), m_Matrix(NULL), m_ObstacleLayer(NULL), m_LinkerLayer(NULL) {}
    GridDescriptor(CKGrid *grid, GridPathFinder *pathfinder);

    ~GridDescriptor();

    // Get / Set
    CKGrid *GetGrid() { return (m_Grid); }
    int GetLinkerLayerCount() { return (m_LinkerLayerCount); }
    int GetObstacleLayerCount() { return (m_ObstacleLayerCount); }
    CKLayer *GetLinkerLayer(int index) { return (m_LinkerLayer[index]); }
    int GetLinkerLayerType(int index) { return (m_LinkerLayer[index]->GetType()); }
    void SetNode(int x, int y, PathNode *node)
    {
        m_Matrix[y * m_Grid->GetWidth() + x] = node;
    }
    float GetLinkerLayerCost(int index) { return (m_LinkerCost[index]); }

    // Test
    int IsClosed(int x, int y);
    float IsBlocked(int x, int y, float cost);
    PathNode *IsBlocked(int x, int y, GridPathFinder *gdp, int direction, int desc);
    int CheckDeparture(int x, int y, int linkerIndex, unsigned char flags);
};

/*-----------------------------------------------------------------------------
    Class : GridPathFinder

    Comment : Intermediate Datas for Path finding computation

    Members :
        - ... pfff ... :)

    Remarks :
        Simplification ; No choice for heuristic function. We get improved set
        of function => G : Euclidian for each movement, H : Manhattan Distance.

        We let the possibility to set the heuristic coefficient.
-----------------------------------------------------------------------------*/

/*
    Flags :

        ............  ....   8 4 2 1
    |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|
                         \___/ | | |
        Not Used           |   | | +-- Main Phases : 1 -> Path Find, 0 -> Result Construction
                           |   | |
                           |   | +-- Secondary Phases :
                           |   |         Path Find,
                           |   |            1 -> Head Check (Statistically impossible)
                           |   |			0 -> Extension (Always ;p)
                           |   |         Result Construction
                           |   |            0 -> Nodes recombinaison
                           |   |			1 -> CKControlPoints & CKCurves Construction
                           |   |
                           |   +-- 'Extension' Phases :
                           |           1 -> Linker extension
                           |		   0 -> Normal extension
                           |
                           +-- Finish : 001 -> Normal, 011 -> Failure, 101 -> Global Timeout, 111 -> No Path
*/

#define GDP_MAINPHASE 1
#define GDP_SECONDARY 2
#define GDP_EXTENSION 4 // No needs. Implicit uses of the indexes ...

#define GDP_FINISHED 8
#define GDP_FAILURE 16
#define GDP_NOPATH 32
#define GDP_GLOBALTIMEOUT 32

#define GDP_NOTINITIALIZE 64

class GridPathFinder
{
    friend class GridDescriptor;

    // --- Static Datas ------------------------------
    static char s_DifX[8];
    static char s_DifY[8];
    static float s_DifC[9];

    // --- Computation Datas -------------------------

    // Grid's Managment
    int m_DescriptorCount;
    GridDescriptor **m_GridDescriptor;

    // Open List (only one for the two side)
    PathNode *m_OpenList;

    // Start and Goal Nodes
    PathNode *m_StartNode, *m_GoalNode;

    // Heuristic coefficient
    float m_Coefficient;

    // Normal Extension Limit (Depends on connexity)
    int m_ExtensionLastIndex;

    // Flags
    unsigned short int m_Flags;

    // --- First Datas -------------------------------

    // Goal Position
    VxVector m_GoalPosition;

    // Pathfind Actor
    CK3dEntity *m_Actor;

    // Layers informations
    int m_ObstacleLayerCount;
    int *m_ObstacleLayerType;
    int *m_ObstacleLimit;

    int m_LinkerLayerCount;
    int *m_LinkerLayerType;
    float *m_LinkerCost;

    // Some Informations about attributs
    int m_AttributeCategoryIndex;

    // --- Docked Datas ------------------------------

    // Informations about time managment
    float m_TimeOut;
    float m_StartTime;
    CKTimeManager *m_TimeManager;

    // Result Paths
    PathNode *m_StartSide;
    PathNode *m_GoalSide; // The two parts of the path

    // Grid Manager
    CKGridManager *m_GridManager;

    // Behavior
    CKBehavior *m_Behavior; // Calling Behavior =>

    // --- Slicing Datas -----------------------------

    // Index for controled loops
    int m_ControlIndex;
    int m_SecondIndex;
    int m_NormalIndex;

    // Node to extend or to insert
    PathNode *m_CurrentNode;

    // Number of nodes extracted
    int m_ExtractedCount;

    // --- Statistics --------------------------------

    // Total Execution Time
    float m_TotalExecutionTime;
    float m_TotalTimeOut;

public:
    // Constructor / Destructor
    GridPathFinder();
    GridPathFinder(CKBehavior *dock, CK3dEntity *actor, VxVector goal, float heuristicCoefficient = 0.3);

    ~GridPathFinder();

    // Get / Set
    void SetActor(CK3dEntity *actor);
    CK3dEntity *GetActor() { return (m_Actor); }

    int GetObstacleLayerCount() { return (m_ObstacleLayerCount); }
    int GetObstacleLayerType(int index) { return ((index < m_ObstacleLayerCount) ? m_ObstacleLayerType[index] : (-1)); }

    int GetLinkerLayerCount() { return (m_LinkerLayerCount); }
    int GetLinkerLayerType(int index) { return ((index < m_LinkerLayerCount) ? m_LinkerLayerType[index] : (-1)); }

    void SetTimeout(float timeout) { m_TimeOut = timeout; }

    void SetFailureMode() { m_Flags |= GDP_FAILURE; }

    void SetHeuristicCoefficient(float coef) { m_Coefficient = coef; }

    void SetFourConnexity(CKBOOL four = TRUE) { m_ExtensionLastIndex = (four == TRUE) ? 4 : 8; }

    // Test
    int CheckForGridDescriptor(CKGrid *grid);
    int IsFailed() { return (m_Flags & GDP_FAILURE); }
    int IsFinished() { return (m_Flags & GDP_FINISHED); }

    // Launch
    void Launch();

private:
    void Search();
    void Construct();
    int HeadCheck();
    int LinkerExtension();
    int NormalExtension();

    void ExtendNode(int gridDescriptor, int x, int y, float cost, int direction = 8);
};

#endif
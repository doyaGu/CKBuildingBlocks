#ifndef COMPUTEPATH_BEHAVIOR
#define COMPUTEPATH_BEHAVIOR

/*
Bidirectional Iterative Deepening Path-Finding
-----------------------------------------------

  --- Beta-version Cool Stuffs : ----------------------------------------

  - Isolated grid path-finding
  - One-squared problem solver
  - Memory leaks
  - Ugly slicing

  --- Next-Version Improvements : ---------------------------------------

  - Multi-grid path-finding
        Problems:
        -- How can we determine the Input/Output point (or square) in a grid?
        -- Which policy to use? (i.e., are there possible obstacles between grids? Must we consider
        the grid set as a nodal path?)

        => Current Policy: Grids are node's docks for nodal paths.
        => Check for direct and implied links (problems in updating implied links: this type of link).
        => Where can we store information about the grid's nodal-path?
        => How to interface this knowledge?

  - Unit (i.e., problem solver) is contained in more than a square (i.e., 2x2, 3x3 ... square schemes)
    (ugly implementation and bad results)
        Problems:
        -- Cursor location and transition

  - Impermeable and reduced memory use    ( ... hmm ... )
        -- Use only one matrix for both sides instead of one for each ...
        -- Better Open and Close list memory allocation

  - Nice slicing    ( ... problems with curve update slicing )
        -- More than a single loop-test ...

  - Improved heuristics
        -- Add Orientation
        -- Obstacle avoidance cost estimation
        -- Moving Obstacle Estimation
        ==> Problems with multi-layered systems

  - Improved child generation
        -- Use orientation to limit proposition generation
        ==>  ... no significant results. Possible results included in Improved Heuristics.

  - Open list manager
        -- Too much time wasted in open list insertion - Use hashtable or local open list
        Problems:
        -- Local open list management: 1st case => Complete position parsing, each grid square
        has its open list. Useful for corrupted open node elimination => Fewer nodes to check.
        2nd case => Score parsing ... o(n/2) => o(c * log n) (c = handicap factor implied by
        added tests)
*/

#include "CKAll.h"
#include "Sliceable.h"

// Computation Matrix's cell
class PathCell
{
public:
    // Path cost when arrived to the cell
    float f_Cost;
    // Direction of the winning path
    unsigned char f_Direction;

    PathCell()
    {
        f_Cost = 400000000;
        f_Direction = 0;
    }
};

// Path Proposition
class PathKey
{
public:
    // Cost of the current path
    float f_Cost, f_g;
    // Position of the node
    int f_x, f_y;
    // Link to the next node in Open and Close List.
    PathKey *f_Next;
    PathKey *f_Father;

    PathKey() : f_Next(NULL), f_Father(NULL) {}
};

// Flags Definition ---------------------------------------

// Problem initialized?
#define INITIALIZED 1

// Current Operation: Path-finding or Path-Reconstruction
#define PATHFINDING 2

// 4 or 8 connectivity
#define FOURCONNEX 4

// Goal or Start Side?
#define GOALSIDE 8

// Success or Failure
#define FAILURE 16

// Final path-construction phase
#define FINALPASS 32

//---------------------------------

class PathFindingData : public Sliceable
{
    // Algorithm Datas

    // Add Start Grid and Goal Grid

    // Start's coordinates
    int m_StartX, m_StartY;
    // Goal's coordinates
    int m_GoalX, m_GoalY;
    // Unit Dimension
    unsigned int m_UnitSize;
    // Grid's dimension
    unsigned int m_Width, m_Height;
    // Start Matrix
    PathCell *m_StartMatrix;
    // End Matrix
    PathCell *m_GoalMatrix;
    // Stack Maximum Length
    unsigned int m_StackLength;
    // Stacks
    PathKey *m_GoalStack, *m_StartStack;
    // Stack Occupation
    unsigned int m_GoalKeyCount, m_StartKeyCount;

    // Temporary Key
    PathKey *m_TempKey;
    // Starting Time
    //	int m_StartTime ;
    // Time Manager
    //	CKTimeManager *m_TimeManager ;
    // Context
    CKContext *m_Context;
    // THE Path
    CKCurve *m_ResultPath;
    CKCurve *m_OldPath;
    // Contact Point between 'Start' and 'Goal' Search space
    PathKey *m_ContactPoint;
    // Heuristic Coefficient
    float m_Coefficient;

    // Algorithm Parameters

    int m_WallLimit;
    // Constraint Grid
    // CKLayer *m_Grid ; // => Create arrays with active grids. (use IsActive).
    // Maybe set up attributes for the threshold depending on the layer

    // 1st Step: Common limit for all layers of the grid
    CKLayer **m_Grid;
    unsigned int m_LayerCount;

    CKGrid *m_GridContainer;
    // Start and Goal real position
    VxVector m_StartPosition, m_GoalPosition;
    // Timeout
    //	int m_Timeout ;
    // Heuristic method
    ASTARPATH_HEURISTIC_METHOD m_HeuristicMethod;

    // Class Management
    unsigned char m_Flags;

    unsigned int m_count;

public:
    // Constructor / Destructor
    PathFindingData() : m_Flags(0) {}

    PathFindingData(
        const CKBehaviorContext &context,
        VxVector *start,
        VxVector *goal,
        CKGrid *grid,
        // int layertype,		// Po besoin, on prend tous les layers actifs.
        float timeout,
        ASTARPATH_HEURISTIC_METHOD method,
        float coeff,
        CKCurve *oldPath);

    ~PathFindingData()
    {
        m_TimeManager = NULL;
        m_ResultPath = NULL;
        /*		if (m_GoalStack) delete m_GoalStack ;
                if (m_StartStack) delete m_StartStack ;*/
        // faire meilleur deletion.
        for (unsigned int i = 0; i < m_LayerCount; i++)
        {
            m_Grid[i] = NULL;
        }
        delete[] m_Grid;
        delete[] m_StartMatrix;
        delete[] m_GoalMatrix;
    }

    // Path Finding Method
    int ComputePath();

    // Get / Set
    void SetTimeout(float timeout) { m_Timeout = timeout; }
    float GetTimeout() { return (m_Timeout); }
    void SetPositions(VxVector start, VxVector goal);

    void SetWallValue(int wallVal) { m_WallLimit = wallVal; }
    void GetStartPosition(int &x, int &y)
    {
        x = m_StartX;
        y = m_StartY;
    }
    void GetGoalPosition(int &x, int &y)
    {
        x = m_GoalX;
        y = m_GoalY;
    }
    VxVector &GetStartPosition(void) { return m_StartPosition; }
    VxVector &GetGoalPosition(void) { return m_GoalPosition; }

    unsigned char GetFlags() { return m_Flags; }

    CKCurve *GetResult() { return (m_ResultPath); }

    void SetFourConnexity(CKBOOL connex)
    {
        if (connex == TRUE)
            m_Flags |= FOURCONNEX;
        else
            m_Flags &= ~FOURCONNEX;
    }
    void SetUnitSize(int unitsize)
    {
        m_UnitSize = unitsize;
    }

protected:
    void SetToGoalSide(CKBOOL side)
    {
        if (side == TRUE)
            m_Flags |= GOALSIDE;
        else
            m_Flags &= ~(GOALSIDE);
    }

    int FindPathInGrid(void);
    int ConstructCurvePath(void);

    void OptimizePath(PathKey *path);

    float ComputeDistance(int dx, int dy);
    int ComputeCost(int x, int y, int dx, int dy);

    PathKey *InsertNewKey(PathKey *toInsert, PathKey *list);
};

#endif
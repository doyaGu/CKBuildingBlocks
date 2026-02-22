#ifndef PATHPROBLEM
#define PATHPROBLEM
//----------------------------------------------------------------------------------------------------------
#include "XArray.h"
#include "XList.h"
#include "PFUtils.h"
#include "NodeLinker.h"
#include "OpenList.h"
#include "Path.h"
//----------------------------------------------------------------------------------------------------------
class GridPathManager;
//----------------------------------------------------------------------------------------------------------
class PathProblem
{
public:
    // Just to have acces to public attributs...
    GridPathManager *m_GridPathManager;

    // The state of the problem.
    CKWORD m_State;
    StatePathFunction m_StateFunction;

    // Time.
    VxTimeProfiler m_Time;

    // Problem description.
    CK3dEntity *m_Target;
    VxVector m_TargetPos;
    VxVector m_GoalPos;
    int m_ObstacleLayer;
    int m_ObstacleThreshold;
    float m_SlowingFactor;
    CKBOOL m_Linker;
    CKBOOL m_LinkerObs;
    float m_HeuristicCoef;
    float m_TimeFrame;
    int m_Heuristic;
    CKBOOL m_Diagonal;
    CKBOOL m_Optimize;

    // Precalculated values.
    float m_SlowingDivTreshold;

    // Problem solution description.
    OpenListLinker *m_OpenListLinker;
    OpenListGrid *m_OpenListGrid;
    CKGrid *m_GridStart;
    CKGrid *m_GridEnd;

    CKGrid *m_GridSearch;
    int m_XStartSearch;
    int m_YStartSearch;
    int m_XEndSearch;
    int m_YEndSearch;
    int m_EndIndex;

    // Intermediary solution.
    NodeLinker *m_NodeLinkerDynStart;
    NodeLinker *m_NodeLinkerDynEnd;
    NodeGrid *m_NodeGrideEnd;
    NodeLinker *m_NodeLinkerEnd;
    NodeLinkerIt m_ItLinkerStart;
    NodeLinkerIt m_ItLinkerEnd;
    CKBOOL m_StartPartSearch;

    // Linker of current extremities subpath.
    NodeLinker *m_Linker1;
    NodeLinker *m_Linker2;

    // Hyp solution
    XArray<SubPath *> m_HypSubPathStart;
    XArray<SubPath *> m_HypSubPathEnd;
    SubPath *m_SubPathStart;
    SubPath *m_SubPathEnd;
    SubPath *m_HypSubPathStart2End;
    float m_HypSubPathStart2EndCoast;

    // The solution !
    Path m_Path;
    int m_FollowPathID;

    // A* tools.
    int m_SuccesorIndex[12];
    CKSquare *m_Square;
    int m_GridW;
    int m_GridL;
    int m_NumGrid;

    PathProblem();
    ~PathProblem();
    void Set(CK3dEntity *, VxVector *, CK3dEntity *, VxVector *, int, int, float, CKBOOL, CKBOOL, float, float, int, CKBOOL, CKBOOL, StatePathFunction);
    void SetOpenList(int, int, int);
    void Reset();
    void ResetOpenList(CKBOOL);
    void BuildSubPath(int, SubPath *, CKBOOL = TRUE) const;
    void ResetTime();
    CKBOOL TimeNotOut();
    CKBOOL Linker2StartIsGood();
    CKBOOL Linker2EndIsGood();
    void SetLinkerType(TypeCase, TypeCase);
    void SetLinker(NodeLinker *, NodeLinker *);
    void InitBeforeCase2Case(CKGrid *, XSHashTable<GridInfo *, uintptr_t> *);
};
//----------------------------------------------------------------------------------------------------------
#endif

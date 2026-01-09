#ifndef FOLLOWPROBLEM
#define FOLLOWPROBLEM
//----------------------------------------------------------------------------------------------------------
#include "PFUtils.h"
#include "Path.h"
//----------------------------------------------------------------------------------------------------------
#define MIN_DISTANCE_FACTOR 0.1
//----------------------------------------------------------------------------------------------------------
class FollowProblem
{
public:
    CKMessageManager *m_MessageManager;

    // FP Collision attribut type
    CKAttributeType m_AttrType;
    CKAttributeManager *m_AttributManager;

    // Follow problem description.
    CK3dEntity *m_Target;
    int m_PathID;
    float m_MinDistanceFactor;
    float m_LimitAngleCos;
    float m_Fuzzyness;
    float m_Fuzzyness2;
    CKBOOL m_Pingpong;
    float m_MaxBlockedTime;
    float m_DistanceBlocked;
    int m_ColisionFilter;
    float m_RadiusColision;
    float m_RadiusAvoid;
    float m_EnterTeleTime;
    float m_TravelTeleTime;
    float m_ExitTeleTime;
    CKBehavior *m_Beh;
    CKBOOL m_ManageTeleporter;

    // Array of all FollowProblem
    XArray<FollowProblem *> *m_ArrayFollowProblem;

    NodeGrid *m_ArrayNodeGrid;

    // Follow problem resolution data.
    FollowState m_State;
    StateFollowFunction m_StateFunction;
    StateFollowFunction m_AlignFunction;
    StateFollowFunction m_MoveFunction;
    CKBOOL m_Invisible;
    CKBOOL m_LinkerObs;
    int m_TestTurnAngleCount;
    CKMessageType m_Message[4];

    // Geometric data.
    CKCharacter *m_Character;
    CK3dEntity *m_Charac;
    Path m_Path;
    int m_NumSubPath;
    SubPath *m_SubPath;
    SubPath *m_NextSubPath;
    int m_NumPoint;
    int m_TotalPoint;
    CKGrid *m_Grid;
    CKLayer *m_Layer;
    int m_W;
    int m_L;
    VxVector m_OldPosition;
    VxVector m_Position;
    VxVector m_RealPosition;
    VxVector m_Dir;
    VxVector m_RealDir;
    VxVector m_Dir1;
    VxVector m_Dir2;
    VxVector m_DirFP;
    VxVector m_NextPoint;
    VxVector m_NextPointFuzzy;
    VxVector m_GridScale;
    VxVector m_PosToMoveFrom;
    float m_Distance2Point;
    float m_RadiusCgrid;
    int m_X;
    int m_Y;
    int m_NextX;
    int m_NextY;
    div_t m_Dt;
    float m_MinDistance;
    float m_Distance;
    TypeCase m_OldTypeCase;

    // Time.
    VxTimeProfiler m_TimeTele;
    VxTimeProfiler m_TimeOut;
    VxTimeProfiler m_TimeBlocked;

    FollowProblem(CKMessageManager *, CKAttributeManager *, NodeGrid *);
    ~FollowProblem();
    void Set(FollowStruct *, Path *, CKAttributeType);
    void Reset();
    void FirstOrient(int = 0);
    void CalculDirection();
    void CorrectDirection();
    void Move();

    // Calcul direction.
    void ComputeDir();
    void PointAndDir();
    void PointAndDir2();
    void EnterTele();
    void TravelTele();
    void ExitTele();

    // Correct direction.
    void RejectByObstacle();
    CKBOOL ScanRadius(FollowProblem *&, FollowProblem *&);
    CKBOOL ScanRadius(CK3dEntity *&, CK3dEntity *&, float &);
    void AnalyseTarget(FollowProblem *, int &, int &, int &);
    void RejectByTarget(FollowProblem *);
    void RejectByTarget(CK3dEntity *, float);
    void AvoidTarget(FollowProblem *, int, int, int);
    void AvoidTarget(CK3dEntity *, float);
    void DetectBlocked();

    // Move
    void TurnAndUp();
    void TurnAndUp2();
    void Up();
};
//----------------------------------------------------------------------------------------------------------
void ComputeDir(FollowProblem *);
void EnterTele(FollowProblem *);
void TravelTele(FollowProblem *);
void ExitTele(FollowProblem *);

void AlignOnPath1(FollowProblem *);
void AlignOnPath2(FollowProblem *);
void AlignOnPath3(FollowProblem *);
void AlignOnPath4(FollowProblem *);
void AlignOnPath5(FollowProblem *);
void AlignOnPath6(FollowProblem *);

void TurnAndUp(FollowProblem *);
void TurnAndUp2(FollowProblem *);
void Up(FollowProblem *);
//----------------------------------------------------------------------------------------------------------
#endif
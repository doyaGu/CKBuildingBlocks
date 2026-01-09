#ifndef PFUTILS
#define PFUTILS
#include "CKAll.h"

//----------------------------------------------------------------------------------------------------------
#define COMMENT 0
//----------------------------------------------------------------------------------------------------------
enum TypeCase
{
    tc_normal,
    tc_tele1,
    tc_tele2,
    tc_tele3,
    tc_door
};
enum FollowState
{
    dontexist,
    on,
    slow,
    timeout,
    off,
    entertele,
    traveltele,
    exittele,
    finish
};
//----------------------------------------------------------------------------------------------------------
class NodeLinker;
class GridInfo;
class IndoorLink;
class PathProblem;
class FollowProblem;
class Path;
//----------------------------------------------------------------------------------------------------------
typedef XListIt<NodeLinker *> NodeLinkerIt;
typedef XListIt<GridInfo *> GridInfoIt;
typedef XListIt<int> IntIt;
typedef XListIt<float> FloatIt;
typedef XListIt<IndoorLink *> IndoorLinkIt;
typedef XSHashTableIt<GridInfo *, int> Grid2GridInfoIt;
typedef XSHashTableIt<Path *, int> PathID2PathIt;
//----------------------------------------------------------------------------------------------------------
#ifdef WIN32
typedef int(__cdecl *StatePathFunction)(PathProblem &, int);
typedef void(__cdecl *StateFollowFunction)(FollowProblem *);
#else
typedef int (*StatePathFunction)(PathProblem &, int);
typedef void (*StateFollowFunction)(FollowProblem *);
#endif

//----------------------------------------------------------------------------------------------------------
typedef struct FollowStruct
{
    CK3dEntity *target;
    int followPathID;
    int direction;
    float minDistanceFactor;
    float limitAngle;
    float fuzzyness;
    CKBOOL pingpong;
    float maxBlockedTime;
    float distanceBlocked;
    float enterTele;
    float travelTele;
    float exitTele;
    CKBehavior *beh;
} FollowStruct;
//----------------------------------------------------------------------------------------------------------
typedef enum
{
    HEURISTIC_EUCLIDIAN_DISTANCE = 1,
    HEURISTIC_MANHATTAN_DISTANCE = 2,
    HEURISTIC_SQUARED_EUCLIDIAN_DISTANCE = 3,
    HEURISTIC_OPTIMIZED_EUCLIDIAN_DISTANCE = 4
} ASTARPATH_HEURISTIC_METHOD;
//----------------------------------------------------------------------------------------------------------
#endif

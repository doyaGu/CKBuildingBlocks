#ifndef PATH
#define PATH
//----------------------------------------------------------------------------------------------------------
#include "XArray.h"
#include "NodeLinker.h"
#include "PFUtils.h"
//----------------------------------------------------------------------------------------------------------
class SubPath
{
public:
    CKGrid *m_Grid;
    int m_NumGrid;
    NodeLinker *m_LinkerStart;
    NodeLinker *m_LinkerEnd;
    XArray<int> m_ArrayCase;
    float m_Coast;
    CKBOOL m_Inversed;
    // Debug
    CK3dEntity *m_Target;
    int m_PathFID;

    SubPath(CKGrid *, float);
    ~SubPath();
    void SetLinker(NodeLinker *, NodeLinker *);
    void PushBack(int);
    void PushFront(int);
    int NbCase() const;
    int GetCase(int) const;
    XArray<int> *GetArrayCase();
    void Inverse();
};
//----------------------------------------------------------------------------------------------------------
class Path
{
public:
    XArray<SubPath *> m_ArraySubPath;
    VxVector m_LastPoint;
    float m_Coast;
    CKBOOL m_Inversed;
    CKBOOL m_LinkerObs;
    int m_ObstacleLayer;
    int m_ObstacleThreshold;

    Path();
    ~Path();
    void Clear();
    void AddSubPathFront(SubPath *);
    void AddSubPathBack(SubPath *);
    int NbSubPath() const;
    SubPath *GetSubPath(int);
    void CopyAndErase(Path &);
    void Inverse();
    void CalculCoast();
};
//----------------------------------------------------------------------------------------------------------
#endif

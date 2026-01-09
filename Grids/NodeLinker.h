#ifndef __NODELINKER_H__
#define __NODELINKER_H__
//----------------------------------------------------------------------------------------------------------
#include "Node.h"
#include "XList.h"
#include "PFUtils.h"
#include "ContextInfo.h"
//----------------------------------------------------------------------------------------------------------
class NodeLinker;
//----------------------------------------------------------------------------------------------------------
class IndoorLink
{
public:
    int m_LayerID;
    XList<NodeLinker *> m_ListSameGrid;
    XList<float> m_ListSameGridCoast;
    XList<int> m_ListSameGridLayerCoast;
    XList<int> m_ListSameGridMaxLayerCoast;

    IndoorLink();
    ~IndoorLink();
};
//----------------------------------------------------------------------------------------------------------
class NodeLinker : public Node
{
public:
    CKGrid *m_Grid;
    int m_NumParallelContext;
    int m_X;
    int m_Y;
    int m_LinkerID;
    TypeCase m_TypeCase;

    // if m_DirID == 4 (a door) m_Pos isn't NULL.
    VxVector *m_Pos;

    // Connected linkers with same id.
    XList<NodeLinker *> m_ListConnected;

    // Connected linkers with different id gut same grid and with a path between them.
    int m_NumLayer;
    IndoorLink *m_ArrayIndoor;
    XArray<LinkerContext *> m_ArrayContextInfo;

    // Dynamic linker (one per context) and there coast.
    XArray<NodeLinker *> m_DynNodeLinker;
    XArray<float> m_DynNodeLinkerCoast;

    // Layer index.
    int m_Attribut;

    // Follow Tools
    CKBOOL m_Occupy;

    NodeLinker(int, int);
    ~NodeLinker();

    void ClearConnected();
    void ClearDynConnected();
    void SetPos(VxVector &);
    void ResizeParallelContext(int);
};
//----------------------------------------------------------------------------------------------------------
#endif

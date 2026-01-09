#ifndef CONTEXTINFO
#define CONTEXTINFO
//----------------------------------------------------------------------------------------------------------
template <class T>
class ContextInfo
{
public:
    float m_Cost;
    float m_LayerCost;
    float m_F;
    CKBOOL m_Closed;
    CKBOOL m_InOpen;
    T m_Parent;

    ContextInfo() : m_Cost(0),
                    m_LayerCost(0),
                    m_F(0),
                    m_Closed(FALSE),
                    m_InOpen(FALSE),
                    m_Parent(0) {}

    ~ContextInfo() {}
};
//----------------------------------------------------------------------------------------------------------
class NodeGrid;
class NodeLinker;
//----------------------------------------------------------------------------------------------------------
typedef ContextInfo<NodeGrid *> GridContext;
typedef ContextInfo<NodeLinker *> LinkerContext;
//----------------------------------------------------------------------------------------------------------

#endif

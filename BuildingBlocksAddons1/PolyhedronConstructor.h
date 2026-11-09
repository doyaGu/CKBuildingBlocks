// PolyhedronConstructor.h: interface for the PolyhedronConstructor class.
//
//////////////////////////////////////////////////////////////////////

#ifndef POLYHEDRONCONSTRUCTOR
#define POLYHEDRONCONSTRUCTOR

#include <unordered_map>

// Groups the faces of a mesh into polyhedrons of (nearly) coplanar faces by
// growing regions across shared edges. This is the edge method used by the
// original BuildingBlocksAddons1.dll.
class PolyhedronConstructor
{
public:
    PolyhedronConstructor(CKMesh *mesh);
    ~PolyhedronConstructor();

    void Simplify(float threshold);

    int GetPolyhedronCount()
    {
        return m_Polyhedrons.Size();
    }

    void GetFaceVertexIndex(CKWORD face, int &a, int &b, int &c)
    {
        m_Mesh->GetFaceVertexIndex(face, a, b, c);
    }
    const VxVector &GetPolyhedronNormal(int i) { return m_Polyhedrons[i]->m_Normal; }
    const VxBbox &GetPolyhedronBox(int i) { return m_Polyhedrons[i]->m_Box; }
    const XArray<int> &GetPolyhedronFaceIndices(int i) { return m_Polyhedrons[i]->m_OriginalFaces; }
    const XArray<int> &GetPolyhedronVertexIndices(int i) { return m_Polyhedrons[i]->m_OriginalVertices; }
    CKBOOL GetPolyhedronSmooth(int i) { return m_Polyhedrons[i]->m_Smooth; }
    // Simplify() can leave merged polyhedrons (magic 1) in the list, as the
    // DLL does, and they pass this test like in the DLL.
    CKBOOL IsPolyhedronValid(int i)
    {
        return ((m_Polyhedrons[i]->m_Magic == 0) || (m_Polyhedrons[i]->m_Magic == 1));
    }

protected:
    // Classes
    struct Edge;
    typedef XArray<Edge *> EdgeArray;

    struct Polyhedron
    {
        Polyhedron() : m_Magic(0), m_Smooth(0), m_Released(FALSE) {}
        int m_Magic;
        CKBOOL m_Smooth;
        VxBbox m_Box;
        VxVector m_Normal;
        XArray<int> m_OriginalFaces;
        XArray<int> m_OriginalVertices;
        EdgeArray m_Edges;
        // Set once the polyhedron has been merged into another one. The DLL
        // deletes it then but can keep pointers to it; it is kept alive here.
        CKBOOL m_Released;
    };

    typedef XArray<Polyhedron *> PolyhedronArray;

    struct Edge
    {
        int a;
        int b;
        union
        {
            struct
            {
                Polyhedron *p1;
                Polyhedron *p2;
            } v;
            Polyhedron *p[2];
        };
        Edge() : a(-1), b(-1)
        {
            v.p1 = NULL;
            v.p2 = NULL;
        }
    };

    CKMesh *m_Mesh;
    int m_VertexCount;

    // The DLL indexes a dense vertexCount * (vertexCount - 1) edge array by
    // a * vertexCount + b; only the edges actually used are stored here.
    std::unordered_map<CKDWORD, Edge> m_Edges;

    PolyhedronArray m_Polyhedrons;         // All the polyhedrons
    PolyhedronArray m_ReleasedPolyhedrons; // Merged polyhedrons, owned until destruction

    Edge *GetEdge(int a, int b, Polyhedron *p);
    void Release(Polyhedron *p);

    // Methods
    void RSimplify(float threshold, Polyhedron &reference, Edge *edgefrom, Polyhedron &p);
};

#endif // POLYHEDRONCONSTRUCTOR

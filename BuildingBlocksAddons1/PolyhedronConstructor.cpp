// PolyhedronConstructor.cpp: implementation of the PolyhedronConstructor class.
//
//////////////////////////////////////////////////////////////////////
#include "CKAll.h"

#include <math.h>

#include "PolyhedronConstructor.h"

// fld/fmul/faddp on the x87 stack: the products of two floats are exact in
// double precision and the sum is accumulated z, y, x.
static inline CKBOOL IsCandidate(const VxVector &a, const VxVector &b, float threshold)
{
    double dp = static_cast<double>(a.z) * b.z + static_cast<double>(a.y) * b.y;
    dp += static_cast<double>(a.x) * b.x;
    // Unordered comparisons (NaN) count as candidates, like the DLL's fcomp test.
    return !(fabs(dp) < threshold);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

PolyhedronConstructor::PolyhedronConstructor(CKMesh *mesh)
    : m_Mesh(NULL), m_VertexCount(0)
{
    if (!mesh)
        return;

    m_Mesh = mesh;
    m_VertexCount = mesh->GetVertexCount();

    int fcount = mesh->GetFaceCount();
    CKWORD *faceindices = mesh->GetFacesIndices();
    for (int i = 0; i < fcount; ++i, faceindices += 3)
    {
        Polyhedron *p = new Polyhedron;
        m_Polyhedrons.PushBack(p);

        // Normal
        p->m_Normal = mesh->GetFaceNormal(i);

        // Bounding Box
        p->m_Box.Merge(mesh->GetFaceVertex(i, 0));
        p->m_Box.Merge(mesh->GetFaceVertex(i, 1));
        p->m_Box.Merge(mesh->GetFaceVertex(i, 2));

        // Face Index
        p->m_OriginalFaces.PushBack(i);

        // Edges
        CKWORD a, b, c;
        XMinMax(faceindices[0], faceindices[1], faceindices[2], a, b, c);

        // Smoothing
        for (int j = 0; j < 3; ++j)
        {
            VxVector vertexNormal;
            mesh->GetVertexNormal(faceindices[j], &vertexNormal);
            if (DotProduct(vertexNormal, p->m_Normal) < 0.99f)
            { // The vertex normal is different than the face normal
                // hence the face is smooth
                p->m_Smooth = TRUE;
                break;
            }
        }

        // Vertices indices
        p->m_OriginalVertices.PushBack(a);
        p->m_OriginalVertices.PushBack(b);
        p->m_OriginalVertices.PushBack(c);

        p->m_Edges.PushBack(GetEdge(a, b, p));
        p->m_Edges.PushBack(GetEdge(a, c, p));
        p->m_Edges.PushBack(GetEdge(b, c, p));
    }
}

PolyhedronConstructor::~PolyhedronConstructor()
{
    // Released polyhedrons can still be referenced from m_Polyhedrons.
    for (PolyhedronArray::Iterator it = m_Polyhedrons.Begin(); it != m_Polyhedrons.End(); ++it)
    {
        if (!(*it)->m_Released)
            delete *it;
    }
    for (PolyhedronArray::Iterator it = m_ReleasedPolyhedrons.Begin(); it != m_ReleasedPolyhedrons.End(); ++it)
        delete *it;
}

PolyhedronConstructor::Edge *PolyhedronConstructor::GetEdge(int a, int b, Polyhedron *p)
{
    const CKDWORD key = static_cast<CKDWORD>(a) * static_cast<CKDWORD>(m_VertexCount) + static_cast<CKDWORD>(b);
    Edge *edge = &m_Edges[key];
    edge->a = a;
    edge->b = b;
    if (edge->v.p1)
        edge->v.p2 = p;
    else
        edge->v.p1 = p;
    return edge;
}

void PolyhedronConstructor::Release(Polyhedron *p)
{
    // The DLL sets the magic, then destroys and deletes the polyhedron without
    // clearing the pointers that edges or the result list may still hold, so
    // it can be reached (and merged, or used by the lightmap) again. Its
    // memory is modelled as left intact: the object stays alive, unchanged,
    // until the constructor is destroyed.
    p->m_Magic = 1;
    if (p->m_Released)
        return;
    p->m_Released = TRUE;
    m_ReleasedPolyhedrons.PushBack(p);
}

void PolyhedronConstructor::RSimplify(float threshold, Polyhedron &reference, Edge *edgefrom, Polyhedron &p)
{
    // we add the polyhedron to the reference
    reference.m_OriginalFaces += p.m_OriginalFaces;
    reference.m_Box.Merge(p.m_Box);

    EdgeArray::Iterator it;
    // we first mark all the returning edges as reference return
    for (it = p.m_Edges.Begin(); it != p.m_Edges.End(); ++it)
    {
        // we find the index of the next face
        int index = 0;
        if (&p != (*it)->p[index])
            index = 1;

        // the edge is set to the reference face
        if ((*it)->p[index])
            (*it)->p[index] = &reference;
    }

    // we iterate on all the edges to propagate, if relevant
    for (it = p.m_Edges.Begin(); it != p.m_Edges.End(); ++it)
    {
        if (*it == edgefrom)
            continue; // this is were we came from, do not grow backward

        // we add the vertices
        if (!reference.m_OriginalVertices.IsHere((*it)->a))
            reference.m_OriginalVertices.PushBack((*it)->a);
        if (!reference.m_OriginalVertices.IsHere((*it)->b))
            reference.m_OriginalVertices.PushBack((*it)->b);

        // we find the index of the next face
        int index = 0;
        if (&reference == (*it)->p[index])
            index = 1;

        Polyhedron *nextpoly = (*it)->p[index];

        if (!nextpoly)
        {
            reference.m_Edges.PushBack(*it);
            continue; // this edge is a frontier
        }

        // If the next poly is the reference one, we are cycling = stop there
        if (nextpoly == &reference)
            continue;

        // Is the face a good candidate
        if (!IsCandidate(nextpoly->m_Normal, reference.m_Normal, threshold))
            continue;

        // we mark the polygon as smooth if the face added is
        reference.m_Smooth |= nextpoly->m_Smooth;

        // the edge is marked as internal
        (*it)->p[1 - index] = NULL;
        (*it)->p[index] = NULL;

        // the recursion
        RSimplify(threshold, reference, *it, *nextpoly);
    }

    // the polyhedron is removed from the pending list (it may already have
    // been moved to the result list as a reference, where the DLL leaves it)
    m_Polyhedrons.Remove(&p);
    Release(&p);
}

void PolyhedronConstructor::Simplify(float threshold)
{
    PolyhedronArray poly;
    poly.Reserve(m_Polyhedrons.Size());

    EdgeArray tempedges;
    tempedges.Reserve(3);

    while (m_Polyhedrons.Size())
    {
        Polyhedron *reference = m_Polyhedrons.PopBack();
        poly.PushBack(reference);

        tempedges.Swap(reference->m_Edges);

        for (EdgeArray::Iterator it = tempedges.Begin(); it != tempedges.End(); ++it)
        {
            // we find the index of the next face
            int index = 0;
            if (reference == (*it)->p[index])
                index = 1;

            // Unlike the SDK sample, the DLL does not clear the reference
            // from the parsed edges, so a later reference can still reach
            // (and absorb) this one through them.
            Polyhedron *nextpoly = (*it)->p[index];

            if (!nextpoly)
            {
                reference->m_Edges.PushBack(*it);
                continue; // this edge is a frontier
            }

            if (nextpoly == reference)
                continue;

            // Is the face a good candidate
            if (!IsCandidate(nextpoly->m_Normal, reference->m_Normal, threshold))
                continue;

            // we mark the polygon as smooth if the face added is not perfectly coplanar
            reference->m_Smooth |= nextpoly->m_Smooth;

            // the recursion
            RSimplify(threshold, *reference, *it, *nextpoly);
        }

        tempedges.Resize(0);
    }

    // Merged polyhedrons that had already been used as references stay in
    // the list, like in the DLL.
    m_Polyhedrons.Swap(poly);
}

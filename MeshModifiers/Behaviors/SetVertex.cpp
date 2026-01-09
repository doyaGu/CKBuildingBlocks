/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            SetVertex
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateSetVertexBehaviorProto(CKBehaviorPrototype **);
int SetVertex(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetVertexDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Set Vertex Properties");
    od->SetDescription("Set the properties of a vertex.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Vertex Index: </SPAN>Index of the vertex to modify.<BR>
    <SPAN CLASS=pin>Vertex Position: </SPAN>New local position of the vertex.<BR>
    <SPAN CLASS=pin>Vertex Normal: </SPAN>New local normal of the vertex.<BR>
    <SPAN CLASS=pin>Vertex Color: </SPAN>New color of the vertex (only in Prelit mode).<BR>
    <SPAN CLASS=pin>Vertex Specular Color: </SPAN>New specular color of the vertex (only in Prelit mode).<BR>
    <SPAN CLASS=pin>Vertex UV: </SPAN>New texture coordinates of the vertex.<BR>
    <SPAN CLASS=pin>UV Channel: </SPAN>Texture Coordinates Channel to modify (-1 means the mesh itself).<BR>
    <BR>
    */
    /* warning:
    If you do not link one input parameter, it will not be affected.<BR>
    */
    od->SetCategory("Mesh Modifications/Basic");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x696353b2, 0x40667087));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSetVertexBehaviorProto);
    od->SetCompatibleClassId(CKCID_MESH);
    return od;
}

CKERROR CreateSetVertexBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Set Vertex Properties");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Vertex Index", CKPGUID_INT);
    proto->DeclareInParameter("Vertex Position", CKPGUID_VECTOR);
    proto->DeclareInParameter("Vertex Normal", CKPGUID_VECTOR);
    proto->DeclareInParameter("Vertex Color", CKPGUID_COLOR, "128,128,128,255");
    proto->DeclareInParameter("Vertex Specular Color", CKPGUID_COLOR, "0,0,0,0");
    proto->DeclareInParameter("Vertex UV", CKPGUID_2DVECTOR);
    proto->DeclareInParameter("UV Channel", CKPGUID_INT, "-1");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);

    proto->SetFunction(SetVertex);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int SetVertex(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CKMesh *mesh = (CKMesh *)beh->GetTarget();
    if (!mesh)
        return CKBR_OWNERERROR;

    int vertex = 0;
    beh->GetInputParameterValue(0, &vertex);

    VxVector v;
    // Position
    if (!beh->GetInputParameterValue(1, &v))
    {
        mesh->SetVertexPosition(vertex, &v);
    }

    if (mesh->GetLitMode() == VX_PRELITMESH)
    {
        VxColor col;
        // Color
        if (!beh->GetInputParameterValue(3, &col))
        {
            mesh->SetVertexColor(vertex, RGBAFTOCOLOR(&col));
        }

        // Specular Color
        if (!beh->GetInputParameterValue(4, &col))
        {
            mesh->SetVertexSpecularColor(vertex, RGBAFTOCOLOR(&col));
        }
    }
    else
    {
        // Normal
        if (!beh->GetInputParameterValue(2, &v))
        {
            mesh->SetVertexNormal(vertex, &v);
        }
    }

    Vx2DVector v2;
    // Texture Coordinates
    if (!beh->GetInputParameterValue(5, &v2))
    {
        int channel = -1;
        beh->GetInputParameterValue(6, &channel);

        mesh->SetVertexTextureCoordinates(vertex, v2.x, v2.y, channel);
        mesh->UVChanged();
    }

    return CKBR_OK;
}

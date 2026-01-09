/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            VerticesTranslate
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateVerticesTranslateBehaviorProto(CKBehaviorPrototype **);
int VerticesTranslate(const CKBehaviorContext &behcontext);
CKERROR MeshModificationsCallBack(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorVerticesTranslateDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Vertex Translate");
    od->SetDescription("Translates one or more vertices of the current mesh.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=in>Reset: </SPAN>put back the vertices in the position they have when the buildin block was attached.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Vertex Index Start: </SPAN>Index of the first vertex to move.<BR>
    <SPAN CLASS=pin>Vertex Count: </SPAN>Number of vertices to move.<BR>
    <SPAN CLASS=pin>Translation Vector: </SPAN>Direction in the World Referential of the translation.<BR>
    <SPAN CLASS=pin>Translation Referential: </SPAN>Referential to the translation vector.<BR>
    */
    od->SetCategory("Mesh Modifications/Local Deformation");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x8032ebc, 0x791960c9));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00020000);
    od->SetCreationFunction(CreateVerticesTranslateBehaviorProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    return od;
}

CKERROR CreateVerticesTranslateBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Vertex Translate");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareInput("Reset");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Vertex Start Index", CKPGUID_INT, "0");
    proto->DeclareInParameter("Vertex Count", CKPGUID_INT, "1");
    proto->DeclareInParameter("Translation Vector", CKPGUID_VECTOR, "0,0,0");
    proto->DeclareInParameter("Translation Referential", CKPGUID_3DENTITY);

    proto->DeclareLocalParameter("Vertex Array", CKPGUID_VOIDBUF);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);

    proto->SetFunction(VerticesTranslate);
    proto->SetBehaviorCallbackFct(MeshModificationsCallBack);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int VerticesTranslate(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CK3dEntity *object = (CK3dEntity *)beh->GetTarget();
    if (!object)
        return CKBR_OWNERERROR;

    CKMesh *mesh = object->GetCurrentMesh();
    if (!mesh)
        return CKBR_OWNERERROR;
    if (beh->IsInputActive(1)) // we get by the reset input
    {
        if (beh->GetVersion() < 0x00020000) // Old Version with vertices stuffed inside
        {
            int nbvert = mesh->GetModifierVertexCount();
            CKDWORD vStride = 0;
            CKBYTE *varray = (CKBYTE *)mesh->GetModifierVertices(&vStride);

            VxVector *savePos = (VxVector *)beh->GetLocalParameterWriteDataPtr(0);
            if (!savePos)
                return 0;

            for (int i = 0; i < nbvert; i++, varray += vStride)
            {
                *(VxVector *)varray = savePos[i];
            }
        }
        else
        {
            CKScene *scn = behcontext.CurrentScene;
            CKStateChunk *chunk = scn->GetObjectInitialValue(mesh);
            if (chunk)
                mesh->LoadVertices(chunk);
            const VxBbox &bbox = mesh->GetLocalBox();
            beh->SetLocalParameterValue(1, &bbox);
        }
        beh->ActivateInput(1, FALSE);
    }
    else
    {
        VxVector nvect;
        beh->GetInputParameterValue(2, &nvect);

        int index;
        beh->GetInputParameterValue(0, &index);

        int count;
        beh->GetInputParameterValue(1, &count);

        CK3dEntity *ref = NULL;
        ref = (CK3dEntity *)beh->GetInputParameterObject(3);

        if (ref != object)
        {
            object->InverseTransformVector(&nvect, &nvect, ref);
        }

        CKDWORD vStride = 0;
        CKBYTE *vxv = (CKBYTE *)mesh->GetModifierVertices(&vStride);
        int vcount = mesh->GetModifierVertexCount();

        if (index + count > vcount)
        {
            behcontext.Context->OutputToConsole("Bad vertices number...");
            return CKBR_PARAMETERERROR;
        }

        vxv += vStride * index;
        for (int i = index; i < index + count; i++, vxv += vStride)
        {
            ((VxVector *)vxv)->x += nvect.x;
            ((VxVector *)vxv)->y += nvect.y;
            ((VxVector *)vxv)->z += nvect.z;
        }
        beh->ActivateInput(0, FALSE);
    }
    // reinitiate the Bounding Box

    mesh->ModifierVertexMove(TRUE, FALSE);

    beh->ActivateOutput(0);

    return CKBR_OK;
}

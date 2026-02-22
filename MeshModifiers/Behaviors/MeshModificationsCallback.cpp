/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//      General Mesh Modifications Callback
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR MeshModificationsCallBack(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORATTACH:
    {
        CK3dEntity *object = (CK3dEntity *)beh->GetTarget();
        // the user press the cancel button
        if (!object)
            return 0;
        CKMesh *mesh = object->GetCurrentMesh();
        if (!mesh)
            return 0;

        if (beh->GetLocalParameterCount() > 1)
        {
            const VxBbox &bbox = mesh->GetLocalBox();
            beh->SetLocalParameterValue(1, &bbox);
        }
    }
    break;
    case CKM_BEHAVIORDELETE:
    {
        if (beh->GetVersion() < 0x00020000)
        { // Old Version
            // we restore the initial mesh vertices
            CK3dEntity *object = (CK3dEntity *)beh->GetTarget();
            // the user press the cancel button
            if (!object)
                return 0;
            CKMesh *mesh = object->GetCurrentMesh();
            if (!mesh)
                return 0;

            int nbvert = mesh->GetModifierVertexCount();
            CKDWORD vStride = 0;
            CKBYTE *varray = (CKBYTE *)mesh->GetModifierVertices(&vStride);

            VxVector *savePos = (VxVector *)beh->GetLocalParameterReadDataPtr(0);
            if (!savePos)
                return 0;

            for (int i = 0; i < nbvert; i++, varray += vStride)
            {
                *(VxVector *)varray = savePos[i];
            }

            mesh->ModifierVertexMove(TRUE, TRUE);
        }
    }
    break;

    case CKM_BEHAVIORPOSTSAVE:
        break;
    }
    return CKBR_OK;
}

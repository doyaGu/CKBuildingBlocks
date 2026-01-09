/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            Explode
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorExplodeDecl();
CKERROR CreateExplodeProto(CKBehaviorPrototype **);
int Explode(const CKBehaviorContext &behcontext);
CKERROR ExplodeCallBackObject(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorExplodeDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Explode");
    od->SetDescription("Makes an object appear to explode by spreading out its faces from the barycenter of the object.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process<BR>
    <SPAN CLASS=in>Loop In: </SPAN>triggers the next step in the process loop.<BR>
    <BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <SPAN CLASS=out>Loop Out: </SPAN>is activated when the needs to loop.<BR>
    <BR>
    <SPAN CLASS=pin>Duration (ms): </SPAN>how long the whole process should last.<BR>
    <SPAN CLASS=pin>Progression Curve: </SPAN>allows you to adjust the effect's progression.<BR>
    <SPAN CLASS=pin>Max Distance Factor: </SPAN>determines how far the faces are spread (it is combined with the object radius to give the final distance) .<BR>
    <SPAN CLASS=pin>Spreading Direction: </SPAN>permits spreading the faces in a forced direction (in the local referential).<BR>
    <SPAN CLASS=pin>Rotation: </SPAN>determines if you want the faces to rotate.<BR>
    <SPAN CLASS=pin>Deformation: </SPAN>determines if you want the faces to be deformed.<BR>
    */
    /* warning: Since this behavior modifies the mesh geometry you have to add initial conditions on you mesh if you want to store it.
       Moreover, this behavior can be slow on objects with many faces.
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x002d010a, 0x003d010a));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateExplodeProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    od->SetCategory("Mesh Modifications/Deformation");
    return od;
}

CKERROR CreateExplodeProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("Explode");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareInput("Loop In");

    proto->DeclareOutput("Out");
    proto->DeclareOutput("Loop Out");

    proto->DeclareInParameter("Duration (ms)", CKPGUID_INT, "1000");
    proto->DeclareInParameter("Explosion Curve", CKPGUID_2DCURVE);
    proto->DeclareInParameter("Max Distance Factor", CKPGUID_FLOAT, "2");
    proto->DeclareInParameter("Spreading Direction", CKPGUID_VECTOR);
    proto->DeclareInParameter("Rotation", CKPGUID_BOOL, "TRUE");
    proto->DeclareInParameter("Deformation", CKPGUID_BOOL, "FALSE");

    // "Time elapsed"
    proto->DeclareLocalParameter(NULL, CKPGUID_INT);
    // "Value"
    proto->DeclareLocalParameter(NULL, CKPGUID_FLOAT);
    // useless
    proto->DeclareLocalParameter("curveDeltaX", CKPGUID_FLOAT);

    proto->DeclareLocalParameter("baryCenter", CKPGUID_VECTOR);
    proto->DeclareLocalParameter("alpha", CKPGUID_FLOAT);
    proto->DeclareLocalParameter("mesh saved", CKPGUID_MESH);
    proto->DeclareLocalParameter("new mesh", CKPGUID_MESH);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(Explode);

    proto->SetBehaviorCallbackFct(ExplodeCallBackObject);

    *pproto = proto;
    return CK_OK;
}

int Explode(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // Local Variables
    int elapsedTime;
    float value, lastValue;
    float delta;

    // we get the 3d entity
    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
    if (!ent)
        return CKBR_OWNERERROR;

    // Get the total time
    int time;
    beh->GetInputParameterValue(0, &time);

    // we get the effect value
    float maxdistance;
    beh->GetInputParameterValue(2, &maxdistance);

    // Get the progression curve
    CK2dCurve *curve = NULL;
    beh->GetInputParameterValue(1, &curve);
    if (!curve)
        return CKBR_OK;

    if (beh->IsInputActive(0)) // Enter by On
    {
        // we create the new mesh
        CKMesh *mmesh = NULL;
        mmesh = (CKMesh *)beh->GetLocalParameterObject(6);
        if (mmesh)
            CKDestroyObject(mmesh);

        CKMesh *originalmesh = ent->GetCurrentMesh();
        // we save the old mesh in a local parameter
        beh->SetLocalParameterObject(5, originalmesh);

        mmesh = (CKMesh *)behcontext.Context->CopyObject(originalmesh, NULL, "Explode");
        mmesh->ModifyObjectFlags(CK_OBJECT_NOTTOBELISTEDANDSAVED, 0);
        // CKGetCurrentLevel()->AddObject(mmesh);
        mmesh->DissociateAllFaces();

        // we save the old mesh in a local parameter
        beh->SetLocalParameterObject(6, mmesh);
        ent->SetCurrentMesh(mmesh);

        // we calculate the initial barycenter
        VxVector center;
        mmesh->GetBaryCenter(&center);
        // writing the barycenter
        beh->SetLocalParameterValue(3, &center);

        // writing the elapsed time
        elapsedTime = 0;
        beh->SetLocalParameterValue(0, &elapsedTime);

        lastValue = maxdistance * curve->GetY(0.0f);
        beh->SetLocalParameterValue(1, &lastValue);

        beh->ActivateInput(0, FALSE);
    }
    else // Enter by "Loop In"
    {
        beh->ActivateInput(1, FALSE);
        beh->GetLocalParameterValue(0, &elapsedTime);
    }

    // we get the explode mesh
    CKMesh *imesh = ent->GetCurrentMesh();
    if (!imesh)
        return CKBR_GENERICERROR;

    // we get the direction
    VxVector direction;
    beh->GetInputParameterValue(3, &direction);

    // we get the direction
    CKBOOL rotate = FALSE;
    beh->GetInputParameterValue(4, &rotate);

    // we get to deform
    CKBOOL deform = FALSE;
    beh->GetInputParameterValue(5, &deform);

    elapsedTime += (int)behcontext.DeltaTime;
    beh->SetLocalParameterValue(0, &elapsedTime);

    value = maxdistance * (curve->GetY((float)elapsedTime / time));

    beh->GetLocalParameterValue(1, &lastValue);
    delta = value - lastValue;
    beh->SetLocalParameterValue(1, &value);

    // getting the center
    VxVector center;
    beh->GetLocalParameterValue(3, &center);

    // getting the vertices array and the faces array
    CKDWORD vStride;
    CKBYTE *varray = (CKBYTE *)imesh->GetPositionsPtr(&vStride);
    CKWORD *findices = imesh->GetFacesIndices();
    int facesNumber = imesh->GetFaceCount();

    VxVector ste;

    // we multiply by the radius of object
    float effectValue = maxdistance * imesh->GetRadius();

    // we move all the points
    float D;
    VxVector pos;
    float angle;
    VxMatrix mat;
    VxVector res;
    VxVector s;
    VxVector *va;
    VxVector *vb;
    VxVector *vc;

    for (int i = 0; i < facesNumber; i++, findices += 3)
    {
        // Vertices modifications
        va = (VxVector *)(varray + findices[0] * vStride);
        vb = (VxVector *)(varray + findices[1] * vStride);
        vc = (VxVector *)(varray + findices[2] * vStride);
        ste = *va - center;
        D = Magnitude(ste);
        if (!deform)
        {
            ste.Normalize();
            ste *= (delta * effectValue * ((float)(rand() % 100) * 0.01f));
            ste += (direction * D * 0.1f);
            *va += ste;
            *vb += ste;
            *vc += ste;
        }
        else
        {
            ste.Normalize();
            s = ste;
            ste *= (delta * effectValue * ((float)(rand() % 100) * 0.01f));
            ste += (direction * D * 0.1f);
            *va += ste;
            ste = s;
            ste *= (delta * effectValue * ((float)(rand() % 100) * 0.01f));
            ste += (direction * D * 0.1f);
            *vb += ste;
            ste = s;
            ste *= (delta * effectValue * ((float)(rand() % 100) * 0.01f));
            ste += (direction * D * 0.1f);
            *vc += ste;
        }
        if (rotate)
        {
            pos = *va + *vb + *vc;
            pos *= 0.33333333f;
            angle = delta * 10;
            Vx3DMatrixFromRotation(mat, pos, angle);
            Vx3DMultiplyMatrixVector(&res, mat, va);
            *va = res;
            Vx3DMultiplyMatrixVector(&res, mat, vb);
            *vb = res;
            Vx3DMultiplyMatrixVector(&res, mat, vc);
            *vc = res;
        }
    }

    imesh->VertexMove();

    if (elapsedTime < time)
    {
        beh->ActivateOutput(1, TRUE);
    }
    else
    {
        // loop accomplished

        // we restore the old mesh
        CKMesh *em = (CKMesh *)beh->GetLocalParameterObject(5);
        ent->SetCurrentMesh(em);

        // we destroy the explode mesh
        CKMesh *explodemesh = (CKMesh *)beh->GetLocalParameterObject(6);
        ent->RemoveMesh(explodemesh);
        CKDestroyObject(explodemesh);

        // we hide the object
        ent->Show(CKHIDE);

        beh->ActivateOutput(0, TRUE);
    }
    return CKBR_OK;
}

CKERROR ExplodeCallBackObject(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORCREATE:
    case CKM_BEHAVIORLOAD:
    {
        // we create the new mesh
        CKMesh *mmesh = NULL;
        // we save the old mesh in a local parameter
        beh->SetLocalParameterObject(6, mmesh);
    }
    break;
    }

    return CKBR_OK;
}

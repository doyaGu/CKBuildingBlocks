#include "BallanceHelpers.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace Ballance
{
    void DebugLog(CKContext *ctx, const char *format, ...)
    {
        if (!ctx || !format)
            return;

        char message[1024];
        va_list args;
        va_start(args, format);
        _vsnprintf(message, sizeof(message) - 1, format, args);
        va_end(args);
        message[sizeof(message) - 1] = '\0';

        ctx->OutputToConsoleEx("[Ballance] %s", message);
    }

    CKObject *FindObject(CKContext *ctx, const char *name, CK_CLASSID cid)
    {
        if (!ctx || !name || !*name)
            return NULL;
        return ctx->GetObjectByNameAndClass(const_cast<CKSTRING>(name), cid, NULL);
    }

    CKBehavior *FindScript(CKContext *ctx, const char *name)
    {
        return (CKBehavior *)FindObject(ctx, name, CKCID_BEHAVIOR);
    }

    CKGroup *FindGroup(CKContext *ctx, const char *name)
    {
        return (CKGroup *)FindObject(ctx, name, CKCID_GROUP);
    }

    CKDataArray *FindArray(CKContext *ctx, const char *name)
    {
        return (CKDataArray *)FindObject(ctx, name, CKCID_DATAARRAY);
    }

    bool GetStringCell(CKDataArray *array, int row, int col, std::string &value)
    {
        if (!array || row < 0 || col < 0 || row >= array->GetRowCount() || col >= array->GetColumnCount())
            return false;

        char buffer[512] = {0};
        if (!array->GetElementStringValue(row, col, buffer))
            return false;
        value = buffer;
        return true;
    }

    bool GetIntCell(CKDataArray *array, int row, int col, int &value)
    {
        if (!array || row < 0 || col < 0 || row >= array->GetRowCount() || col >= array->GetColumnCount())
            return false;
        return array->GetElementValue(row, col, &value) != FALSE;
    }

    bool GetBoolCell(CKDataArray *array, int row, int col, CKBOOL &value)
    {
        if (!array || row < 0 || col < 0 || row >= array->GetRowCount() || col >= array->GetColumnCount())
            return false;
        return array->GetElementValue(row, col, &value) != FALSE;
    }

    bool SetObjectCell(CKDataArray *array, int row, int col, CKObject *object)
    {
        if (!array || row < 0 || col < 0 || row >= array->GetRowCount() || col >= array->GetColumnCount())
            return false;
        CK_ID id = object ? object->GetID() : 0;
        return array->SetElementValue(row, col, &id, sizeof(CK_ID)) != FALSE;
    }

    bool SetIntCell(CKDataArray *array, int row, int col, int value)
    {
        if (!array || row < 0 || col < 0 || row >= array->GetRowCount() || col >= array->GetColumnCount())
            return false;
        return array->SetElementValue(row, col, &value, sizeof(int)) != FALSE;
    }

    std::string JoinPath(const std::string &left, const std::string &right)
    {
        if (left.empty())
            return right;
        if (right.empty())
            return left;
        const char last = left[left.size() - 1];
        if (last == '\\' || last == '/')
            return left + right;
        return left + "\\" + right;
    }

    std::string StripNmoExtension(const std::string &fileName)
    {
        std::string base = fileName;
        const std::string::size_type slash = base.find_last_of("\\/");
        if (slash != std::string::npos)
            base = base.substr(slash + 1);

        if (base.size() >= 4)
        {
            const std::string ext = base.substr(base.size() - 4);
            if (_stricmp(ext.c_str(), ".nmo") == 0)
                base.resize(base.size() - 4);
        }
        return base;
    }

    std::string MakeLoadedGroupName(const std::string &fileName)
    {
        return std::string("All_") + StripNmoExtension(fileName);
    }

    std::string MakeInitScriptName(const std::string &fileName)
    {
        return StripNmoExtension(fileName) + "_Init";
    }

    CKGroup *FindOrCreateLoadedGroup(CKContext *ctx, CKLevel *level, const char *name, CKObjectArray *loadedObjects)
    {
        CKGroup *group = FindGroup(ctx, name);
        if (group || !ctx || !name || !*name)
            return group;

        group = (CKGroup *)ctx->CreateObject(CKCID_GROUP, const_cast<CKSTRING>(name), CK_OBJECTCREATION_NONAMECHECK);
        if (!group)
            return NULL;
        if (level)
            level->AddObject(group);

        if (loadedObjects)
        {
            for (loadedObjects->Reset(); !loadedObjects->EndOfList(); loadedObjects->Next())
            {
                CKObject *object = loadedObjects->GetData(ctx);
                if (CKIsChildClassOf(object, CKCID_BEOBJECT))
                    group->AddObject((CKBeObject *)object);
            }
        }

        return group;
    }

    bool LoadFile(CKContext *ctx, CKLevel *level, CKScene *scene, const char *fileName,
                  CKBOOL addToScene, CKBOOL reuseMeshes, CKBOOL reuseMaterials,
                  CKObjectArray **loadedObjects)
    {
        if (loadedObjects)
            *loadedObjects = NULL;
        if (!ctx || !level || !fileName || !*fileName)
            return false;

        CKObjectArray *array = CreateCKObjectArray();
        if (!array)
            return false;

        CK_LOAD_FLAGS loadOptions = CK_LOAD_FLAGS(CK_LOAD_DEFAULT | CK_LOAD_AUTOMATICMODE | CK_LOAD_AS_DYNAMIC_OBJECT);
        ctx->SetAutomaticLoadMode(CKLOAD_OK, CKLOAD_OK,
                                  reuseMeshes ? CKLOAD_USECURRENT : CKLOAD_OK,
                                  reuseMaterials ? CKLOAD_USECURRENT : CKLOAD_OK);

        XString resolved(fileName);
        ctx->GetPathManager()->ResolveFileName(resolved, DATA_PATH_IDX, -1);
        if (ctx->Load(resolved.Str(), array, loadOptions) != CK_OK)
        {
            DeleteCKObjectArray(array);
            return false;
        }

        CKLevel *loadedLevel = NULL;
        for (array->Reset(); !array->EndOfList(); array->Next())
        {
            CKObject *object = array->GetData(ctx);
            if (CKIsChildClassOf(object, CKCID_LEVEL))
                loadedLevel = (CKLevel *)object;
        }

        if (loadedLevel)
        {
            level->Merge(loadedLevel, FALSE);
        }
        else
        {
            level->BeginAddSequence(TRUE);
            for (array->Reset(); !array->EndOfList(); array->Next())
            {
                CKObject *object = array->GetData(ctx);
                if (CKIsChildClassOf(object, CKCID_SCENE))
                    level->AddScene((CKScene *)object);
                else
                    level->AddObject(object);

                if (addToScene && scene && CKIsChildClassOf(object, CKCID_SCENEOBJECT) &&
                    !(CKIsChildClassOf(object, CKCID_LEVEL) || CKIsChildClassOf(object, CKCID_SCENE)))
                {
                    scene->AddObjectToScene((CKSceneObject *)object);
                }
            }
            level->BeginAddSequence(FALSE);
        }

        if (loadedObjects)
            *loadedObjects = array;
        else
            DeleteCKObjectArray(array);
        return true;
    }

    void ActivateScript(CKScene *scene, CKBehavior *script, CKBOOL reset)
    {
        if (!scene || !script)
            return;
        scene->Activate(script, reset);

        CKBeObject *owner = script->GetOwner();
        if (owner && !owner->IsActiveInCurrentScene())
            scene->Activate(owner, FALSE);
    }

    int AddRegisterRow(CKDataArray *registerArray, CKGroup *group, int loadId)
    {
        if (!registerArray || !group || registerArray->GetColumnCount() < 2)
            return 0;

        const int row = registerArray->GetRowCount();
        registerArray->InsertRow();
        SetObjectCell(registerArray, row, 0, group);
        SetIntCell(registerArray, row, 1, loadId);
        return 1;
    }

    int AddScriptRow(CKDataArray *scriptArray, CKBehavior *script)
    {
        if (!scriptArray || !script || scriptArray->GetColumnCount() < 1)
            return 0;

        for (int row = 0; row < scriptArray->GetRowCount(); ++row)
        {
            CKObject *existing = scriptArray->GetElementObject(row, 0);
            if (existing == script)
                return 0;
        }

        const int row = scriptArray->GetRowCount();
        scriptArray->InsertRow();
        SetObjectCell(scriptArray, row, 0, script);
        return 1;
    }

    int FillScriptsFromGroup(CKDataArray *scriptArray, CKGroup *group)
    {
        if (!scriptArray || !group)
            return 0;

        int added = 0;
        for (int i = 0; i < group->GetObjectCount(); ++i)
        {
            CKBeObject *object = group->GetObject(i);
            if (!object)
                continue;
            for (int scriptIndex = 0; scriptIndex < object->GetScriptCount(); ++scriptIndex)
                added += AddScriptRow(scriptArray, object->GetScript(scriptIndex));
        }
        return added;
    }

    int DeleteGroupObjects(CKContext *ctx, CKGroup *group)
    {
        if (!ctx || !group)
            return 0;

        std::vector<CK_ID> ids;
        ids.reserve(group->GetObjectCount());
        for (int i = 0; i < group->GetObjectCount(); ++i)
        {
            CKBeObject *object = group->GetObject(i);
            if (object)
                ids.push_back(object->GetID());
        }

        if (!ids.empty())
            ctx->DestroyObjects(&ids[0], (int)ids.size());

        return (int)ids.size();
    }

    void SendBroadcast(CKMessageManager *messages, int messageType, CKBeObject *sender)
    {
        if (messages && messageType >= 0 && sender)
            messages->SendMessageBroadcast(messageType, CKCID_BEOBJECT, sender);
    }
}

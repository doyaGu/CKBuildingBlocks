#pragma once

#include "CKAll.h"
#include <string>
#include <vector>

namespace Ballance
{
    void DebugLog(CKContext *ctx, const char *format, ...);

    CKObject *FindObject(CKContext *ctx, const char *name, CK_CLASSID cid);
    CKBehavior *FindScript(CKContext *ctx, const char *name);
    CKGroup *FindGroup(CKContext *ctx, const char *name);
    CKDataArray *FindArray(CKContext *ctx, const char *name);

    bool GetStringCell(CKDataArray *array, int row, int col, std::string &value);
    bool GetIntCell(CKDataArray *array, int row, int col, int &value);
    bool GetBoolCell(CKDataArray *array, int row, int col, CKBOOL &value);

    bool SetObjectCell(CKDataArray *array, int row, int col, CKObject *object);
    bool SetIntCell(CKDataArray *array, int row, int col, int value);

    std::string JoinPath(const std::string &left, const std::string &right);
    std::string StripNmoExtension(const std::string &fileName);
    std::string MakeLoadedGroupName(const std::string &fileName);
    std::string MakeInitScriptName(const std::string &fileName);

    CKGroup *FindOrCreateLoadedGroup(CKContext *ctx, CKLevel *level, const char *name, CKObjectArray *loadedObjects);
    bool LoadFile(CKContext *ctx, CKLevel *level, CKScene *scene, const char *fileName,
                  CKBOOL addToScene, CKBOOL reuseMeshes, CKBOOL reuseMaterials,
                  CKObjectArray **loadedObjects);

    void ActivateScript(CKScene *scene, CKBehavior *script, CKBOOL reset);
    int AddRegisterRow(CKDataArray *registerArray, CKGroup *group, int loadId);
    int FillScriptsFromGroup(CKDataArray *scriptArray, CKGroup *group);
    int AddScriptRow(CKDataArray *scriptArray, CKBehavior *script);

    int DeleteGroupObjects(CKContext *ctx, CKGroup *group);
    void SendBroadcast(CKMessageManager *messages, int messageType, CKBeObject *sender);
}

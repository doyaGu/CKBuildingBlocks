#ifndef TT_INTERFACEMANAGER_REGISTRYUTILS_H
#define TT_INTERFACEMANAGER_REGISTRYUTILS_H

#include "CKAll.h"
#include "GameInfo.h"

CKBOOL TTBuildRegistrySection(const CGameInfo *gameInfo, const char *regKey, char *buffer, size_t bufferSize);
CKBOOL TTReadRegistryString(const CGameInfo *gameInfo, const char *regKey, const char *valueName, char *buffer, size_t bufferSize);
CKBOOL TTWriteRegistryString(const CGameInfo *gameInfo, const char *regKey, const char *valueName, const char *value);
CKBOOL TTReadRegistryInteger(const CGameInfo *gameInfo, const char *regKey, const char *valueName, int *value);
CKBOOL TTWriteRegistryInteger(const CGameInfo *gameInfo, const char *regKey, const char *valueName, int value);
CKBOOL TTReadRegistryFloat(const CGameInfo *gameInfo, const char *regKey, const char *valueName, float *value);
CKBOOL TTWriteRegistryFloat(const CGameInfo *gameInfo, const char *regKey, const char *valueName, float value);
CKBOOL TTReadRegistryBoolean(const CGameInfo *gameInfo, const char *regKey, const char *valueName, XBOOL *value);
CKBOOL TTWriteRegistryBoolean(const CGameInfo *gameInfo, const char *regKey, const char *valueName, XBOOL value);

CKBOOL TTReadRegistryString(VxConfigRoot root, const char *subKey, const char *valueName, char *buffer, size_t bufferSize);
CKBOOL TTWriteRegistryString(VxConfigRoot root, const char *subKey, const char *valueName, const char *value);
CKBOOL TTReadRegistryInteger(VxConfigRoot root, const char *subKey, const char *valueName, int *value);
CKBOOL TTWriteRegistryInteger(VxConfigRoot root, const char *subKey, const char *valueName, int value);
CKBOOL TTReadRegistryFloat(VxConfigRoot root, const char *subKey, const char *valueName, float *value);
CKBOOL TTWriteRegistryFloat(VxConfigRoot root, const char *subKey, const char *valueName, float value);

#endif

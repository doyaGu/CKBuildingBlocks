#ifndef TT_INTERFACEMANAGER_REGISTRYUTILS_H
#define TT_INTERFACEMANAGER_REGISTRYUTILS_H

#include "CKAll.h"
#include "GameInfo.h"

const char *TTRegistryFile(CKContext *context);

CKBOOL TTBuildRegistrySection(const CGameInfo *gameInfo, const char *regKey, char *buffer, size_t bufferSize);
CKBOOL TTReadRegistryString(const CGameInfo *gameInfo, const char *regKey, const char *valueName, char *buffer, size_t bufferSize, const char *registryFile = NULL);
CKBOOL TTWriteRegistryString(const CGameInfo *gameInfo, const char *regKey, const char *valueName, const char *value, const char *registryFile = NULL);
CKBOOL TTReadRegistryInteger(const CGameInfo *gameInfo, const char *regKey, const char *valueName, int *value, const char *registryFile = NULL);
CKBOOL TTWriteRegistryInteger(const CGameInfo *gameInfo, const char *regKey, const char *valueName, int value, const char *registryFile = NULL);
CKBOOL TTReadRegistryFloat(const CGameInfo *gameInfo, const char *regKey, const char *valueName, float *value, const char *registryFile = NULL);
CKBOOL TTWriteRegistryFloat(const CGameInfo *gameInfo, const char *regKey, const char *valueName, float value, const char *registryFile = NULL);
CKBOOL TTReadRegistryBoolean(const CGameInfo *gameInfo, const char *regKey, const char *valueName, XBOOL *value, const char *registryFile = NULL);
CKBOOL TTWriteRegistryBoolean(const CGameInfo *gameInfo, const char *regKey, const char *valueName, XBOOL value, const char *registryFile = NULL);

CKBOOL TTReadRegistryString(VxConfigRoot root, const char *subKey, const char *valueName, char *buffer, size_t bufferSize, const char *registryFile = NULL);
CKBOOL TTWriteRegistryString(VxConfigRoot root, const char *subKey, const char *valueName, const char *value, const char *registryFile = NULL);
CKBOOL TTReadRegistryInteger(VxConfigRoot root, const char *subKey, const char *valueName, int *value, const char *registryFile = NULL);
CKBOOL TTWriteRegistryInteger(VxConfigRoot root, const char *subKey, const char *valueName, int value, const char *registryFile = NULL);
CKBOOL TTReadRegistryFloat(VxConfigRoot root, const char *subKey, const char *valueName, float *value, const char *registryFile = NULL);
CKBOOL TTWriteRegistryFloat(VxConfigRoot root, const char *subKey, const char *valueName, float value, const char *registryFile = NULL);

// RegQueryValueEx into a 4-byte buffer: float entries yield their IEEE bits (what a
// REG_DWORD written from a float holds), other entries their integer value.
CKBOOL TTQueryRegistryDword(VxConfigRoot root, const char *subKey, const char *valueName, CKDWORD *value, const char *registryFile = NULL);
// RegQueryValueEx into a fixed buffer: a value that does not fit fails instead of truncating.
CKBOOL TTQueryRegistryString(VxConfigRoot root, const char *subKey, const char *valueName, char *buffer, size_t bufferSize, const char *registryFile = NULL);

#endif

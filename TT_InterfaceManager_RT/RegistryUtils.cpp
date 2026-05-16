#include "RegistryUtils.h"

#include "VxConfiguration.h"

#include <string.h>

static const char *TTRegistryBaseSection() {
    return "";
}

CKBOOL TTBuildRegistrySection(const CGameInfo *gameInfo, const char *regKey, char *buffer, size_t bufferSize) {
    if (!gameInfo || !buffer || bufferSize == 0)
        return FALSE;
    const char *suffix = regKey ? regKey : "";
    if (strlen(gameInfo->regSubkey) + strlen(suffix) >= bufferSize)
        return FALSE;
    strcpy(buffer, gameInfo->regSubkey);
    strcat(buffer, suffix);
    return TRUE;
}

static CKBOOL TTOpenConfig(VxConfigRoot root, const char *section, VxConfig::Mode mode, VxConfig &config) {
    if (!section)
        return FALSE;
    config.OpenRoot(root, TTRegistryBaseSection());
    config.OpenSection(section, mode);
    return TRUE;
}

CKBOOL TTReadRegistryString(VxConfigRoot root, const char *subKey, const char *valueName, char *buffer, size_t bufferSize) {
    VxConfig config(root, TTRegistryBaseSection());
    if (!TTOpenConfig(root, subKey, VxConfig::READ, config))
        return FALSE;
    return config.ReadStringEntry(valueName, buffer, bufferSize);
}

CKBOOL TTWriteRegistryString(VxConfigRoot root, const char *subKey, const char *valueName, const char *value) {
    VxConfig config(root, TTRegistryBaseSection());
    if (!TTOpenConfig(root, subKey, VxConfig::WRITE, config))
        return FALSE;
    config.WriteStringEntry(valueName, value ? value : "");
    return TRUE;
}

CKBOOL TTReadRegistryInteger(VxConfigRoot root, const char *subKey, const char *valueName, int *value) {
    VxConfig config(root, TTRegistryBaseSection());
    if (!TTOpenConfig(root, subKey, VxConfig::READ, config))
        return FALSE;
    return config.ReadIntegerEntry(valueName, value);
}

CKBOOL TTWriteRegistryInteger(VxConfigRoot root, const char *subKey, const char *valueName, int value) {
    VxConfig config(root, TTRegistryBaseSection());
    if (!TTOpenConfig(root, subKey, VxConfig::WRITE, config))
        return FALSE;
    config.WriteIntegerEntry(valueName, value);
    return TRUE;
}

CKBOOL TTReadRegistryFloat(VxConfigRoot root, const char *subKey, const char *valueName, float *value) {
    VxConfig config(root, TTRegistryBaseSection());
    if (!TTOpenConfig(root, subKey, VxConfig::READ, config))
        return FALSE;
    return config.ReadFloatEntry(valueName, value);
}

CKBOOL TTWriteRegistryFloat(VxConfigRoot root, const char *subKey, const char *valueName, float value) {
    VxConfig config(root, TTRegistryBaseSection());
    if (!TTOpenConfig(root, subKey, VxConfig::WRITE, config))
        return FALSE;
    config.WriteFloatEntry(valueName, value);
    return TRUE;
}

CKBOOL TTReadRegistryString(const CGameInfo *gameInfo, const char *regKey, const char *valueName, char *buffer, size_t bufferSize) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    return TTReadRegistryString(gameInfo->registryRoot, section, valueName, buffer, bufferSize);
}

CKBOOL TTWriteRegistryString(const CGameInfo *gameInfo, const char *regKey, const char *valueName, const char *value) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    return TTWriteRegistryString(gameInfo->registryRoot, section, valueName, value);
}

CKBOOL TTReadRegistryInteger(const CGameInfo *gameInfo, const char *regKey, const char *valueName, int *value) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    return TTReadRegistryInteger(gameInfo->registryRoot, section, valueName, value);
}

CKBOOL TTWriteRegistryInteger(const CGameInfo *gameInfo, const char *regKey, const char *valueName, int value) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    return TTWriteRegistryInteger(gameInfo->registryRoot, section, valueName, value);
}

CKBOOL TTReadRegistryFloat(const CGameInfo *gameInfo, const char *regKey, const char *valueName, float *value) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    return TTReadRegistryFloat(gameInfo->registryRoot, section, valueName, value);
}

CKBOOL TTWriteRegistryFloat(const CGameInfo *gameInfo, const char *regKey, const char *valueName, float value) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    return TTWriteRegistryFloat(gameInfo->registryRoot, section, valueName, value);
}

CKBOOL TTReadRegistryBoolean(const CGameInfo *gameInfo, const char *regKey, const char *valueName, XBOOL *value) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    VxConfig config(gameInfo->registryRoot, TTRegistryBaseSection());
    config.OpenSection(section, VxConfig::READ);
    return config.ReadBooleanEntry(valueName, value);
}

CKBOOL TTWriteRegistryBoolean(const CGameInfo *gameInfo, const char *regKey, const char *valueName, XBOOL value) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    VxConfig config(gameInfo->registryRoot, TTRegistryBaseSection());
    config.OpenSection(section, VxConfig::WRITE);
    config.WriteBooleanEntry(valueName, value);
    return TRUE;
}

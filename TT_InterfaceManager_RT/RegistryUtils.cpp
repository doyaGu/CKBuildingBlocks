#include "RegistryUtils.h"
#include "InterfaceManager.h"

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

CKBOOL TTReadRegistryString(VxConfigRoot root, const char *subKey, const char *valueName, char *buffer, size_t bufferSize, const char *registryFile) {
    VxConfig config(root, TTRegistryBaseSection(), registryFile);
    if (!TTOpenConfig(root, subKey, VxConfig::READ, config))
        return FALSE;
    return config.ReadStringEntry(valueName, buffer, bufferSize);
}

CKBOOL TTWriteRegistryString(VxConfigRoot root, const char *subKey, const char *valueName, const char *value, const char *registryFile) {
    VxConfig config(root, TTRegistryBaseSection(), registryFile);
    if (!TTOpenConfig(root, subKey, VxConfig::WRITE, config))
        return FALSE;
    config.WriteStringEntry(valueName, value ? value : "");
    return TRUE;
}

CKBOOL TTReadRegistryInteger(VxConfigRoot root, const char *subKey, const char *valueName, int *value, const char *registryFile) {
    VxConfig config(root, TTRegistryBaseSection(), registryFile);
    if (!TTOpenConfig(root, subKey, VxConfig::READ, config))
        return FALSE;
    return config.ReadIntegerEntry(valueName, value);
}

CKBOOL TTWriteRegistryInteger(VxConfigRoot root, const char *subKey, const char *valueName, int value, const char *registryFile) {
    VxConfig config(root, TTRegistryBaseSection(), registryFile);
    if (!TTOpenConfig(root, subKey, VxConfig::WRITE, config))
        return FALSE;
    config.WriteIntegerEntry(valueName, value);
    return TRUE;
}

CKBOOL TTReadRegistryFloat(VxConfigRoot root, const char *subKey, const char *valueName, float *value, const char *registryFile) {
    VxConfig config(root, TTRegistryBaseSection(), registryFile);
    if (!TTOpenConfig(root, subKey, VxConfig::READ, config))
        return FALSE;
    return config.ReadFloatEntry(valueName, value);
}

CKBOOL TTWriteRegistryFloat(VxConfigRoot root, const char *subKey, const char *valueName, float value, const char *registryFile) {
    VxConfig config(root, TTRegistryBaseSection(), registryFile);
    if (!TTOpenConfig(root, subKey, VxConfig::WRITE, config))
        return FALSE;
    config.WriteFloatEntry(valueName, value);
    return TRUE;
}

CKBOOL TTQueryRegistryDword(VxConfigRoot root, const char *subKey, const char *valueName, CKDWORD *value, const char *registryFile) {
    if (!value)
        return FALSE;
    VxConfig config(root, TTRegistryBaseSection(), registryFile);
    if (!TTOpenConfig(root, subKey, VxConfig::READ, config))
        return FALSE;
    VxConfigValueType type = VXCONFIG_VALUE_STRING;
    if (!config.GetEntryType(valueName, &type))
        return FALSE;
    if (type == VXCONFIG_VALUE_FLOAT) {
        float floatValue = 0.0f;
        if (!config.ReadFloatEntry(valueName, &floatValue))
            return FALSE;
        memcpy(value, &floatValue, sizeof(*value));
        return TRUE;
    }
    int intValue = 0;
    if (!config.ReadIntegerEntry(valueName, &intValue))
        return FALSE;
    *value = (CKDWORD)intValue;
    return TRUE;
}

CKBOOL TTQueryRegistryString(VxConfigRoot root, const char *subKey, const char *valueName, char *buffer, size_t bufferSize, const char *registryFile) {
    if (!buffer || bufferSize == 0)
        return FALSE;
    // One spare byte tells a value that fits from one that would be truncated.
    XArray<char> value;
    value.Resize((int)bufferSize + 1);
    if (!TTReadRegistryString(root, subKey, valueName, value.Begin(), bufferSize + 1))
        return FALSE;
    const size_t length = strlen(value.Begin());
    if (length >= bufferSize)
        return FALSE;
    memcpy(buffer, value.Begin(), length + 1);
    return TRUE;
}

CKBOOL TTReadRegistryString(const CGameInfo *gameInfo, const char *regKey, const char *valueName, char *buffer, size_t bufferSize, const char *registryFile) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    return TTReadRegistryString(gameInfo->registryRoot, section, valueName, buffer, bufferSize, registryFile);
}

CKBOOL TTWriteRegistryString(const CGameInfo *gameInfo, const char *regKey, const char *valueName, const char *value, const char *registryFile) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    return TTWriteRegistryString(gameInfo->registryRoot, section, valueName, value, registryFile);
}

CKBOOL TTReadRegistryInteger(const CGameInfo *gameInfo, const char *regKey, const char *valueName, int *value, const char *registryFile) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    return TTReadRegistryInteger(gameInfo->registryRoot, section, valueName, value, registryFile);
}

CKBOOL TTWriteRegistryInteger(const CGameInfo *gameInfo, const char *regKey, const char *valueName, int value, const char *registryFile) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    return TTWriteRegistryInteger(gameInfo->registryRoot, section, valueName, value, registryFile);
}

CKBOOL TTReadRegistryFloat(const CGameInfo *gameInfo, const char *regKey, const char *valueName, float *value, const char *registryFile) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    return TTReadRegistryFloat(gameInfo->registryRoot, section, valueName, value, registryFile);
}

CKBOOL TTWriteRegistryFloat(const CGameInfo *gameInfo, const char *regKey, const char *valueName, float value, const char *registryFile) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    return TTWriteRegistryFloat(gameInfo->registryRoot, section, valueName, value, registryFile);
}

CKBOOL TTReadRegistryBoolean(const CGameInfo *gameInfo, const char *regKey, const char *valueName, XBOOL *value, const char *registryFile) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    VxConfig config(gameInfo->registryRoot, TTRegistryBaseSection(), registryFile);
    config.OpenSection(section, VxConfig::READ);
    return config.ReadBooleanEntry(valueName, value);
}

CKBOOL TTWriteRegistryBoolean(const CGameInfo *gameInfo, const char *regKey, const char *valueName, XBOOL value, const char *registryFile) {
    char section[512];
    if (!TTBuildRegistrySection(gameInfo, regKey, section, sizeof(section)))
        return FALSE;
    VxConfig config(gameInfo->registryRoot, TTRegistryBaseSection(), registryFile);
    config.OpenSection(section, VxConfig::WRITE);
    config.WriteBooleanEntry(valueName, value);
    return TRUE;
}

const char *TTRegistryFile(CKContext *context) {
    InterfaceManager *manager = context ? InterfaceManager::GetManager(context) : NULL;
    return manager ? manager->GetRegistryFile() : NULL;
}

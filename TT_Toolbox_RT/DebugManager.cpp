#include "DebugManager.h"

DebugManager::DebugManager(CKContext *context) : CKBaseManager(context, TT_DEBUG_MANAGER_GUID, "TT_DebugManager")
{
    m_LogFile = NULL;
    m_DebugMode = false;
    m_LogFileNotOpened = true;
    context->RegisterNewManager(this);
}

DebugManager::~DebugManager() {}

void DebugManager::WriteToLogFile(const char *filename, const char *format, ...) {
    if (!filename || !format)
        return;

    m_LogFile = fopen(filename, m_LogFileNotOpened ? "w" : "a");
    if (!m_LogFile)
        return;

    m_LogFileNotOpened = false;

    va_list ap;
    va_start(ap, format);
    vfprintf(m_LogFile, format, ap);
    va_end(ap);

    fclose(m_LogFile);
    m_LogFile = NULL;
}

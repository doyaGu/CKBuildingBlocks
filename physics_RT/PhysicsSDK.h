#ifndef PHYSICS_RT_PHYSICSSDK_H
#define PHYSICS_RT_PHYSICSSDK_H

// VxMath's legacy compiler header defaults DLL_EXPORT to dllexport. Select
// the host's DLL import or static-link definitions before declaring CK2 SDK classes here.
#include "VxDefines.h"
#undef DLL_EXPORT
#if defined(_WIN32) && !defined(CK_LIB)
#define DLL_EXPORT __declspec(dllimport)
#else
#define DLL_EXPORT
#endif
#include "CKAll.h"

#endif

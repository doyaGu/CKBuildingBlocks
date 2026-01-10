# Interface
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Interface
        SOURCES
        CKBitmapFont.cpp
        CKBitmapFont.h
        CKFontManager.cpp
        CKFontManager.h
        CKTextureFont.cpp
        CKTextureFont.h
        CompiledData.cpp
        CompiledData.h
        DrawingPrimitives.cpp
        DrawingPrimitives.h
        Layout.h
        Interface.cpp
        Interface.rc
        ${BEHAVIORS_SRCS}
)
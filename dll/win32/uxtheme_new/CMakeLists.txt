set(CMAKE_CXX_STANDARD 17)

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# The in-tree spec still lists GetThemeStream as a stub; this engine implements it
set(UXTHEME_TREE_SPEC ${REACTOS_SOURCE_DIR}/dll/win32/uxtheme/uxtheme_vista.spec)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${UXTHEME_TREE_SPEC})
file(READ ${UXTHEME_TREE_SPEC} UXTHEME_SPEC)
string(REPLACE "102 stub GetThemeStream" "102 stdcall GetThemeStream(ptr long long long ptr ptr ptr)" UXTHEME_SPEC "${UXTHEME_SPEC}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/uxtheme_vista.spec "${UXTHEME_SPEC}")
file(RELATIVE_PATH UXTHEME_SPEC_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/uxtheme_vista.spec)
spec2def(uxtheme.dll ${UXTHEME_SPEC_PATH} ADD_IMPORTLIB)

list(APPEND SOURCE
    src/main.cpp
    src/UxThemeP.cpp
    src/model/ThemeFile.cpp
    src/load/PeModule.cpp
    src/load/Primitives.cpp
    src/load/VSUnpack.cpp
    src/load/ThemeLoader.cpp
    src/image/Decode.cpp
    src/image/Atlas.cpp
    src/render/DrawBackground.cpp
    src/render/DrawText.cpp
    src/nc/NcSupport.cpp
    src/nc/ThemeHooks.c
    src/nc/NonClient.c
    src/nc/NcScrollBar.c
    src/api/OpenThemeData.cpp
    src/api/Property.cpp
    src/api/WindowTheme.cpp
    src/api/CurrentVisualStyle.cpp
    src/api/Stream.cpp
    src/api/Stubs.cpp)

file(GLOB uxtheme_rc_deps ${CMAKE_CURRENT_SOURCE_DIR}/lang/*.rc)
add_rc_deps(uxtheme.rc ${uxtheme_rc_deps})

add_library(uxtheme MODULE
    ${SOURCE}
    uxtheme.rc
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme.def
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme_stubs.c)

if(MSVC)
    set_property(TARGET uxtheme APPEND_STRING PROPERTY LINK_FLAGS " /MAP")
endif()

set_module_type(uxtheme win32dll)
add_importlibs(uxtheme user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(uxtheme psdk)
add_cd_file(TARGET uxtheme DESTINATION reactos/system32 FOR all)

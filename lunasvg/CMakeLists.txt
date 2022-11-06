cmake_minimum_required(VERSION 3.3)

# cmake MSVC_RUNTIME_LIBRARY chicken bones
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

project(lunasvg VERSION 2.3.5 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

option(BUILD_SHARED_LIBS "Builds as shared library" OFF)
option(LUNASVG_BUILD_EXAMPLES "Builds examples" OFF)

add_library(lunasvg)

add_subdirectory(include)
add_subdirectory(source)
add_subdirectory(3rdparty/plutovg)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(lunasvg PUBLIC LUNASVG_SHARED)
    target_compile_definitions(lunasvg PRIVATE LUNASVG_EXPORT)
endif()

if(LUNASVG_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

set(LUNASVG_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
set(LUNASVG_INCDIR ${CMAKE_INSTALL_PREFIX}/include)

set_property(TARGET lunasvg PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

install(FILES
    include/lunasvg.h
    DESTINATION ${LUNASVG_INCDIR}
)

install(TARGETS lunasvg
    LIBRARY     DESTINATION    ${LUNASVG_LIBDIR}
    ARCHIVE     DESTINATION    ${LUNASVG_LIBDIR}
    INCLUDES    DESTINATION    ${LUNASVG_INCDIR}
)

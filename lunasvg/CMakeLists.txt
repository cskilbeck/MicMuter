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

if (MSVC AND NOT BUILD_SHARED_LIBS)
    # Set the output name for debugger files, otherwise they are not generated:
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
        COMPILE_PDB_NAME_DEBUG ${PROJECT_NAME}
        COMPILE_PDB_NAME_RELEASE ${PROJECT_NAME}
        COMPILE_PDB_NAME_MINSIZEREL ${PROJECT_NAME}
        COMPILE_PDB_NAME_RELWITHDEBINFO ${PROJECT_NAME})
endif()

if(BUILD_SHARED_LIBS)
    target_compile_definitions(lunasvg PUBLIC LUNASVG_SHARED)
    target_compile_definitions(lunasvg PRIVATE LUNASVG_EXPORT)
endif()

if(LUNASVG_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

set(COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG, ${CMAKE_INSTALL_PREFIX}/lib)
set(COMPILE_PDB_OUTPUT_DIRECTORY_RELEASE, ${CMAKE_INSTALL_PREFIX}/lib)
set(COMPILE_PDB_OUTPUT_DIRECTORY_MINSIZEREL, ${CMAKE_INSTALL_PREFIX}/lib)
set(COMPILE_PDB_OUTPUT_DIRECTORY_RELWITHDEBINFO , ${CMAKE_INSTALL_PREFIX}/lib)

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

# libarea had no build system of its own: it was compiled as part of FreeCAD's
# Area module, which is where these sources come from. This is that build,
# lifted out, with the FreeCAD-only pieces left behind.
#
# Clipper is built in rather than depended on. It is not on conda-forge under
# any name, it is a single translation unit, and libarea is the only thing here
# that drives it -- so a package of its own would buy nothing and would put a
# second copy of ClipperLib in the process, which the loader would interpose on
# libarea's. Its header is installed alongside libarea's own, so a consumer
# that wants Clipper directly has it.
#
# What FreeCAD keeps compiling itself, and why it is not here:
#   Adaptive.cpp  -- FreeCAD's adaptive-clearing toolpath code, LGPL and
#                    unrelated to CArea. It uses Clipper directly, not
#                    libarea.
#   AreaDxf.cpp   -- needs FreeCAD's Import module for dxf.h.
#   PythonStuff.cpp / pyarea.cpp -- the Python wrapper, which binds both of
#                    those as well as CArea.

cmake_minimum_required(VERSION 3.16)
project(libarea VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build a shared library instead of a static one" ON)

add_library(area
    Arc.cpp
    Area.cpp
    AreaOrderer.cpp
    AreaPocket.cpp
    Circle.cpp
    Curve.cpp
    kurve/Construction.cpp
    kurve/Finite.cpp
    kurve/kurve.cpp
    kurve/Matrix.cpp
    kurve/offset.cpp
    AreaClipper.cpp
    clipper.cpp
)
add_library(libarea::area ALIAS area)

set_target_properties(area PROPERTIES
    OUTPUT_NAME area
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    # Nothing in these headers is annotated for export, so a Windows DLL would
    # otherwise be empty.
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# The headers include each other by plain relative name ("Curve.h",
# "clipper.hpp", "kurve/geometry.h"), so they have to keep sitting in one
# directory together.
# Consumers say #include <libarea/Area.h> and the relative includes resolve
# from there.
target_include_directories(area PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS area
    EXPORT libareaTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES
    Arc.h
    Area.h
    AreaOrderer.h
    Box2D.h
    Circle.h
    Curve.h
    Point.h
    clipper.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libarea)
install(FILES kurve/geometry.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libarea/kurve)

install(EXPORT libareaTargets
    FILE libareaTargets.cmake
    NAMESPACE libarea::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libarea
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libareaConfig.cmake.in"
"@PACKAGE_INIT@\ninclude(\"\${CMAKE_CURRENT_LIST_DIR}/libareaTargets.cmake\")\n")
configure_package_config_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libareaConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libareaConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libarea
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libareaConfigVersion.cmake"
    COMPATIBILITY SameMinorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libareaConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/libareaConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libarea
)

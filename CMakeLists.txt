# libarea had no build system of its own: it was compiled as part of FreeCAD's
# Area module, which is where these sources come from. This is that build,
# lifted out, with the FreeCAD-only pieces left behind.
#
# Clipper is built in rather than depended on. Neither version is on
# conda-forge under any name, each is a handful of translation units, and
# libarea is the only thing here that drives them -- so a package of its own
# would buy nothing and would put a second copy of Clipper in the process,
# which the loader would interpose on libarea's. Their headers are installed
# alongside libarea's own, so a consumer that wants Clipper directly has it.
#
# Both versions are built, as upstream FreeCAD does:
#   clipper2/  -- Clipper2 2.0.1, from AngusJohnson/Clipper2 CPP/. CArea runs
#                 on this. It is the one to reach for in new code.
#   clipper.*  -- Clipper1 6.x. Kept because FreeCAD's Adaptive.cpp still
#                 drives it directly and upstream has not migrated theirs
#                 either. Nothing in CArea uses it any more.
#
# What FreeCAD keeps compiling itself, and why it is not here:
#   Adaptive.cpp  -- FreeCAD's adaptive-clearing toolpath code, LGPL and
#                    unrelated to CArea. It uses Clipper directly, not
#                    libarea.
#   AreaDxf.cpp   -- needs FreeCAD's Import module for dxf.h.
#   PythonStuff.cpp / pyarea.cpp -- the Python wrapper, which binds both of
#                    those as well as CArea.

cmake_minimum_required(VERSION 3.16)
project(libarea VERSION 0.3.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

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
    clipper2/clipper.engine.cpp
    clipper2/clipper.offset.cpp
    clipper2/clipper.rectclip.cpp
    clipper2/clipper.triangulation.cpp
)
add_library(libarea::area ALIAS area)

# C++17 is a requirement of the installed headers, not just of building this
# library, so it is a PUBLIC compile feature rather than CMAKE_CXX_STANDARD:
# Clipper2's Group carries a std::optional member, and PolyPath64::AddChild
# reads the reference that vector::emplace_back only returns from C++17 on.
# Set as a variable it would govern the targets built here and stop at the
# install, leaving libarea::area advertising no requirement at all -- which
# every consumer whose compiler defaults to C++17 would get away with, and
# MSVC, defaulting to C++14, would not. As a feature it travels with the
# exported target.
target_compile_features(area PUBLIC cxx_std_17)

# Clipper2 is built with USINGZ, because FreeCAD's Adaptive.cpp carries an
# index on the Z member through the clipping and reads it back off the points
# the clip returns. That changes the layout of Point64, so the definition is
# PUBLIC: a consumer compiled without it would disagree with the library about
# the ABI. Clipper1 has the matching switch, use_xyz, in clipper.hpp itself,
# where consumers pick it up by reading the header.
#
# The cost is eight bytes on every point, paid by CArea too, which has no use
# for Z. Upstream FreeCAD avoids that by building a second Clipper2 with
# USINGZ under the same namespace and linking only that into its adaptive
# code; here CArea is on Clipper2 as well, so both copies would end up in one
# process with one set of symbols between them.
target_compile_definitions(area PUBLIC USINGZ)

set_target_properties(area PROPERTIES
    OUTPUT_NAME area
    VERSION ${PROJECT_VERSION}
    # Bumped to 1 when CArea moved to Clipper2: the Clipper types in Area.h's
    # signatures changed, so this is not the same ABI. Bumped to 2 when
    # use_xyz and USINGZ went on, which changes those same types again.
    SOVERSION 2
    # Nothing in these headers is annotated for export, so a Windows DLL would
    # otherwise be empty.
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# The headers include each other by plain relative name ("Curve.h",
# "clipper.hpp", "kurve/geometry.h", "clipper2/clipper.core.h"), so they have
# to keep sitting in one directory together.
# Consumers say #include <libarea/Area.h> and the relative includes resolve
# from there -- except Clipper2's own headers, which include each other as
# "clipper2/...", so the directory holding clipper2/ has to be on the search
# path too. That is what the second entry of each pair is for.
target_include_directories(area PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libarea>
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
install(FILES
    clipper2/clipper.h
    clipper2/clipper.core.h
    clipper2/clipper.engine.h
    clipper2/clipper.offset.h
    clipper2/clipper.rectclip.h
    clipper2/clipper.minkowski.h
    clipper2/clipper.triangulation.h
    clipper2/clipper.export.h
    clipper2/clipper.version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libarea/clipper2)

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

# Tests. Off by default so a packaging build does not pay for them; the
# feedstock's own consumer check is separate and lighter.
include(CTest)
if(BUILD_TESTING)
    add_executable(area_ops_test tests/area_ops.cpp)
    target_link_libraries(area_ops_test PRIVATE area)
    add_test(NAME area_ops COMMAND area_ops_test)
endif()

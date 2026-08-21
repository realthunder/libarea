# libarea

2D closed-curve arithmetic -- boolean operations, offsetting and pocketing --
built on Clipper, and keeping arcs across all of it.

Clipper works on an integer lattice, so every boundary handed to it comes back
as straight segments.  libarea is what makes that survivable.  Going in, an arc
is carried as a centre and an end point and is only walked into segments at the
last moment; coming out, `FitArcs` recognises runs of segments that lie on a
circle and puts the arcs back.  It also rebuilds the nesting of the result and
re-winds contours by depth, so it tolerates input whose winding says nothing
about what is a hole.

Keeping arcs matters beyond fidelity.  A profile that comes back faceted
carries its face count into every operation downstream of it, which can cost
more than the boolean the 2D path replaced.

## Where this came from

Dan Heeks' original repository (https://github.com/Heeks/libarea) has no
releases and no tarball.  The copy that has actually been maintained is the one
FreeCAD vendored in `src/Mod/Area/libarea`, and this repository is that copy
lifted out so that FreeCAD and IfcOpenShell can link one library instead of
each carrying a fork.  It is packaged for conda by `libarea-feedstock`, which
builds from this checkout.

Clipper is built in rather than depended on.  It is not packaged on conda-forge
under any name, it is a single translation unit, and libarea is the only thing
here that drives it -- and two copies of `ClipperLib` in one process would be
interposed on each other by the loader.  `clipper.hpp` is installed alongside
libarea's own headers, so a consumer that wants `ClipperLib` directly gets it
from the same library.

Three files that live in FreeCAD's `libarea` directory are deliberately not
here: `Adaptive.cpp` (FreeCAD's adaptive-clearing toolpaths -- LGPL, unrelated
to `CArea`, and it drives Clipper directly rather than libarea),
`AreaDxf.cpp` (needs FreeCAD's Import module for `dxf.h`), and the Python
wrapper `PythonStuff.cpp` / `pyarea.cpp`.  FreeCAD keeps compiling those
itself, against this library.

## Building

    cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target install

There are no dependencies beyond a C++17 compiler and the standard library.

## Using it

    find_package(libarea CONFIG REQUIRED)
    target_link_libraries(mytarget PRIVATE libarea::area)

    #include <libarea/Area.h>
    #include <libarea/clipper.hpp>   // only if ClipperLib is wanted directly

## Licensing

libarea itself is BSD 3-Clause (`libarea-License.txt`), `kurve/` is BSD
3-Clause under its own copyright (`kurve/License.txt`), and Clipper is Boost
Software License 1.0 (`clipper-License.txt`).

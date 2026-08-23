// Golden values for CArea's clipping operations.
//
// These were taken from the Clipper1 build and re-checked against Clipper2
// when CArea moved over: every case here but the last agreed to six decimals
// across the change. The last one, a discretised arc unioned with a
// rectangle, is the one place the engines differ -- Clipper1 gave 39.291272,
// Clipper2 gives 39.315679, and the exact answer is 39.336954, so the value
// below is the closer of the two. If a Clipper update moves any of these,
// that is the change worth looking at rather than a number to re-bless.

#include "Area.h"

#include <cmath>
#include <cstdio>

static int failures = 0;

static void check(const char* what, double got, double want, size_t curves, size_t want_curves)
{
    const bool ok = std::fabs(got - want) <= 1e-5 && curves == want_curves;
    std::printf(
        "%-38s %13.6f (want %13.6f)  curves %zu (want %zu)  %s\n",
        what, got, want, curves, want_curves, ok ? "ok" : "FAIL");
    if (!ok) {
        ++failures;
    }
}

static CCurve rect(double x0, double y0, double x1, double y1)
{
    CCurve c;
    c.append(CVertex(Point(x0, y0)));
    c.append(CVertex(Point(x1, y0)));
    c.append(CVertex(Point(x1, y1)));
    c.append(CVertex(Point(x0, y1)));
    c.append(CVertex(Point(x0, y0)));
    return c;
}

static CArea ra(double x0, double y0, double x1, double y1)
{
    CArea a;
    a.append(rect(x0, y0, x1, y1));
    return a;
}

static void run(const char* what, CArea& a, double want, size_t want_curves)
{
    check(what, std::fabs(a.GetArea()), want, a.m_curves.size(), want_curves);
}

int main()
{
    CArea a;

    a = ra(0, 0, 10, 10);
    run("square", a, 100.0, 1);

    a = ra(0, 0, 10, 10);
    a.Union(ra(5, 0, 15, 10));
    run("union", a, 150.0, 1);

    a = ra(0, 0, 20, 20);
    a.Subtract(ra(5, 5, 15, 15));
    run("subtract, leaving a hole", a, 300.0, 2);

    // the fill rules this fork keeps on Subtract and Union still reach Clipper
    a = ra(0, 0, 20, 20);
    a.Subtract(ra(5, 5, 15, 15), Clipper2Lib::FillRule::NonZero, Clipper2Lib::FillRule::NonZero);
    run("subtract, NonZero fill rule", a, 300.0, 2);

    a = ra(0, 0, 10, 10);
    a.Intersect(ra(5, 0, 15, 10));
    run("intersect", a, 50.0, 1);

    a = ra(0, 0, 10, 10);
    a.Xor(ra(5, 0, 15, 10));
    run("xor", a, 100.0, 2);

    // Offset counts inwards positive; OffsetWithClipper counts outwards positive
    a = ra(0, 0, 10, 10);
    a.Offset(1.0);
    run("Offset(+1), inward", a, 64.0, 1);

    a = ra(0, 0, 10, 10);
    a.Offset(-1.0);
    run("Offset(-1), outward", a, 143.141592, 1);

    a = ra(0, 0, 10, 10);
    a.OffsetWithClipper(1.0);
    run("OffsetWithClipper(+1), outward", a, 143.141593, 1);

    a = ra(0, 0, 10, 10);
    a.OffsetWithClipper(-1.0);
    run("OffsetWithClipper(-1), inward", a, 64.0, 1);

    a = ra(0, 0, 10, 10);
    a.Thicken(0.5);
    run("Thicken(0.5)", a, 39.785398, 2);

    // Clip takes the clip area by reference since the Clipper2 move
    a = ra(0, 0, 10, 10);
    a.Clip(
        Clipper2Lib::ClipType::Intersection,
        ra(5, 0, 15, 10),
        Clipper2Lib::FillRule::NonZero,
        Clipper2Lib::FillRule::NonZero);
    run("Clip(Intersection)", a, 50.0, 1);

    // the two exports IfcOpenShell drives Clipper through, round tripped
    Clipper2Lib::Path64 path;
    CurveToClipperPath(rect(0, 0, 10, 10), path);
    CCurve back;
    CurveFromClipperPath(back, path);
    CArea round_tripped;
    round_tripped.append(back);
    run("CurveTo/FromClipperPath round trip", round_tripped, 100.0, 1);

    // an arc discretised onto the lattice, recognised again on the way out
    CCurve half;
    half.append(CVertex(Point(0, 0)));
    half.append(CVertex(1, Point(10, 0), Point(5, 0)));
    half.append(CVertex(Point(0, 0)));
    CArea disc;
    disc.append(half);
    disc.Union(ra(0, -1, 10, 0));
    run("half-disc union rectangle", disc, 39.315679, 1);

    std::printf("\n%s (%d failure%s)\n",
                failures ? "FAILED" : "PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}

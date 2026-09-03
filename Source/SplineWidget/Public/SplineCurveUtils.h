#pragma once

#include "CoreMinimal.h"
#include "SplineControlPoint.h"

/** Stateless helpers for evaluating and tessellating cubic Bézier splines. */
class SPLINEWIDGET_API FSplineCurveUtils
{
public:
	/** Default number of line segments used to approximate each Bézier segment. */
	static constexpr int32 DefaultSegmentsPerCurve = 16;

	/**
	 * Evaluate a cubic Bézier at parameter T in [0,1] given four control points.
	 */
	static FVector2D EvaluateCubicBezier(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, float T);

	/**
	 * Sample the whole spline into a flat polyline.
	 * @param Points            Control points.
	 * @param bClosed           When true, a wrap-around segment from the last point back to the first is appended.
	 * @param SegmentsPerCurve  Line segments generated per Bézier segment.
	 * @return                  Ordered polyline points. Empty if fewer than two control points.
	 */
	static TArray<FVector2D> SampleSpline(const TArray<FSplineControlPoint>& Points, bool bClosed, int32 SegmentsPerCurve = DefaultSegmentsPerCurve);

	/**
	 * Compute Catmull-Rom-style default tangents for every control point in-place,
	 * so a curve authored by Location only stays smooth.
	 */
	static void ComputeDefaultTangents(TArray<FSplineControlPoint>& Points, bool bClosed);

	/**
	 * Ear-clipping triangulation of a (possibly concave) simple polygon.
	 * @param Polygon    Ordered outline points.
	 * @param OutIndices Triangle index triplets into Polygon.
	 * @return           True if triangulation produced at least one triangle.
	 */
	static bool TriangulatePolygon(const TArray<FVector2D>& Polygon, TArray<int32>& OutIndices);
};

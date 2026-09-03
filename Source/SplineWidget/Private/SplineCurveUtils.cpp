#include "SplineCurveUtils.h"

FVector2D FSplineCurveUtils::EvaluateCubicBezier(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, float T)
{
	const float U = 1.0f - T;
	const float UU = U * U;
	const float TT = T * T;
	return (UU * U) * P0
		+ (3.0f * UU * T) * P1
		+ (3.0f * U * TT) * P2
		+ (TT * T) * P3;
}

TArray<FVector2D> FSplineCurveUtils::SampleSpline(const TArray<FSplineControlPoint>& Points, bool bClosed, int32 SegmentsPerCurve)
{
	TArray<FVector2D> Result;

	const int32 NumPoints = Points.Num();
	if (NumPoints < 2)
	{
		return Result;
	}

	const int32 Steps = FMath::Max(1, SegmentsPerCurve);
	const int32 NumSegments = bClosed ? NumPoints : NumPoints - 1;

	Result.Add(Points[0].Location);

	for (int32 Seg = 0; Seg < NumSegments; ++Seg)
	{
		const FSplineControlPoint& A = Points[Seg];
		const FSplineControlPoint& B = Points[(Seg + 1) % NumPoints];

		const FVector2D P0 = A.Location;
		const FVector2D P1 = A.Location + A.LeaveTangent;
		const FVector2D P2 = B.Location + B.ArriveTangent;
		const FVector2D P3 = B.Location;

		for (int32 Step = 1; Step <= Steps; ++Step)
		{
			const float T = static_cast<float>(Step) / static_cast<float>(Steps);
			Result.Add(EvaluateCubicBezier(P0, P1, P2, P3, T));
		}
	}

	// For a closed loop the final sample equals the first point; drop the duplicate.
	if (bClosed && Result.Num() > 1 && Result.Last().Equals(Result[0], KINDA_SMALL_NUMBER))
	{
		Result.Pop(EAllowShrinking::No);
	}

	return Result;
}

void FSplineCurveUtils::ComputeDefaultTangents(TArray<FSplineControlPoint>& Points, bool bClosed)
{
	const int32 NumPoints = Points.Num();
	if (NumPoints < 2)
	{
		if (NumPoints == 1)
		{
			Points[0].ArriveTangent = FVector2D::ZeroVector;
			Points[0].LeaveTangent = FVector2D::ZeroVector;
		}
		return;
	}

	for (int32 i = 0; i < NumPoints; ++i)
	{
		FVector2D Tangent;

		if (bClosed)
		{
			const FVector2D& Prev = Points[(i - 1 + NumPoints) % NumPoints].Location;
			const FVector2D& Next = Points[(i + 1) % NumPoints].Location;
			Tangent = (Next - Prev) / 6.0f;
		}
		else if (i == 0)
		{
			Tangent = (Points[1].Location - Points[0].Location) / 3.0f;
		}
		else if (i == NumPoints - 1)
		{
			Tangent = (Points[NumPoints - 1].Location - Points[NumPoints - 2].Location) / 3.0f;
		}
		else
		{
			const FVector2D& Prev = Points[i - 1].Location;
			const FVector2D& Next = Points[i + 1].Location;
			Tangent = (Next - Prev) / 6.0f;
		}

		Points[i].LeaveTangent = Tangent;
		Points[i].ArriveTangent = -Tangent;
	}
}

namespace
{
	float SignedArea(const TArray<FVector2D>& Polygon)
	{
		float Area = 0.0f;
		const int32 N = Polygon.Num();
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D& A = Polygon[i];
			const FVector2D& B = Polygon[(i + 1) % N];
			Area += (A.X * B.Y) - (B.X * A.Y);
		}
		return 0.5f * Area;
	}

	bool IsPointInTriangle(const FVector2D& P, const FVector2D& A, const FVector2D& B, const FVector2D& C)
	{
		const float D1 = (P.X - B.X) * (A.Y - B.Y) - (A.X - B.X) * (P.Y - B.Y);
		const float D2 = (P.X - C.X) * (B.Y - C.Y) - (B.X - C.X) * (P.Y - C.Y);
		const float D3 = (P.X - A.X) * (C.Y - A.Y) - (C.X - A.X) * (P.Y - A.Y);

		const bool HasNeg = (D1 < 0) || (D2 < 0) || (D3 < 0);
		const bool HasPos = (D1 > 0) || (D2 > 0) || (D3 > 0);
		return !(HasNeg && HasPos);
	}
}

bool FSplineCurveUtils::TriangulatePolygon(const TArray<FVector2D>& Polygon, TArray<int32>& OutIndices)
{
	OutIndices.Reset();

	const int32 N = Polygon.Num();
	if (N < 3)
	{
		return false;
	}

	// Working list of vertex indices, ordered CCW.
	TArray<int32> V;
	V.Reserve(N);
	if (SignedArea(Polygon) < 0.0f)
	{
		for (int32 i = N - 1; i >= 0; --i)
		{
			V.Add(i);
		}
	}
	else
	{
		for (int32 i = 0; i < N; ++i)
		{
			V.Add(i);
		}
	}

	int32 Remaining = V.Num();
	int32 GuardCounter = 2 * Remaining;

	int32 Cursor = 0;
	while (Remaining > 2 && GuardCounter-- > 0)
	{
		const int32 Count = V.Num();
		const int32 IPrev = (Cursor + Count - 1) % Count;
		const int32 ICurr = Cursor % Count;
		const int32 INext = (Cursor + 1) % Count;

		const FVector2D& A = Polygon[V[IPrev]];
		const FVector2D& B = Polygon[V[ICurr]];
		const FVector2D& C = Polygon[V[INext]];

		// Convex vertex test (CCW winding).
		const float Cross = (B.X - A.X) * (C.Y - A.Y) - (B.Y - A.Y) * (C.X - A.X);
		bool bIsEar = Cross > 0.0f;

		if (bIsEar)
		{
			for (int32 j = 0; j < Count; ++j)
			{
				if (j == IPrev || j == ICurr || j == INext)
				{
					continue;
				}
				if (IsPointInTriangle(Polygon[V[j]], A, B, C))
				{
					bIsEar = false;
					break;
				}
			}
		}

		if (bIsEar)
		{
			OutIndices.Add(V[IPrev]);
			OutIndices.Add(V[ICurr]);
			OutIndices.Add(V[INext]);

			V.RemoveAt(ICurr, 1, EAllowShrinking::No);
			--Remaining;
			GuardCounter = 2 * Remaining;
			Cursor = 0;
		}
		else
		{
			++Cursor;
			if (Cursor >= V.Num())
			{
				Cursor = 0;
			}
		}
	}

	return OutIndices.Num() >= 3;
}

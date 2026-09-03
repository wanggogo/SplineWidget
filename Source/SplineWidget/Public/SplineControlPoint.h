#pragma once

#include "CoreMinimal.h"
#include "SplineControlPoint.generated.h"

/** A single control point of a cubic Bézier spline, in local pixel-absolute coordinates. */
USTRUCT(BlueprintType)
struct SPLINEWIDGET_API FSplineControlPoint
{
	GENERATED_BODY()

	/** Position of the control point, in local widget pixels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
	FVector2D Location = FVector2D::ZeroVector;

	/** Incoming tangent handle, relative to Location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
	FVector2D ArriveTangent = FVector2D::ZeroVector;

	/** Outgoing tangent handle, relative to Location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
	FVector2D LeaveTangent = FVector2D::ZeroVector;

	FSplineControlPoint() = default;

	explicit FSplineControlPoint(const FVector2D& InLocation)
		: Location(InLocation)
	{
	}
};

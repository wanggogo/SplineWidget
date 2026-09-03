#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "SplineControlPoint.h"
#include "SplineWidget.generated.h"

class SSplineWidget;

/**
 * UMG control that displays a cubic Bézier spline defined by editable control points.
 * Supports stroke styling (thickness, color) and optional closed-loop fill.
 */
UCLASS()
class SPLINEWIDGET_API USplineWidget : public UWidget
{
	GENERATED_BODY()

public:
	USplineWidget();

	/** Control points of the spline, in local pixel-absolute coordinates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
	TArray<FSplineControlPoint> ControlPoints;

	/** When true, the curve closes into a loop and its interior is filled with FillColor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
	bool bClosed = false;

	/**
	 * When true, tangent handles are auto-computed from control point positions for a smooth curve.
	 * Disable to edit tangent handles manually (e.g. via the Designer canvas).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
	bool bAutoTangents = true;

	/** Stroke width of the curve, in pixels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline|Style", meta = (ClampMin = "0.0"))
	float Thickness = 2.0f;

	/** Color of the curve stroke. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline|Style")
	FLinearColor LineColor = FLinearColor::White;

	/** Fill color used when bClosed is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline|Style")
	FLinearColor FillColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.25f);

	/** Number of line segments used to approximate each Bézier segment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline", meta = (ClampMin = "1", ClampMax = "128"))
	int32 SegmentsPerCurve = 16;

	UFUNCTION(BlueprintCallable, Category = "Spline")
	void SetControlPoints(const TArray<FSplineControlPoint>& InControlPoints);

	UFUNCTION(BlueprintCallable, Category = "Spline")
	void SetClosed(bool bInClosed);

	UFUNCTION(BlueprintCallable, Category = "Spline|Style")
	void SetThickness(float InThickness);

	UFUNCTION(BlueprintCallable, Category = "Spline|Style")
	void SetLineColor(FLinearColor InLineColor);

	UFUNCTION(BlueprintCallable, Category = "Spline|Style")
	void SetFillColor(FLinearColor InFillColor);

	//~ Begin UWidget interface
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif
	//~ End UWidget interface

protected:
	//~ Begin UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	//~ End UWidget interface

	/** Returns control points with tangents resolved (auto-computed when bAutoTangents is set). */
	TArray<FSplineControlPoint> GetEffectiveControlPoints() const;

	TSharedPtr<SSplineWidget> MySplineWidget;
};

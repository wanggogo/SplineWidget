#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "SplineControlPoint.h"

/** Slate leaf widget that paints a cubic Bézier spline with optional closed-loop fill. */
class SPLINEWIDGET_API SSplineWidget : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SSplineWidget)
		: _bClosed(false)
		, _Thickness(2.0f)
		, _LineColor(FLinearColor::White)
		, _FillColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.25f))
		, _SegmentsPerCurve(16)
	{
	}
		SLATE_ARGUMENT(TArray<FSplineControlPoint>, ControlPoints)
		SLATE_ARGUMENT(bool, bClosed)
		SLATE_ARGUMENT(float, Thickness)
		SLATE_ARGUMENT(FLinearColor, LineColor)
		SLATE_ARGUMENT(FLinearColor, FillColor)
		SLATE_ARGUMENT(int32, SegmentsPerCurve)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetControlPoints(const TArray<FSplineControlPoint>& InControlPoints);
	void SetClosed(bool bInClosed);
	void SetThickness(float InThickness);
	void SetLineColor(const FLinearColor& InLineColor);
	void SetFillColor(const FLinearColor& InFillColor);
	void SetSegmentsPerCurve(int32 InSegmentsPerCurve);

	const TArray<FSplineControlPoint>& GetControlPoints() const { return ControlPoints; }

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

protected:
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	TArray<FSplineControlPoint> ControlPoints;
	bool bClosed = false;
	float Thickness = 2.0f;
	FLinearColor LineColor = FLinearColor::White;
	FLinearColor FillColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.25f);
	int32 SegmentsPerCurve = 16;
};

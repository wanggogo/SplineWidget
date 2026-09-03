#include "SplineWidget.h"
#include "SSplineWidget.h"
#include "SplineCurveUtils.h"

#define LOCTEXT_NAMESPACE "SplineWidget"

USplineWidget::USplineWidget()
{
	// A simple default curve so a freshly placed widget shows something.
	ControlPoints.Add(FSplineControlPoint(FVector2D(0.0f, 50.0f)));
	ControlPoints.Add(FSplineControlPoint(FVector2D(75.0f, 0.0f)));
	ControlPoints.Add(FSplineControlPoint(FVector2D(150.0f, 50.0f)));
}

TArray<FSplineControlPoint> USplineWidget::GetEffectiveControlPoints() const
{
	TArray<FSplineControlPoint> Effective = ControlPoints;
	if (bAutoTangents)
	{
		FSplineCurveUtils::ComputeDefaultTangents(Effective, bClosed);
	}
	return Effective;
}

TSharedRef<SWidget> USplineWidget::RebuildWidget()
{
	MySplineWidget = SNew(SSplineWidget)
		.ControlPoints(GetEffectiveControlPoints())
		.bClosed(bClosed)
		.Thickness(Thickness)
		.LineColor(LineColor)
		.FillColor(FillColor)
		.SegmentsPerCurve(SegmentsPerCurve);

	return MySplineWidget.ToSharedRef();
}

void USplineWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (!MySplineWidget.IsValid())
	{
		return;
	}

	MySplineWidget->SetSegmentsPerCurve(SegmentsPerCurve);
	MySplineWidget->SetClosed(bClosed);
	MySplineWidget->SetThickness(Thickness);
	MySplineWidget->SetLineColor(LineColor);
	MySplineWidget->SetFillColor(FillColor);
	MySplineWidget->SetControlPoints(GetEffectiveControlPoints());
}

void USplineWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MySplineWidget.Reset();
}

void USplineWidget::SetControlPoints(const TArray<FSplineControlPoint>& InControlPoints)
{
	ControlPoints = InControlPoints;
	if (MySplineWidget.IsValid())
	{
		MySplineWidget->SetControlPoints(GetEffectiveControlPoints());
	}
}

void USplineWidget::SetClosed(bool bInClosed)
{
	bClosed = bInClosed;
	if (MySplineWidget.IsValid())
	{
		MySplineWidget->SetClosed(bClosed);
		MySplineWidget->SetControlPoints(GetEffectiveControlPoints());
	}
}

void USplineWidget::SetThickness(float InThickness)
{
	Thickness = InThickness;
	if (MySplineWidget.IsValid())
	{
		MySplineWidget->SetThickness(Thickness);
	}
}

void USplineWidget::SetLineColor(FLinearColor InLineColor)
{
	LineColor = InLineColor;
	if (MySplineWidget.IsValid())
	{
		MySplineWidget->SetLineColor(LineColor);
	}
}

void USplineWidget::SetFillColor(FLinearColor InFillColor)
{
	FillColor = InFillColor;
	if (MySplineWidget.IsValid())
	{
		MySplineWidget->SetFillColor(FillColor);
	}
}

#if WITH_EDITOR
const FText USplineWidget::GetPaletteCategory()
{
	return LOCTEXT("Spline", "Spline");
}
#endif

#undef LOCTEXT_NAMESPACE

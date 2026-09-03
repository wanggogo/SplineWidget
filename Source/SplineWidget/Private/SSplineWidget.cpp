#include "SSplineWidget.h"
#include "SplineCurveUtils.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"

void SSplineWidget::Construct(const FArguments& InArgs)
{
	ControlPoints = InArgs._ControlPoints;
	bClosed = InArgs._bClosed;
	Thickness = InArgs._Thickness;
	LineColor = InArgs._LineColor;
	FillColor = InArgs._FillColor;
	SegmentsPerCurve = InArgs._SegmentsPerCurve;
}

void SSplineWidget::SetControlPoints(const TArray<FSplineControlPoint>& InControlPoints)
{
	ControlPoints = InControlPoints;
	Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
}

void SSplineWidget::SetClosed(bool bInClosed)
{
	if (bClosed != bInClosed)
	{
		bClosed = bInClosed;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SSplineWidget::SetThickness(float InThickness)
{
	if (Thickness != InThickness)
	{
		Thickness = InThickness;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SSplineWidget::SetLineColor(const FLinearColor& InLineColor)
{
	if (LineColor != InLineColor)
	{
		LineColor = InLineColor;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SSplineWidget::SetFillColor(const FLinearColor& InFillColor)
{
	if (FillColor != InFillColor)
	{
		FillColor = InFillColor;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SSplineWidget::SetSegmentsPerCurve(int32 InSegmentsPerCurve)
{
	const int32 Clamped = FMath::Max(1, InSegmentsPerCurve);
	if (SegmentsPerCurve != Clamped)
	{
		SegmentsPerCurve = Clamped;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

int32 SSplineWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (ControlPoints.Num() < 2)
	{
		return LayerId;
	}

	const TArray<FVector2D> Polyline = FSplineCurveUtils::SampleSpline(ControlPoints, bClosed, SegmentsPerCurve);
	if (Polyline.Num() < 2)
	{
		return LayerId;
	}

	// Fill layer (closed loops only).
	if (bClosed && FillColor.A > 0.0f && Polyline.Num() >= 3)
	{
		TArray<int32> TriIndices;
		if (FSplineCurveUtils::TriangulatePolygon(Polyline, TriIndices))
		{
			const FSlateRenderTransform& PaintRenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();
			const FColor VertexColor = FillColor.ToFColor(true);

			TArray<FSlateVertex> Vertices;
			Vertices.Reserve(Polyline.Num());
			for (const FVector2D& P : Polyline)
			{
				Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
					PaintRenderTransform,
					FVector2f(static_cast<float>(P.X), static_cast<float>(P.Y)),
					FVector2f::ZeroVector,
					VertexColor));
			}

			TArray<SlateIndex> Indices;
			Indices.Reserve(TriIndices.Num());
			for (int32 Index : TriIndices)
			{
				Indices.Add(static_cast<SlateIndex>(Index));
			}

			const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
			if (WhiteBrush && FSlateApplication::IsInitialized())
			{
				const FSlateResourceHandle Handle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*WhiteBrush);
				FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, Handle, Vertices, Indices, nullptr, 0, 0);
			}
		}
	}

	// Stroke layer, drawn on top of any fill.
	const int32 StrokeLayer = LayerId + 1;
	TArray<FVector2D> LinePoints = Polyline;
	if (bClosed)
	{
		LinePoints.Add(Polyline[0]);
	}

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		StrokeLayer,
		AllottedGeometry.ToPaintGeometry(),
		LinePoints,
		ESlateDrawEffect::None,
		LineColor,
		true,
		Thickness);

	return StrokeLayer;
}

FVector2D SSplineWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	if (ControlPoints.Num() == 0)
	{
		return FVector2D::ZeroVector;
	}

	FVector2D Min = ControlPoints[0].Location;
	FVector2D Max = ControlPoints[0].Location;
	for (const FSplineControlPoint& Point : ControlPoints)
	{
		Min = FVector2D::Min(Min, Point.Location);
		Max = FVector2D::Max(Max, Point.Location);
	}

	return Max - Min;
}

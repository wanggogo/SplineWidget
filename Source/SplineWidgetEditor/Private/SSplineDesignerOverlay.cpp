#include "SSplineDesignerOverlay.h"
#include "IUMGDesigner.h"
#include "SplineWidget.h"
#include "SplineControlPoint.h"
#include "SplineCurveUtils.h"
#include "Components/CanvasPanelSlot.h"
#include "UObject/UObjectGlobals.h"
#include "Editor.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "SplineWidgetEditor"

namespace SplineOverlay
{
	static const FLinearColor ControlColor(1.0f, 0.55f, 0.1f);
	static const FLinearColor ControlHoverColor(1.0f, 0.9f, 0.2f);
	static const FLinearColor TangentColor(0.2f, 0.8f, 1.0f);
	static const FLinearColor TangentLineColor(0.2f, 0.8f, 1.0f, 0.6f);
}

void SSplineDesignerOverlay::Construct(const FArguments& InArgs)
{
	Designer = InArgs._Designer;
	SelectedWidget = InArgs._SelectedWidget;
	SetCanTick(false);
}

USplineWidget* SSplineDesignerOverlay::GetTemplateWidget() const
{
	if (!SelectedWidget.IsValid())
	{
		return nullptr;
	}
	return Cast<USplineWidget>(SelectedWidget.GetTemplate());
}

void SSplineDesignerOverlay::SyncPreview() const
{
	const USplineWidget* Template = GetTemplateWidget();
	if (!Template || !SelectedWidget.IsValid())
	{
		return;
	}

	// The Designer renders a separate preview instance; MarkDesignModifed(false) does not
	// recompile, so mirror the edited data onto the preview and re-synchronize its slate
	// widget for an immediate curve update.
	if (USplineWidget* Preview = Cast<USplineWidget>(SelectedWidget.GetPreview()))
	{
		Preview->ControlPoints = Template->ControlPoints;
		Preview->bClosed = Template->bClosed;
		Preview->bAutoTangents = Template->bAutoTangents;
		Preview->SynchronizeProperties();
	}
}

void SSplineDesignerOverlay::FitSlotToControlPoints() const
{
	USplineWidget* Template = GetTemplateWidget();
	if (!Template || Template->ControlPoints.Num() == 0)
	{
		return;
	}

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Template->Slot);
	if (!CanvasSlot)
	{
		// Only Canvas-parented widgets have an absolute slot to fit; no-op otherwise.
		return;
	}

	// Bounding box of the control-point Locations (tangent handles are intentionally excluded).
	FVector2D Min(TNumericLimits<double>::Max(), TNumericLimits<double>::Max());
	FVector2D Max(TNumericLimits<double>::Lowest(), TNumericLimits<double>::Lowest());
	for (const FSplineControlPoint& Point : Template->ControlPoints)
	{
		Min.X = FMath::Min(Min.X, Point.Location.X);
		Min.Y = FMath::Min(Min.Y, Point.Location.Y);
		Max.X = FMath::Max(Max.X, Point.Location.X);
		Max.Y = FMath::Max(Max.Y, Point.Location.Y);
	}

	// Points are expressed relative to the current slot origin (point-space (0,0) == slot origin).
	// The new slot origin should sit at the padded box top-left, so shift the slot by OriginDelta
	// and subtract the same delta from every point to keep the curve visually fixed.
	const FVector2D OriginDelta = Min - FVector2D(SlotPadding, SlotPadding);
	const FVector2D NewSize = (Max - Min) + FVector2D(2.0 * SlotPadding, 2.0 * SlotPadding);

	const FVector2D CurrentSize = CanvasSlot->GetSize();
	const bool bOriginUnchanged = OriginDelta.IsNearlyZero(0.5);
	const bool bSizeUnchanged = (CurrentSize - NewSize).IsNearlyZero(0.5);
	if (bOriginUnchanged && bSizeUnchanged)
	{
		return;
	}

	Template->Modify();
	CanvasSlot->Modify();
	CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
	CanvasSlot->SetAutoSize(false);
	CanvasSlot->SetPosition(CanvasSlot->GetPosition() + OriginDelta);
	CanvasSlot->SetSize(NewSize);

	if (!bOriginUnchanged)
	{
		for (FSplineControlPoint& Point : Template->ControlPoints)
		{
			Point.Location -= OriginDelta;
		}
	}

	// Mirror the slot geometry onto the preview instance so the canvas updates without a compile.
	if (UWidget* PreviewWidget = SelectedWidget.GetPreview())
	{
		if (UCanvasPanelSlot* PreviewSlot = Cast<UCanvasPanelSlot>(PreviewWidget->Slot))
		{
			PreviewSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			PreviewSlot->SetAutoSize(false);
			PreviewSlot->SetPosition(CanvasSlot->GetPosition());
			PreviewSlot->SetSize(NewSize);
		}
	}

	// Refresh the Details panel's Position/Size fields immediately. Broadcast the change
	// directly to listeners (the details view) instead of calling PostEditChangeProperty:
	// the slot's editor PostEditChangeChainProperty runs RebaseLayout against its saved
	// PreEditGeometry, which is only valid mid-designer-drag; invoking it here would corrupt
	// the slot's base layout and break the next widget move.
	if (FProperty* LayoutProperty = FindFProperty<FProperty>(UCanvasPanelSlot::StaticClass(), TEXT("LayoutData")))
	{
		FPropertyChangedEvent ChangedEvent(LayoutProperty, EPropertyChangeType::ValueSet);
		FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(CanvasSlot, ChangedEvent);
	}
}

bool SSplineDesignerOverlay::GetWidgetGeometry(FGeometry& OutGeometry) const
{
	if (!Designer || !SelectedWidget.IsValid())
	{
		return false;
	}
	return Designer->GetWidgetGeometry(SelectedWidget, OutGeometry);
}

void SSplineDesignerOverlay::GetHandleBoundsControl(FVector2D& OutMin, FVector2D& OutMax) const
{
	TArray<FSplineControlPoint> Points;
	bool bClosed = false;
	GetDisplayPoints(Points, bClosed);

	if (Points.Num() == 0)
	{
		OutMin = FVector2D::ZeroVector;
		OutMax = FVector2D::ZeroVector;
		return;
	}

	OutMin = FVector2D(TNumericLimits<double>::Max(), TNumericLimits<double>::Max());
	OutMax = FVector2D(TNumericLimits<double>::Lowest(), TNumericLimits<double>::Lowest());

	auto Accumulate = [&OutMin, &OutMax](const FVector2D& P)
	{
		OutMin.X = FMath::Min(OutMin.X, P.X);
		OutMin.Y = FMath::Min(OutMin.Y, P.Y);
		OutMax.X = FMath::Max(OutMax.X, P.X);
		OutMax.Y = FMath::Max(OutMax.Y, P.Y);
	};

	for (const FSplineControlPoint& Point : Points)
	{
		Accumulate(Point.Location);
		Accumulate(Point.Location + Point.ArriveTangent);
		Accumulate(Point.Location + Point.LeaveTangent);
	}

	// Always include the widget's own local box so the overlay never shrinks below it.
	Accumulate(FVector2D::ZeroVector);
}

FVector2D SSplineDesignerOverlay::GetOriginControl() const
{
	FVector2D MinC, MaxC;
	GetHandleBoundsControl(MinC, MaxC);
	return MinC - FVector2D(HandleMarginControl, HandleMarginControl);
}

FVector2D SSplineDesignerOverlay::ComputeSlotOffset() const
{
	if (!Designer || !SelectedWidget.IsValid())
	{
		return FVector2D::ZeroVector;
	}

	FGeometry WidgetGeom;
	FGeometry ParentGeom;
	if (!Designer->GetWidgetGeometry(SelectedWidget, WidgetGeom) ||
		!Designer->GetWidgetParentGeometry(SelectedWidget, ParentGeom))
	{
		return FVector2D::ZeroVector;
	}

	// RelativeFromParent places the slot at the parent's top-left plus this offset, all in
	// designer-local pixels. Move it so the slot's top-left lands on the handle-bounds origin.
	const FGeometry DesignerGeom = Designer->GetDesignerGeometry();
	const FVector2D OriginControl = GetOriginControl();
	const FVector2D OriginDesigner = DesignerGeom.AbsoluteToLocal(WidgetGeom.LocalToAbsolute(OriginControl));
	const FVector2D ParentDesigner = DesignerGeom.AbsoluteToLocal(ParentGeom.LocalToAbsolute(FVector2D::ZeroVector));
	return OriginDesigner - ParentDesigner;
}

float SSplineDesignerOverlay::GetContentScale(const FGeometry& OverlayGeometry) const
{
	if (!Designer)
	{
		return 1.0f;
	}
	const float LayoutScale = OverlayGeometry.GetAccumulatedLayoutTransform().GetScale();
	if (LayoutScale <= KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}
	return Designer->GetPreviewScale() / LayoutScale;
}

void SSplineDesignerOverlay::GetDisplayPoints(TArray<FSplineControlPoint>& OutPoints, bool& bOutClosed) const
{
	OutPoints.Reset();
	bOutClosed = false;

	const USplineWidget* Template = GetTemplateWidget();
	if (!Template)
	{
		return;
	}

	OutPoints = Template->ControlPoints;
	bOutClosed = Template->bClosed;
	if (Template->bAutoTangents)
	{
		FSplineCurveUtils::ComputeDefaultTangents(OutPoints, bOutClosed);
	}
}

// The overlay is positioned and sized by the designer to exactly cover the selected
// widget, so the overlay's local space equals the widget's control-point space
// (widget-local pixels). Hit-tests therefore compare control point locations directly
// against the mouse position converted to overlay-local space.
int32 SSplineDesignerOverlay::HitTestControlPoint(const FVector2D& MouseLocal, float ContentScale) const
{
	TArray<FSplineControlPoint> Points;
	bool bClosed = false;
	GetDisplayPoints(Points, bClosed);

	const FVector2D OriginControl = GetOriginControl();
	int32 BestIndex = INDEX_NONE;
	double BestDistSq = static_cast<double>(ControlHandleRadius * ControlHandleRadius);
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const FVector2D HandleLocal = (Points[i].Location - OriginControl) * ContentScale;
		const double DistSq = FVector2D::DistSquared(HandleLocal, MouseLocal);
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			BestIndex = i;
		}
	}
	return BestIndex;
}

bool SSplineDesignerOverlay::HitTestTangent(const FVector2D& MouseLocal, float ContentScale, int32& OutPointIndex, bool& bOutArrive) const
{
	TArray<FSplineControlPoint> Points;
	bool bClosed = false;
	GetDisplayPoints(Points, bClosed);

	const FVector2D OriginControl = GetOriginControl();
	double BestDistSq = static_cast<double>(TangentHandleRadius * TangentHandleRadius);
	OutPointIndex = INDEX_NONE;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const FVector2D ArrivePos = (Points[i].Location + Points[i].ArriveTangent - OriginControl) * ContentScale;
		const FVector2D LeavePos = (Points[i].Location + Points[i].LeaveTangent - OriginControl) * ContentScale;

		const double ArriveDistSq = FVector2D::DistSquared(ArrivePos, MouseLocal);
		if (ArriveDistSq <= BestDistSq)
		{
			BestDistSq = ArriveDistSq;
			OutPointIndex = i;
			bOutArrive = true;
		}
		const double LeaveDistSq = FVector2D::DistSquared(LeavePos, MouseLocal);
		if (LeaveDistSq <= BestDistSq)
		{
			BestDistSq = LeaveDistSq;
			OutPointIndex = i;
			bOutArrive = false;
		}
	}
	return OutPointIndex != INDEX_NONE;
}

void SSplineDesignerOverlay::BeginEdit(const FText& Description)
{
	if (!bTransactionOpen && GEditor)
	{
		GEditor->BeginTransaction(Description);
		bTransactionOpen = true;
	}
	if (USplineWidget* Template = GetTemplateWidget())
	{
		Template->Modify();
	}
}

void SSplineDesignerOverlay::EndEdit()
{
	if (bTransactionOpen && GEditor)
	{
		GEditor->EndTransaction();
		bTransactionOpen = false;
	}
}

int32 SSplineDesignerOverlay::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	TArray<FSplineControlPoint> Points;
	bool bClosed = false;
	GetDisplayPoints(Points, bClosed);
	if (Points.Num() == 0)
	{
		return LayerId;
	}

	const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
	const int32 LineLayer = LayerId + 1;
	const int32 HandleLayer = LayerId + 2;
	const float ContentScale = GetContentScale(AllottedGeometry);
	const FVector2D OriginControl = GetOriginControl();

	auto DrawHandle = [&](const FVector2D& LocalPos, float Radius, const FLinearColor& Color, int32 Layer)
	{
		const FVector2f Size(Radius * 2.0f, Radius * 2.0f);
		const FVector2f TopLeft(static_cast<float>(LocalPos.X) - Radius, static_cast<float>(LocalPos.Y) - Radius);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			Layer,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D(TopLeft))),
			Brush,
			ESlateDrawEffect::None,
			Color);
	};

	for (int32 i = 0; i < Points.Num(); ++i)
	{
		// Overlay-local space = (control-point space - origin) * ContentScale, so handles
		// outside the widget box (origin < 0) still land inside the overlay's slot.
		const FVector2D CenterLocal = (Points[i].Location - OriginControl) * ContentScale;
		const FVector2D ArriveLocal = (Points[i].Location + Points[i].ArriveTangent - OriginControl) * ContentScale;
		const FVector2D LeaveLocal = (Points[i].Location + Points[i].LeaveTangent - OriginControl) * ContentScale;

		// Tangent connector lines.
		TArray<FVector2D> ArriveLine = { CenterLocal, ArriveLocal };
		TArray<FVector2D> LeaveLine = { CenterLocal, LeaveLocal };
		FSlateDrawElement::MakeLines(OutDrawElements, LineLayer, AllottedGeometry.ToPaintGeometry(), ArriveLine, ESlateDrawEffect::None, SplineOverlay::TangentLineColor, true, 1.0f);
		FSlateDrawElement::MakeLines(OutDrawElements, LineLayer, AllottedGeometry.ToPaintGeometry(), LeaveLine, ESlateDrawEffect::None, SplineOverlay::TangentLineColor, true, 1.0f);

		// Tangent handles.
		DrawHandle(ArriveLocal, TangentHandleRadius, SplineOverlay::TangentColor, HandleLayer);
		DrawHandle(LeaveLocal, TangentHandleRadius, SplineOverlay::TangentColor, HandleLayer);

		// Control point handle.
		const FLinearColor PointColor = (i == HoveredPointIndex) ? SplineOverlay::ControlHoverColor : SplineOverlay::ControlColor;
		DrawHandle(CenterLocal, ControlHandleRadius, PointColor, HandleLayer);
	}

	return HandleLayer;
}

FReply SSplineDesignerOverlay::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !GetTemplateWidget())
	{
		return FReply::Unhandled();
	}

	const FVector2D MouseLocal = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const float ContentScale = GetContentScale(MyGeometry);

	// Tangent handles take priority (drawn on top, smaller).
	int32 TangentPoint = INDEX_NONE;
	bool bArrive = false;
	if (HitTestTangent(MouseLocal, ContentScale, TangentPoint, bArrive))
	{
		USplineWidget* Template = GetTemplateWidget();
		BeginEdit(LOCTEXT("EditTangent", "Edit Spline Tangent"));
		Template->bAutoTangents = false; // preserve manual handle edits
		DragMode = bArrive ? EDragMode::TangentArrive : EDragMode::TangentLeave;
		ActivePointIndex = TangentPoint;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	const int32 PointIndex = HitTestControlPoint(MouseLocal, ContentScale);
	if (PointIndex != INDEX_NONE)
	{
		USplineWidget* Template = GetTemplateWidget();

		// Alt+click deletes the point (keeping at least two).
		if (MouseEvent.IsAltDown())
		{
			if (Template->ControlPoints.Num() > 2)
			{
				BeginEdit(LOCTEXT("DeletePoint", "Delete Spline Point"));
				Template->ControlPoints.RemoveAt(PointIndex);
				FitSlotToControlPoints();
				SyncPreview();
				if (Designer)
				{
					Designer->MarkDesignModifed(false);
				}
				EndEdit();
			}
			return FReply::Handled();
		}

		BeginEdit(LOCTEXT("MovePoint", "Move Spline Point"));
		DragMode = EDragMode::ControlPoint;
		ActivePointIndex = PointIndex;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}

FReply SSplineDesignerOverlay::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	USplineWidget* Template = GetTemplateWidget();
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !Template || Template->ControlPoints.Num() < 2)
	{
		return FReply::Unhandled();
	}

	const float ContentScale = GetContentScale(MyGeometry);
	const FVector2D MouseLocal = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D MouseControl = ((ContentScale > KINDA_SMALL_NUMBER) ? MouseLocal / ContentScale : MouseLocal) + GetOriginControl();

	// Find the nearest segment by sampling.
	TArray<FSplineControlPoint> Points;
	bool bClosed = false;
	GetDisplayPoints(Points, bClosed);

	const int32 NumSegments = bClosed ? Points.Num() : Points.Num() - 1;
	const int32 Steps = 16;
	double BestDistSq = TNumericLimits<double>::Max();
	int32 BestSegment = 0;
	for (int32 Seg = 0; Seg < NumSegments; ++Seg)
	{
		const FSplineControlPoint& A = Points[Seg];
		const FSplineControlPoint& B = Points[(Seg + 1) % Points.Num()];
		const FVector2D P0 = A.Location;
		const FVector2D P1 = A.Location + A.LeaveTangent;
		const FVector2D P2 = B.Location + B.ArriveTangent;
		const FVector2D P3 = B.Location;
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float T = static_cast<float>(Step) / static_cast<float>(Steps);
			const FVector2D Sample = FSplineCurveUtils::EvaluateCubicBezier(P0, P1, P2, P3, T);
			const double DistSq = FVector2D::DistSquared(Sample, MouseControl);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestSegment = Seg;
			}
		}
	}

	BeginEdit(LOCTEXT("AddPoint", "Add Spline Point"));
	FSplineControlPoint NewPoint(MouseControl);
	const int32 InsertIndex = FMath::Min(BestSegment + 1, Template->ControlPoints.Num());
	Template->ControlPoints.Insert(NewPoint, InsertIndex);
	FitSlotToControlPoints();
	SyncPreview();
	if (Designer)
	{
		Designer->MarkDesignModifed(false);
	}
	EndEdit();

	return FReply::Handled();
}

FReply SSplineDesignerOverlay::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	USplineWidget* Template = GetTemplateWidget();
	if (!Template)
	{
		return FReply::Unhandled();
	}

	const float ContentScale = GetContentScale(MyGeometry);
	const FVector2D MouseLocal = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D MouseControl = ((ContentScale > KINDA_SMALL_NUMBER) ? MouseLocal / ContentScale : MouseLocal) + GetOriginControl();

	if (DragMode == EDragMode::None)
	{
		// Update hover feedback.
		const int32 NewHover = HitTestControlPoint(MouseLocal, ContentScale);
		if (NewHover != HoveredPointIndex)
		{
			HoveredPointIndex = NewHover;
			Invalidate(EInvalidateWidgetReason::Paint);
		}
		return FReply::Unhandled();
	}

	if (!Template->ControlPoints.IsValidIndex(ActivePointIndex))
	{
		return FReply::Handled();
	}

	FSplineControlPoint& Point = Template->ControlPoints[ActivePointIndex];

	switch (DragMode)
	{
	case EDragMode::ControlPoint:
		Point.Location = MouseControl;
		break;
	case EDragMode::TangentArrive:
		Point.ArriveTangent = MouseControl - Point.Location;
		break;
	case EDragMode::TangentLeave:
		Point.LeaveTangent = MouseControl - Point.Location;
		break;
	default:
		break;
	}

	SyncPreview();
	if (Designer)
	{
		Designer->MarkDesignModifed(false);
	}
	Invalidate(EInvalidateWidgetReason::Paint);
	return FReply::Handled();
}

FReply SSplineDesignerOverlay::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (DragMode != EDragMode::None)
	{
		DragMode = EDragMode::None;
		ActivePointIndex = INDEX_NONE;
		// Fit the slot once at the end of the drag (not per-move) to avoid mid-drag jitter.
		FitSlotToControlPoints();
		SyncPreview();
		if (Designer)
		{
			Designer->MarkDesignModifed(false);
		}
		EndEdit();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

void SSplineDesignerOverlay::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	if (DragMode != EDragMode::None)
	{
		DragMode = EDragMode::None;
		ActivePointIndex = INDEX_NONE;
		EndEdit();
	}
}

FCursorReply SSplineDesignerOverlay::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	const FVector2D MouseLocal = MyGeometry.AbsoluteToLocal(CursorEvent.GetScreenSpacePosition());
	const float ContentScale = GetContentScale(MyGeometry);
	int32 TangentPoint = INDEX_NONE;
	bool bArrive = false;
	if (HitTestControlPoint(MouseLocal, ContentScale) != INDEX_NONE ||
		HitTestTangent(MouseLocal, ContentScale, TangentPoint, bArrive))
	{
		return FCursorReply::Cursor(EMouseCursor::CardinalCross);
	}
	return FCursorReply::Unhandled();
}

FVector2D SSplineDesignerOverlay::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	// The designer sizes this overlay's canvas slot from GetDesiredSize(). A zero size means
	// no hit-test area (no mouse input) and culled painting, so the overlay must span every
	// interactive handle (control points and tangent endpoints), including handles that fall
	// outside the selected widget's own box. Size is in overlay-local space (control units
	// scaled by the zoom/DPI factor).
	FVector2D MinC, MaxC;
	GetHandleBoundsControl(MinC, MaxC);

	const FVector2D OriginControl = GetOriginControl();
	FVector2D SpanControl = (MaxC + FVector2D(HandleMarginControl, HandleMarginControl)) - OriginControl;

	if (Designer && LayoutScaleMultiplier > KINDA_SMALL_NUMBER)
	{
		SpanControl *= Designer->GetPreviewScale() / LayoutScaleMultiplier;
	}
	return FVector2D(FMath::Max(SpanControl.X, 1.0), FMath::Max(SpanControl.Y, 1.0));
}

#undef LOCTEXT_NAMESPACE

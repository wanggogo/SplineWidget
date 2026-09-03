#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "WidgetReference.h"

class IUMGDesigner;
class USplineWidget;

/**
 * Interactive overlay drawn over a selected USplineWidget in the UMG Designer.
 * Draws draggable control point and tangent handles and edits the widget template.
 */
class SSplineDesignerOverlay : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SSplineDesignerOverlay) {}
		SLATE_ARGUMENT(IUMGDesigner*, Designer)
		SLATE_ARGUMENT(FWidgetReference, SelectedWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * Offset (in designer-local pixels) for a RelativeFromParent surface element that moves
	 * the overlay slot's top-left to the handle bounds origin, so the slot covers handles
	 * that fall outside the selected widget's box. Recomputed each layout via a bound attribute.
	 */
	FVector2D ComputeSlotOffset() const;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

protected:
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	enum class EDragMode : uint8
	{
		None,
		ControlPoint,
		TangentArrive,
		TangentLeave,
	};

	USplineWidget* GetTemplateWidget() const;

	/** Pushes the template's edited data to the live preview instance so the curve
	 *  updates immediately without requiring a Blueprint compile. */
	void SyncPreview() const;

	/** Gets the widget geometry in designer space, used to size the overlay. */
	bool GetWidgetGeometry(FGeometry& OutGeometry) const;

	/**
	 * Control-space bounds covering every interactive handle (control points and their
	 * arrive/leave tangent endpoints). Used so the overlay slot spans all handles, even
	 * those outside the widget's own bounding box.
	 */
	void GetHandleBoundsControl(FVector2D& OutMin, FVector2D& OutMax) const;

	/**
	 * Control-space coordinate that maps to the overlay's local origin (top-left). Equal to
	 * the handle bounds minimum minus a fixed margin, so handles near/beyond the widget edge
	 * remain inside the overlay and stay clickable.
	 */
	FVector2D GetOriginControl() const;

	/**
	 * Overlay-local units per control-point (widget-local) unit. The overlay lives in
	 * the non-zoomed extension canvas (DPI scale only), while the widget content is
	 * rendered at the preview scale (DPI * zoom), so control points must be scaled by
	 * PreviewScale / overlayLayoutScale to line up with the curve.
	 */
	float GetContentScale(const FGeometry& OverlayGeometry) const;

	/** Returns control points with tangents resolved for display (auto or manual). */
	void GetDisplayPoints(TArray<struct FSplineControlPoint>& OutPoints, bool& bOutClosed) const;

	/** Hit-tests compare against control points scaled into overlay-local space. */
	int32 HitTestControlPoint(const FVector2D& MouseLocal, float ContentScale) const;
	bool HitTestTangent(const FVector2D& MouseLocal, float ContentScale, int32& OutPointIndex, bool& bOutArrive) const;

	void BeginEdit(const FText& Description);
	void EndEdit();

	IUMGDesigner* Designer = nullptr;
	FWidgetReference SelectedWidget;

	EDragMode DragMode = EDragMode::None;
	int32 ActivePointIndex = INDEX_NONE;
	int32 HoveredPointIndex = INDEX_NONE;
	bool bTransactionOpen = false;

	/** Handle sizes / hit radii in screen pixels. */
	static constexpr float ControlHandleRadius = 6.0f;
	static constexpr float TangentHandleRadius = 5.0f;

	/** Extra control-space padding around the handle bounds so handle boxes are not clipped. */
	static constexpr float HandleMarginControl = 16.0f;
};

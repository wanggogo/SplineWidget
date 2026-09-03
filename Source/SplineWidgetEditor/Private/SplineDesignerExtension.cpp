#include "SplineDesignerExtension.h"
#include "SSplineDesignerOverlay.h"
#include "SplineWidget.h"
#include "WidgetReference.h"
#include "Widgets/SWidget.h"

FSplineDesignerExtension::FSplineDesignerExtension()
{
	ExtensionId = FName(TEXT("SplineDesignerExtension"));
}

bool FSplineDesignerExtension::CanExtendSelection(const TArray<FWidgetReference>& Selection) const
{
	if (Selection.Num() != 1)
	{
		return false;
	}

	return Selection[0].IsValid() && Cast<USplineWidget>(Selection[0].GetTemplate()) != nullptr;
}

void FSplineDesignerExtension::ExtendSelection(const TArray<FWidgetReference>& Selection, TArray<TSharedRef<FDesignerSurfaceElement>>& SurfaceElements)
{
	SelectionCache = Selection;

	if (Selection.Num() != 1)
	{
		return;
	}

	const FWidgetReference& Selected = Selection[0];
	if (!Selected.IsValid() || Cast<USplineWidget>(Selected.GetTemplate()) == nullptr)
	{
		return;
	}

	TSharedRef<SSplineDesignerOverlay> Overlay = SNew(SSplineDesignerOverlay)
		.Designer(Designer)
		.SelectedWidget(Selected);

	// Position the overlay relative to the parent with a dynamic offset so its slot top-left
	// tracks the handle-bounds origin. This lets the slot cover control points and tangent
	// handles that lie outside the selected widget's box, keeping them clickable.
	TWeakPtr<SSplineDesignerOverlay> WeakOverlay = Overlay;
	TAttribute<FVector2D> OffsetAttr = TAttribute<FVector2D>::Create([WeakOverlay]()
	{
		if (TSharedPtr<SSplineDesignerOverlay> Pinned = WeakOverlay.Pin())
		{
			return Pinned->ComputeSlotOffset();
		}
		return FVector2D::ZeroVector;
	});

	SurfaceElements.Add(MakeShared<FDesignerSurfaceElement>(Overlay, EExtensionLayoutLocation::RelativeFromParent, OffsetAttr));
}

TSharedRef<FDesignerExtension> FSplineDesignerExtensionFactory::CreateDesignerExtension() const
{
	return StaticCastSharedRef<FDesignerExtension>(MakeShared<FSplineDesignerExtension>());
}

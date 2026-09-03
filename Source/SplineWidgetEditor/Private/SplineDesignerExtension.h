#pragma once

#include "CoreMinimal.h"
#include "DesignerExtension.h"
#include "IHasDesignerExtensibility.h"

/**
 * Designer extension that adds on-canvas control point / tangent editing for USplineWidget.
 * It surfaces an interactive overlay widget over the selected spline.
 */
class FSplineDesignerExtension : public FDesignerExtension
{
public:
	FSplineDesignerExtension();

	//~ Begin FDesignerExtension interface
	virtual bool CanExtendSelection(const TArray<FWidgetReference>& Selection) const override;
	virtual void ExtendSelection(const TArray<FWidgetReference>& Selection, TArray<TSharedRef<FDesignerSurfaceElement>>& SurfaceElements) override;
	//~ End FDesignerExtension interface
};

/** Factory that creates one FSplineDesignerExtension per UMG designer instance. */
class FSplineDesignerExtensionFactory : public IDesignerExtensionFactory, public TSharedFromThis<FSplineDesignerExtensionFactory>
{
public:
	FSplineDesignerExtensionFactory() = default;
	virtual ~FSplineDesignerExtensionFactory() = default;

	virtual TSharedRef<FDesignerExtension> CreateDesignerExtension() const override;
};

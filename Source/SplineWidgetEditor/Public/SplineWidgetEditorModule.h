#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSplineDesignerExtensionFactory;

class FSplineWidgetEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterDesignerExtension();
	void OnModulesChanged(FName ModuleName, EModuleChangeReason Reason);

	TSharedPtr<FSplineDesignerExtensionFactory> DesignerExtensionFactory;
	FDelegateHandle ModulesChangedHandle;
};

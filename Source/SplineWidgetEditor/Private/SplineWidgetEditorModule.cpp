#include "SplineWidgetEditorModule.h"
#include "SplineDesignerExtension.h"
#include "UMGEditorModule.h"
#include "IHasDesignerExtensibility.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FSplineWidgetEditorModule, SplineWidgetEditor)

void FSplineWidgetEditorModule::StartupModule()
{
	// UMGEditor loads at PostEngineInit too, so it may not be available yet when this
	// module starts. Register immediately if it is loaded; otherwise wait for it.
	if (FModuleManager::Get().IsModuleLoaded("UMGEditor"))
	{
		RegisterDesignerExtension();
	}
	else
	{
		ModulesChangedHandle = FModuleManager::Get().OnModulesChanged().AddRaw(this, &FSplineWidgetEditorModule::OnModulesChanged);
	}
}

void FSplineWidgetEditorModule::ShutdownModule()
{
	if (ModulesChangedHandle.IsValid())
	{
		FModuleManager::Get().OnModulesChanged().Remove(ModulesChangedHandle);
		ModulesChangedHandle.Reset();
	}

	if (DesignerExtensionFactory.IsValid())
	{
		if (IUMGEditorModule* UMGEditor = FModuleManager::GetModulePtr<IUMGEditorModule>("UMGEditor"))
		{
			if (TSharedPtr<FDesignerExtensibilityManager> Manager = UMGEditor->GetDesignerExtensibilityManager())
			{
				Manager->RemoveDesignerExtensionFactory(DesignerExtensionFactory.ToSharedRef());
			}
		}
		DesignerExtensionFactory.Reset();
	}
}

void FSplineWidgetEditorModule::OnModulesChanged(FName ModuleName, EModuleChangeReason Reason)
{
	if (ModuleName == TEXT("UMGEditor") && Reason == EModuleChangeReason::ModuleLoaded)
	{
		RegisterDesignerExtension();
	}
}

void FSplineWidgetEditorModule::RegisterDesignerExtension()
{
	if (DesignerExtensionFactory.IsValid())
	{
		return; // already registered
	}

	IUMGEditorModule* UMGEditor = FModuleManager::GetModulePtr<IUMGEditorModule>("UMGEditor");
	if (!UMGEditor)
	{
		return;
	}

	TSharedPtr<FDesignerExtensibilityManager> Manager = UMGEditor->GetDesignerExtensibilityManager();
	if (!Manager.IsValid())
	{
		return;
	}

	DesignerExtensionFactory = MakeShared<FSplineDesignerExtensionFactory>();
	Manager->AddDesignerExtensionFactory(DesignerExtensionFactory.ToSharedRef());
}

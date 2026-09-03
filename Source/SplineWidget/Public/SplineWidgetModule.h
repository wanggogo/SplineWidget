#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSplineWidgetModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

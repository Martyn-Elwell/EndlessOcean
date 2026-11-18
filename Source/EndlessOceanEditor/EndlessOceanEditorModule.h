#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * Editor module entry point. Registers Details panel customizations on startup and removes them on shutdown.
 *
 * Unity comparison: like an Editor folder script with [InitializeOnLoad], except Unreal loads it as its own DLL
 * that is never built into the shipped game.
 */
class FEndlessOceanEditorModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	/** Class names we registered layouts for, so we can unregister exactly those. */
	TArray<FName> RegisteredClassLayouts;
};

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SurvivorSettings.generated.h"

/**
 * Project-wide defaults for the survivor player. Shown in Project Settings > Game > Survivor and saved to
 * Config/DefaultGame.ini (defaultconfig), so they're shared through git.
 *
 * Unity comparison: like a config ScriptableObject in Resources, except the engine creates and loads it for you.
 * Read it anywhere with GetDefault<USurvivorSettings>().
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Survivor"))
class ENDLESSOCEAN_API USurvivorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	USurvivorSettings();

	/** Whether head-bob, sway and look lag start enabled. Players can toggle them at runtime (accessibility). */
	UPROPERTY(Config, EditAnywhere, Category="Camera")
	bool bCameraMotionEnabledByDefault = true;
};

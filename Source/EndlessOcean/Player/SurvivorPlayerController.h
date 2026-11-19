#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SurvivorPlayerController.generated.h"

class AActor;
class UHealthComponent;
class UInputMappingContext;
class USurvivorCameraModifier;
class USurvivorHUDWidget;
class UUnderwaterCameraModifier;

/**
 * Player controller for the survivor. It handles input mapping contexts, the camera-motion modifier
 * (and its toggle), and the HUD.
 *
 * The controller is the "player" and the character is the "body" it possesses. Things that belong to the
 * person playing (input setup, options like camera motion, UI) live here, so they survive the pawn dying or
 * being swapped.
 *
 * Make a Blueprint child (BP_SurvivorPlayerController) to assign the mapping contexts, modifier and HUD classes.
 */
UCLASS(Abstract, PrioritizeCategories=(Camera, Death, UI, Input))
class ENDLESSOCEAN_API ASurvivorPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ASurvivorPlayerController();

	/**
	 * Turns head-bob, sway and look lag on or off (fades over the modifier's alpha time).
	 * Exec makes this a console command too: type "SetCameraMotionEnabled 0" in the ~ console.
	 */
	UFUNCTION(BlueprintCallable, Exec, Category="Survivor|Camera")
	void SetCameraMotionEnabled(bool bEnabled);

	/** True if camera motion is currently on. */
	UFUNCTION(BlueprintPure, Category="Survivor|Camera")
	bool IsCameraMotionEnabled() const { return bCameraMotionEnabled; }

protected:

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** Input mapping contexts added for the local player. */
	UPROPERTY(EditAnywhere, Category="Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	/** Camera modifier for head-bob, sway and look lag. Use a Blueprint subclass to tune it. */
	UPROPERTY(EditAnywhere, Category="Camera")
	TSubclassOf<USurvivorCameraModifier> CameraModifierClass;

	/** Underwater look (tint, blur), always active. Use a Blueprint subclass to tune it. */
	UPROPERTY(EditAnywhere, Category="Camera")
	TSubclassOf<UUnderwaterCameraModifier> UnderwaterModifierClass;

	/** Fade to black on death. */
	UPROPERTY(EditAnywhere, Category="Death", meta=(ClampMin=0, ClampMax=10, Units="s"))
	float DeathFadeTime = 1.5f;

	/** Time from death until the level restarts. Placeholder until checkpoints exist. */
	UPROPERTY(EditAnywhere, Category="Death", meta=(ClampMin=0, ClampMax=30, Units="s"))
	float RestartDelay = 3.0f;

	/** HUD widget to create. */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<USurvivorHUDWidget> HUDWidgetClass;

private:

	/** Health of the possessed pawn, so we can unbind when it changes. */
	TWeakObjectPtr<UHealthComponent> PawnHealth;

	FTimerHandle RestartTimer;

	/** The possessed pawn died: stop input, fade out, then restart the level. */
	UFUNCTION()
	void HandlePawnDeath(UHealthComponent* HealthComponent, AActor* DamageCauser);

	/** The modifier instance owned by our PlayerCameraManager. */
	UPROPERTY(Transient)
	TObjectPtr<USurvivorCameraModifier> CameraModifier;

	UPROPERTY(Transient)
	TObjectPtr<USurvivorHUDWidget> HUDWidget;

	bool bCameraMotionEnabled = true;
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStaminaChangedSignature, float, NormalizedStamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStaminaExhaustedChangedSignature, bool, bExhausted);

/**
 * A reusable stamina pool. It doesn't know about sprinting; something tells it when to drain
 * (SetDraining) and it reports back through delegates.
 *
 * Exhaustion: hitting zero sets Exhausted, which stays set until stamina recovers past RecoverThreshold.
 * That stops the player sprinting in short stutters at near-zero stamina.
 *
 * No Tick: a fixed-rate timer runs only while stamina is draining or regenerating, and stops when full.
 */
UCLASS(ClassGroup=(EndlessOcean), PrioritizeCategories=(Stamina), meta=(BlueprintSpawnableComponent))
class ENDLESSOCEAN_API UStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UStaminaComponent();

	/** Start or stop draining stamina. */
	UFUNCTION(BlueprintCallable, Category="Stamina")
	void SetDraining(bool bDrain);

	/** Instantly add (or with a negative amount, remove) stamina, e.g. for an adrenaline pickup or a scare. */
	UFUNCTION(BlueprintCallable, Category="Stamina")
	void ModifyStamina(float Amount);

	/** Current stamina, from 0 to MaxStamina. */
	UFUNCTION(BlueprintPure, Category="Stamina")
	float GetStamina() const { return Stamina; }

	/** Current stamina from 0 to 1. */
	UFUNCTION(BlueprintPure, Category="Stamina")
	float GetStaminaNormalized() const { return MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f; }

	/** True from hitting zero until recovered past the threshold. */
	UFUNCTION(BlueprintPure, Category="Stamina")
	bool IsExhausted() const { return bExhausted; }

	/** Broadcast whenever the stamina value changes (normalized 0 to 1). */
	UPROPERTY(BlueprintAssignable, Category="Stamina")
	FStaminaChangedSignature OnStaminaChanged;

	/** Broadcast when becoming exhausted or recovering. */
	UPROPERTY(BlueprintAssignable, Category="Stamina")
	FStaminaExhaustedChangedSignature OnExhaustedChanged;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Full stamina. With DrainRate 1, this is the number of seconds you can sprint. */
	UPROPERTY(EditAnywhere, Category="Stamina", meta=(ClampMin=0.1, ClampMax=60, UIMax=15))
	float MaxStamina = 4.0f;

	/** Stamina lost per second while draining. */
	UPROPERTY(EditAnywhere, Category="Stamina", meta=(ClampMin=0, ClampMax=20))
	float DrainRate = 1.0f;

	/** Stamina regained per second while recovering. */
	UPROPERTY(EditAnywhere, Category="Stamina", meta=(ClampMin=0, ClampMax=20))
	float RegenRate = 0.7f;

	/** Wait after draining stops before regeneration begins. */
	UPROPERTY(EditAnywhere, Category="Stamina", meta=(ClampMin=0, ClampMax=10, Units="s"))
	float RegenDelay = 1.0f;

	/** Fraction of MaxStamina that must be regained before an exhausted character can sprint again. */
	UPROPERTY(EditAnywhere, Category="Stamina", meta=(ClampMin=0, ClampMax=1, UIMin=0, UIMax=1))
	float RecoverThreshold = 0.5f;

	/** How often stamina updates. Smaller is smoother for the HUD. */
	UPROPERTY(EditAnywhere, Category="Stamina", meta=(ClampMin=0.01, ClampMax=0.25, Units="s"))
	float UpdateInterval = 0.05f;

private:

	float Stamina = 0.0f;
	bool bDraining = false;
	bool bExhausted = false;

	/** Repeating update while draining or regenerating. */
	FTimerHandle UpdateTimer;

	/** One-shot delay before regeneration starts. */
	FTimerHandle RegenDelayTimer;

	/** Advances stamina by one UpdateInterval. */
	void UpdateStamina();

	/** Starts the repeating update timer if it isn't already running. */
	void StartUpdating();

	/** Applies a new value, handles exhaustion, broadcasts. */
	void SetStamina(float NewStamina);

	void SetExhausted(bool bNewExhausted);
};

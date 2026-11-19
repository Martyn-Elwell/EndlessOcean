#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DamageScreenEffectComponent.generated.h"

class UBreathComponent;
class UCameraComponent;
class UDamageType;
class UHealthComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

/**
 * Shows the player's condition on screen instead of a health bar.
 *
 *  - Drowning: a black vignette appears once the breath runs out (when drowning damage starts) and closes in as that
 *    damage takes health away. It opens again at the surface.
 *  - Anything else (falls, cuts, the monster): a red, blood-like vignette that pulses on each hit and fades away.
 *
 * By default the screen is clean whenever the player isn't taking damage, whatever their health. Optional extras for
 * tuning: a lasting blood level that scales with missing health, and a heartbeat throb at low health (both off).
 *
 * It drives two scalar parameters ("DrownAmount" and "BloodAmount", 0 to 1) on a post-process material
 * (M_PP_DamageVignette) added to the owner's camera. All the look lives in the material, and all the timing lives
 * here. An update timer runs only while an effect is visible.
 */
UCLASS(ClassGroup=(EndlessOcean), PrioritizeCategories=(Material, Blood, Heartbeat, Drowning), meta=(BlueprintSpawnableComponent))
class ENDLESSOCEAN_API UDamageScreenEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UDamageScreenEffectComponent();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Post-process material with "DrownAmount", "BloodAmount" (scalars) and "BloodColor" (vector) parameters. */
	UPROPERTY(EditAnywhere, Category="Material")
	TObjectPtr<UMaterialInterface> VignetteMaterial;

	/** How often the effect updates while visible. */
	UPROPERTY(EditAnywhere, Category="Material", meta=(ClampMin=0.01, ClampMax=0.1, Units="s"))
	float UpdateInterval = 0.02f;

	/** Colour of the blood vignette. */
	UPROPERTY(EditAnywhere, Category="Blood")
	FLinearColor BloodColor = FLinearColor(0.35f, 0.0f, 0.0f);

	/** Strength of the pulse from a hit that deals HitPulseFullDamage or more. Smaller hits pulse proportionally less. */
	UPROPERTY(EditAnywhere, Category="Blood", meta=(ClampMin=0, ClampMax=1))
	float HitPulseStrength = 0.85f;

	/** Damage that produces a full-strength pulse. */
	UPROPERTY(EditAnywhere, Category="Blood", meta=(ClampMin=1, ClampMax=200))
	float HitPulseFullDamage = 25.0f;

	/** Time for a hit pulse to fade away. */
	UPROPERTY(EditAnywhere, Category="Blood", meta=(ClampMin=0.1, ClampMax=10, Units="s"))
	float HitPulseFadeTime = 1.2f;

	/**
	 * Optional lasting blood vignette at 0 health; it scales down linearly to nothing at full health. 0 (default) shows
	 * blood only while being hit.
	 */
	UPROPERTY(EditAnywhere, Category="Blood", meta=(ClampMin=0, ClampMax=1))
	float PersistentBloodAtZeroHealth = 0.0f;

	/** Optional red throb at low health. Off by default, so the screen is clean between hits. */
	UPROPERTY(EditAnywhere, Category="Heartbeat")
	bool bHeartbeat = false;

	/** Below this fraction of health, the heartbeat throb plays. */
	UPROPERTY(EditAnywhere, Category="Heartbeat", meta=(EditCondition="bHeartbeat", EditConditionHides, ClampMin=0, ClampMax=1))
	float HeartbeatHealthThreshold = 0.35f;

	/** Heartbeats per second. */
	UPROPERTY(EditAnywhere, Category="Heartbeat", meta=(EditCondition="bHeartbeat", EditConditionHides, ClampMin=0.2, ClampMax=4, Units="Hz"))
	float HeartbeatRate = 1.1f;

	/** Strength of the heartbeat throb. */
	UPROPERTY(EditAnywhere, Category="Heartbeat", meta=(EditCondition="bHeartbeat", EditConditionHides, ClampMin=0, ClampMax=1))
	float HeartbeatStrength = 0.15f;

	/** How closed the black vignette is the moment drowning starts. Drowning damage closes it the rest of the way. */
	UPROPERTY(EditAnywhere, Category="Drowning", meta=(ClampMin=0, ClampMax=1))
	float DrownAmountAtStart = 0.3f;

	/** How quickly the black vignette follows its target (higher = snappier). */
	UPROPERTY(EditAnywhere, Category="Drowning", meta=(ClampMin=0.1, ClampMax=20))
	float DrownBlendSpeed = 3.0f;

private:

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> VignetteMID;

	TWeakObjectPtr<UHealthComponent> Health;
	TWeakObjectPtr<UBreathComponent> Breath;

	float BloodPulse = 0.0f;
	float CurrentDrown = 0.0f;
	float HeartbeatPhase = 0.0f;

	FTimerHandle UpdateTimer;

	UFUNCTION()
	void HandleHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta, const UDamageType* DamageType);

	UFUNCTION()
	void HandleBreathChanged(float NormalizedBreath);

	/** Starts the update timer if it isn't running. */
	void WakeUp();

	/** Advances pulses and blends, writes material parameters, and stops the timer once nothing is visible. */
	void UpdateEffect();

	float GetHealthNormalized() const;
	float GetTargetDrown() const;
	bool IsHeartbeatActive(float HealthNorm) const;
};

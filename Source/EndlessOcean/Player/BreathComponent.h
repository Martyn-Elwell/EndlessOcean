#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BreathComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBreathChangedSignature, float, NormalizedBreath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBreathStateChangedSignature, bool, bActive);

/**
 * Breath held underwater. Drains while the owner's eyes are below the surface, refills quickly above it, and once
 * empty deals drowning damage to the owner through the normal damage pipeline (so UHealthComponent handles it).
 *
 * Polls on a timer (10 times a second by default) instead of Tick. Breath doesn't need frame precision, and the
 * check is a cheap scan of the level's water volumes.
 */
UCLASS(ClassGroup=(EndlessOcean), PrioritizeCategories=(Breath, Drowning), meta=(BlueprintSpawnableComponent))
class ENDLESSOCEAN_API UBreathComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UBreathComponent();

	/** Remaining breath, in seconds of swimming. */
	UFUNCTION(BlueprintPure, Category="Breath")
	float GetBreath() const { return Breath; }

	/** Remaining breath from 0 to 1. */
	UFUNCTION(BlueprintPure, Category="Breath")
	float GetBreathNormalized() const { return MaxBreath > 0.0f ? Breath / MaxBreath : 0.0f; }

	/** True while the owner's eyes are underwater. */
	UFUNCTION(BlueprintPure, Category="Breath")
	bool IsSubmerged() const { return bSubmerged; }

	/** True while out of breath underwater (taking drowning damage). */
	UFUNCTION(BlueprintPure, Category="Breath")
	bool IsDrowning() const { return bDrowning; }

	/** Instantly restores breath, e.g. from an air canister. */
	UFUNCTION(BlueprintCallable, Category="Breath")
	void AddBreath(float Seconds);

	/** Breath changed (0 to 1). */
	UPROPERTY(BlueprintAssignable, Category="Breath")
	FBreathChangedSignature OnBreathChanged;

	/** Eyes went under (true) or came up (false). */
	UPROPERTY(BlueprintAssignable, Category="Breath")
	FBreathStateChangedSignature OnSubmergedChanged;

	/** Started (true) or stopped (false) drowning. */
	UPROPERTY(BlueprintAssignable, Category="Breath")
	FBreathStateChangedSignature OnDrowningChanged;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Seconds of breath when full. */
	UPROPERTY(EditAnywhere, Category="Breath", meta=(ClampMin=1, ClampMax=300, Units="s"))
	float MaxBreath = 40.0f;

	/** Breath lost per second underwater. Raise it for exertion, e.g. sprint-swimming. */
	UPROPERTY(EditAnywhere, Category="Breath", meta=(ClampMin=0, ClampMax=10))
	float DrainRate = 1.0f;

	/** Breath regained per second above water. */
	UPROPERTY(EditAnywhere, Category="Breath", meta=(ClampMin=0, ClampMax=100))
	float RefillRate = 8.0f;

	/** How far below the surface the eyes must be to count as submerged. Small values stop surface bobbing from flickering it. */
	UPROPERTY(EditAnywhere, Category="Breath", meta=(ClampMin=0, ClampMax=50, Units="cm"))
	float SubmergeDepth = 2.0f;

	/** How often breath updates. */
	UPROPERTY(EditAnywhere, Category="Breath", meta=(ClampMin=0.02, ClampMax=0.5, Units="s"))
	float UpdateInterval = 0.1f;

	/**
	 * Seconds from running out of breath to death, starting at full health. Each tick deals the matching share of the
	 * owner's max health (from its UHealthComponent), so this stays true if MaxHealth is retuned. Already-injured
	 * players die sooner.
	 */
	UPROPERTY(EditAnywhere, Category="Drowning", meta=(ClampMin=0.5, ClampMax=60, Units="s"))
	float DrowningTimeToDeath = 5.0f;

	/** Time between drowning damage ticks. Shorter ticks close the black vignette more smoothly. */
	UPROPERTY(EditAnywhere, Category="Drowning", meta=(ClampMin=0.1, ClampMax=5, Units="s"))
	float DrowningDamageInterval = 0.5f;

	/** Max health assumed when the owner has no UHealthComponent (it then just receives raw damage events). */
	UPROPERTY(EditAnywhere, Category="Drowning", AdvancedDisplay, meta=(ClampMin=1))
	float FallbackMaxHealth = 100.0f;

private:

	float Breath = 0.0f;
	bool bSubmerged = false;
	bool bDrowning = false;

	FTimerHandle UpdateTimer;
	FTimerHandle DrowningTimer;

	void UpdateBreath();
	void ApplyDrowningDamage();
	void SetBreath(float NewBreath);
	void SetDrowning(bool bNewDrowning);
};

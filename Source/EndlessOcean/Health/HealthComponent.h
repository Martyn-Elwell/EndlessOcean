#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class AController;
class UDamageType;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FHealthChangedSignature, UHealthComponent*, HealthComponent, float, NewHealth, float, Delta, const UDamageType*, DamageType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHealthDeathSignature, UHealthComponent*, HealthComponent, AActor*, DamageCauser);

/**
 * Health for anything that can be hurt: the player, the monster, a test dummy.
 *
 * It listens to the engine's own damage pipeline (AActor::OnTakeAnyDamage), so damage is dealt with the standard
 * UGameplayStatics::ApplyDamage / ApplyPointDamage and the attacker never needs to know this component exists.
 * Unity comparison: like every object implementing IDamageable, except the engine routes the call for you.
 *
 * Optional regeneration (timer-driven, no Tick) so the player's damage screen effects fade after a scare.
 */
UCLASS(ClassGroup=(EndlessOcean), PrioritizeCategories=(Health, Regeneration), meta=(BlueprintSpawnableComponent))
class ENDLESSOCEAN_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UHealthComponent();

	/** Current health, 0 to MaxHealth. */
	UFUNCTION(BlueprintPure, Category="Health")
	float GetHealth() const { return Health; }

	/** Maximum health. */
	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const { return MaxHealth; }

	/** Current health from 0 to 1. */
	UFUNCTION(BlueprintPure, Category="Health")
	float GetHealthNormalized() const { return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f; }

	/** True once health has reached 0. Dead components ignore further damage and healing. */
	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const { return bIsDead; }

	/** Restores health (clamped to MaxHealth). Does nothing when dead. */
	UFUNCTION(BlueprintCallable, Category="Health")
	void Heal(float Amount);

	/** Broadcast on every change (damage or healing). Delta is negative for damage. DamageType is null for healing. */
	UPROPERTY(BlueprintAssignable, Category="Health")
	FHealthChangedSignature OnHealthChanged;

	/** Broadcast once, when health reaches 0. */
	UPROPERTY(BlueprintAssignable, Category="Health")
	FHealthDeathSignature OnDeath;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Health at full. */
	UPROPERTY(EditAnywhere, Category="Health", meta=(ClampMin=1, ClampMax=10000, UIMax=500))
	float MaxHealth = 100.0f;

	/** Slowly restore health after not taking damage for a while. */
	UPROPERTY(EditAnywhere, Category="Regeneration")
	bool bRegenerate = true;

	/** Time without damage before regeneration starts. */
	UPROPERTY(EditAnywhere, Category="Regeneration", meta=(EditCondition="bRegenerate", ClampMin=0, ClampMax=60, Units="s"))
	float RegenDelay = 8.0f;

	/** Health restored per second while regenerating. */
	UPROPERTY(EditAnywhere, Category="Regeneration", meta=(EditCondition="bRegenerate", ClampMin=0, ClampMax=100))
	float RegenRate = 4.0f;

	/** Regeneration stops at this fraction of MaxHealth (1 = full). Lower values keep injuries lingering. */
	UPROPERTY(EditAnywhere, Category="Regeneration", meta=(EditCondition="bRegenerate", ClampMin=0, ClampMax=1))
	float RegenCap = 1.0f;

	/** How often regeneration applies. */
	UPROPERTY(EditAnywhere, Category="Regeneration", meta=(EditCondition="bRegenerate", ClampMin=0.05, ClampMax=1, Units="s"))
	float RegenInterval = 0.1f;

private:

	float Health = 0.0f;
	bool bIsDead = false;

	FTimerHandle RegenDelayTimer;
	FTimerHandle RegenTimer;

	/** Bound to the owner's OnTakeAnyDamage. */
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	/** Restarts the "no damage for a while" countdown. */
	void RestartRegenDelay();

	void StartRegenerating();
	void RegenerateTick();
};

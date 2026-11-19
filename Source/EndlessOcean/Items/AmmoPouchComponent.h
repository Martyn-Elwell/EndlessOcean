#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "AmmoPouchComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAmmoPouchChangedSignature, FGameplayTag, AmmoType, int32, NewCount);

/**
 * Spare ammunition, counted per ammo type (a gameplay tag such as Ammo.SPP1) rather than taking inventory slots.
 * Each type has a small cap so the player is always short: the SPP-1's default is 4 spare rounds.
 *
 * Weapons take from it while reloading; ammo pickups add to it.
 */
UCLASS(ClassGroup=(EndlessOcean), PrioritizeCategories=(Ammo), meta=(BlueprintSpawnableComponent))
class ENDLESSOCEAN_API UAmmoPouchComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UAmmoPouchComponent();

	/** Spare rounds of this type. */
	UFUNCTION(BlueprintPure, Category="Ammo")
	int32 GetCount(FGameplayTag AmmoType) const;

	/** Maximum spare rounds of this type. */
	UFUNCTION(BlueprintPure, Category="Ammo")
	int32 GetCapacity(FGameplayTag AmmoType) const;

	/** True if at least one more round of this type fits. */
	UFUNCTION(BlueprintPure, Category="Ammo")
	bool HasRoomFor(FGameplayTag AmmoType) const { return GetCount(AmmoType) < GetCapacity(AmmoType); }

	/** Adds up to Amount rounds (limited by the cap). Returns how many were actually added. */
	UFUNCTION(BlueprintCallable, Category="Ammo")
	int32 Add(FGameplayTag AmmoType, int32 Amount);

	/** Removes Amount rounds if that many are available. Returns false (removing nothing) otherwise. */
	UFUNCTION(BlueprintCallable, Category="Ammo")
	bool TryTake(FGameplayTag AmmoType, int32 Amount = 1);

	/** Broadcast whenever a count changes. */
	UPROPERTY(BlueprintAssignable, Category="Ammo")
	FAmmoPouchChangedSignature OnAmmoChanged;

protected:

	/** Per-type caps. Types not listed use DefaultCapacity. */
	UPROPERTY(EditAnywhere, Category="Ammo", meta=(Categories="Ammo"))
	TMap<FGameplayTag, int32> Capacities;

	/** Cap for ammo types not in Capacities. */
	UPROPERTY(EditAnywhere, Category="Ammo", meta=(ClampMin=0, ClampMax=99))
	int32 DefaultCapacity = 4;

	/** Spare rounds the player starts with. */
	UPROPERTY(EditAnywhere, Category="Ammo", meta=(Categories="Ammo"))
	TMap<FGameplayTag, int32> StartingAmmo;

	virtual void BeginPlay() override;

private:

	/** Current counts. Visible in the Details panel during play for debugging. */
	UPROPERTY(VisibleInstanceOnly, Category="Ammo")
	TMap<FGameplayTag, int32> Counts;
};

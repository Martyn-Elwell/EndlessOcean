#include "AmmoPouchComponent.h"

#include "ItemTags.h"

UAmmoPouchComponent::UAmmoPouchComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Scarce on purpose: four spare SPP-1 rounds, at most eight counting a fully loaded gun.
	Capacities.Add(EndlessOceanTags::Ammo_SPP1, 4);
}

void UAmmoPouchComponent::BeginPlay()
{
	Super::BeginPlay();

	for (const TPair<FGameplayTag, int32>& Start : StartingAmmo)
	{
		Add(Start.Key, Start.Value);
	}
}

int32 UAmmoPouchComponent::GetCount(FGameplayTag AmmoType) const
{
	const int32* Count = Counts.Find(AmmoType);
	return Count ? *Count : 0;
}

int32 UAmmoPouchComponent::GetCapacity(FGameplayTag AmmoType) const
{
	const int32* Capacity = Capacities.Find(AmmoType);
	return Capacity ? *Capacity : DefaultCapacity;
}

int32 UAmmoPouchComponent::Add(FGameplayTag AmmoType, int32 Amount)
{
	if (!AmmoType.IsValid() || Amount <= 0)
	{
		return 0;
	}

	const int32 Current = GetCount(AmmoType);
	const int32 Added = FMath::Min(Amount, GetCapacity(AmmoType) - Current);
	if (Added <= 0)
	{
		return 0;
	}

	Counts.Add(AmmoType, Current + Added);
	OnAmmoChanged.Broadcast(AmmoType, Current + Added);
	return Added;
}

bool UAmmoPouchComponent::TryTake(FGameplayTag AmmoType, int32 Amount)
{
	const int32 Current = GetCount(AmmoType);
	if (Amount <= 0 || Current < Amount)
	{
		return false;
	}

	Counts.Add(AmmoType, Current - Amount);
	OnAmmoChanged.Broadcast(AmmoType, Current - Amount);
	return true;
}

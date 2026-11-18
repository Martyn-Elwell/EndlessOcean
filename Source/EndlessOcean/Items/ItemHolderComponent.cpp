#include "ItemHolderComponent.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "HeldItem.h"
#include "InventoryComponent.h"
#include "ItemDefinition.h"

namespace ItemHolder
{
	/** Fallback release speeds for items without an ItemDefinition. Shares the ItemDefinition's own defaults. */
	static const UItemDefinition* GetDefaults(const AHeldItem* Item)
	{
		const UItemDefinition* Definition = Item ? Item->GetDefinition() : nullptr;
		return Definition ? Definition : GetDefault<UItemDefinition>();
	}

	/** How far to keep a released item from a wall it would otherwise be pushed into. */
	static constexpr float WallClearance = 8.0f;
}

UItemHolderComponent::UItemHolderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UItemHolderComponent::BeginPlay()
{
	Super::BeginPlay();

	Inventory = GetOwner()->FindComponentByClass<UInventoryComponent>();

	// Fall back to the root so we never attach to nothing if the owner forgot to set an anchor.
	if (!HandAnchor)
	{
		HandAnchor = GetOwner()->GetRootComponent();
	}
}

void UItemHolderComponent::SetHandAnchor(USceneComponent* InHandAnchor)
{
	HandAnchor = InHandAnchor;
}

bool UItemHolderComponent::PickUp(AHeldItem* Item)
{
	if (!Item || Item->GetItemLocation() != EHeldItemLocation::World || !Item->CanBePickedUp())
	{
		return false;
	}

	AHeldItem* Previous = HeldItem;

	// Put the current item back in the slot it came from if possible, otherwise any free slot, otherwise drop it.
	MakeHandEmpty(ActiveSlot);
	SetHeldItem(Item, INDEX_NONE);

	NotifyHeldItemChanged(Previous);
	return true;
}

void UItemHolderComponent::UseHeldItem()
{
	if (HeldItem)
	{
		HeldItem->UsePrimary();
	}
}

void UItemHolderComponent::DropHeldItem()
{
	AHeldItem* Previous = HeldItem;
	const UItemDefinition* Defaults = ItemHolder::GetDefaults(HeldItem);
	ReleaseHeldItem(Defaults->DropSpeed, 0.0f, 0.0f);
	NotifyHeldItemChanged(Previous);
}

void UItemHolderComponent::ThrowHeldItem()
{
	AHeldItem* Previous = HeldItem;
	const UItemDefinition* Defaults = ItemHolder::GetDefaults(HeldItem);
	ReleaseHeldItem(Defaults->ThrowSpeed, Defaults->ThrowUpSpeed, Defaults->ThrowSpin);
	NotifyHeldItemChanged(Previous);
}

bool UItemHolderComponent::StowHeldItem()
{
	if (!HeldItem || !Inventory || !HeldItem->CanBeStowed())
	{
		return false;
	}

	// Prefer the slot the item came from so it keeps its number.
	const int32 Slot = (ActiveSlot != INDEX_NONE && ActiveSlot < Inventory->GetCapacity() && !Inventory->GetItemInSlot(ActiveSlot))
		? ActiveSlot
		: Inventory->FindFirstEmptySlot();

	if (Slot == INDEX_NONE)
	{
		return false;
	}

	AHeldItem* Previous = ClearHand();
	Inventory->StoreInSlot(Previous, Slot);
	NotifyHeldItemChanged(Previous);
	return true;
}

bool UItemHolderComponent::EquipFromSlot(int32 SlotIndex)
{
	if (!Inventory)
	{
		return false;
	}

	AHeldItem* Item = Inventory->GetItemInSlot(SlotIndex);
	if (!Item)
	{
		// Pressing the key of the slot whose item is already in hand puts it away (toggle).
		return SlotIndex == ActiveSlot && StowHeldItem();
	}

	AHeldItem* Previous = HeldItem;

	Inventory->RemoveFromSlot(SlotIndex);
	MakeHandEmpty(SlotIndex); // the slot we just emptied is the preferred spot for the old item: a clean swap
	SetHeldItem(Item, SlotIndex);

	NotifyHeldItemChanged(Previous);
	return true;
}

void UItemHolderComponent::CycleInventory(int32 Direction)
{
	if (!Inventory || Direction == 0)
	{
		return;
	}

	const int32 Capacity = Inventory->GetCapacity();
	const int32 Step = Direction > 0 ? 1 : -1;
	const int32 Start = ActiveSlot != INDEX_NONE ? ActiveSlot : (Step > 0 ? -1 : Capacity);

	for (int32 Offset = 1; Offset <= Capacity; ++Offset)
	{
		// Wrap around in both directions (C++ % can return negatives, so add Capacity back).
		const int32 Index = ((Start + Offset * Step) % Capacity + Capacity) % Capacity;
		if (Inventory->GetItemInSlot(Index))
		{
			EquipFromSlot(Index);
			return;
		}
	}
}

void UItemHolderComponent::SetHeldItem(AHeldItem* Item, int32 FromSlot)
{
	HeldItem = Item;
	ActiveSlot = FromSlot;

	if (Item)
	{
		Item->SetHeld(this, HandAnchor);
		Item->OnDestroyed.AddUniqueDynamic(this, &UItemHolderComponent::HandleHeldItemDestroyed);
	}
}

AHeldItem* UItemHolderComponent::ClearHand()
{
	AHeldItem* Previous = HeldItem;
	if (Previous)
	{
		Previous->OnDestroyed.RemoveDynamic(this, &UItemHolderComponent::HandleHeldItemDestroyed);
	}

	HeldItem = nullptr;
	ActiveSlot = INDEX_NONE;
	return Previous;
}

void UItemHolderComponent::ReleaseHeldItem(float ForwardSpeed, float UpSpeed, float SpinDegrees)
{
	AHeldItem* Item = ClearHand();
	if (!Item)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetViewPoint(ViewLocation, ViewRotation);

	// If the hand is poking through a wall, pull the item back to our side first so it doesn't drop into the next room.
	if (UWorld* World = GetWorld())
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(HeldItemRelease), false, GetOwner());
		Params.AddIgnoredActor(Item);

		FHitResult Hit;
		const FVector ItemLocation = Item->GetActorLocation();
		if (World->SweepSingleByChannel(Hit, ViewLocation, ItemLocation, FQuat::Identity, ECC_Visibility,
			FCollisionShape::MakeSphere(ItemHolder::WallClearance), Params))
		{
			Item->SetActorLocation(Hit.Location);
		}
	}

	FVector Velocity = ViewRotation.Vector() * ForwardSpeed + FVector::UpVector * UpSpeed;
	if (const AActor* Owner = GetOwner())
	{
		// Keep only a little of our own movement: enough that a throw while walking doesn't look like it hits an
		// invisible wall, without sprint-throws flying across the room.
		Velocity += Owner->GetVelocity() * ItemHolder::GetDefaults(Item)->InheritedVelocityScale;
	}

	// Tumble end over end around the camera's right axis.
	const FVector Spin = FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::Y) * SpinDegrees;

	Item->ReleaseToWorld(Velocity, Spin);
}

void UItemHolderComponent::MakeHandEmpty(int32 PreferredSlot)
{
	if (!HeldItem)
	{
		return;
	}

	if (Inventory && HeldItem->CanBeStowed())
	{
		const bool bPreferredFree = PreferredSlot >= 0 && PreferredSlot < Inventory->GetCapacity() && !Inventory->GetItemInSlot(PreferredSlot);
		const int32 Slot = bPreferredFree ? PreferredSlot : Inventory->FindFirstEmptySlot();
		if (Slot != INDEX_NONE)
		{
			AHeldItem* Item = ClearHand();
			Inventory->StoreInSlot(Item, Slot);
			return;
		}
	}

	// Can't be stowed (lit flare) or no room: drop it.
	const UItemDefinition* Defaults = ItemHolder::GetDefaults(HeldItem);
	ReleaseHeldItem(Defaults->DropSpeed, 0.0f, 0.0f);
}

void UItemHolderComponent::GetViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->GetController())
	{
		Pawn->GetController()->GetPlayerViewPoint(OutLocation, OutRotation);
	}
	else if (const AActor* Owner = GetOwner())
	{
		Owner->GetActorEyesViewPoint(OutLocation, OutRotation);
	}
}

void UItemHolderComponent::NotifyHeldItemChanged(AHeldItem* Previous)
{
	if (Previous != HeldItem)
	{
		OnHeldItemChanged.Broadcast(HeldItem, Previous);
	}
}

void UItemHolderComponent::HandleHeldItemDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor == HeldItem)
	{
		AHeldItem* Previous = ClearHand();
		NotifyHeldItemChanged(Previous);
	}
}

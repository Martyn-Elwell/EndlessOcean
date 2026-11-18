#include "InventoryComponent.h"

#include "GameFramework/Actor.h"
#include "HeldItem.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Ask for InitializeComponent so the slots exist before any BeginPlay can try to use them.
	bWantsInitializeComponent = true;
}

void UInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();
	Slots.SetNum(Capacity);
}

AHeldItem* UInventoryComponent::GetItemInSlot(int32 SlotIndex) const
{
	return Slots.IsValidIndex(SlotIndex) ? Slots[SlotIndex].Get() : nullptr;
}

TArray<AHeldItem*> UInventoryComponent::GetSlots() const
{
	// Blueprint can't take TArray<TObjectPtr<T>> by value from a function, so convert to raw pointers.
	TArray<AHeldItem*> Result;
	Result.Reserve(Slots.Num());
	for (const TObjectPtr<AHeldItem>& Item : Slots)
	{
		Result.Add(Item.Get());
	}
	return Result;
}

int32 UInventoryComponent::FindFirstEmptySlot() const
{
	return Slots.IndexOfByPredicate([](const TObjectPtr<AHeldItem>& Item) { return Item == nullptr; });
}

bool UInventoryComponent::StoreInSlot(AHeldItem* Item, int32 SlotIndex)
{
	if (!Item || !Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex] || !Item->CanBeStowed())
	{
		return false;
	}

	Slots[SlotIndex] = Item;
	Item->SetStowed(GetOwner() ? GetOwner()->GetRootComponent() : nullptr);
	OnInventoryChanged.Broadcast();
	return true;
}

int32 UInventoryComponent::TryAdd(AHeldItem* Item)
{
	const int32 SlotIndex = FindFirstEmptySlot();
	return StoreInSlot(Item, SlotIndex) ? SlotIndex : INDEX_NONE;
}

AHeldItem* UInventoryComponent::RemoveFromSlot(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex])
	{
		return nullptr;
	}

	AHeldItem* Item = Slots[SlotIndex];
	Slots[SlotIndex] = nullptr;
	OnInventoryChanged.Broadcast();
	return Item;
}

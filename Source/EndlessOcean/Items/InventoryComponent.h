#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class AHeldItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryChangedSignature);

/**
 * A small, fixed number of slots for stowed items. Slots can be empty (nullptr), so an item keeps its slot
 * number when you swap things around, which keeps the HUD and the 1-2-3 keys predictable.
 *
 * This is storage only. The hand lives in UItemHolderComponent. Keeping them separate lets other systems
 * (a repair point asking "do you have a wrench?", the HUD) query the inventory without knowing about hands.
 */
UCLASS(ClassGroup=(EndlessOcean), PrioritizeCategories=(Inventory), meta=(BlueprintSpawnableComponent))
class ENDLESSOCEAN_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UInventoryComponent();

	/** Number of slots. */
	UFUNCTION(BlueprintPure, Category="Inventory")
	int32 GetCapacity() const { return Slots.Num(); }

	/** The item in a slot, or nullptr if the slot is empty or out of range. */
	UFUNCTION(BlueprintPure, Category="Inventory")
	AHeldItem* GetItemInSlot(int32 SlotIndex) const;

	/** All slots, including empty ones (nullptr). */
	UFUNCTION(BlueprintPure, Category="Inventory")
	TArray<AHeldItem*> GetSlots() const;

	/** Index of the first empty slot, or INDEX_NONE if full. */
	UFUNCTION(BlueprintPure, Category="Inventory")
	int32 FindFirstEmptySlot() const;

	/** True if at least one slot is empty. */
	UFUNCTION(BlueprintPure, Category="Inventory")
	bool HasRoom() const { return FindFirstEmptySlot() != INDEX_NONE; }

	/** Stows the item in a specific empty slot. Returns false if the slot is taken or the item can't be stowed. */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool StoreInSlot(AHeldItem* Item, int32 SlotIndex);

	/** Stows the item in the first empty slot. Returns the slot used, or INDEX_NONE on failure. */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	int32 TryAdd(AHeldItem* Item);

	/** Removes and returns the item in a slot (the slot becomes empty). The caller decides what to do with it next. */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	AHeldItem* RemoveFromSlot(int32 SlotIndex);

	/** Broadcast whenever any slot changes. */
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FInventoryChangedSignature OnInventoryChanged;

protected:

	virtual void InitializeComponent() override;

	/** How many items can be stowed. */
	UPROPERTY(EditAnywhere, Category="Inventory", meta=(ClampMin=1, ClampMax=8))
	int32 Capacity = 3;

private:

	/**
	 * Fixed-size slot array. UPROPERTY so the garbage collector knows we reference these actors
	 * (an un-marked raw pointer could be left dangling). VisibleInstanceOnly shows it in the Details panel during play.
	 */
	UPROPERTY(VisibleInstanceOnly, Category="Inventory")
	TArray<TObjectPtr<AHeldItem>> Slots;
};

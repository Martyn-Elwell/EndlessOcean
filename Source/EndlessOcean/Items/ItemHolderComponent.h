#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemHolderComponent.generated.h"

class AHeldItem;
class UInventoryComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHeldItemChangedSignature, AHeldItem*, NewItem, AHeldItem*, OldItem);

/**
 * The character's hand: holds at most one AHeldItem and handles use, drop, throw, and swapping with the inventory.
 *
 * Rules:
 *  - Picking something up while holding an item stows the held item if it can be stowed and there's room,
 *    otherwise drops it (a lit flare is always dropped).
 *  - Equipping from a slot swaps the held item into that slot (or drops it if it can't be stowed).
 *
 * Works with an optional UInventoryComponent on the same actor. Without one, held items are always dropped.
 */
UCLASS(ClassGroup=(EndlessOcean), meta=(BlueprintSpawnableComponent))
class ENDLESSOCEAN_API UItemHolderComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UItemHolderComponent();

	/** Sets the component that held items attach to (usually a scene component under the camera). */
	UFUNCTION(BlueprintCallable, Category="Item Holder")
	void SetHandAnchor(USceneComponent* InHandAnchor);

	/** The item in hand, or nullptr. */
	UFUNCTION(BlueprintPure, Category="Item Holder")
	AHeldItem* GetHeldItem() const { return HeldItem; }

	/** The inventory slot the current item came from, or INDEX_NONE. Used to highlight the slot in the HUD. */
	UFUNCTION(BlueprintPure, Category="Item Holder")
	int32 GetActiveSlot() const { return ActiveSlot; }

	/** Takes an item from the world into the hand. Returns false if it can't be picked up. */
	UFUNCTION(BlueprintCallable, Category="Item Holder")
	bool PickUp(AHeldItem* Item);

	/** Uses the held item (e.g. lights a flare). */
	UFUNCTION(BlueprintCallable, Category="Item Holder")
	void UseHeldItem();

	/** Gently drops the held item in front of the character. */
	UFUNCTION(BlueprintCallable, Category="Item Holder")
	void DropHeldItem();

	/** Throws the held item along the view direction. */
	UFUNCTION(BlueprintCallable, Category="Item Holder")
	void ThrowHeldItem();

	/** Puts the held item away, leaving the hand empty. Returns false if it can't be stowed (full, or a lit flare). */
	UFUNCTION(BlueprintCallable, Category="Item Holder")
	bool StowHeldItem();

	/** Takes the item in a slot into the hand, swapping the current item into that slot. */
	UFUNCTION(BlueprintCallable, Category="Item Holder")
	bool EquipFromSlot(int32 SlotIndex);

	/** Equips the next (Direction > 0) or previous (Direction < 0) occupied slot. */
	UFUNCTION(BlueprintCallable, Category="Item Holder")
	void CycleInventory(int32 Direction);

	/** Broadcast whenever the item in hand changes (either value may be nullptr). */
	UPROPERTY(BlueprintAssignable, Category="Item Holder")
	FHeldItemChangedSignature OnHeldItemChanged;

protected:

	virtual void BeginPlay() override;

private:

	/** What held items attach to. */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> HandAnchor;

	/** Optional inventory on the same actor. */
	UPROPERTY(Transient)
	TObjectPtr<UInventoryComponent> Inventory;

	/** The item in hand. */
	UPROPERTY(Transient)
	TObjectPtr<AHeldItem> HeldItem;

	/** Slot the held item was taken from, for HUD highlighting. */
	int32 ActiveSlot = INDEX_NONE;

	/** Puts Item in the hand and broadcasts. Item must already be out of the world/inventory bookkeeping. */
	void SetHeldItem(AHeldItem* Item, int32 FromSlot);

	/** Clears the hand without moving the item, returns what was held. */
	AHeldItem* ClearHand();

	/** Releases the held item into the world with the given speeds. */
	void ReleaseHeldItem(float ForwardSpeed, float UpSpeed, float SpinDegrees);

	/**
	 * Gets the held item out of the hand to make room: stow it in PreferredSlot (or the first free slot),
	 * or drop it if it can't be stowed.
	 */
	void MakeHandEmpty(int32 PreferredSlot);

	/** The owner's view point (camera), used for throw direction and wall checks. */
	void GetViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

	/** Broadcasts OnHeldItemChanged if the held item differs from Previous. */
	void NotifyHeldItemChanged(AHeldItem* Previous);

	/** Bound to the held item's OnDestroyed, so the hand doesn't keep a dead item. */
	UFUNCTION()
	void HandleHeldItemDestroyed(AActor* DestroyedActor);
};

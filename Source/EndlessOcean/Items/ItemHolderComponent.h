#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemHolderComponent.generated.h"

class AHeldItem;
class UCameraComponent;
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
 *
 * The hand also poses the held item every frame (procedural sway, lowering it while swimming, the item's own aim or
 * swing pose) and applies its aim FOV to the owner's camera. That's the one place Tick is used, and only while
 * holding something.
 */
UCLASS(ClassGroup=(EndlessOcean), PrioritizeCategories=(ItemHolder), meta=(BlueprintSpawnableComponent))
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

	/** Uses the held item (e.g. lights a flare). Ignored if the item can't be used right now (e.g. swimming). */
	UFUNCTION(BlueprintCallable, Category="Item Holder")
	void UseHeldItem();

	/** Starts the held item's alternate action (right mouse: aim, or throw by default). */
	UFUNCTION(BlueprintCallable, Category="Item Holder")
	void StartAltAction();

	/** Ends the held item's alternate action (right mouse released). */
	UFUNCTION(BlueprintCallable, Category="Item Holder")
	void StopAltAction();

	/** Asks the held item to reload. */
	UFUNCTION(BlueprintCallable, Category="Item Holder")
	void ReloadHeldItem();

	/** True if the held item can be used right now (not lowered for swimming). */
	UFUNCTION(BlueprintPure, Category="Item Holder")
	bool CanUseHeldItem() const;

	/** The hand's current procedural sway rotation. Weapons add it to their aim so shots go where the item points. */
	UFUNCTION(BlueprintPure, Category="Item Holder")
	FRotator GetSwayRotation() const { return CurrentSwayRotation; }

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

	/**
	 * Poses the held item every frame: base offset + lowered-for-swimming offset + procedural sway + the item's own
	 * additive pose. Also applies the item's aim FOV. Only enabled while something is in the hand.
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Owner speed at which movement sway reaches its full MovementSwayMultiplier. */
	UPROPERTY(EditAnywhere, Category="ItemHolder", meta=(ClampMin=1, ClampMax=1000, Units="cm/s"))
	float MovementSwayReferenceSpeed = 250.0f;

	/** How quickly items lower and raise when entering or leaving water. */
	UPROPERTY(EditAnywhere, Category="ItemHolder", meta=(ClampMin=0.1, ClampMax=30))
	float LowerBlendSpeed = 6.0f;

	/** How quickly the camera FOV follows an aiming item. */
	UPROPERTY(EditAnywhere, Category="ItemHolder", meta=(ClampMin=0.1, ClampMax=30))
	float FOVBlendSpeed = 12.0f;

private:

	/** Owner's camera, for aim FOV. */
	TWeakObjectPtr<UCameraComponent> OwnerCamera;

	/** The camera's FOV before any aiming, captured on BeginPlay. */
	float BaseFOV = 90.0f;

	float SwayTime = 0.0f;
	float LoweredAlpha = 0.0f;
	bool bWasSwimming = false;
	FRotator CurrentSwayRotation = FRotator::ZeroRotator;

	/** True if the owner is a character currently swimming. */
	bool IsOwnerSwimming() const;

	/** Blends the owner's camera FOV towards the held item's aim FOV (or back to BaseFOV). */
	void UpdateCameraFOV(float DeltaTime);

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

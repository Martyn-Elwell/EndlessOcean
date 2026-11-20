#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "HeldItem.generated.h"

class UItemDefinition;
class UItemHolderComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;

/** Where an item currently is. */
UENUM(BlueprintType)
enum class EHeldItemLocation : uint8
{
	/** Lying in the level, physics on, can be picked up. */
	World,
	/** In a character's hand. */
	Held,
	/** Hidden in a character's inventory. */
	Stowed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHeldItemLocationChangedSignature, AHeldItem*, Item, EHeldItemLocation, NewLocation);

/**
 * Base class for anything the player can pick up, hold, stow, drop and throw.
 *
 * The item is a real actor the whole time, even while stowed (hidden and with collision off). That way per-item
 * state, such as a half-burnt flare, survives being put away and taken out again. Subclasses add behaviour by
 * overriding UsePrimary and the Can* checks.
 *
 * The static mesh is the root component so physics moves the whole actor when it's dropped.
 *
 * HideCategories is inherited by every item subclass, removing engine sections that never matter for a held item.
 */
UCLASS(Abstract, Blueprintable,
	PrioritizeCategories=(Item),
	HideCategories=(Replication, Networking, HLOD, Cooking, LOD, DataLayers, WorldPartition, Input))
class ENDLESSOCEAN_API AHeldItem : public AActor, public IInteractable
{
	GENERATED_BODY()

public:

	AHeldItem();

	/** Shared data for this item type (name, hold offset, throw speeds). */
	UFUNCTION(BlueprintPure, Category="Item")
	UItemDefinition* GetDefinition() const { return Definition; }

	/** Player-facing name from the definition (falls back to the actor name). */
	UFUNCTION(BlueprintPure, Category="Item")
	FText GetDisplayName() const;

	/** Where the item currently is. */
	UFUNCTION(BlueprintPure, Category="Item")
	EHeldItemLocation GetItemLocation() const { return ItemLocation; }

	/** The holder currently holding this item, or nullptr if it isn't in a hand. */
	UFUNCTION(BlueprintPure, Category="Item")
	UItemHolderComponent* GetHolder() const { return Holder.Get(); }

	/** The main "use" action (left mouse). Lights a flare, and so on. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item")
	void UsePrimary();

	/**
	 * Right mouse pressed. By default this throws the item; items with a better use override it
	 * (the pistol aims, the wrench does nothing so it isn't thrown by accident).
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item")
	void AltActionStarted();

	/** Right mouse released (ends aiming, for example). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item")
	void AltActionCompleted();

	/** Reload key pressed. Does nothing by default. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item")
	void ReloadPressed();

	/**
	 * Stop whatever the item is doing: aiming, reloading, mid-swing. Called when it leaves the hand, and when the
	 * holder starts swimming with an item that can't be used underwater.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item")
	void CancelActions();

	/** Extra pose on top of the held offset (aim-down-sights, swing arc, recoil kick), relative to the hand anchor. */
	virtual FTransform GetAdditivePose() const { return FTransform::Identity; }

	/** Multiplier for the hand's procedural sway. Aiming lowers it. */
	virtual float GetSwayScale() const { return 1.0f; }

	/**
	 * Field of view this item wants while aiming, and how far towards it to blend (0 = normal view).
	 * Returns false if the item never changes the view.
	 */
	virtual bool GetAimView(float& OutFOV, float& OutAlpha) const { return false; }

	/** True if the definition allows using this item while swimming. */
	UFUNCTION(BlueprintPure, Category="Item")
	bool IsUsableWhileSwimming() const;

	/** True if the item can currently be picked up from the world. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item")
	bool CanBePickedUp() const;

	/** True if the item can go into an inventory. False items get dropped instead (e.g. a lit flare). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item")
	bool CanBeStowed() const;

	/** Called by UItemHolderComponent: attach to the hand anchor, physics and collision off. */
	void SetHeld(UItemHolderComponent* InHolder, USceneComponent* HandAnchor);

	/** Called by UInventoryComponent: hide and attach to the owner, physics and collision off. */
	void SetStowed(USceneComponent* AttachParent);

	/** Detach and drop back into the world with the given velocities. */
	void ReleaseToWorld(const FVector& LinearVelocity, const FVector& AngularVelocityDegrees);

	/** Broadcast whenever the item moves between World, Held and Stowed. */
	UPROPERTY(BlueprintAssignable, Category="Item")
	FHeldItemLocationChangedSignature OnItemLocationChanged;

protected:

	// IInteractable
	virtual bool CanInteract_Implementation(AActor* InteractInstigator) const override;
	virtual void Interact_Implementation(AActor* InteractInstigator) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	virtual void UsePrimary_Implementation() {}
	virtual void AltActionStarted_Implementation();
	virtual void AltActionCompleted_Implementation() {}
	virtual void ReloadPressed_Implementation() {}
	virtual void CancelActions_Implementation() {}
	virtual bool CanBePickedUp_Implementation() const { return true; }
	virtual bool CanBeStowed_Implementation() const { return true; }

	/** Hook for subclasses after the location changes (call Super). */
	virtual void OnItemLocationUpdated(EHeldItemLocation NewLocation);

	/** Visible mesh and physics body. Placeholder primitives for now. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Shared data for this item type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	TObjectPtr<UItemDefinition> Definition;

private:

	/** Current location state. */
	EHeldItemLocation ItemLocation = EHeldItemLocation::World;

	/** The hand holding us. Weak because the holder owns the relationship, not us. */
	TWeakObjectPtr<UItemHolderComponent> Holder;

	/** A part whose collision was switched off while carried, and the setting to give back when released. */
	struct FSuspendedCollision
	{
		TWeakObjectPtr<UPrimitiveComponent> Component;
		ECollisionEnabled::Type Collision;
	};

	/**
	 * Parts (anything but the root Mesh) that had collision when the item was picked up. Carried items must not
	 * collide with anything, or a part in front of the camera would block the holder's own capsule and traces.
	 */
	TArray<FSuspendedCollision> SuspendedCollision;

	/** Switches between "in the world" physics and "carried" (no physics, no collision on any part). */
	void SetPhysicalInWorld(bool bInWorld);

	/** Updates the state and broadcasts. */
	void SetItemLocation(EHeldItemLocation NewLocation);
};

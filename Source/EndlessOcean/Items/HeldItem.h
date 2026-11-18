#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "HeldItem.generated.h"

class UItemDefinition;
class UItemHolderComponent;
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
 */
UCLASS(Abstract, Blueprintable)
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

	/** Switches between "in the world" physics and "carried" (no physics, no collision). */
	void SetPhysicalInWorld(bool bInWorld);

	/** Updates the state and broadcasts. */
	void SetItemLocation(EHeldItemLocation NewLocation);
};

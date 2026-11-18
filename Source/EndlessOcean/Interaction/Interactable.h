#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

/**
 * Reflection stub for IInteractable. Unreal needs a UObject-side class (UInteractable) so the interface
 * can be used from Blueprints and checked with Implements<UInteractable>(). Never use this type directly.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything the player can look at and press Interact on: items to pick up, doors, valves, repair points.
 *
 * Functions are BlueprintNativeEvents, so they can be implemented in C++ (override the *_Implementation version)
 * or in a Blueprint (Class Settings > Interfaces). Because a Blueprint might be the implementer, always call them
 * through the generated static helpers, e.g. IInteractable::Execute_Interact(Actor, Instigator), not Actor->Interact().
 */
class ENDLESSOCEAN_API IInteractable
{
	GENERATED_BODY()

public:

	/** Returns true if InteractInstigator is currently allowed to interact with this object. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	bool CanInteract(AActor* InteractInstigator) const;

	/** Performs the interaction. Only called after CanInteract returned true. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void Interact(AActor* InteractInstigator);

	/** Short, player-facing text describing the interaction, e.g. "Pick up Flare". */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	FText GetInteractionPrompt() const;

protected:

	/** Default: interactable by anyone. */
	virtual bool CanInteract_Implementation(AActor* InteractInstigator) const { return true; }

	/** Default: does nothing. */
	virtual void Interact_Implementation(AActor* InteractInstigator) {}

	/** Default: generic prompt. */
	virtual FText GetInteractionPrompt_Implementation() const { return NSLOCTEXT("Interaction", "DefaultPrompt", "Interact"); }
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interactable.h"
#include "AmmoPickup.generated.h"

class UStaticMeshComponent;

/**
 * Loose ammunition lying in the level (one SPP-1 round by default). Interacting moves as many rounds as fit into the
 * player's UAmmoPouchComponent. Whatever doesn't fit stays behind, and the prompt says the pouch is full.
 *
 * It isn't an AHeldItem: bullets are never held in the hand, only counted.
 */
UCLASS(Blueprintable, PrioritizeCategories=(Ammo), HideCategories=(Replication, Networking, HLOD, Cooking, LOD, DataLayers, WorldPartition, Input))
class ENDLESSOCEAN_API AAmmoPickup : public AActor, public IInteractable
{
	GENERATED_BODY()

public:

	AAmmoPickup();

protected:

	// IInteractable
	virtual bool CanInteract_Implementation(AActor* InteractInstigator) const override;
	virtual void Interact_Implementation(AActor* InteractInstigator) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	/** Physical placeholder mesh; it simulates physics so it can be knocked around. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Which ammunition this is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(Categories="Ammo"))
	FGameplayTag AmmoType;

	/** Rounds in this pickup. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin=1, ClampMax=99))
	int32 Count = 1;

	/** Name used in the prompt, e.g. "SPP-1 round". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo")
	FText DisplayName;
};

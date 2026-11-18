#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDefinition.generated.h"

/** How many hands an item needs. Stored now; used by the upcoming visual pass for hand poses. */
UENUM(BlueprintType)
enum class EItemHandedness : uint8
{
	OneHanded	UMETA(DisplayName="One Handed"),
	TwoHanded	UMETA(DisplayName="Two Handed")
};

/**
 * Shared, per-item-type data (like a Unity ScriptableObject). Every flare in the level points at the same DA_Flare,
 * so tuning it in one place updates them all. Per-instance state (is this flare lit?) lives on the actor instead.
 *
 * Create in the Content Browser: right-click > Miscellaneous > Data Asset > ItemDefinition.
 */
UCLASS(BlueprintType, Const)
class ENDLESSOCEAN_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** Player-facing name, used in prompts ("Pick up {Name}") and the inventory HUD. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FText DisplayName;

	/** Whether the item needs one or both hands. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	EItemHandedness Handedness = EItemHandedness::OneHanded;

	/** Location of the item relative to the character's hand anchor while held. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Held", meta=(Units="cm"))
	FVector HeldLocationOffset = FVector::ZeroVector;

	/** Rotation of the item relative to the character's hand anchor while held. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Held")
	FRotator HeldRotationOffset = FRotator::ZeroRotator;

	/** Launch speed along the view direction when thrown. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Release", meta=(ClampMin=0, ClampMax=3000, UIMax=1500, Units="cm/s"))
	float ThrowSpeed = 900.0f;

	/** Extra upward speed added to a throw so it arcs instead of flying flat. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Release", meta=(ClampMin=0, ClampMax=1000, Units="cm/s"))
	float ThrowUpSpeed = 150.0f;

	/** Tumble applied to a thrown item. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Release", meta=(ClampMin=0, ClampMax=1440, Units="deg/s"))
	float ThrowSpin = 360.0f;

	/** Speed along the view direction when simply dropped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Release", meta=(ClampMin=0, ClampMax=500, Units="cm/s"))
	float DropSpeed = 100.0f;

	/** Groups all item definitions under one Asset Manager type so they can be found and loaded by ID later. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};

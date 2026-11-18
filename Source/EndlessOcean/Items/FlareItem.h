#pragma once

#include "CoreMinimal.h"
#include "HeldItem.h"
#include "FlareItem.generated.h"

class UAudioComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class USoundBase;
class UStaticMeshComponent;

/** Lifecycle of a flare. It only ever moves forward: Unlit, then Burning, then BurntOut. */
UENUM(BlueprintType)
enum class EFlareState : uint8
{
	Unlit,
	Burning,
	BurntOut	UMETA(DisplayName="Burnt Out")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlareStateChangedSignature, AFlareItem*, Flare, EFlareState, NewState);

/**
 * A single-use road flare: the player's main light source.
 *
 * Use (left mouse) lights it. It burns for BurnDuration seconds, flickering and dimming at the end, then goes out
 * for good. A lit flare can be held or thrown (it keeps burning on the floor and can be picked up again), but
 * can't be stowed, so picking up something else drops it.
 *
 * Placeholder visuals: a grey cylinder with a red cap. Make BP_Flare from this class to tune it and add sounds.
 *
 * Details panel layout: PrioritizeCategories puts our categories directly under Transform (in this order), and
 * HideCategories removes engine sections that never matter for a held item. Child Blueprints inherit both.
 */
UCLASS(Blueprintable,
	PrioritizeCategories=(Flare, Item),
	HideCategories=(Replication, Networking, HLOD, Cooking, LOD, DataLayers, WorldPartition, Input))
class ENDLESSOCEAN_API AFlareItem : public AHeldItem
{
	GENERATED_BODY()

public:

	AFlareItem();

	/** Lights the flare. Does nothing if it's already lit or burnt out. */
	UFUNCTION(BlueprintCallable, Category="Flare")
	void Ignite();

	/** Current state. */
	UFUNCTION(BlueprintPure, Category="Flare")
	EFlareState GetFlareState() const { return FlareState; }

	/** True while lit. */
	UFUNCTION(BlueprintPure, Category="Flare")
	bool IsBurning() const { return FlareState == EFlareState::Burning; }

	/** Seconds of burn left (the full duration if unlit, 0 if burnt out). */
	UFUNCTION(BlueprintPure, Category="Flare")
	float GetBurnTimeRemaining() const;

	/** Burn time left from 0 to 1. */
	UFUNCTION(BlueprintPure, Category="Flare")
	float GetBurnTimeRemainingNormalized() const;

	/** Broadcast when the flare is lit and when it burns out. Future AI can listen for this (the monster noticing light). */
	UPROPERTY(BlueprintAssignable, Category="Flare")
	FFlareStateChangedSignature OnFlareStateChanged;

protected:

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// AHeldItem
	virtual void UsePrimary_Implementation() override;
	virtual bool CanBePickedUp_Implementation() const override;
	virtual bool CanBeStowed_Implementation() const override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual void OnItemLocationUpdated(EHeldItemLocation NewLocation) override;

	/** Editor button: shows or hides the light so you can tune it in the level without playing. */
	UFUNCTION(CallInEditor, Category="Flare")
	void TogglePreviewLight();

	// --- Components ---

	/** Red striker cap at the top of the flare. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> CapMesh;

	/** The flare's light. Off until lit. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPointLightComponent> Light;

	/** Looping burn sound. Assign a looping Sound on this component in the Blueprint. Plays while lit. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UAudioComponent> BurnAudio;

	// --- Burn ---

	/** How long the flare burns once lit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Burn", meta=(ClampMin=1, ClampMax=600, UIMax=120, Units="s"))
	float BurnDuration = 30.0f;

	/** The light dims over this many seconds at the end of the burn. 0 = goes out at full brightness. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Burn", meta=(ClampMin=0, ClampMax=60, Units="s"))
	float FadeOutTime = 4.0f;

	/** Drop the flare automatically when it burns out in the player's hand. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Burn")
	bool bAutoDropWhenBurntOut = true;

	/** A burnt-out flare lying in the world is destroyed after this long. 0 = never. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Burn", meta=(ClampMin=0, ClampMax=600, Units="s"))
	float BurntOutLifeSpan = 30.0f;

	// --- Light ---

	/** Light colour while burning. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Light")
	FLinearColor LightColor = FLinearColor(1.0f, 0.18f, 0.08f);

	/** Brightness while burning, before flicker and fade. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Light", meta=(ClampMin=0, ClampMax=10000, UIMax=500, Units="cd"))
	float LightIntensity = 60.0f;

	/** How far the light reaches. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Light", meta=(ClampMin=50, ClampMax=5000, UIMax=2500, Units="cm"))
	float LightRadius = 1000.0f;

	/** Whether the light flickers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Light")
	bool bFlicker = true;

	/** Flicker strength as a fraction of intensity. It doubles while the flare is fading out, so it sputters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Light", meta=(EditCondition="bFlicker", EditConditionHides, ClampMin=0, ClampMax=1))
	float FlickerAmount = 0.25f;

	/** Flicker speed. Higher is more erratic. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Light", meta=(EditCondition="bFlicker", EditConditionHides, ClampMin=0.1, ClampMax=50))
	float FlickerSpeed = 9.0f;

	/** How often the light intensity updates (flicker and fade). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Light", meta=(ClampMin=0.01, ClampMax=0.25, Units="s"))
	float LightUpdateInterval = 0.033f;

	// --- Look (placeholder primitives) ---

	/** Body colour. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Look")
	FLinearColor BodyColor = FLinearColor(0.35f, 0.35f, 0.35f);

	/** Cap colour while unlit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Look")
	FLinearColor CapColor = FLinearColor(0.6f, 0.02f, 0.02f);

	/** Colour of body and cap once burnt out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Look")
	FLinearColor BurntOutColor = FLinearColor(0.04f, 0.04f, 0.04f);

	// --- Sound ---

	/** One-shot sound when lit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Sound")
	TObjectPtr<USoundBase> IgniteSound;

	/** One-shot sound when it burns out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flare|Sound")
	TObjectPtr<USoundBase> BurnOutSound;

private:

	EFlareState FlareState = EFlareState::Unlit;

	/** One-shot timer for the whole burn. Its remaining time is the burn time left (and it pauses with the game). */
	FTimerHandle BurnTimer;

	/** Repeating timer for flicker and fade. */
	FTimerHandle LightUpdateTimer;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CapMaterial;

	/** Called by BurnTimer. */
	void BurnOut();

	/** Applies flicker and fade to the light. */
	void UpdateLight();

	/** Sets the placeholder colours. */
	void ApplyColors(const FLinearColor& Body, const FLinearColor& Cap);

	void SetFlareState(EFlareState NewState);
};

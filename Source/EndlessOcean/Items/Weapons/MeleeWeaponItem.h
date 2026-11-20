#pragma once

#include "CoreMinimal.h"
#include "HeldItem.h"
#include "MeleeWeaponItem.generated.h"

class UCurveFloat;
class USoundBase;

/** Where a melee swing is in its sequence. */
UENUM(BlueprintType)
enum class EMeleeSwingPhase : uint8
{
	Idle,
	/** Drawing the weapon back. */
	WindUp,
	/** The fast arc; the hit happens halfway through. */
	Strike,
	/** Slowly bringing the weapon back. */
	Recovery,
	/** Tried to swing without enough stamina: a short, weak heave and no hit. */
	Strain
};

/**
 * A heavy hand-held melee weapon or tool: the large wrench, and later a pipe or crowbar as Blueprints.
 *
 * Each swing costs a big share of the stamina bar (half by default) and can only start if that much is available.
 * A swing is wind-up, strike, recovery, all driven by timers. The visible arc is a procedural pose the hand reads
 * every frame (GetAdditivePose), so no animation assets are needed yet.
 *
 * The hit is a sphere sweep from the camera on the Weapon trace channel at the middle of the strike. It applies
 * engine point damage (so anything with a UHealthComponent reacts), pushes physics objects, and reports a noise
 * for the monster's hearing.
 */
UCLASS(Blueprintable, PrioritizeCategories=(Melee, MeleeTiming, MeleePose, MeleeSound, Item))
class ENDLESSOCEAN_API AMeleeWeaponItem : public AHeldItem
{
	GENERATED_BODY()

public:

	AMeleeWeaponItem();

	/** Current swing phase. */
	UFUNCTION(BlueprintPure, Category="Melee")
	EMeleeSwingPhase GetSwingPhase() const { return Phase; }

	virtual FTransform GetAdditivePose() const override;

protected:

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// AHeldItem
	virtual void UsePrimary_Implementation() override;
	virtual void AltActionStarted_Implementation() override;
	virtual void CancelActions_Implementation() override;

	/** Damage dealt by a hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=0, ClampMax=500))
	float Damage = 35.0f;

	/** Push given to physics objects that are hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=0, ClampMax=200000))
	float HitImpulse = 40000.0f;

	/** How far the swing reaches from the eyes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=10, ClampMax=400, Units="cm"))
	float Reach = 160.0f;

	/** Radius of the swept hit sphere. Larger is more forgiving. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=1, ClampMax=100, Units="cm"))
	float SweepRadius = 20.0f;

	/** Fraction of max stamina each swing costs. The swing can't start without this much stamina. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=0, ClampMax=1))
	float StaminaCostFraction = 0.5f;

	/** Trace channel for the hit ("Weapon", GameTraceChannel3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel3;

	/** Loudness of an impact for AI hearing (0 to 1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin=0, ClampMax=1))
	float ImpactNoiseLoudness = 0.6f;

	/** Time spent drawing back before the strike. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MeleeTiming", meta=(ClampMin=0.05, ClampMax=3, Units="s"))
	float WindUpTime = 0.45f;

	/** Duration of the fast arc. The hit lands halfway through. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MeleeTiming", meta=(ClampMin=0.05, ClampMax=1, Units="s"))
	float StrikeTime = 0.18f;

	/** Time to bring the weapon back before another swing is allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MeleeTiming", meta=(ClampMin=0, ClampMax=5, Units="s"))
	float RecoveryTime = 1.2f;

	/** Duration of the failed "too tired" heave. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MeleeTiming", meta=(ClampMin=0.1, ClampMax=3, Units="s"))
	float StrainTime = 0.6f;

	/** Optional shape for the strike arc (X: time 0 to 1, Y: progress 0 to 1). Without one, the strike accelerates (ease-in). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MeleeTiming")
	TObjectPtr<UCurveFloat> StrikeCurve;

	/** Pose at the top of the wind-up, added to the held offset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MeleePose")
	FVector WindUpLocation = FVector(-8.0f, 12.0f, 14.0f);

	/** Rotation at the top of the wind-up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MeleePose")
	FRotator WindUpRotation = FRotator(35.0f, -25.0f, 25.0f);

	/** Pose at the end of the strike. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MeleePose")
	FVector StrikeLocation = FVector(18.0f, -28.0f, -12.0f);

	/** Rotation at the end of the strike. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MeleePose")
	FRotator StrikeRotation = FRotator(-55.0f, 40.0f, -35.0f);

	/** Wobble size of the failed "too tired" heave. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MeleePose")
	FRotator StrainRotation = FRotator(10.0f, -5.0f, 6.0f);

	/** Played when the strike starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MeleeSound")
	TObjectPtr<USoundBase> SwingSound;

	/** Played at the impact point on a hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MeleeSound")
	TObjectPtr<USoundBase> HitSound;

	/** Played for a failed, too-tired swing (a grunt). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MeleeSound")
	TObjectPtr<USoundBase> StrainSound;

private:

	EMeleeSwingPhase Phase = EMeleeSwingPhase::Idle;
	double PhaseStartTime = 0.0;
	float PhaseDuration = 0.0f;

	FTimerHandle PhaseTimer;
	FTimerHandle HitTimer;

	/** Enters a phase and schedules the next one. */
	void BeginPhase(EMeleeSwingPhase NewPhase);

	/** Timer callback: moves to the phase after the current one. */
	void AdvancePhase();

	/** The actual hit test, at mid-strike. */
	void PerformHit();

	/** 0 to 1 progress through the current phase. */
	float GetPhaseAlpha() const;
};

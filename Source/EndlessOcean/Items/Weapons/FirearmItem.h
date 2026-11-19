#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "HeldItem.h"
#include "FirearmItem.generated.h"

class AFirearmItem;
class UAmmoPouchComponent;
class UCameraShakeBase;
class UCurveFloat;
class UPointLightComponent;
class USoundBase;

/** What the reload sequence is doing. */
UENUM(BlueprintType)
enum class EFirearmReloadState : uint8
{
	None,
	/** Breaking the gun open. */
	Opening,
	/** Inserting rounds one at a time. */
	Loading,
	/** Closing it again. */
	Closing
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFirearmAmmoChangedSignature, AFirearmItem*, Firearm, int32, LoadedRounds, int32, Capacity);

/**
 * A hitscan firearm held by an untrained survivor. The SPP-1 underwater pistol is a Blueprint/data set-up of this
 * class: 4 barrels, loaded one round at a time, heavy sway and spread, big recoil, and a slow rate of fire.
 *
 *  - Fire (left mouse): the shot goes where the gun points, i.e. camera direction + the hand's sway + a random cone.
 *    The cone is wide from the hip, narrow while aiming, and grows with movement and right after a shot.
 *  - Aim (hold right mouse): blends to a sights pose, narrows the FOV, and cuts sway and spread (never to zero).
 *  - Reload (R): open, then load one round per PerRoundLoadTime from the ammo pouch, then close. Firing, aiming,
 *    pressing R again or putting the gun away interrupts it, keeping whatever was already loaded.
 *  - Range shrinks with depth: RangeAtSurface at or above the surface, down to RangeAtMaxDepth at MaxRangeDepth.
 *  - Recoil kicks the view up hard and only partly recovers.
 *
 * Tick runs only while something animates (aim blend, kick, recoil recovery) and switches itself off.
 */
UCLASS(Blueprintable, PrioritizeCategories=(Firearm, Ammo, Reload, Spread, Aim, Recoil, Range, FirearmSound, Item))
class ENDLESSOCEAN_API AFirearmItem : public AHeldItem
{
	GENERATED_BODY()

public:

	AFirearmItem();

	/** Rounds currently loaded. */
	UFUNCTION(BlueprintPure, Category="Firearm")
	int32 GetLoadedRounds() const { return LoadedRounds; }

	/** Number of barrels / chambers. */
	UFUNCTION(BlueprintPure, Category="Firearm")
	int32 GetCapacity() const { return Capacity; }

	/** Ammo type this gun takes. */
	UFUNCTION(BlueprintPure, Category="Firearm")
	FGameplayTag GetAmmoType() const { return AmmoType; }

	/** Current reload state. */
	UFUNCTION(BlueprintPure, Category="Firearm")
	EFirearmReloadState GetReloadState() const { return ReloadState; }

	/** Progress through the current reload step (0 to 1), for the HUD. */
	UFUNCTION(BlueprintPure, Category="Firearm")
	float GetReloadStepProgress() const;

	/** Current spread cone (full angle, degrees). Used for the crosshair size. */
	UFUNCTION(BlueprintPure, Category="Firearm")
	float GetCurrentSpread() const;

	/** How far into aim-down-sights we are (0 to 1). */
	UFUNCTION(BlueprintPure, Category="Firearm")
	float GetAimAlpha() const { return AimAlpha; }

	/** Effective range at the muzzle's current depth. */
	UFUNCTION(BlueprintPure, Category="Firearm")
	float GetEffectiveRange() const;

	/** Broadcast when the loaded round count changes. */
	UPROPERTY(BlueprintAssignable, Category="Firearm")
	FFirearmAmmoChangedSignature OnAmmoChanged;

	// AHeldItem
	virtual FTransform GetAdditivePose() const override;
	virtual float GetSwayScale() const override;
	virtual bool GetAimView(float& OutFOV, float& OutAlpha) const override;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	// AHeldItem
	virtual void UsePrimary_Implementation() override;
	virtual void AltActionStarted_Implementation() override;
	virtual void AltActionCompleted_Implementation() override;
	virtual void ReloadPressed_Implementation() override;
	virtual void CancelActions_Implementation() override;

	/** Where shots visually come from and where range depth is measured. Move it to the barrel in the Blueprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> Muzzle;

	/** Brief light when firing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPointLightComponent> MuzzleFlash;

	/** Damage per shot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm", meta=(ClampMin=0, ClampMax=1000))
	float Damage = 60.0f;

	/** Push given to physics objects that are hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm", meta=(ClampMin=0, ClampMax=200000))
	float HitImpulse = 30000.0f;

	/** Minimum time between shots. Long: the survivor isn't trained. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm", meta=(ClampMin=0.05, ClampMax=5, Units="s"))
	float FireInterval = 1.4f;

	/** Trace channel for shots ("Weapon", GameTraceChannel3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel3;

	/** Loudness of a shot for AI hearing (0 to 1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm", meta=(ClampMin=0, ClampMax=1))
	float ShotNoiseLoudness = 1.0f;

	/** Ammo type taken from the pouch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(Categories="Ammo"))
	FGameplayTag AmmoType;

	/** Number of barrels / chambers (the SPP-1 has 4). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin=1, ClampMax=12))
	int32 Capacity = 4;

	/** Rounds loaded when the gun is found. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin=0, ClampMax=12))
	int32 InitialLoadedRounds = 0;

	/** Time to break the gun open. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reload", meta=(ClampMin=0, ClampMax=5, Units="s"))
	float OpenTime = 0.8f;

	/** Time to load each single round. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reload", meta=(ClampMin=0.05, ClampMax=5, Units="s"))
	float PerRoundLoadTime = 1.2f;

	/** Time to close the gun again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reload", meta=(ClampMin=0, ClampMax=5, Units="s"))
	float CloseTime = 0.6f;

	/** Pose while open for loading (tilted up and in). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reload")
	FRotator ReloadRotation = FRotator(35.0f, -20.0f, 25.0f);

	/** Spread from the hip (full cone angle). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread", meta=(ClampMin=0, ClampMax=45, Units="deg"))
	float HipSpread = 6.0f;

	/** Spread while fully aimed. Deliberately not zero. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread", meta=(ClampMin=0, ClampMax=45, Units="deg"))
	float AimedSpread = 1.2f;

	/** Extra spread while moving at walking speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread", meta=(ClampMin=0, ClampMax=45, Units="deg"))
	float MovementSpread = 2.5f;

	/** Speed at which MovementSpread fully applies. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread", meta=(ClampMin=1, ClampMax=1000, Units="cm/s"))
	float MovementSpreadSpeed = 220.0f;

	/** Extra spread right after a shot; fades over FireInterval. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread", meta=(ClampMin=0, ClampMax=45, Units="deg"))
	float ShotSpread = 3.0f;

	/** Time to blend fully into or out of aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin=0.01, ClampMax=2, Units="s"))
	float AimBlendTime = 0.25f;

	/** Field of view while aimed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin=20, ClampMax=120, Units="deg"))
	float AimFOV = 65.0f;

	/** Sway multiplier while aimed. Never zero: shaky hands. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin=0.01, ClampMax=1))
	float AimedSwayMultiplier = 0.25f;

	/** Offset from the hip position to the sights position (centred, raised to the eye). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	FVector AimLocationOffset = FVector(-8.0f, -15.0f, 9.0f);

	/** Rotation adjustment while aimed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	FRotator AimRotationOffset = FRotator::ZeroRotator;

	/** Upward view kick per shot (random between min and max). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil", meta=(ClampMin=0, ClampMax=45, Units="deg"))
	float RecoilPitchMin = 7.0f;

	/** Upward view kick per shot (random between min and max). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil", meta=(ClampMin=0, ClampMax=45, Units="deg"))
	float RecoilPitchMax = 10.0f;

	/** Sideways kick per shot (random, either direction). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil", meta=(ClampMin=0, ClampMax=45, Units="deg"))
	float RecoilYawMax = 2.5f;

	/** Fraction of the upward kick that drifts back down by itself. The rest the player must correct. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil", meta=(ClampMin=0, ClampMax=1))
	float RecoilRecoveryFraction = 0.4f;

	/** Time for that recovery. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil", meta=(ClampMin=0.05, ClampMax=3, Units="s"))
	float RecoilRecoveryTime = 0.35f;

	/** How far the gun itself jerks back and up in the hand on a shot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil")
	FVector KickLocation = FVector(-6.0f, 0.0f, 2.0f);

	/** Gun rotation at the peak of the kick. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil")
	FRotator KickRotation = FRotator(18.0f, 0.0f, 4.0f);

	/** Time for the gun to settle after the kick. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil", meta=(ClampMin=0.05, ClampMax=2, Units="s"))
	float KickRecoverTime = 0.3f;

	/** Optional camera shake played on each shot (create a Camera Shake Blueprint). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil")
	TSubclassOf<UCameraShakeBase> FireCameraShake;

	/** Range at or above the water surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Range", meta=(ClampMin=100, ClampMax=20000, Units="cm"))
	float RangeAtSurface = 2000.0f;

	/** Range at MaxRangeDepth and deeper. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Range", meta=(ClampMin=100, ClampMax=20000, Units="cm"))
	float RangeAtMaxDepth = 500.0f;

	/** Depth at which range bottoms out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Range", meta=(ClampMin=100, ClampMax=20000, Units="cm"))
	float MaxRangeDepth = 4000.0f;

	/** Optional falloff shape (X: depth 0 to 1 of MaxRangeDepth, Y: 0 = surface range, 1 = deep range). Linear without one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Range")
	TObjectPtr<UCurveFloat> RangeFalloffCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FirearmSound")
	TObjectPtr<USoundBase> FireSound;

	/** Pulling the trigger on an empty chamber. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FirearmSound")
	TObjectPtr<USoundBase> DryFireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FirearmSound")
	TObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FirearmSound")
	TObjectPtr<USoundBase> LoadRoundSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FirearmSound")
	TObjectPtr<USoundBase> CloseSound;

private:

	int32 LoadedRounds = 0;
	EFirearmReloadState ReloadState = EFirearmReloadState::None;
	double ReloadStepStartTime = 0.0;
	float ReloadStepDuration = 0.0f;

	bool bAimHeld = false;
	float AimAlpha = 0.0f;
	float ReloadPoseAlpha = 0.0f;
	float KickAlpha = 0.0f;
	double LastFireTime = -1000.0;

	/** Pitch still to be given back by recoil recovery. */
	float RecoilToRecover = 0.0f;
	/** Recovery speed (degrees per second) for the current shot. */
	float RecoilRecoverySpeed = 0.0f;

	FTimerHandle ReloadTimer;
	FTimerHandle FlashTimer;

	void Fire();
	void DryFire();

	void StartReload();
	void CancelReload();
	void BeginReloadStep(EFirearmReloadState State, float Duration);
	void AdvanceReload();

	void SetLoadedRounds(int32 NewCount);

	/** The pawn holding us, if any. */
	APawn* GetHolderPawn() const;

	/** The holder's ammo pouch, if any. */
	UAmmoPouchComponent* GetPouch() const;

	/** Turns Tick on for blending. Tick turns itself off when settled. */
	void WakeTick();

	void PlaySound(USoundBase* Sound) const;
};

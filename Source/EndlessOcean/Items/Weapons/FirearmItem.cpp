#include "FirearmItem.h"

#include "AmmoPouchComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "ItemHolderComponent.h"
#include "ItemTags.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"
#include "WaterQueryLibrary.h"

namespace FirearmCVars
{
	/** "eo.Weapons.Debug 1" draws each shot's trace (green = hit, red = out of range) and its range. */
	static TAutoConsoleVariable<bool> CVarDebug(
		TEXT("eo.Weapons.Debug"),
		false,
		TEXT("Draw firearm shot traces and effective range."),
		ECVF_Cheat);
}

namespace Firearm
{
	/** Blend time for the open-for-loading pose. */
	static constexpr float ReloadPoseBlendTime = 0.25f;

	/** After a dry click, the trigger can be pulled again this soon. */
	static constexpr float DryFireRepeatTime = 0.35f;

	/** How long the muzzle flash light stays on. */
	static constexpr float FlashDuration = 0.05f;
}

AFirearmItem::AFirearmItem()
{
	// Tick is only switched on while something is animating (see WakeTick).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(Mesh);

	MuzzleFlash = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleFlash"));
	MuzzleFlash->SetupAttachment(Muzzle);
	MuzzleFlash->SetIntensityUnits(ELightUnits::Candelas);
	MuzzleFlash->SetIntensity(400.0f);
	MuzzleFlash->SetAttenuationRadius(800.0f);
	MuzzleFlash->SetLightColor(FLinearColor(1.0f, 0.75f, 0.45f));
	MuzzleFlash->SetVisibility(false);
	MuzzleFlash->SetCastShadows(false);

	AmmoType = EndlessOceanTags::Ammo_SPP1;
}

void AFirearmItem::BeginPlay()
{
	Super::BeginPlay();
	LoadedRounds = FMath::Clamp(InitialLoadedRounds, 0, Capacity);
}

void AFirearmItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(ReloadTimer);
	Timers.ClearTimer(FlashTimer);
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------------------------------------------
// Input

void AFirearmItem::UsePrimary_Implementation()
{
	// Pulling the trigger abandons a reload but keeps the rounds already loaded.
	if (ReloadState != EFirearmReloadState::None)
	{
		CancelReload();
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFireTime < FireInterval)
	{
		return;
	}

	if (LoadedRounds <= 0)
	{
		DryFire();
		return;
	}

	Fire();
}

void AFirearmItem::AltActionStarted_Implementation()
{
	bAimHeld = true;
	if (ReloadState != EFirearmReloadState::None)
	{
		CancelReload();
	}
	WakeTick();
}

void AFirearmItem::AltActionCompleted_Implementation()
{
	bAimHeld = false;
	WakeTick();
}

void AFirearmItem::ReloadPressed_Implementation()
{
	// Pressing reload again stops loading early.
	if (ReloadState != EFirearmReloadState::None)
	{
		CancelReload();
		return;
	}
	StartReload();
}

void AFirearmItem::CancelActions_Implementation()
{
	bAimHeld = false;
	CancelReload();
	WakeTick();
}

// ---------------------------------------------------------------------------------------------------------------
// Firing

void AFirearmItem::Fire()
{
	UWorld* World = GetWorld();
	APawn* Pawn = GetHolderPawn();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	FVector ViewLocation;
	FRotator ViewRotation;
	if (Controller)
	{
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		GetActorEyesViewPoint(ViewLocation, ViewRotation);
	}

	// The gun points where the camera looks plus however far the shaking hand has drifted. Shots go there, plus a
	// random cone: VRandCone takes the cone's half-angle in radians.
	const UItemHolderComponent* CurrentHolder = GetHolder();
	const FRotator Sway = CurrentHolder ? CurrentHolder->GetSwayRotation() : FRotator::ZeroRotator;
	const FVector AimDirection = (ViewRotation + Sway).Vector();
	const FVector ShotDirection = FMath::VRandCone(AimDirection, FMath::DegreesToRadians(GetCurrentSpread() * 0.5f));
	const FVector TraceEnd = ViewLocation + ShotDirection * GetEffectiveRange();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(FirearmShot), true, Pawn);
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, TraceChannel, Params);
	if (bHit)
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			UGameplayStatics::ApplyPointDamage(HitActor, Damage, ShotDirection, Hit, Controller, this, UDamageType::StaticClass());
		}
		UPrimitiveComponent* HitComponent = Hit.GetComponent();
		if (HitComponent && HitComponent->IsSimulatingPhysics())
		{
			HitComponent->AddImpulseAtLocation(ShotDirection * HitImpulse, Hit.ImpactPoint);
		}
	}

#if ENABLE_DRAW_DEBUG
	if (FirearmCVars::CVarDebug.GetValueOnGameThread())
	{
		const FVector End = bHit ? Hit.ImpactPoint : TraceEnd;
		DrawDebugLine(World, Muzzle->GetComponentLocation(), End, bHit ? FColor::Green : FColor::Red, false, 3.0f, 0, 0.5f);
		DrawDebugSphere(World, End, 6.0f, 8, bHit ? FColor::Green : FColor::Red, false, 3.0f);
	}
#endif

	// Flash, bang, and let the monster hear it.
	MuzzleFlash->SetVisibility(true);
	World->GetTimerManager().SetTimer(FlashTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		MuzzleFlash->SetVisibility(false);
	}), Firearm::FlashDuration, false);
	PlaySound(FireSound);
	UAISense_Hearing::ReportNoiseEvent(this, GetActorLocation(), ShotNoiseLoudness, Pawn);

	// Recoil: kick the view up (and a little sideways). Only part of it drifts back down on its own.
	if (Controller)
	{
		const float KickPitch = FMath::FRandRange(RecoilPitchMin, RecoilPitchMax);
		const float KickYaw = FMath::FRandRange(-RecoilYawMax, RecoilYawMax);
		FRotator Control = Controller->GetControlRotation();
		Control.Pitch += KickPitch;
		Control.Yaw += KickYaw;
		Controller->SetControlRotation(Control);

		RecoilToRecover += KickPitch * RecoilRecoveryFraction;
		RecoilRecoverySpeed = RecoilToRecover / RecoilRecoveryTime;

		APlayerController* PlayerController = Cast<APlayerController>(Controller);
		if (PlayerController && FireCameraShake)
		{
			PlayerController->ClientStartCameraShake(FireCameraShake);
		}
	}

	KickAlpha = 1.0f;
	LastFireTime = World->GetTimeSeconds();
	SetLoadedRounds(LoadedRounds - 1);
	WakeTick();
}

void AFirearmItem::DryFire()
{
	PlaySound(DryFireSound);
	// Let the trigger be pulled again soon, but not every frame.
	LastFireTime = GetWorld()->GetTimeSeconds() - FireInterval + Firearm::DryFireRepeatTime;
}

float AFirearmItem::GetCurrentSpread() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return HipSpread;
	}

	const float Base = FMath::Lerp(HipSpread, AimedSpread, AimAlpha);

	const APawn* Pawn = GetHolderPawn();
	const float Speed = Pawn ? Pawn->GetVelocity().Size() : 0.0f;
	const float Movement = MovementSpread * FMath::Clamp(Speed / MovementSpreadSpeed, 0.0f, 1.0f);

	const float SinceShot = static_cast<float>(World->GetTimeSeconds() - LastFireTime);
	const float Shot = ShotSpread * FMath::Clamp(1.0f - SinceShot / FireInterval, 0.0f, 1.0f);

	return Base + Movement + Shot;
}

float AFirearmItem::GetEffectiveRange() const
{
	const float Depth = UWaterQueryLibrary::GetWaterDepthAt(this, Muzzle->GetComponentLocation());
	float Alpha = FMath::Clamp(Depth / MaxRangeDepth, 0.0f, 1.0f);
	if (RangeFalloffCurve)
	{
		Alpha = FMath::Clamp(RangeFalloffCurve->GetFloatValue(Alpha), 0.0f, 1.0f);
	}
	return FMath::Lerp(RangeAtSurface, RangeAtMaxDepth, Alpha);
}

// ---------------------------------------------------------------------------------------------------------------
// Reloading

void AFirearmItem::StartReload()
{
	const UAmmoPouchComponent* Pouch = GetPouch();
	if (!Pouch || LoadedRounds >= Capacity || Pouch->GetCount(AmmoType) <= 0)
	{
		return;
	}

	PlaySound(OpenSound);
	BeginReloadStep(EFirearmReloadState::Opening, OpenTime);
}

void AFirearmItem::BeginReloadStep(EFirearmReloadState State, float Duration)
{
	ReloadState = State;
	ReloadStepStartTime = GetWorld()->GetTimeSeconds();
	ReloadStepDuration = Duration;
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &AFirearmItem::AdvanceReload, FMath::Max(Duration, KINDA_SMALL_NUMBER), false);
	WakeTick();
}

void AFirearmItem::AdvanceReload()
{
	switch (ReloadState)
	{
	case EFirearmReloadState::Opening:
		// Open: start pushing the first round in.
		BeginReloadStep(EFirearmReloadState::Loading, PerRoundLoadTime);
		break;

	case EFirearmReloadState::Loading:
	{
		// One round finished loading.
		UAmmoPouchComponent* Pouch = GetPouch();
		if (Pouch && Pouch->TryTake(AmmoType, 1))
		{
			SetLoadedRounds(LoadedRounds + 1);
			PlaySound(LoadRoundSound);
		}

		if (Pouch && LoadedRounds < Capacity && Pouch->GetCount(AmmoType) > 0)
		{
			BeginReloadStep(EFirearmReloadState::Loading, PerRoundLoadTime);
		}
		else
		{
			PlaySound(CloseSound);
			BeginReloadStep(EFirearmReloadState::Closing, CloseTime);
		}
		break;
	}

	default:
		ReloadState = EFirearmReloadState::None;
		WakeTick();
		break;
	}
}

void AFirearmItem::CancelReload()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadTimer);
	}
	ReloadState = EFirearmReloadState::None;
}

float AFirearmItem::GetReloadStepProgress() const
{
	if (ReloadState == EFirearmReloadState::None || ReloadStepDuration <= 0.0f || !GetWorld())
	{
		return 0.0f;
	}
	return FMath::Clamp(static_cast<float>((GetWorld()->GetTimeSeconds() - ReloadStepStartTime) / ReloadStepDuration), 0.0f, 1.0f);
}

void AFirearmItem::SetLoadedRounds(int32 NewCount)
{
	LoadedRounds = FMath::Clamp(NewCount, 0, Capacity);
	OnAmmoChanged.Broadcast(this, LoadedRounds, Capacity);
}

// ---------------------------------------------------------------------------------------------------------------
// Pose and animation

void AFirearmItem::WakeTick()
{
	SetActorTickEnabled(true);
}

void AFirearmItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Constant-rate blends, so AimBlendTime really is the time to go from hip to sights.
	const float AimTarget = (bAimHeld && ReloadState == EFirearmReloadState::None) ? 1.0f : 0.0f;
	AimAlpha = FMath::FInterpConstantTo(AimAlpha, AimTarget, DeltaSeconds, 1.0f / AimBlendTime);

	const float ReloadTarget = ReloadState != EFirearmReloadState::None ? 1.0f : 0.0f;
	ReloadPoseAlpha = FMath::FInterpConstantTo(ReloadPoseAlpha, ReloadTarget, DeltaSeconds, 1.0f / Firearm::ReloadPoseBlendTime);

	KickAlpha = FMath::Max(0.0f, KickAlpha - DeltaSeconds / KickRecoverTime);

	// Give back part of the recoil, a little each frame.
	if (RecoilToRecover > 0.0f)
	{
		const float Step = FMath::Min(RecoilToRecover, RecoilRecoverySpeed * DeltaSeconds);
		RecoilToRecover -= Step;

		const APawn* Pawn = GetHolderPawn();
		if (AController* Controller = Pawn ? Pawn->GetController() : nullptr)
		{
			FRotator Control = Controller->GetControlRotation();
			Control.Pitch -= Step;
			Controller->SetControlRotation(Control);
		}
	}

	const bool bSettled = AimAlpha == AimTarget && ReloadPoseAlpha == ReloadTarget && KickAlpha <= 0.0f && RecoilToRecover <= 0.0f;
	if (bSettled)
	{
		SetActorTickEnabled(false);
	}
}

FTransform AFirearmItem::GetAdditivePose() const
{
	// Smooth the linear blends so the gun eases into place rather than moving robotically.
	const float Aim = FMath::SmoothStep(0.0f, 1.0f, AimAlpha);
	const float Reload = FMath::SmoothStep(0.0f, 1.0f, ReloadPoseAlpha);
	const float Kick = KickAlpha * KickAlpha; // sharp at first, then settling

	const FVector Location = AimLocationOffset * Aim + KickLocation * Kick;
	const FRotator Rotation = AimRotationOffset * Aim + KickRotation * Kick + ReloadRotation * Reload;
	return FTransform(Rotation, Location);
}

float AFirearmItem::GetSwayScale() const
{
	return FMath::Lerp(1.0f, AimedSwayMultiplier, AimAlpha);
}

bool AFirearmItem::GetAimView(float& OutFOV, float& OutAlpha) const
{
	OutFOV = AimFOV;
	OutAlpha = FMath::SmoothStep(0.0f, 1.0f, AimAlpha);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------
// Helpers

APawn* AFirearmItem::GetHolderPawn() const
{
	const UItemHolderComponent* CurrentHolder = GetHolder();
	return CurrentHolder ? Cast<APawn>(CurrentHolder->GetOwner()) : nullptr;
}

UAmmoPouchComponent* AFirearmItem::GetPouch() const
{
	const APawn* Pawn = GetHolderPawn();
	return Pawn ? Pawn->FindComponentByClass<UAmmoPouchComponent>() : nullptr;
}

void AFirearmItem::PlaySound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

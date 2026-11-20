#include "MeleeWeaponItem.h"

#include "Components/PrimitiveComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "ItemHolderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "StaminaComponent.h"
#include "TimerManager.h"

AMeleeWeaponItem::AMeleeWeaponItem()
{
	// The swing pose is read by the hand each frame; the weapon itself needs no Tick.
	PrimaryActorTick.bCanEverTick = false;
}

void AMeleeWeaponItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(PhaseTimer);
	Timers.ClearTimer(HitTimer);
	Super::EndPlay(EndPlayReason);
}

void AMeleeWeaponItem::UsePrimary_Implementation()
{
	if (Phase != EMeleeSwingPhase::Idle)
	{
		return;
	}

	const UItemHolderComponent* CurrentHolder = GetHolder();
	UStaminaComponent* Stamina = CurrentHolder ? CurrentHolder->GetOwner()->FindComponentByClass<UStaminaComponent>() : nullptr;

	// No stamina component (e.g. an AI holder) means swings are free; otherwise pay up front or strain instead.
	const bool bCanSwing = !Stamina || Stamina->TryConsume(Stamina->GetMaxStamina() * StaminaCostFraction);
	if (!bCanSwing)
	{
		if (StrainSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, StrainSound, GetActorLocation());
		}
		BeginPhase(EMeleeSwingPhase::Strain);
		return;
	}

	BeginPhase(EMeleeSwingPhase::WindUp);
}

void AMeleeWeaponItem::AltActionStarted_Implementation()
{
	// Deliberately empty: right-clicking shouldn't throw away your only weapon. G still throws it.
}

void AMeleeWeaponItem::CancelActions_Implementation()
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(PhaseTimer);
	Timers.ClearTimer(HitTimer);
	Phase = EMeleeSwingPhase::Idle;
}

void AMeleeWeaponItem::BeginPhase(EMeleeSwingPhase NewPhase)
{
	Phase = NewPhase;
	PhaseStartTime = GetWorld()->GetTimeSeconds();

	switch (NewPhase)
	{
	case EMeleeSwingPhase::WindUp:   PhaseDuration = WindUpTime; break;
	case EMeleeSwingPhase::Strike:   PhaseDuration = StrikeTime; break;
	case EMeleeSwingPhase::Recovery: PhaseDuration = RecoveryTime; break;
	case EMeleeSwingPhase::Strain:   PhaseDuration = StrainTime; break;
	default:                         PhaseDuration = 0.0f; return;
	}

	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.SetTimer(PhaseTimer, this, &AMeleeWeaponItem::AdvancePhase, FMath::Max(PhaseDuration, KINDA_SMALL_NUMBER), false);

	if (NewPhase == EMeleeSwingPhase::Strike)
	{
		if (SwingSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, SwingSound, GetActorLocation());
		}
		Timers.SetTimer(HitTimer, this, &AMeleeWeaponItem::PerformHit, FMath::Max(PhaseDuration * 0.5f, KINDA_SMALL_NUMBER), false);
	}
}

void AMeleeWeaponItem::AdvancePhase()
{
	switch (Phase)
	{
	case EMeleeSwingPhase::WindUp: BeginPhase(EMeleeSwingPhase::Strike); break;
	case EMeleeSwingPhase::Strike: BeginPhase(EMeleeSwingPhase::Recovery); break;
	default:                       Phase = EMeleeSwingPhase::Idle; break;
	}
}

void AMeleeWeaponItem::PerformHit()
{
	const UItemHolderComponent* CurrentHolder = GetHolder();
	APawn* OwnerPawn = CurrentHolder ? Cast<APawn>(CurrentHolder->GetOwner()) : nullptr;
	if (!OwnerPawn)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	AController* Controller = OwnerPawn->GetController();
	if (Controller)
	{
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		OwnerPawn->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	}

	const FVector Direction = ViewRotation.Vector();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MeleeHit), false, OwnerPawn);
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	if (!GetWorld()->SweepSingleByChannel(Hit, ViewLocation, ViewLocation + Direction * Reach, FQuat::Identity, TraceChannel,
		FCollisionShape::MakeSphere(SweepRadius), Params))
	{
		return;
	}

	if (AActor* HitActor = Hit.GetActor())
	{
		UGameplayStatics::ApplyPointDamage(HitActor, Damage, Direction, Hit, Controller, this, UDamageType::StaticClass());
	}

	UPrimitiveComponent* HitComponent = Hit.GetComponent();
	if (HitComponent && HitComponent->IsSimulatingPhysics())
	{
		HitComponent->AddImpulseAtLocation(Direction * HitImpulse, Hit.ImpactPoint);
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, Hit.ImpactPoint);
	}

	// Let AI hear it. Harmless until the monster has a hearing sense configured.
	UAISense_Hearing::ReportNoiseEvent(this, Hit.ImpactPoint, ImpactNoiseLoudness, OwnerPawn);
}

float AMeleeWeaponItem::GetPhaseAlpha() const
{
	if (PhaseDuration <= 0.0f || !GetWorld())
	{
		return 1.0f;
	}
	return FMath::Clamp(static_cast<float>((GetWorld()->GetTimeSeconds() - PhaseStartTime) / PhaseDuration), 0.0f, 1.0f);
}

FTransform AMeleeWeaponItem::GetAdditivePose() const
{
	const float Alpha = GetPhaseAlpha();

	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;

	switch (Phase)
	{
	case EMeleeSwingPhase::WindUp:
	{
		// Ease out: quick start, settling at the top of the backswing.
		const float T = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
		Location = WindUpLocation * T;
		Rotation = WindUpRotation * T;
		break;
	}
	case EMeleeSwingPhase::Strike:
	{
		// Accelerating arc from the backswing to the follow-through.
		const float T = StrikeCurve ? StrikeCurve->GetFloatValue(Alpha) : FMath::InterpEaseIn(0.0f, 1.0f, Alpha, 2.0f);
		Location = FMath::Lerp(WindUpLocation, StrikeLocation, T);
		Rotation = WindUpRotation + (StrikeRotation - WindUpRotation) * T;
		break;
	}
	case EMeleeSwingPhase::Recovery:
	{
		const float T = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
		Location = FMath::Lerp(StrikeLocation, FVector::ZeroVector, T);
		Rotation = StrikeRotation * (1.0f - T);
		break;
	}
	case EMeleeSwingPhase::Strain:
	{
		// A couple of weak, fading wobbles.
		const float Wobble = FMath::Sin(Alpha * UE_PI * 3.0f) * (1.0f - Alpha);
		Rotation = StrainRotation * Wobble;
		break;
	}
	default:
		break;
	}

	return FTransform(Rotation, Location);
}

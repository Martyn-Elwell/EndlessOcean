#include "DamageScreenEffectComponent.h"

#include "BreathComponent.h"
#include "Camera/CameraComponent.h"
#include "DrowningDamageType.h"
#include "Engine/World.h"
#include "HealthComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

namespace DamageScreenEffect
{
	static const FName DrownParam(TEXT("DrownAmount"));
	static const FName BloodParam(TEXT("BloodAmount"));
	static const FName BloodColorParam(TEXT("BloodColor"));

	/** Below this, an effect counts as invisible. */
	static constexpr float VisibleThreshold = 0.001f;
}

UDamageScreenEffectComponent::UDamageScreenEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDamageScreenEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	Health = Owner->FindComponentByClass<UHealthComponent>();
	Breath = Owner->FindComponentByClass<UBreathComponent>();

	if (UHealthComponent* HealthComp = Health.Get())
	{
		HealthComp->OnHealthChanged.AddDynamic(this, &UDamageScreenEffectComponent::HandleHealthChanged);
	}
	if (UBreathComponent* BreathComp = Breath.Get())
	{
		BreathComp->OnBreathChanged.AddDynamic(this, &UDamageScreenEffectComponent::HandleBreathChanged);
	}

	// Add our material to the camera's own post-process blendables. Only this player's camera sees it, so there's no
	// need to place a post-process volume in every room.
	UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>();
	if (Camera && VignetteMaterial)
	{
		VignetteMID = UMaterialInstanceDynamic::Create(VignetteMaterial, this);
		VignetteMID->SetVectorParameterValue(DamageScreenEffect::BloodColorParam, BloodColor);
		VignetteMID->SetScalarParameterValue(DamageScreenEffect::DrownParam, 0.0f);
		VignetteMID->SetScalarParameterValue(DamageScreenEffect::BloodParam, 0.0f);
		Camera->AddOrUpdateBlendable(VignetteMID, 1.0f);
	}
}

void UDamageScreenEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
	Super::EndPlay(EndPlayReason);
}

void UDamageScreenEffectComponent::HandleHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta, const UDamageType* DamageType)
{
	// Drowning shows as the black vignette (driven by breath and health), so only other damage pulses blood.
	const bool bPhysicalDamage = Delta < 0.0f && !(DamageType && DamageType->IsA<UDrowningDamageType>());
	if (bPhysicalDamage)
	{
		const float Scale = FMath::Clamp(-Delta / HitPulseFullDamage, 0.2f, 1.0f);
		BloodPulse = FMath::Min(1.0f, BloodPulse + HitPulseStrength * Scale);
	}
	WakeUp();
}

void UDamageScreenEffectComponent::HandleBreathChanged(float NormalizedBreath)
{
	WakeUp();
}

void UDamageScreenEffectComponent::WakeUp()
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	if (VignetteMID && !Timers.IsTimerActive(UpdateTimer))
	{
		Timers.SetTimer(UpdateTimer, this, &UDamageScreenEffectComponent::UpdateEffect, UpdateInterval, true);
	}
}

float UDamageScreenEffectComponent::GetHealthNormalized() const
{
	const UHealthComponent* HealthComp = Health.Get();
	return HealthComp ? HealthComp->GetHealthNormalized() : 1.0f;
}

float UDamageScreenEffectComponent::GetTargetDrown() const
{
	const UBreathComponent* BreathComp = Breath.Get();
	if (!BreathComp)
	{
		return 0.0f;
	}

	if (BreathComp->IsDrowning())
	{
		// Out of air: start mostly closed, and close completely as drowning damage takes health away.
		return DrownAmountAtNoBreath + (1.0f - DrownAmountAtNoBreath) * (1.0f - GetHealthNormalized());
	}

	// Closing as breath runs low, from 0 at DrownStartBreath to DrownAmountAtNoBreath at empty.
	const float Low = 1.0f - FMath::Clamp(BreathComp->GetBreathNormalized() / DrownStartBreath, 0.0f, 1.0f);
	return Low * DrownAmountAtNoBreath;
}

void UDamageScreenEffectComponent::UpdateEffect()
{
	const float DeltaTime = UpdateInterval;
	const float HealthNorm = GetHealthNormalized();

	// Drowning: ease towards the target so the vignette visibly creeps in and opens up again at the surface.
	CurrentDrown = FMath::FInterpTo(CurrentDrown, GetTargetDrown(), DeltaTime, DrownBlendSpeed);

	// Blood: hit pulse (fading) + lasting level from missing health + heartbeat at low health.
	BloodPulse = FMath::Max(0.0f, BloodPulse - DeltaTime / HitPulseFadeTime);
	const float Persistent = PersistentBloodAtZeroHealth * (1.0f - HealthNorm);

	float Heartbeat = 0.0f;
	if (HealthNorm < HeartbeatHealthThreshold && HealthNorm > 0.0f)
	{
		HeartbeatPhase = FMath::Fmod(HeartbeatPhase + DeltaTime * HeartbeatRate, 1.0f);
		// A sharp "thump" rather than a smooth sine: raise a half-wave to a high power.
		const float Wave = FMath::Max(0.0f, FMath::Sin(HeartbeatPhase * UE_TWO_PI));
		Heartbeat = HeartbeatStrength * FMath::Pow(Wave, 6.0f);
	}

	const float Blood = FMath::Clamp(Persistent + BloodPulse + Heartbeat, 0.0f, 1.0f);

	VignetteMID->SetScalarParameterValue(DamageScreenEffect::DrownParam, CurrentDrown);
	VignetteMID->SetScalarParameterValue(DamageScreenEffect::BloodParam, Blood);

	// Stop once nothing is animating. The material keeps the last values (e.g. a lasting blood level), and any
	// health or breath change wakes us again. The heartbeat animates on its own, so it keeps the timer alive.
	const bool bDrownSettled = FMath::IsNearlyEqual(CurrentDrown, GetTargetDrown(), DamageScreenEffect::VisibleThreshold);
	const bool bHeartbeatActive = HealthNorm < HeartbeatHealthThreshold && HealthNorm > 0.0f;
	if (bDrownSettled && BloodPulse <= 0.0f && !bHeartbeatActive)
	{
		GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
	}
}

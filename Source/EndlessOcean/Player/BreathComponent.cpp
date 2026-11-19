#include "BreathComponent.h"

#include "DrowningDamageType.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "WaterQueryLibrary.h"

UBreathComponent::UBreathComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBreathComponent::BeginPlay()
{
	Super::BeginPlay();

	Breath = MaxBreath;
	GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &UBreathComponent::UpdateBreath, UpdateInterval, true);
}

void UBreathComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(UpdateTimer);
	Timers.ClearTimer(DrowningTimer);
	Super::EndPlay(EndPlayReason);
}

void UBreathComponent::AddBreath(float Seconds)
{
	SetBreath(Breath + Seconds);
}

void UBreathComponent::UpdateBreath()
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// For a pawn this is its location plus BaseEyeHeight, which follows crouching.
	FVector EyeLocation;
	FRotator EyeRotation;
	Owner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	const bool bNowSubmerged = UWaterQueryLibrary::GetWaterDepthAt(this, EyeLocation) > SubmergeDepth;
	if (bNowSubmerged != bSubmerged)
	{
		bSubmerged = bNowSubmerged;
		OnSubmergedChanged.Broadcast(bSubmerged);
	}

	SetBreath(Breath + (bSubmerged ? -DrainRate : RefillRate) * UpdateInterval);
	SetDrowning(bSubmerged && Breath <= 0.0f);
}

void UBreathComponent::SetBreath(float NewBreath)
{
	NewBreath = FMath::Clamp(NewBreath, 0.0f, MaxBreath);
	if (!FMath::IsNearlyEqual(NewBreath, Breath))
	{
		Breath = NewBreath;
		OnBreathChanged.Broadcast(GetBreathNormalized());
	}
}

void UBreathComponent::SetDrowning(bool bNewDrowning)
{
	if (bNewDrowning == bDrowning)
	{
		return;
	}

	bDrowning = bNewDrowning;
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	if (bDrowning)
	{
		// First tick after one interval: the black vignette has a moment to close before damage starts.
		Timers.SetTimer(DrowningTimer, this, &UBreathComponent::ApplyDrowningDamage, DrowningDamageInterval, true);
	}
	else
	{
		Timers.ClearTimer(DrowningTimer);
	}

	OnDrowningChanged.Broadcast(bDrowning);
}

void UBreathComponent::ApplyDrowningDamage()
{
	AActor* Owner = GetOwner();
	const APawn* Pawn = Cast<APawn>(Owner);
	UGameplayStatics::ApplyDamage(Owner, DrowningDamagePerSecond * DrowningDamageInterval,
		Pawn ? Pawn->GetController() : nullptr, Owner, UDrowningDamageType::StaticClass());
}

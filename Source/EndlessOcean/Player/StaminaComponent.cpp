#include "StaminaComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"

UStaminaComponent::UStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();
	Stamina = MaxStamina;
	OnStaminaChanged.Broadcast(1.0f);
}

void UStaminaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(UpdateTimer);
	Timers.ClearTimer(RegenDelayTimer);
	Super::EndPlay(EndPlayReason);
}

void UStaminaComponent::SetDraining(bool bDrain)
{
	if (bDraining == bDrain)
	{
		return;
	}

	bDraining = bDrain;
	FTimerManager& Timers = GetWorld()->GetTimerManager();

	if (bDraining)
	{
		Timers.ClearTimer(RegenDelayTimer);
		StartUpdating();
	}
	else
	{
		// Pause, then regenerate. A lambda keeps this tiny callback next to where it's used,
		// and CreateWeakLambda makes it safe if this component is destroyed before the timer fires.
		Timers.ClearTimer(UpdateTimer);
		Timers.SetTimer(RegenDelayTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { StartUpdating(); }),
			FMath::Max(RegenDelay, KINDA_SMALL_NUMBER), false);
	}
}

bool UStaminaComponent::TryConsume(float Amount)
{
	if (!CanAfford(Amount))
	{
		return false;
	}

	SetStamina(Stamina - Amount);

	// Effort pauses recovery just like stopping a sprint does.
	if (!bDraining)
	{
		FTimerManager& Timers = GetWorld()->GetTimerManager();
		Timers.ClearTimer(UpdateTimer);
		Timers.SetTimer(RegenDelayTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { StartUpdating(); }),
			FMath::Max(RegenDelay, KINDA_SMALL_NUMBER), false);
	}
	return true;
}

void UStaminaComponent::ModifyStamina(float Amount)
{
	SetStamina(Stamina + Amount);
	if (!bDraining && Stamina < MaxStamina && !GetWorld()->GetTimerManager().IsTimerActive(RegenDelayTimer))
	{
		StartUpdating();
	}
}

void UStaminaComponent::StartUpdating()
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	if (!Timers.IsTimerActive(UpdateTimer))
	{
		Timers.SetTimer(UpdateTimer, this, &UStaminaComponent::UpdateStamina, UpdateInterval, true);
	}
}

void UStaminaComponent::UpdateStamina()
{
	const float Rate = bDraining ? -DrainRate : RegenRate;
	SetStamina(Stamina + Rate * UpdateInterval);

	// Fully recovered: nothing left to do until the next drain, so stop the timer.
	if (!bDraining && Stamina >= MaxStamina)
	{
		GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
	}
}

void UStaminaComponent::SetStamina(float NewStamina)
{
	NewStamina = FMath::Clamp(NewStamina, 0.0f, MaxStamina);
	if (FMath::IsNearlyEqual(NewStamina, Stamina))
	{
		return;
	}

	Stamina = NewStamina;

	if (Stamina <= 0.0f)
	{
		SetExhausted(true);
	}
	else if (bExhausted && Stamina >= MaxStamina * RecoverThreshold)
	{
		SetExhausted(false);
	}

	OnStaminaChanged.Broadcast(GetStaminaNormalized());
}

void UStaminaComponent::SetExhausted(bool bNewExhausted)
{
	if (bExhausted != bNewExhausted)
	{
		bExhausted = bNewExhausted;
		OnExhaustedChanged.Broadcast(bExhausted);
	}
}

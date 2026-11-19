#include "HealthComponent.h"

#include "DrowningDamageType.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace HealthCommands
{
	/**
	 * "eo.Damage <amount> [drown]" hurts the local player, for testing the damage screen effects.
	 * FAutoConsoleCommandWithWorldAndArgs registers itself at startup and is handed the world it was typed in.
	 */
	static FAutoConsoleCommandWithWorldAndArgs DamageCommand(
		TEXT("eo.Damage"),
		TEXT("Damage the local player. Usage: eo.Damage <amount> [drown]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
			if (!Pawn)
			{
				return;
			}

			const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.0f;
			const bool bDrown = Args.Num() > 1 && Args[1].Equals(TEXT("drown"), ESearchCase::IgnoreCase);
			const TSubclassOf<UDamageType> Type = bDrown ? UDrowningDamageType::StaticClass() : UDamageType::StaticClass();
			UGameplayStatics::ApplyDamage(Pawn, Amount, Pawn->GetController(), Pawn, Type);
		}),
		ECVF_Cheat);
}

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	GetOwner()->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
}

void UHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(RegenDelayTimer);
	Timers.ClearTimer(RegenTimer);
	Super::EndPlay(EndPlayReason);
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (bIsDead || Damage <= 0.0f)
	{
		return;
	}

	const float OldHealth = Health;
	Health = FMath::Max(Health - Damage, 0.0f);
	OnHealthChanged.Broadcast(this, Health, Health - OldHealth, DamageType);

	if (Health <= 0.0f)
	{
		bIsDead = true;
		GetWorld()->GetTimerManager().ClearTimer(RegenDelayTimer);
		GetWorld()->GetTimerManager().ClearTimer(RegenTimer);
		OnDeath.Broadcast(this, DamageCauser);
		return;
	}

	RestartRegenDelay();
}

void UHealthComponent::Heal(float Amount)
{
	if (bIsDead || Amount <= 0.0f || Health >= MaxHealth)
	{
		return;
	}

	const float OldHealth = Health;
	Health = FMath::Min(Health + Amount, MaxHealth);
	OnHealthChanged.Broadcast(this, Health, Health - OldHealth, nullptr);
}

void UHealthComponent::Revive()
{
	const float OldHealth = Health;
	bIsDead = false;
	Health = MaxHealth;
	OnHealthChanged.Broadcast(this, Health, Health - OldHealth, nullptr);
}

void UHealthComponent::RestartRegenDelay()
{
	if (!bRegenerate)
	{
		return;
	}

	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(RegenTimer);
	Timers.SetTimer(RegenDelayTimer, this, &UHealthComponent::StartRegenerating, FMath::Max(RegenDelay, KINDA_SMALL_NUMBER), false);
}

void UHealthComponent::StartRegenerating()
{
	GetWorld()->GetTimerManager().SetTimer(RegenTimer, this, &UHealthComponent::RegenerateTick, RegenInterval, true);
}

void UHealthComponent::RegenerateTick()
{
	const float Cap = MaxHealth * RegenCap;
	if (bIsDead || Health >= Cap)
	{
		GetWorld()->GetTimerManager().ClearTimer(RegenTimer);
		return;
	}

	Heal(FMath::Min(RegenRate * RegenInterval, Cap - Health));
}

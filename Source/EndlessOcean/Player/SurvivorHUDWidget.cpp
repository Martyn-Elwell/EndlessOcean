#include "SurvivorHUDWidget.h"

#include "BreathComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HeldItem.h"
#include "InteractionComponent.h"
#include "InventoryComponent.h"
#include "ItemHolderComponent.h"
#include "StaminaComponent.h"
#include "TimerManager.h"

void USurvivorHUDWidget::ObservePawn(APawn* Pawn)
{
	StopObserving();

	if (!Pawn)
	{
		return;
	}

	Stamina = Pawn->FindComponentByClass<UStaminaComponent>();
	Interaction = Pawn->FindComponentByClass<UInteractionComponent>();
	ItemHolder = Pawn->FindComponentByClass<UItemHolderComponent>();
	Inventory = Pawn->FindComponentByClass<UInventoryComponent>();

	if (UStaminaComponent* StaminaComp = Stamina.Get())
	{
		StaminaComp->OnStaminaChanged.AddDynamic(this, &USurvivorHUDWidget::HandleStaminaChanged);
		StaminaComp->OnExhaustedChanged.AddDynamic(this, &USurvivorHUDWidget::HandleExhaustedChanged);
		HandleStaminaChanged(StaminaComp->GetStaminaNormalized());
		HandleExhaustedChanged(StaminaComp->IsExhausted());
	}

	if (UInteractionComponent* InteractionComp = Interaction.Get())
	{
		InteractionComp->OnFocusChanged.AddDynamic(this, &USurvivorHUDWidget::HandleFocusChanged);
		HandleFocusChanged(InteractionComp->GetFocusedActor(), nullptr);
	}

	if (UItemHolderComponent* HolderComp = ItemHolder.Get())
	{
		HolderComp->OnHeldItemChanged.AddDynamic(this, &USurvivorHUDWidget::HandleHeldItemChanged);
		HandleHeldItemChanged(HolderComp->GetHeldItem(), nullptr);
	}

	if (UInventoryComponent* InventoryComp = Inventory.Get())
	{
		InventoryComp->OnInventoryChanged.AddDynamic(this, &USurvivorHUDWidget::HandleInventoryChanged);
		HandleInventoryChanged();
	}

	Breath = Pawn->FindComponentByClass<UBreathComponent>();
	if (UBreathComponent* BreathComp = Breath.Get())
	{
		BreathComp->OnBreathChanged.AddDynamic(this, &USurvivorHUDWidget::HandleBreathChanged);
		BreathComp->OnSubmergedChanged.AddDynamic(this, &USurvivorHUDWidget::HandleSubmergedChanged);
		HandleBreathChanged(BreathComp->GetBreathNormalized());
	}
}

void USurvivorHUDWidget::HandleBreathChanged(float NormalizedBreath)
{
	const UBreathComponent* BreathComp = Breath.Get();
	const bool bVisible = BreathComp && (BreathComp->IsSubmerged() || NormalizedBreath < 1.0f);
	OnBreathChanged(NormalizedBreath, bVisible);
}

void USurvivorHUDWidget::HandleSubmergedChanged(bool bSubmerged)
{
	if (const UBreathComponent* BreathComp = Breath.Get())
	{
		HandleBreathChanged(BreathComp->GetBreathNormalized());
	}
}

void USurvivorHUDWidget::NativeDestruct()
{
	StopObserving();
	Super::NativeDestruct();
}

void USurvivorHUDWidget::StopObserving()
{
	// RemoveDynamic needs the same object + function that AddDynamic used; safe to call even if never bound.
	if (UStaminaComponent* StaminaComp = Stamina.Get())
	{
		StaminaComp->OnStaminaChanged.RemoveDynamic(this, &USurvivorHUDWidget::HandleStaminaChanged);
		StaminaComp->OnExhaustedChanged.RemoveDynamic(this, &USurvivorHUDWidget::HandleExhaustedChanged);
	}
	if (UInteractionComponent* InteractionComp = Interaction.Get())
	{
		InteractionComp->OnFocusChanged.RemoveDynamic(this, &USurvivorHUDWidget::HandleFocusChanged);
	}
	if (UItemHolderComponent* HolderComp = ItemHolder.Get())
	{
		HolderComp->OnHeldItemChanged.RemoveDynamic(this, &USurvivorHUDWidget::HandleHeldItemChanged);
	}
	if (UInventoryComponent* InventoryComp = Inventory.Get())
	{
		InventoryComp->OnInventoryChanged.RemoveDynamic(this, &USurvivorHUDWidget::HandleInventoryChanged);
	}
	if (AFlareItem* Flare = WatchedFlare.Get())
	{
		Flare->OnFlareStateChanged.RemoveDynamic(this, &USurvivorHUDWidget::HandleFlareStateChanged);
	}
	if (UBreathComponent* BreathComp = Breath.Get())
	{
		BreathComp->OnBreathChanged.RemoveDynamic(this, &USurvivorHUDWidget::HandleBreathChanged);
		BreathComp->OnSubmergedChanged.RemoveDynamic(this, &USurvivorHUDWidget::HandleSubmergedChanged);
	}

	Breath.Reset();
	Stamina.Reset();
	Interaction.Reset();
	ItemHolder.Reset();
	Inventory.Reset();
	WatchedFlare.Reset();

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FlareTimer);
	}
}

void USurvivorHUDWidget::HandleStaminaChanged(float NormalizedStamina)
{
	OnStaminaChanged(NormalizedStamina);
}

void USurvivorHUDWidget::HandleExhaustedChanged(bool bExhausted)
{
	OnExhaustedChanged(bExhausted);
}

void USurvivorHUDWidget::HandleFocusChanged(AActor* NewFocus, AActor* OldFocus)
{
	const UInteractionComponent* InteractionComp = Interaction.Get();
	OnInteractionPromptChanged(NewFocus != nullptr, InteractionComp ? InteractionComp->GetFocusedPrompt() : FText::GetEmpty());
}

void USurvivorHUDWidget::HandleHeldItemChanged(AHeldItem* NewItem, AHeldItem* OldItem)
{
	OnHeldItemChanged(NewItem, NewItem ? NewItem->GetDisplayName() : FText::GetEmpty());

	// Follow the flare in hand (if any) so the burn timer appears and disappears with it.
	if (AFlareItem* OldFlare = WatchedFlare.Get())
	{
		OldFlare->OnFlareStateChanged.RemoveDynamic(this, &USurvivorHUDWidget::HandleFlareStateChanged);
	}
	WatchedFlare = Cast<AFlareItem>(NewItem);
	if (AFlareItem* NewFlare = WatchedFlare.Get())
	{
		NewFlare->OnFlareStateChanged.AddDynamic(this, &USurvivorHUDWidget::HandleFlareStateChanged);
	}

	RefreshFlareTimer();

	// The active slot changes with the held item, so refresh the slot highlight too.
	HandleInventoryChanged();
}

void USurvivorHUDWidget::HandleInventoryChanged()
{
	if (const UInventoryComponent* InventoryComp = Inventory.Get())
	{
		const UItemHolderComponent* HolderComp = ItemHolder.Get();
		OnInventoryChanged(InventoryComp->GetSlots(), HolderComp ? HolderComp->GetActiveSlot() : INDEX_NONE);
	}
}

void USurvivorHUDWidget::HandleFlareStateChanged(AFlareItem* Flare, EFlareState NewState)
{
	RefreshFlareTimer();
}

void USurvivorHUDWidget::RefreshFlareTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const AFlareItem* Flare = WatchedFlare.Get();
	if (Flare && Flare->IsBurning())
	{
		if (!World->GetTimerManager().IsTimerActive(FlareTimer))
		{
			World->GetTimerManager().SetTimer(FlareTimer, this, &USurvivorHUDWidget::UpdateFlareTimer, FlareTimerInterval, true);
		}
		UpdateFlareTimer();
	}
	else
	{
		World->GetTimerManager().ClearTimer(FlareTimer);
		OnFlareTimeChanged(false, 0.0f, 0.0f);
	}
}

void USurvivorHUDWidget::UpdateFlareTimer()
{
	if (const AFlareItem* Flare = WatchedFlare.Get())
	{
		OnFlareTimeChanged(true, Flare->GetBurnTimeRemaining(), Flare->GetBurnTimeRemainingNormalized());
	}
}

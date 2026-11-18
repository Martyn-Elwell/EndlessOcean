#include "InteractionComponent.h"

#include "Interactable.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

namespace InteractionCVars
{
	/** Console variable: "eo.Interaction.Debug 1" draws the focus traces. Compiled out of shipping builds by ENABLE_DRAW_DEBUG. */
	static TAutoConsoleVariable<bool> CVarDebug(
		TEXT("eo.Interaction.Debug"),
		false,
		TEXT("Draw interaction focus traces (green = interactable hit, red = nothing)."),
		ECVF_Cheat);
}

UInteractionComponent::UInteractionComponent()
{
	// Timer-driven: no Tick needed.
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// Repeating timer: Unreal's equivalent of InvokeRepeating / a coroutine with WaitForSeconds.
	GetWorld()->GetTimerManager().SetTimer(TraceTimer, this, &UInteractionComponent::UpdateFocus, TraceInterval, true);
}

void UInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(TraceTimer);
	Super::EndPlay(EndPlayReason);
}

bool UInteractionComponent::TryInteract()
{
	// Refresh first so we never act on a focus that's ~60ms out of date.
	UpdateFocus();

	AActor* Target = FocusedActor.Get();
	if (!Target)
	{
		return false;
	}

	IInteractable::Execute_Interact(Target, GetOwner());

	// The interaction may have changed the target (e.g. it's now in our hand), so re-check right away.
	UpdateFocus();
	return true;
}

void UInteractionComponent::UpdateFocus()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	// Prefer the controller's view point: for a player that's the camera manager's final view (after camera modifiers).
	FVector ViewLocation;
	FRotator ViewRotation;
	const APawn* Pawn = Cast<APawn>(Owner);
	if (Pawn && Pawn->GetController())
	{
		Pawn->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		Owner->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	}

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * TraceDistance;

	// SCENE_QUERY_STAT names the trace for the profiler; ignore our own capsule.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionTrace), false, Owner);

	FHitResult Hit;
	const bool bHit = World->SweepSingleByChannel(Hit, ViewLocation, TraceEnd, FQuat::Identity, TraceChannel,
		FCollisionShape::MakeSphere(TraceRadius), Params);

	// The first blocking hit wins, so walls correctly occlude items behind them.
	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
	AActor* NewFocus = nullptr;
	if (HitActor && HitActor->Implements<UInteractable>() && IInteractable::Execute_CanInteract(HitActor, Owner))
	{
		NewFocus = HitActor;
	}

#if ENABLE_DRAW_DEBUG
	if (InteractionCVars::CVarDebug.GetValueOnGameThread())
	{
		const FColor Color = NewFocus ? FColor::Green : FColor::Red;
		DrawDebugLine(World, ViewLocation, bHit ? Hit.Location : TraceEnd, Color, false, TraceInterval);
		if (bHit)
		{
			DrawDebugSphere(World, Hit.Location, FMath::Max(TraceRadius, 2.0f), 8, Color, false, TraceInterval);
		}
	}
#endif

	SetFocusedActor(NewFocus);
}

FText UInteractionComponent::GetFocusedPrompt() const
{
	if (const AActor* Target = FocusedActor.Get())
	{
		return IInteractable::Execute_GetInteractionPrompt(Target);
	}
	return FText::GetEmpty();
}

void UInteractionComponent::SetFocusedActor(AActor* NewFocus)
{
	AActor* OldFocus = FocusedActor.Get();
	if (OldFocus != NewFocus)
	{
		FocusedActor = NewFocus;
		OnFocusChanged.Broadcast(NewFocus, OldFocus);
	}
}

#include "InteractionComponent.h"

#include "Interactable.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
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

	const FVector ViewDirection = ViewRotation.Vector();
	const FVector TraceEnd = ViewLocation + ViewDirection * TraceDistance;

	// SCENE_QUERY_STAT names the trace for the profiler; ignore our own capsule.
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionTrace), false, Owner);

	// Pass 1: the first blocking hit wins, so walls correctly occlude items behind them.
	FHitResult Hit;
	const bool bHit = World->SweepSingleByChannel(Hit, ViewLocation, TraceEnd, FQuat::Identity, TraceChannel,
		FCollisionShape::MakeSphere(TraceRadius), Params);

	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
	AActor* NewFocus = IsUsable(HitActor) ? HitActor : nullptr;

	// Pass 2: nothing usable straight ahead, so look for the closest near-miss.
	const bool bAssisted = !NewFocus && bAimAssist;
	if (bAssisted)
	{
		NewFocus = FindAssistTarget(ViewLocation, ViewDirection, Params);
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
		if (bAssisted && NewFocus)
		{
			// Yellow box: picked by aim assist rather than hit directly.
			const FBox Bounds = NewFocus->GetComponentsBoundingBox(true);
			DrawDebugBox(World, Bounds.GetCenter(), Bounds.GetExtent(), FColor::Yellow, false, TraceInterval);
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

bool UInteractionComponent::IsUsable(AActor* Actor) const
{
	return Actor && Actor->Implements<UInteractable>() && IInteractable::Execute_CanInteract(Actor, GetOwner());
}

AActor* UInteractionComponent::FindAssistTarget(const FVector& ViewLocation, const FVector& ViewDirection,
	const FCollisionQueryParams& Params) const
{
	const UWorld* World = GetWorld();

	// Everything on the interaction channel within reach. Overlap queries report blocking and overlapping
	// components alike, so this includes walls and floors; those are filtered out by IsUsable below.
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(Overlaps, ViewLocation, FQuat::Identity, TraceChannel,
		FCollisionShape::MakeSphere(TraceDistance), Params);

	struct FCandidate
	{
		AActor* Actor;
		FVector AimPoint;
		float Score;
	};
	// TInlineAllocator keeps the first 8 entries on the stack: no heap allocation in the usual case.
	// (Unity has nothing quite like it; think of a List<T> with a pre-sized internal buffer that lives on the stack.)
	TArray<FCandidate, TInlineAllocator<8>> Candidates;

	const AActor* CurrentFocus = FocusedActor.Get();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		// An actor with several colliding parts shows up once per part.
		if (!IsUsable(Actor) || Candidates.ContainsByPredicate([Actor](const FCandidate& C) { return C.Actor == Actor; }))
		{
			continue;
		}

		// Bounds of every part, including visual-only ones with no collision (the pistol's grip, for example),
		// so aiming at any visible bit of the item counts.
		const FBox Bounds = Actor->GetComponentsBoundingBox(true);
		if (!Bounds.IsValid)
		{
			continue;
		}

		// Distance along the view ray to the point level with the item's centre. Behind us = not a candidate.
		const float Along = FVector::DotProduct(Bounds.GetCenter() - ViewLocation, ViewDirection);
		if (Along <= 0.0f)
		{
			continue;
		}

		// How far the ray misses the item's box there, as a distance and as an angle seen from the eye.
		const FVector RayPoint = ViewLocation + ViewDirection * FMath::Min(Along, TraceDistance);
		const float Miss = FMath::Sqrt(Bounds.ComputeSquaredDistanceToPoint(RayPoint));
		const float MissAngle = FMath::RadiansToDegrees(FMath::Atan2(Miss, Along));

		// The item already in focus gets looser limits and a better score, so the focus doesn't flicker.
		const float Leniency = Actor == CurrentFocus ? FocusStickiness : 1.0f;
		if (MissAngle > AssistAngle * Leniency && Miss > AssistRadius * Leniency)
		{
			continue;
		}

		Candidates.Add({Actor, Bounds.GetCenter(), MissAngle / Leniency});
	}

	// Best (smallest miss) first, then take the first one we can actually see.
	Candidates.Sort([](const FCandidate& A, const FCandidate& B) { return A.Score < B.Score; });
	for (const FCandidate& Candidate : Candidates)
	{
		FHitResult Hit;
		const bool bBlocked = World->LineTraceSingleByChannel(Hit, ViewLocation, Candidate.AimPoint, TraceChannel, Params)
			&& Hit.GetActor() != Candidate.Actor;
		if (!bBlocked)
		{
			return Candidate.Actor;
		}
	}
	return nullptr;
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

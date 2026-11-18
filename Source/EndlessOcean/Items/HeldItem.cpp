#include "HeldItem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "ItemDefinition.h"
#include "ItemHolderComponent.h"

#define LOCTEXT_NAMESPACE "HeldItem"

AHeldItem::AHeldItem()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// PhysicsActor: simulates, blocks the world and (by default) blocks the Interaction trace channel.
	Mesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Mesh->SetSimulatePhysics(true);

	// Continuous collision detection: small, fast thrown items would otherwise tunnel through thin walls.
	Mesh->BodyInstance.bUseCCD = true;
}

FText AHeldItem::GetDisplayName() const
{
	if (Definition && !Definition->DisplayName.IsEmpty())
	{
		return Definition->DisplayName;
	}
	return FText::FromString(GetName());
}

void AHeldItem::SetHeld(UItemHolderComponent* InHolder, USceneComponent* HandAnchor)
{
	check(HandAnchor);

	Holder = InHolder;
	SetPhysicalInWorld(false);
	SetActorHiddenInGame(false);

	// Snap location and rotation to the anchor but keep our world scale (the placeholder mesh is scaled on the root).
	const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
	AttachToComponent(HandAnchor, Rules);

	if (Definition)
	{
		SetActorRelativeLocation(Definition->HeldLocationOffset);
		SetActorRelativeRotation(Definition->HeldRotationOffset);
	}

	SetItemLocation(EHeldItemLocation::Held);
}

void AHeldItem::SetStowed(USceneComponent* AttachParent)
{
	Holder.Reset();
	SetPhysicalInWorld(false);
	SetActorHiddenInGame(true);

	if (AttachParent)
	{
		const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
		AttachToComponent(AttachParent, Rules);
	}

	SetItemLocation(EHeldItemLocation::Stowed);
}

void AHeldItem::ReleaseToWorld(const FVector& LinearVelocity, const FVector& AngularVelocityDegrees)
{
	Holder.Reset();
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorHiddenInGame(false);
	SetPhysicalInWorld(true);

	Mesh->SetPhysicsLinearVelocity(LinearVelocity);
	Mesh->SetPhysicsAngularVelocityInDegrees(AngularVelocityDegrees);

	SetItemLocation(EHeldItemLocation::World);
}

bool AHeldItem::CanInteract_Implementation(AActor* InteractInstigator) const
{
	return ItemLocation == EHeldItemLocation::World
		&& CanBePickedUp()
		&& InteractInstigator
		&& InteractInstigator->FindComponentByClass<UItemHolderComponent>() != nullptr;
}

void AHeldItem::Interact_Implementation(AActor* InteractInstigator)
{
	if (UItemHolderComponent* InstigatorHolder = InteractInstigator ? InteractInstigator->FindComponentByClass<UItemHolderComponent>() : nullptr)
	{
		InstigatorHolder->PickUp(this);
	}
}

FText AHeldItem::GetInteractionPrompt_Implementation() const
{
	return FText::Format(LOCTEXT("PickUpPrompt", "Pick up {0}"), GetDisplayName());
}

void AHeldItem::OnItemLocationUpdated(EHeldItemLocation NewLocation)
{
}

void AHeldItem::SetPhysicalInWorld(bool bInWorld)
{
	if (bInWorld)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetSimulatePhysics(true);
	}
	else
	{
		// Physics must be off before attaching, or the body would keep simulating and ignore the attachment.
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AHeldItem::SetItemLocation(EHeldItemLocation NewLocation)
{
	ItemLocation = NewLocation;
	OnItemLocationUpdated(NewLocation);
	OnItemLocationChanged.Broadcast(this, NewLocation);
}

#undef LOCTEXT_NAMESPACE

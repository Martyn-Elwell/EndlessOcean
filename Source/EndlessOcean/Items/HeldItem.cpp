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

bool AHeldItem::IsUsableWhileSwimming() const
{
	return Definition && Definition->bUsableWhileSwimming;
}

void AHeldItem::AltActionStarted_Implementation()
{
	// Default right-click: throw. Most items have nothing better to do with it.
	if (UItemHolderComponent* CurrentHolder = GetHolder())
	{
		CurrentHolder->ThrowHeldItem();
	}
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
		// Give the parts their collision back first, so they weld into the root's body when it starts simulating.
		for (const FSuspendedCollision& Suspended : SuspendedCollision)
		{
			if (UPrimitiveComponent* Part = Suspended.Component.Get())
			{
				Part->SetCollisionEnabled(Suspended.Collision);
			}
		}
		SuspendedCollision.Reset();

		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetSimulatePhysics(true);
	}
	else
	{
		// Physics must be off before attaching, or the body would keep simulating and ignore the attachment.
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Every other part goes quiet too. Only parts that still collide are recorded, so a second call (held, then
		// stowed) doesn't overwrite the saved settings with NoCollision.
		TInlineComponentArray<UPrimitiveComponent*> Parts(this);
		for (UPrimitiveComponent* Part : Parts)
		{
			if (Part != Mesh && Part->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			{
				SuspendedCollision.Add({Part, Part->GetCollisionEnabled()});
				Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}
}

void AHeldItem::SetItemLocation(EHeldItemLocation NewLocation)
{
	ItemLocation = NewLocation;
	OnItemLocationUpdated(NewLocation);
	OnItemLocationChanged.Broadcast(this, NewLocation);
}

#undef LOCTEXT_NAMESPACE

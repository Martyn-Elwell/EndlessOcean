#include "AmmoPickup.h"

#include "AmmoPouchComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "ItemTags.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "AmmoPickup"

AAmmoPickup::AAmmoPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// A small brass-ish cylinder: about 1.5 x 6 cm, deliberately a bit oversized so it can be found in the dark.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetRelativeScale3D(FVector(0.025f, 0.025f, 0.08f));
	Mesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Mesh->SetSimulatePhysics(true);
	Mesh->BodyInstance.bUseCCD = true;
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, ShapeMaterial.Object);
	}

	AmmoType = EndlessOceanTags::Ammo_SPP1;
	DisplayName = LOCTEXT("DefaultName", "SPP-1 round");
}

bool AAmmoPickup::CanInteract_Implementation(AActor* InteractInstigator) const
{
	// Stay interactable even when the pouch is full, so the prompt can explain why nothing happens.
	return Count > 0 && InteractInstigator && InteractInstigator->FindComponentByClass<UAmmoPouchComponent>() != nullptr;
}

void AAmmoPickup::Interact_Implementation(AActor* InteractInstigator)
{
	UAmmoPouchComponent* Pouch = InteractInstigator ? InteractInstigator->FindComponentByClass<UAmmoPouchComponent>() : nullptr;
	if (!Pouch)
	{
		return;
	}

	Count -= Pouch->Add(AmmoType, Count);
	if (Count <= 0)
	{
		Destroy();
	}
}

FText AAmmoPickup::GetInteractionPrompt_Implementation() const
{
	// The interface doesn't pass the looker, and this is a single-player game, so ask the local player's pouch.
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const UAmmoPouchComponent* Pouch = Player ? Player->FindComponentByClass<UAmmoPouchComponent>() : nullptr;
	if (Pouch && !Pouch->HasRoomFor(AmmoType))
	{
		return LOCTEXT("PouchFull", "Ammo pouch full");
	}

	return Count > 1
		? FText::Format(LOCTEXT("PickUpMany", "Pick up {0} x{1}"), DisplayName, Count)
		: FText::Format(LOCTEXT("PickUpOne", "Pick up {0}"), DisplayName);
}

#undef LOCTEXT_NAMESPACE

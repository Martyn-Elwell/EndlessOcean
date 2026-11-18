#include "FlareItem.h"

#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "ItemHolderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "FlareItem"

namespace FlareItem
{
	/** Vector parameter exposed by the engine's BasicShapeMaterial. */
	static const FName ColorParam(TEXT("Color"));
}

AFlareItem::AFlareItem()
{
	/*
	 * ConstructorHelpers::FObjectFinder loads an asset by path while the class default object is built.
	 * It's fine for engine placeholder shapes. For real art, leave these empty in C++ and set them in the
	 * Blueprint instead, so artists can change them without a recompile.
	 */
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// The engine cylinder is 100 x 100 cm. Scaling the root gives a ~4 x 25 cm flare; children inherit the scale,
	// so their offsets below are in the cylinder's unscaled space (top face at Z = 50).
	Mesh->SetRelativeScale3D(FVector(0.04f, 0.04f, 0.25f));
	Mesh->SetCastShadow(false); // the light sits right on top of the flare; a shadow from its own body looks wrong
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, ShapeMaterial.Object);
	}

	CapMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CapMesh"));
	CapMesh->SetupAttachment(Mesh);
	CapMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	CapMesh->SetRelativeScale3D(FVector(1.15f, 1.15f, 0.15f));
	CapMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapMesh->SetCastShadow(false);
	if (CylinderMesh.Succeeded())
	{
		CapMesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		CapMesh->SetMaterial(0, ShapeMaterial.Object);
	}

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Mesh);
	Light->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetVisibility(false);
	Light->SetCastShadows(true);

	BurnAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("BurnAudio"));
	BurnAudio->SetupAttachment(Mesh);
	BurnAudio->bAutoActivate = false;
}

void AFlareItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// OnConstruction runs in the editor too (like Unity's OnValidate), so colour and light edits show without playing.
	Light->SetLightColor(LightColor);
	Light->SetIntensity(LightIntensity);
	Light->SetAttenuationRadius(LightRadius);

	BodyMaterial = Mesh->CreateDynamicMaterialInstance(0);
	CapMaterial = CapMesh->CreateDynamicMaterialInstance(0);
	ApplyColors(BodyColor, CapColor);
}

void AFlareItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(BurnTimer);
	Timers.ClearTimer(LightUpdateTimer);
	Super::EndPlay(EndPlayReason);
}

void AFlareItem::Ignite()
{
	if (FlareState != EFlareState::Unlit)
	{
		return;
	}

	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.SetTimer(BurnTimer, this, &AFlareItem::BurnOut, BurnDuration, false);
	Timers.SetTimer(LightUpdateTimer, this, &AFlareItem::UpdateLight, LightUpdateInterval, true);

	// The cap glows in the light colour while burning.
	ApplyColors(BodyColor, LightColor);
	Light->SetVisibility(true);

	if (IgniteSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, IgniteSound, GetActorLocation());
	}
	BurnAudio->Play();

	// Set the state before the first light update so GetBurnTimeRemaining reads the running timer.
	SetFlareState(EFlareState::Burning);
	UpdateLight();
}

float AFlareItem::GetBurnTimeRemaining() const
{
	switch (FlareState)
	{
	case EFlareState::Unlit:
		return BurnDuration;
	case EFlareState::Burning:
		return FMath::Max(GetWorld()->GetTimerManager().GetTimerRemaining(BurnTimer), 0.0f);
	default:
		return 0.0f;
	}
}

float AFlareItem::GetBurnTimeRemainingNormalized() const
{
	return BurnDuration > 0.0f ? GetBurnTimeRemaining() / BurnDuration : 0.0f;
}

void AFlareItem::UsePrimary_Implementation()
{
	Ignite();
}

bool AFlareItem::CanBePickedUp_Implementation() const
{
	return FlareState != EFlareState::BurntOut;
}

bool AFlareItem::CanBeStowed_Implementation() const
{
	return FlareState != EFlareState::Burning;
}

FText AFlareItem::GetInteractionPrompt_Implementation() const
{
	if (FlareState == EFlareState::Burning)
	{
		return FText::Format(LOCTEXT("PickUpBurning", "Pick up {0} (burning)"), GetDisplayName());
	}
	return Super::GetInteractionPrompt_Implementation();
}

void AFlareItem::OnItemLocationUpdated(EHeldItemLocation NewLocation)
{
	Super::OnItemLocationUpdated(NewLocation);

	// Burnt-out flares lying around get cleaned up after a while.
	if (FlareState == EFlareState::BurntOut && NewLocation == EHeldItemLocation::World && BurntOutLifeSpan > 0.0f)
	{
		SetLifeSpan(BurntOutLifeSpan);
	}
}

void AFlareItem::TogglePreviewLight()
{
	Light->SetIntensity(LightIntensity);
	Light->SetVisibility(!Light->IsVisible());
}

void AFlareItem::BurnOut()
{
	GetWorld()->GetTimerManager().ClearTimer(LightUpdateTimer);

	Light->SetVisibility(false);
	BurnAudio->Stop();
	ApplyColors(BurntOutColor, BurntOutColor);

	if (BurnOutSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BurnOutSound, GetActorLocation());
	}

	SetFlareState(EFlareState::BurntOut);

	UItemHolderComponent* CurrentHolder = GetHolder();
	if (bAutoDropWhenBurntOut && CurrentHolder)
	{
		CurrentHolder->DropHeldItem(); // ends in OnItemLocationUpdated(World), which starts the cleanup life span
	}
	else if (GetItemLocation() == EHeldItemLocation::World && BurntOutLifeSpan > 0.0f)
	{
		SetLifeSpan(BurntOutLifeSpan); // burnt out while lying on the floor
	}
}

void AFlareItem::UpdateLight()
{
	const float Remaining = GetBurnTimeRemaining();
	const float Fade = FadeOutTime > 0.0f ? FMath::Clamp(Remaining / FadeOutTime, 0.0f, 1.0f) : 1.0f;

	float Flicker = 1.0f;
	if (bFlicker)
	{
		// Perlin noise gives a smooth but irregular wobble (random numbers each update would just strobe).
		// The flicker doubles while fading so the flare sputters as it dies.
		const float Noise = FMath::PerlinNoise1D(GetWorld()->GetTimeSeconds() * FlickerSpeed);
		const float Amount = FlickerAmount * FMath::Lerp(2.0f, 1.0f, Fade);
		Flicker = FMath::Max(1.0f + Noise * Amount, 0.0f);
	}

	Light->SetIntensity(LightIntensity * Fade * Flicker);
}

void AFlareItem::ApplyColors(const FLinearColor& Body, const FLinearColor& Cap)
{
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(FlareItem::ColorParam, Body);
	}
	if (CapMaterial)
	{
		CapMaterial->SetVectorParameterValue(FlareItem::ColorParam, Cap);
	}
}

void AFlareItem::SetFlareState(EFlareState NewState)
{
	FlareState = NewState;
	OnFlareStateChanged.Broadcast(this, NewState);
}

#undef LOCTEXT_NAMESPACE

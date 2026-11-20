#include "TargetDummy.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HealthComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace TargetDummy
{
	static const FName ColorParam(TEXT("Color"));
	static const FLinearColor IdleColor(0.3f, 0.3f, 0.3f);
	static const FLinearColor HitColor(0.8f, 0.05f, 0.05f);
	static const FLinearColor DeadColor(0.05f, 0.05f, 0.05f);
}

ATargetDummy::ATargetDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// A person-sized pillar (the engine cylinder is 100 x 100 cm).
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.8f));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, ShapeMaterial.Object);
	}

	Readout = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Readout"));
	Readout->SetupAttachment(Mesh);
	// Positioned above the pillar; the mesh's scale is undone so the text isn't stretched.
	Readout->SetUsingAbsoluteScale(true);
	Readout->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	Readout->SetHorizontalAlignment(EHTA_Center);
	Readout->SetWorldSize(18.0f);
	Readout->SetTextRenderColor(FColor::White);

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
}

void ATargetDummy::BeginPlay()
{
	Super::BeginPlay();

	MeshMaterial = Mesh->CreateDynamicMaterialInstance(0);
	SetColor(TargetDummy::IdleColor);

	Health->OnHealthChanged.AddDynamic(this, &ATargetDummy::HandleHealthChanged);
	Health->OnDeath.AddDynamic(this, &ATargetDummy::HandleDeath);
	UpdateReadout();
}

void ATargetDummy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(FlashTimer);
	Timers.ClearTimer(ReviveTimer);
	Super::EndPlay(EndPlayReason);
}

void ATargetDummy::HandleHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta, const UDamageType* DamageType)
{
	if (Delta < 0.0f)
	{
		LastDamage = -Delta;
		SetColor(TargetDummy::HitColor);
		GetWorld()->GetTimerManager().SetTimer(FlashTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			SetColor(Health->IsDead() ? TargetDummy::DeadColor : TargetDummy::IdleColor);
		}), FlashTime, false);
	}
	UpdateReadout();
}

void ATargetDummy::HandleDeath(UHealthComponent* HealthComponent, AActor* DamageCauser)
{
	GetWorld()->GetTimerManager().SetTimer(ReviveTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		Health->Revive();
		SetColor(TargetDummy::IdleColor);
	}), FMath::Max(ReviveDelay, KINDA_SMALL_NUMBER), false);
	UpdateReadout();
}

void ATargetDummy::UpdateReadout()
{
	const FString Text = Health->IsDead()
		? FString::Printf(TEXT("DEAD (last hit %.0f)"), LastDamage)
		: FString::Printf(TEXT("%.0f / %.0f  (last hit %.0f)"), Health->GetHealth(), Health->GetMaxHealth(), LastDamage);
	Readout->SetText(FText::FromString(Text));
}

void ATargetDummy::SetColor(const FLinearColor& Color)
{
	if (MeshMaterial)
	{
		MeshMaterial->SetVectorParameterValue(TargetDummy::ColorParam, Color);
	}
}

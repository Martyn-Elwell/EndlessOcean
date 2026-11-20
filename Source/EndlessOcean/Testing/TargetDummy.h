#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TargetDummy.generated.h"

class UDamageType;
class UHealthComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * A grey pillar with health, for tuning weapons. It shows its health and the last hit above itself, flashes red
 * when hit, and revives a few seconds after "dying". Test levels only; not part of the game.
 */
UCLASS(Blueprintable, PrioritizeCategories=(TargetDummy))
class ENDLESSOCEAN_API ATargetDummy : public AActor
{
	GENERATED_BODY()

public:

	ATargetDummy();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UTextRenderComponent> Readout;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UHealthComponent> Health;

	/** Time after "dying" before the dummy comes back at full health. */
	UPROPERTY(EditAnywhere, Category="TargetDummy", meta=(ClampMin=0, ClampMax=30, Units="s"))
	float ReviveDelay = 3.0f;

	/** How long the red hit flash lasts. */
	UPROPERTY(EditAnywhere, Category="TargetDummy", meta=(ClampMin=0.01, ClampMax=2, Units="s"))
	float FlashTime = 0.15f;

private:

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MeshMaterial;

	float LastDamage = 0.0f;
	FTimerHandle FlashTimer;
	FTimerHandle ReviveTimer;

	UFUNCTION()
	void HandleHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta, const UDamageType* DamageType);

	UFUNCTION()
	void HandleDeath(UHealthComponent* HealthComponent, AActor* DamageCauser);

	void UpdateReadout();
	void SetColor(const FLinearColor& Color);
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SurvivorCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInteractionComponent;
class UInventoryComponent;
class UItemHolderComponent;
class UStaminaComponent;
class USurvivorMovementComponent;
struct FInputActionValue;

/**
 * The player: an ordinary ship worker. Slow, can't jump, can crouch, has a short stamina sprint,
 * and carries one item in hand plus a few stowed.
 *
 * The character is deliberately thin. Behaviour lives in components (stamina, interaction, hand, inventory) and
 * in the movement component; this class creates them, wires them together with delegates, and turns input
 * into calls on them. New systems (oxygen, hiding, health) should be new components too.
 *
 * Make a Blueprint child (BP_SurvivorCharacter) to assign Input Actions and tune values.
 */
UCLASS(Abstract)
class ENDLESSOCEAN_API ASurvivorCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	/**
	 * Takes an FObjectInitializer so we can replace ACharacter's default movement component with our own
	 * before it's created. See the .cpp for details.
	 */
	explicit ASurvivorCharacter(const FObjectInitializer& ObjectInitializer);

	/** First-person camera. */
	UFUNCTION(BlueprintPure, Category="Survivor")
	UCameraComponent* GetCamera() const { return Camera; }

	/** Our movement component, already cast. */
	UFUNCTION(BlueprintPure, Category="Survivor")
	USurvivorMovementComponent* GetSurvivorMovement() const;

	UFUNCTION(BlueprintPure, Category="Survivor")
	UStaminaComponent* GetStamina() const { return Stamina; }

	UFUNCTION(BlueprintPure, Category="Survivor")
	UInteractionComponent* GetInteraction() const { return Interaction; }

	UFUNCTION(BlueprintPure, Category="Survivor")
	UItemHolderComponent* GetItemHolder() const { return ItemHolder; }

	UFUNCTION(BlueprintPure, Category="Survivor")
	UInventoryComponent* GetInventory() const { return Inventory; }

protected:

	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Never allow jumping, whatever calls Jump(). */
	virtual bool CanJumpInternal_Implementation() const override { return false; }

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	// --- Components ---

	/** First-person camera, at eye height on the capsule. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> Camera;

	/** Where held items attach. Move it in the Blueprint viewport to change where items sit on screen. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> HandAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaminaComponent> Stamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UInteractionComponent> Interaction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UItemHolderComponent> ItemHolder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UInventoryComponent> Inventory;

	// --- Input ---

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Gamepad look. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> LookAction;

	/** Mouse look. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> InteractAction;

	/** Use the held item (light a flare). */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> UseItemAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ThrowItemAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> DropItemAction;

	/** Put the held item away (empty hands). */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> StowItemAction;

	/** Axis1D (mouse wheel): positive = next slot, negative = previous. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> CycleInventoryAction;

	/** One action per inventory slot, in order (element 0 = key 1). */
	UPROPERTY(EditAnywhere, Category="Input")
	TArray<TObjectPtr<UInputAction>> SelectSlotActions;

	// --- Tuning ---

	/** If true, the crouch key toggles crouching; if false, crouch only while held. */
	UPROPERTY(EditAnywhere, Category="Crouch")
	bool bToggleCrouch = true;

	/** How quickly the camera moves between standing and crouched height. */
	UPROPERTY(EditAnywhere, Category="Crouch", meta=(ClampMin=1, ClampMax=40))
	float CrouchCameraInterpSpeed = 10.0f;

private:

	// Input handlers. Each does one small thing and hands off to a component.
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleSprintStarted();
	void HandleSprintCompleted();
	void HandleCrouchStarted();
	void HandleCrouchCompleted();
	void HandleInteract();
	void HandleUseItem();
	void HandleThrowItem();
	void HandleDropItem();
	void HandleStowItem();
	void HandleCycleInventory(const FInputActionValue& Value);
	void HandleSelectSlot(int32 SlotIndex);

	/** Movement started/stopped sprinting: drain or regenerate stamina. */
	UFUNCTION()
	void HandleSprintStateChanged(bool bIsSprinting);

	/** Out of stamina, or recovered: block or allow sprinting. */
	UFUNCTION()
	void HandleExhaustedChanged(bool bExhausted);

	/** Camera height when standing, read from the camera's position in the Blueprint. */
	float StandingCameraHeight = 0.0f;

	/** Camera height we're blending towards (relative to the capsule centre). */
	float TargetCameraHeight = 0.0f;
};

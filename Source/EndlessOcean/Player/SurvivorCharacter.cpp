#include "SurvivorCharacter.h"

#include "AmmoPouchComponent.h"
#include "BreathComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "DamageScreenEffectComponent.h"
#include "EndlessOcean.h"
#include "EnhancedInputComponent.h"
#include "FallDamageType.h"
#include "HealthComponent.h"
#include "InputActionValue.h"
#include "InteractionComponent.h"
#include "InventoryComponent.h"
#include "ItemHolderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StaminaComponent.h"
#include "SurvivorMovementComponent.h"

/*
 * SetDefaultSubobjectClass tells ACharacter's constructor to create a USurvivorMovementComponent wherever it
 * would have created a UCharacterMovementComponent. This must happen in the initializer list, before
 * ACharacter's constructor runs. Unity has no direct equivalent: there you'd swap the component type
 * on the prefab instead.
 */
ASurvivorCharacter::ASurvivorCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USurvivorMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Tick exists only to blend the camera height when crouching. It's switched on and off as needed.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	GetCapsuleComponent()->InitCapsuleSize(34.0f, 90.0f);

	// Eye heights are measured from the capsule centre. ACharacter uses them for AI sight and similar.
	BaseEyeHeight = 70.0f;
	CrouchedEyeHeight = 40.0f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.0f, 0.0f, BaseEyeHeight));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(80.0f);

	HandAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("HandAnchor"));
	HandAnchor->SetupAttachment(Camera);
	HandAnchor->SetRelativeLocation(FVector(35.0f, 15.0f, -18.0f));

	Stamina = CreateDefaultSubobject<UStaminaComponent>(TEXT("Stamina"));
	Interaction = CreateDefaultSubobject<UInteractionComponent>(TEXT("Interaction"));
	ItemHolder = CreateDefaultSubobject<UItemHolderComponent>(TEXT("ItemHolder"));
	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Breath = CreateDefaultSubobject<UBreathComponent>(TEXT("Breath"));
	AmmoPouch = CreateDefaultSubobject<UAmmoPouchComponent>(TEXT("AmmoPouch"));
	DamageScreenEffect = CreateDefaultSubobject<UDamageScreenEffectComponent>(TEXT("DamageScreenEffect"));

	// The inherited skeletal mesh is unused until the visual pass.
	GetMesh()->SetOwnerNoSee(true);
}

USurvivorMovementComponent* ASurvivorCharacter::GetSurvivorMovement() const
{
	// CastChecked asserts in development builds if the type is wrong, which the constructor guarantees it isn't.
	return CastChecked<USurvivorMovementComponent>(GetCharacterMovement());
}

void ASurvivorCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Read the standing height from the camera, so moving the camera in the Blueprint viewport just works.
	StandingCameraHeight = Camera->GetRelativeLocation().Z;
	TargetCameraHeight = StandingCameraHeight;

	ItemHolder->SetHandAnchor(HandAnchor);

	// Wire the two systems together through delegates so neither class knows about the other.
	GetSurvivorMovement()->OnSprintStateChanged.AddDynamic(this, &ASurvivorCharacter::HandleSprintStateChanged);
	Stamina->OnExhaustedChanged.AddDynamic(this, &ASurvivorCharacter::HandleExhaustedChanged);
}

void ASurvivorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogEndlessOcean, Error, TEXT("'%s' needs an Enhanced Input Component."), *GetNameSafe(this));
		return;
	}

	// BindAction does nothing with a null action, so unassigned optional actions are safe.
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASurvivorCharacter::HandleMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASurvivorCharacter::HandleLook);
	Input->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ASurvivorCharacter::HandleLook);

	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &ASurvivorCharacter::HandleSprintStarted);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &ASurvivorCharacter::HandleSprintCompleted);
	Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &ASurvivorCharacter::HandleCrouchStarted);
	Input->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ASurvivorCharacter::HandleCrouchHeld);
	Input->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ASurvivorCharacter::HandleCrouchCompleted);

	// "Triggered" fires every frame while the key is held, which is what continuous swimming needs.
	Input->BindAction(SwimUpAction, ETriggerEvent::Triggered, this, &ASurvivorCharacter::HandleSwimUpHeld);

	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &ASurvivorCharacter::HandleInteract);
	Input->BindAction(UseItemAction, ETriggerEvent::Started, this, &ASurvivorCharacter::HandleUseItem);
	Input->BindAction(AltUseAction, ETriggerEvent::Started, this, &ASurvivorCharacter::HandleAltUseStarted);
	Input->BindAction(AltUseAction, ETriggerEvent::Completed, this, &ASurvivorCharacter::HandleAltUseCompleted);
	Input->BindAction(ReloadAction, ETriggerEvent::Started, this, &ASurvivorCharacter::HandleReload);
	Input->BindAction(ThrowItemAction, ETriggerEvent::Started, this, &ASurvivorCharacter::HandleThrowItem);
	Input->BindAction(DropItemAction, ETriggerEvent::Started, this, &ASurvivorCharacter::HandleDropItem);
	Input->BindAction(StowItemAction, ETriggerEvent::Started, this, &ASurvivorCharacter::HandleStowItem);
	Input->BindAction(CycleInventoryAction, ETriggerEvent::Triggered, this, &ASurvivorCharacter::HandleCycleInventory);

	// Payload binding: the extra argument (the slot index) is stored in the binding and passed to the handler,
	// so one function serves every slot key. Like capturing a loop variable in a C# lambda.
	for (int32 SlotIndex = 0; SlotIndex < SelectSlotActions.Num(); ++SlotIndex)
	{
		Input->BindAction(SelectSlotActions[SlotIndex], ETriggerEvent::Started, this, &ASurvivorCharacter::HandleSelectSlot, SlotIndex);
	}
}

void ASurvivorCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector CameraLocation = Camera->GetRelativeLocation();
	CameraLocation.Z = FMath::FInterpTo(CameraLocation.Z, TargetCameraHeight, DeltaSeconds, CrouchCameraInterpSpeed);

	if (FMath::IsNearlyEqual(CameraLocation.Z, TargetCameraHeight, 0.1f))
	{
		CameraLocation.Z = TargetCameraHeight;
		SetActorTickEnabled(false); // done blending: stop ticking until the next crouch change
	}

	Camera->SetRelativeLocation(CameraLocation);
}

void ASurvivorCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	// The capsule just shrank and its centre dropped by HalfHeightAdjust. Raise the camera by the same amount so
	// it doesn't jump in the world, then let Tick blend it down to crouched height.
	FVector CameraLocation = Camera->GetRelativeLocation();
	CameraLocation.Z += HalfHeightAdjust;
	Camera->SetRelativeLocation(CameraLocation);

	TargetCameraHeight = CrouchedEyeHeight;
	SetActorTickEnabled(true);
}

void ASurvivorCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	FVector CameraLocation = Camera->GetRelativeLocation();
	CameraLocation.Z -= HalfHeightAdjust;
	Camera->SetRelativeLocation(CameraLocation);

	TargetCameraHeight = StandingCameraHeight;
	SetActorTickEnabled(true);
}

void ASurvivorCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// At this point the movement component still holds the falling velocity from just before impact.
	const float ImpactSpeed = -GetCharacterMovement()->Velocity.Z;
	if (ImpactSpeed <= FallDamageMinSpeed)
	{
		return;
	}

	const float Damage = FMath::GetMappedRangeValueClamped(
		FVector2f(FallDamageMinSpeed, FallDamageMaxSpeed), FVector2f(FallDamageMin, FallDamageMax), ImpactSpeed);
	UGameplayStatics::ApplyDamage(this, Damage, GetController(), this, UFallDamageType::StaticClass());
}

void ASurvivorCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();

	if (GetCharacterMovement()->IsSwimming())
	{
		// Underwater, forward follows the camera, so looking down and pressing W dives. The engine's swim mode
		// accepts full 3D input; on land the vertical part would just be ignored.
		const FRotationMatrix ControlMatrix(GetControlRotation());
		AddMovementInput(ControlMatrix.GetUnitAxis(EAxis::X), Axis.Y);
		AddMovementInput(ControlMatrix.GetUnitAxis(EAxis::Y), Axis.X);
		return;
	}

	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void ASurvivorCharacter::HandleSwimUpHeld()
{
	if (!GetCharacterMovement()->IsSwimming())
	{
		return;
	}

	// At the surface against a low ledge, climb out; otherwise just swim up.
	if (!GetSurvivorMovement()->TryClimbOutOfWater())
	{
		AddMovementInput(FVector::UpVector, 1.0f);
	}
}

void ASurvivorCharacter::HandleCrouchHeld()
{
	if (GetCharacterMovement()->IsSwimming())
	{
		AddMovementInput(FVector::UpVector, -1.0f);
	}
}

void ASurvivorCharacter::HandleAltUseStarted()
{
	ItemHolder->StartAltAction();
}

void ASurvivorCharacter::HandleAltUseCompleted()
{
	ItemHolder->StopAltAction();
}

void ASurvivorCharacter::HandleReload()
{
	ItemHolder->ReloadHeldItem();
}

void ASurvivorCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void ASurvivorCharacter::HandleSprintStarted()
{
	// Sprinting from a crouch stands up first (if there's room).
	if (bIsCrouched)
	{
		UnCrouch();
	}
	GetSurvivorMovement()->SetWantsToSprint(true);
}

void ASurvivorCharacter::HandleSprintCompleted()
{
	GetSurvivorMovement()->SetWantsToSprint(false);
}

void ASurvivorCharacter::HandleCrouchStarted()
{
	// In water the crouch key dives instead (see HandleCrouchHeld).
	if (GetCharacterMovement()->IsSwimming())
	{
		return;
	}

	if (bToggleCrouch && bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ASurvivorCharacter::HandleCrouchCompleted()
{
	if (!bToggleCrouch)
	{
		UnCrouch();
	}
}

void ASurvivorCharacter::HandleInteract()
{
	Interaction->TryInteract();
}

void ASurvivorCharacter::HandleUseItem()
{
	ItemHolder->UseHeldItem();
}

void ASurvivorCharacter::HandleThrowItem()
{
	ItemHolder->ThrowHeldItem();
}

void ASurvivorCharacter::HandleDropItem()
{
	ItemHolder->DropHeldItem();
}

void ASurvivorCharacter::HandleStowItem()
{
	ItemHolder->StowHeldItem();
}

void ASurvivorCharacter::HandleCycleInventory(const FInputActionValue& Value)
{
	const float Direction = Value.Get<float>();
	if (!FMath::IsNearlyZero(Direction))
	{
		ItemHolder->CycleInventory(Direction > 0.0f ? 1 : -1);
	}
}

void ASurvivorCharacter::HandleSelectSlot(int32 SlotIndex)
{
	ItemHolder->EquipFromSlot(SlotIndex);
}

void ASurvivorCharacter::HandleSprintStateChanged(bool bIsSprinting)
{
	Stamina->SetDraining(bIsSprinting);
}

void ASurvivorCharacter::HandleExhaustedChanged(bool bExhausted)
{
	GetSurvivorMovement()->SetSprintBlocked(bExhausted);
}

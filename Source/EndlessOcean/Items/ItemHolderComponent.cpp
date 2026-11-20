#include "ItemHolderComponent.h"

#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "HeldItem.h"
#include "InventoryComponent.h"
#include "ItemDefinition.h"

namespace ItemHolder
{
	/** Fallback release speeds for items without an ItemDefinition. Shares the ItemDefinition's own defaults. */
	static const UItemDefinition* GetDefaults(const AHeldItem* Item)
	{
		const UItemDefinition* Definition = Item ? Item->GetDefinition() : nullptr;
		return Definition ? Definition : GetDefault<UItemDefinition>();
	}

	/** How far to keep a released item from a wall it would otherwise be pushed into. */
	static constexpr float WallClearance = 8.0f;

	/** Offsets into the noise function so each sway axis wanders independently. */
	static constexpr float NoiseSeeds[5] = { 0.0f, 17.3f, 31.7f, 47.1f, 59.9f };
}

UItemHolderComponent::UItemHolderComponent()
{
	// Ticks only while an item is in the hand (see SetHeldItem / ClearHand).
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UItemHolderComponent::BeginPlay()
{
	Super::BeginPlay();

	Inventory = GetOwner()->FindComponentByClass<UInventoryComponent>();

	// Fall back to the root so we never attach to nothing if the owner forgot to set an anchor.
	if (!HandAnchor)
	{
		HandAnchor = GetOwner()->GetRootComponent();
	}

	OwnerCamera = GetOwner()->FindComponentByClass<UCameraComponent>();
	if (const UCameraComponent* Camera = OwnerCamera.Get())
	{
		BaseFOV = Camera->FieldOfView;
	}
}

void UItemHolderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateCameraFOV(DeltaTime);

	if (!HeldItem)
	{
		// Keep ticking only until the FOV has returned to normal after letting go of an aimed item.
		const UCameraComponent* Camera = OwnerCamera.Get();
		if (!Camera || FMath::IsNearlyEqual(Camera->FieldOfView, BaseFOV, 0.05f))
		{
			SetComponentTickEnabled(false);
		}
		return;
	}

	const UItemDefinition* Definition = ItemHolder::GetDefaults(HeldItem);

	// Entering water with an item that can't be used there: stop what it's doing and lower it.
	const bool bSwimming = IsOwnerSwimming();
	const bool bLower = bSwimming && !HeldItem->IsUsableWhileSwimming();
	if (bSwimming != bWasSwimming)
	{
		bWasSwimming = bSwimming;
		if (bLower)
		{
			HeldItem->CancelActions();
		}
	}
	LoweredAlpha = FMath::FInterpTo(LoweredAlpha, bLower ? 1.0f : 0.0f, DeltaTime, LowerBlendSpeed);

	// Procedural sway: smooth Perlin noise per axis, bigger while moving, scaled by the item (aiming shrinks it).
	SwayTime += DeltaTime * Definition->SwaySpeed;
	const float Speed = GetOwner()->GetVelocity().Size();
	const float MoveAlpha = FMath::Clamp(Speed / MovementSwayReferenceSpeed, 0.0f, 1.0f);
	const float Scale = (1.0f + Definition->MovementSwayMultiplier * MoveAlpha) * HeldItem->GetSwayScale();

	auto Noise = [this](int32 Axis) { return FMath::PerlinNoise1D(SwayTime + ItemHolder::NoiseSeeds[Axis]); };
	const float RotAmp = Definition->SwayRotationAmplitude * Scale;
	const float LocAmp = Definition->SwayLocationAmplitude * Scale;
	CurrentSwayRotation = FRotator(Noise(0) * RotAmp, Noise(1) * RotAmp, Noise(2) * RotAmp * 0.5f);
	const FVector SwayLocation(0.0f, Noise(3) * LocAmp, Noise(4) * LocAmp);

	const FTransform Additive = HeldItem->GetAdditivePose();

	const FVector Location = Definition->HeldLocationOffset + Definition->LoweredLocationOffset * LoweredAlpha
		+ SwayLocation + Additive.GetLocation();
	const FRotator Rotation = Definition->HeldRotationOffset + Definition->LoweredRotationOffset * LoweredAlpha
		+ CurrentSwayRotation + Additive.Rotator();

	HeldItem->SetActorRelativeLocation(Location);
	HeldItem->SetActorRelativeRotation(Rotation);
}

void UItemHolderComponent::UpdateCameraFOV(float DeltaTime)
{
	UCameraComponent* Camera = OwnerCamera.Get();
	if (!Camera)
	{
		return;
	}

	float TargetFOV = BaseFOV;
	float AimFOV = BaseFOV;
	float AimAlpha = 0.0f;
	if (HeldItem && HeldItem->GetAimView(AimFOV, AimAlpha))
	{
		TargetFOV = FMath::Lerp(BaseFOV, AimFOV, FMath::Clamp(AimAlpha, 0.0f, 1.0f));
	}

	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, TargetFOV, DeltaTime, FOVBlendSpeed));
}

bool UItemHolderComponent::IsOwnerSwimming() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	return Character && Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsSwimming();
}

bool UItemHolderComponent::CanUseHeldItem() const
{
	return HeldItem && (!IsOwnerSwimming() || HeldItem->IsUsableWhileSwimming());
}

void UItemHolderComponent::StartAltAction()
{
	if (CanUseHeldItem())
	{
		HeldItem->AltActionStarted();
	}
}

void UItemHolderComponent::StopAltAction()
{
	// Always forwarded, so an item never gets stuck aiming.
	if (HeldItem)
	{
		HeldItem->AltActionCompleted();
	}
}

void UItemHolderComponent::ReloadHeldItem()
{
	if (CanUseHeldItem())
	{
		HeldItem->ReloadPressed();
	}
}

void UItemHolderComponent::SetHandAnchor(USceneComponent* InHandAnchor)
{
	HandAnchor = InHandAnchor;
}

bool UItemHolderComponent::PickUp(AHeldItem* Item)
{
	if (!Item || Item->GetItemLocation() != EHeldItemLocation::World || !Item->CanBePickedUp())
	{
		return false;
	}

	AHeldItem* Previous = HeldItem;

	// Put the current item back in the slot it came from if possible, otherwise any free slot, otherwise drop it.
	MakeHandEmpty(ActiveSlot);
	SetHeldItem(Item, INDEX_NONE);

	NotifyHeldItemChanged(Previous);
	return true;
}

void UItemHolderComponent::UseHeldItem()
{
	if (CanUseHeldItem())
	{
		HeldItem->UsePrimary();
	}
}

void UItemHolderComponent::DropHeldItem()
{
	AHeldItem* Previous = HeldItem;
	const UItemDefinition* Defaults = ItemHolder::GetDefaults(HeldItem);
	ReleaseHeldItem(Defaults->DropSpeed, 0.0f, 0.0f);
	NotifyHeldItemChanged(Previous);
}

void UItemHolderComponent::ThrowHeldItem()
{
	AHeldItem* Previous = HeldItem;
	const UItemDefinition* Defaults = ItemHolder::GetDefaults(HeldItem);
	ReleaseHeldItem(Defaults->ThrowSpeed, Defaults->ThrowUpSpeed, Defaults->ThrowSpin);
	NotifyHeldItemChanged(Previous);
}

bool UItemHolderComponent::StowHeldItem()
{
	if (!HeldItem || !Inventory || !HeldItem->CanBeStowed())
	{
		return false;
	}

	// Prefer the slot the item came from so it keeps its number.
	const int32 Slot = (ActiveSlot != INDEX_NONE && ActiveSlot < Inventory->GetCapacity() && !Inventory->GetItemInSlot(ActiveSlot))
		? ActiveSlot
		: Inventory->FindFirstEmptySlot();

	if (Slot == INDEX_NONE)
	{
		return false;
	}

	AHeldItem* Previous = ClearHand();
	Inventory->StoreInSlot(Previous, Slot);
	NotifyHeldItemChanged(Previous);
	return true;
}

bool UItemHolderComponent::EquipFromSlot(int32 SlotIndex)
{
	if (!Inventory)
	{
		return false;
	}

	AHeldItem* Item = Inventory->GetItemInSlot(SlotIndex);
	if (!Item)
	{
		// Pressing the key of the slot whose item is already in hand puts it away (toggle).
		return SlotIndex == ActiveSlot && StowHeldItem();
	}

	AHeldItem* Previous = HeldItem;

	Inventory->RemoveFromSlot(SlotIndex);
	MakeHandEmpty(SlotIndex); // the slot we just emptied is the preferred spot for the old item: a clean swap
	SetHeldItem(Item, SlotIndex);

	NotifyHeldItemChanged(Previous);
	return true;
}

void UItemHolderComponent::CycleInventory(int32 Direction)
{
	if (!Inventory || Direction == 0)
	{
		return;
	}

	const int32 Capacity = Inventory->GetCapacity();
	const int32 Step = Direction > 0 ? 1 : -1;
	const int32 Start = ActiveSlot != INDEX_NONE ? ActiveSlot : (Step > 0 ? -1 : Capacity);

	for (int32 Offset = 1; Offset <= Capacity; ++Offset)
	{
		// Wrap around in both directions (C++ % can return negatives, so add Capacity back).
		const int32 Index = ((Start + Offset * Step) % Capacity + Capacity) % Capacity;
		if (Inventory->GetItemInSlot(Index))
		{
			EquipFromSlot(Index);
			return;
		}
	}
}

void UItemHolderComponent::SetHeldItem(AHeldItem* Item, int32 FromSlot)
{
	HeldItem = Item;
	ActiveSlot = FromSlot;

	if (Item)
	{
		Item->SetHeld(this, HandAnchor);
		Item->OnDestroyed.AddUniqueDynamic(this, &UItemHolderComponent::HandleHeldItemDestroyed);

		// Start the new item lowered if we're already swimming, so it doesn't flash up and back down.
		bWasSwimming = IsOwnerSwimming();
		LoweredAlpha = (bWasSwimming && !Item->IsUsableWhileSwimming()) ? 1.0f : 0.0f;
		SetComponentTickEnabled(true);
	}
}

AHeldItem* UItemHolderComponent::ClearHand()
{
	AHeldItem* Previous = HeldItem;
	if (Previous)
	{
		// Leaving the hand ends any aim, reload or swing.
		Previous->CancelActions();
		Previous->OnDestroyed.RemoveDynamic(this, &UItemHolderComponent::HandleHeldItemDestroyed);
	}
	CurrentSwayRotation = FRotator::ZeroRotator;

	HeldItem = nullptr;
	ActiveSlot = INDEX_NONE;
	return Previous;
}

void UItemHolderComponent::ReleaseHeldItem(float ForwardSpeed, float UpSpeed, float SpinDegrees)
{
	AHeldItem* Item = ClearHand();
	if (!Item)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetViewPoint(ViewLocation, ViewRotation);

	// If the hand is poking through a wall, pull the item back to our side first so it doesn't drop into the next room.
	if (UWorld* World = GetWorld())
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(HeldItemRelease), false, GetOwner());
		Params.AddIgnoredActor(Item);

		FHitResult Hit;
		const FVector ItemLocation = Item->GetActorLocation();
		if (World->SweepSingleByChannel(Hit, ViewLocation, ItemLocation, FQuat::Identity, ECC_Visibility,
			FCollisionShape::MakeSphere(ItemHolder::WallClearance), Params))
		{
			Item->SetActorLocation(Hit.Location);
		}
	}

	FVector Velocity = ViewRotation.Vector() * ForwardSpeed + FVector::UpVector * UpSpeed;
	if (const AActor* Owner = GetOwner())
	{
		// Keep only a little of our own movement: enough that a throw while walking doesn't look like it hits an
		// invisible wall, without sprint-throws flying across the room.
		Velocity += Owner->GetVelocity() * ItemHolder::GetDefaults(Item)->InheritedVelocityScale;
	}

	// Tumble end over end around the camera's right axis.
	const FVector Spin = FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::Y) * SpinDegrees;

	Item->ReleaseToWorld(Velocity, Spin);
}

void UItemHolderComponent::MakeHandEmpty(int32 PreferredSlot)
{
	if (!HeldItem)
	{
		return;
	}

	if (Inventory && HeldItem->CanBeStowed())
	{
		const bool bPreferredFree = PreferredSlot >= 0 && PreferredSlot < Inventory->GetCapacity() && !Inventory->GetItemInSlot(PreferredSlot);
		const int32 Slot = bPreferredFree ? PreferredSlot : Inventory->FindFirstEmptySlot();
		if (Slot != INDEX_NONE)
		{
			AHeldItem* Item = ClearHand();
			Inventory->StoreInSlot(Item, Slot);
			return;
		}
	}

	// Can't be stowed (lit flare) or no room: drop it.
	const UItemDefinition* Defaults = ItemHolder::GetDefaults(HeldItem);
	ReleaseHeldItem(Defaults->DropSpeed, 0.0f, 0.0f);
}

void UItemHolderComponent::GetViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->GetController())
	{
		Pawn->GetController()->GetPlayerViewPoint(OutLocation, OutRotation);
	}
	else if (const AActor* Owner = GetOwner())
	{
		Owner->GetActorEyesViewPoint(OutLocation, OutRotation);
	}
}

void UItemHolderComponent::NotifyHeldItemChanged(AHeldItem* Previous)
{
	if (Previous != HeldItem)
	{
		OnHeldItemChanged.Broadcast(HeldItem, Previous);
	}
}

void UItemHolderComponent::HandleHeldItemDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor == HeldItem)
	{
		AHeldItem* Previous = ClearHand();
		NotifyHeldItemChanged(Previous);
	}
}

#include "SurvivorPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "EndlessOceanCameraManager.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "SurvivorCameraModifier.h"
#include "SurvivorHUDWidget.h"
#include "SurvivorSettings.h"

ASurvivorPlayerController::ASurvivorPlayerController()
{
	// Reuse the template's camera manager for its look pitch limits.
	PlayerCameraManagerClass = AEndlessOceanCameraManager::StaticClass();
}

void ASurvivorPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	// Camera modifiers belong to the camera manager, which only exists for local players.
	if (PlayerCameraManager && CameraModifierClass)
	{
		CameraModifier = Cast<USurvivorCameraModifier>(PlayerCameraManager->AddNewCameraModifier(CameraModifierClass));
	}
	SetCameraMotionEnabled(GetDefault<USurvivorSettings>()->bCameraMotionEnabledByDefault);

	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<USurvivorHUDWidget>(this, HUDWidgetClass);
		HUDWidget->AddToPlayerScreen();
		HUDWidget->ObservePawn(GetPawn()); // may be null if we haven't possessed yet; OnPossess covers that
	}
}

void ASurvivorPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	// Enhanced Input keeps mapping contexts per local player in a subsystem.
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (const TObjectPtr<UInputMappingContext>& Context : DefaultMappingContexts)
		{
			if (Context)
			{
				Subsystem->AddMappingContext(Context, 0);
			}
		}
	}
}

void ASurvivorPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (HUDWidget)
	{
		HUDWidget->ObservePawn(InPawn);
	}
}

void ASurvivorPlayerController::OnUnPossess()
{
	if (HUDWidget)
	{
		HUDWidget->ObservePawn(nullptr);
	}

	Super::OnUnPossess();
}

void ASurvivorPlayerController::SetCameraMotionEnabled(bool bEnabled)
{
	bCameraMotionEnabled = bEnabled;

	if (CameraModifier)
	{
		if (bEnabled)
		{
			CameraModifier->EnableModifier();
		}
		else
		{
			CameraModifier->DisableModifier(false); // false = fade out over AlphaOutTime instead of snapping
		}
	}
}

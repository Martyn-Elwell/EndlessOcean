// Copyright Epic Games, Inc. All Rights Reserved.


#include "EndlessOceanCameraManager.h"

AEndlessOceanCameraManager::AEndlessOceanCameraManager()
{
	// Look (almost) straight down and up. Not a full 90: at exactly +-90 the view's "up" becomes undefined and the
	// camera can flip, and the camera modifiers add a little bob on top of the clamped aim.
	ViewPitchMin = -88.0f;
	ViewPitchMax = 88.0f;
}

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "FallDamageType.generated.h"

/** Damage from landing too hard. Shows the red blood vignette like any other physical damage. */
UCLASS()
class ENDLESSOCEAN_API UFallDamageType : public UDamageType
{
	GENERATED_BODY()

public:

	UFallDamageType()
	{
		DamageImpulse = 0.0f;
	}
};

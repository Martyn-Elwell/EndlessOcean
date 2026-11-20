#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "DrowningDamageType.generated.h"

/**
 * Damage from running out of breath underwater.
 *
 * Damage types let receivers react differently to the same "you took damage" event. The player's screen effects
 * use this one to close a black vignette instead of showing blood. Unreal passes the damage type as a class and
 * receivers see its default object; in Unity terms it's like tagging a hit with an enum or a ScriptableObject.
 */
UCLASS()
class ENDLESSOCEAN_API UDrowningDamageType : public UDamageType
{
	GENERATED_BODY()

public:

	UDrowningDamageType()
	{
		// Drowning shouldn't knock physics objects around.
		DamageImpulse = 0.0f;
	}
};

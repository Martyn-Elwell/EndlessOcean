#pragma once

#include "NativeGameplayTags.h"

/**
 * Gameplay tags defined in C++ ("native" tags). A gameplay tag is a hierarchical name like "Ammo.SPP1" that the
 * engine registers once and compares cheaply. Defining them in code means typos are compile errors and no config
 * file has to be edited by hand. They still appear in every tag picker in the editor.
 *
 * Unity comparison: like a set of string constants, but registered with the engine, hierarchical
 * ("Ammo" matches "Ammo.SPP1"), and selectable from dropdowns.
 */
namespace EndlessOceanTags
{
	/** Rounds for the SPP-1 underwater pistol. */
	ENDLESSOCEAN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ammo_SPP1);
}

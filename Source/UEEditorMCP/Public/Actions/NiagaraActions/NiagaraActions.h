#pragma once

#include "CoreMinimal.h"
#include "Actions/EditorAction.h"

/**
 * Register every Niagara command into the shared action map.
 */
namespace NiagaraActionRegistration
{
	UEEDITORMCP_API void RegisterAll(TMap<FString, TSharedRef<FEditorAction>>& ActionHandlers);
}

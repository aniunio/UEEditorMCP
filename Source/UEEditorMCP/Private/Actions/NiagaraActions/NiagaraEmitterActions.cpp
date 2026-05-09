#include "Actions/NiagaraActions/NiagaraEmitterActions.h"

#include "MCPCommonUtils.h"
#include "Actions/NiagaraActions/NiagaraCommonUtils.h"

#include "EditorAssetLibrary.h"
#include "NiagaraCommon.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScript.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include "UObject/UObjectIterator.h"

#include "NiagaraSimulationStageBase.h"

// ---------------------------------------------------------------------------
// FGetNiagaraEmittersAction::ExecuteInternal
// ---------------------------------------------------------------------------
FString FGetNiagaraEmittersAction::GetActionName() const
{
	return TEXT("get_niagara_emitters");
}

bool FGetNiagaraEmittersAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString SystemPath;
	return GetRequiredString(Params, TEXT("system_path"), SystemPath, OutError);
}

TSharedPtr<FJsonObject> FGetNiagaraEmittersAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadSystemFromParams(Params, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
	TArray<TSharedPtr<FJsonValue>> EmitterArr;
	for (int32 Index = 0; Index < Handles.Num(); ++Index)
	{
		if (!Filter.IsEmpty() &&
			!Handles[Index].GetName().ToString().Contains(Filter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		EmitterArr.Add(MakeShared<FJsonValueObject>(NiagaraCommonUtils::EmitterHandleToJson(Handles[Index], Index)));
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("emitters"), EmitterArr);
	Result->SetNumberField(TEXT("count"), EmitterArr.Num());
	return Result;
}

// ---------------------------------------------------------------------------
// FAddNiagaraEmitterAction::ExecuteInternal
// ---------------------------------------------------------------------------
FString FAddNiagaraEmitterAction::GetActionName() const
{
	return TEXT("add_niagara_emitter");
}

bool FAddNiagaraEmitterAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError);
}

TSharedPtr<FJsonObject> FAddNiagaraEmitterAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadSystemFromParams(Params, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString EmitterPath;
	FString EmitterName;
	FString TemplateName;
	Params->TryGetStringField(TEXT("emitter_path"), EmitterPath);
	Params->TryGetStringField(TEXT("emitter_name"), EmitterName);
	Params->TryGetStringField(TEXT("template"), TemplateName);

	UNiagaraEmitter* SourceEmitter = nullptr;
	if (!EmitterPath.IsEmpty())
	{
		SourceEmitter = Cast<UNiagaraEmitter>(UEditorAssetLibrary::LoadAsset(EmitterPath));
		if (!SourceEmitter)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Emitter not found: %s"), *EmitterPath));
		}
	}
	else if (!TemplateName.IsEmpty())
	{
		const FString TemplatePath = NiagaraCommonUtils::ResolveTemplateReference(TemplateName);
		SourceEmitter = Cast<UNiagaraEmitter>(UEditorAssetLibrary::LoadAsset(TemplatePath));
		if (!SourceEmitter)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Template emitter not found: %s"), *TemplateName));
		}
	}
	else
	{
		const FString DefaultPath =
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst.SimpleSpriteBurst");
		SourceEmitter = Cast<UNiagaraEmitter>(UEditorAssetLibrary::LoadAsset(DefaultPath));
		if (!SourceEmitter)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("No emitter_path/template provided and default emitter not found. "
					"Provide an 'emitter_path' to a UNiagaraEmitter asset."));
		}
	}

	System->Modify();
	const FGuid Version = SourceEmitter->GetExposedVersion().VersionGuid;
	const int32 PreviousEmitterCount = System->GetEmitterHandles().Num();
	const FGuid NewEmitterId = FNiagaraEditorUtilities::AddEmitterToSystem(*System, *SourceEmitter, Version, true);

	FNiagaraEmitterHandle* NewHandle = nullptr;
	for (FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		if (Handle.GetId() == NewEmitterId)
		{
			NewHandle = &Handle;
			break;
		}
	}
	if (!NewHandle)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter was added but could not be resolved in system"));
	}

	if (!EmitterName.IsEmpty() && !NewHandle->GetName().IsEqual(FName(*EmitterName), ENameCase::IgnoreCase))
	{
		TSet<FName> ExistingNames;
		for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
		{
			if (Handle.GetId() != NewHandle->GetId())
			{
				ExistingNames.Add(Handle.GetName());
			}
		}

		const FName DesiredName = FNiagaraUtilities::GetUniqueName(FName(*EmitterName), ExistingNames);
		NewHandle->SetName(DesiredName, *System);
	}

	NiagaraCommonUtils::CompileAndSync(System);
	UEditorAssetLibrary::SaveAsset(Params->GetStringField(TEXT("system_path")));

	const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
	for (int32 Index = 0; Index < Handles.Num(); ++Index)
	{
		if (Handles[Index].GetId() == NewEmitterId)
		{
			TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
			Result->SetBoolField(TEXT("success"), true);
			Result->SetStringField(TEXT("emitter_name"), Handles[Index].GetName().ToString());
			Result->SetStringField(TEXT("emitter_id"), Handles[Index].GetId().ToString());
			Result->SetNumberField(TEXT("emitter_index"), Index);
			Result->SetNumberField(TEXT("total_emitters"), Handles.Num());
			Result->SetNumberField(TEXT("previous_emitter_count"), PreviousEmitterCount);
			return Result;
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("emitter_id"), NewEmitterId.ToString());
	return Result;
}

// ---------------------------------------------------------------------------
// FRemoveNiagaraEmitterAction::ExecuteInternal
// ---------------------------------------------------------------------------
FString FRemoveNiagaraEmitterAction::GetActionName() const
{
	return TEXT("remove_niagara_emitter");
}

bool FRemoveNiagaraEmitterAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError);
}

TSharedPtr<FJsonObject> FRemoveNiagaraEmitterAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadSystemFromParams(Params, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 FoundIndex = INDEX_NONE;
	FString EmitterName;
	Params->TryGetStringField(TEXT("emitter_name"), EmitterName);
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, FoundIndex, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	TSet<FGuid> IdsToRemove;
	IdsToRemove.Add(Handle->GetId());

	System->Modify();
	System->RemoveEmitterHandlesById(IdsToRemove);

	NiagaraCommonUtils::CompileAndSync(System);
	UEditorAssetLibrary::SaveAsset(Params->GetStringField(TEXT("system_path")));

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("removed_emitter"), EmitterName);
	Result->SetNumberField(TEXT("remaining_emitters"), System->GetNumEmitters());
	return Result;
}

// ---------------------------------------------------------------------------
// FSetNiagaraEmitterPropertyAction::ExecuteInternal
// ---------------------------------------------------------------------------
FString FSetNiagaraEmitterPropertyAction::GetActionName() const
{
	return TEXT("set_niagara_emitter_property");
}

bool FSetNiagaraEmitterPropertyAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("property"), Unused, OutError))
	{
		return false;
	}
	if (!Params.IsValid() || !Params->HasField(TEXT("value")))
	{
		OutError = TEXT("Required parameter 'value' is missing");
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FSetNiagaraEmitterPropertyAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	FString EmitterName;
	FString Property;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath) ||
		!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) ||
		!Params->TryGetStringField(TEXT("property"), Property))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing required parameters: system_path, emitter_name, property"));
	}

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 EmitterIndex = INDEX_NONE;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, EmitterIndex, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FVersionedNiagaraEmitterData* Data = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!Data)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data found"));
	}

	const FString LowerProperty = Property.ToLower();
	FString NewValueString;

	if (LowerProperty == TEXT("enabled"))
	{
		bool bValue = false;
		if (!Params->TryGetBoolField(TEXT("value"), bValue))
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("'value' must be bool for 'enabled'"));
		}
		Handle->SetIsEnabled(bValue, *System, true);
		NewValueString = bValue ? TEXT("true") : TEXT("false");
	}
	else if (LowerProperty == TEXT("sim_target"))
	{
		FString Value;
		if (!Params->TryGetStringField(TEXT("value"), Value))
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("'value' must be string for 'sim_target'"));
		}

		const FString LowerValue = Value.ToLower();
		if (LowerValue == TEXT("gpu"))
		{
			Data->SimTarget = ENiagaraSimTarget::GPUComputeSim;
		}
		else if (LowerValue == TEXT("cpu"))
		{
			Data->SimTarget = ENiagaraSimTarget::CPUSim;
		}
		else
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("sim_target must be 'cpu' or 'gpu'"));
		}
		NewValueString = LowerValue;
	}
	else if (LowerProperty == TEXT("local_space"))
	{
		bool bValue = false;
		if (!Params->TryGetBoolField(TEXT("value"), bValue))
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("'value' must be bool for 'local_space'"));
		}
		Data->bLocalSpace = bValue;
		NewValueString = bValue ? TEXT("true") : TEXT("false");
	}
	else if (LowerProperty == TEXT("determinism"))
	{
		bool bValue = false;
		if (!Params->TryGetBoolField(TEXT("value"), bValue))
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("'value' must be bool for 'determinism'"));
		}
		Data->bDeterminism = bValue;
		NewValueString = bValue ? TEXT("true") : TEXT("false");
	}
	else if (LowerProperty == TEXT("bounds_mode"))
	{
		FString Value;
		if (!Params->TryGetStringField(TEXT("value"), Value))
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("'value' must be string for 'bounds_mode'"));
		}

		const FString LowerValue = Value.ToLower();
		if (LowerValue == TEXT("dynamic"))
		{
			Data->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Dynamic;
		}
		else if (LowerValue == TEXT("fixed"))
		{
			Data->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Fixed;
		}
		else if (LowerValue == TEXT("programmable"))
		{
			Data->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Programmable;
		}
		else
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("bounds_mode must be 'dynamic', 'fixed', or 'programmable'"));
		}
		NewValueString = LowerValue;
	}
	else if (LowerProperty == TEXT("max_particles"))
	{
		double NumberValue = 0.0;
		if (!Params->TryGetNumberField(TEXT("value"), NumberValue))
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("'value' must be a number for 'max_particles'"));
		}

		const int32 MaxParticles = FMath::Max(0, static_cast<int32>(NumberValue));
		Data->AllocationMode = EParticleAllocationMode::ManualEstimate;
		Data->PreAllocationCount = MaxParticles;
		NewValueString = FString::FromInt(MaxParticles);
	}
	else
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(
				TEXT("Unsupported property: '%s'. Supported: enabled, sim_target, "
					"local_space, determinism, bounds_mode, max_particles"),
				*Property));
	}

	System->RequestCompile(false);
	System->PostEditChange();
	UEditorAssetLibrary::SaveAsset(SystemPath);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("emitter_name"), EmitterName);
	Result->SetStringField(TEXT("property"), Property);
	Result->SetStringField(TEXT("value"), NewValueString);
	return Result;
}

// ---------------------------------------------------------------------------
// FDuplicateNiagaraEmitterAction::ExecuteInternal
// ---------------------------------------------------------------------------
FString FDuplicateNiagaraEmitterAction::GetActionName() const
{
	return TEXT("duplicate_niagara_emitter");
}

bool FDuplicateNiagaraEmitterAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError);
}

TSharedPtr<FJsonObject> FDuplicateNiagaraEmitterAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath) ||
		!Params->TryGetStringField(TEXT("emitter_name"), EmitterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing required parameters: system_path, emitter_name"));
	}

	FString NewName;
	Params->TryGetStringField(TEXT("new_name"), NewName);

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 SourceIndex = INDEX_NONE;
	FNiagaraEmitterHandle* SourceHandle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, SourceIndex, Error);
	if (!SourceHandle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FName DuplicateName;
	if (NewName.IsEmpty())
	{
		TSet<FName> ExistingNames;
		for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
		{
			ExistingNames.Add(Handle.GetName());
		}
		DuplicateName = FNiagaraUtilities::GetUniqueName(SourceHandle->GetName(), ExistingNames);
	}
	else
	{
		DuplicateName = FName(*NewName);
	}

	System->Modify();
	const FNiagaraEmitterHandle NewHandle = System->DuplicateEmitterHandle(*SourceHandle, DuplicateName);

	NiagaraCommonUtils::CompileAndSync(System);
	UEditorAssetLibrary::SaveAsset(SystemPath);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("new_emitter_name"), NewHandle.GetName().ToString());
	Result->SetStringField(TEXT("new_emitter_id"), NewHandle.GetId().ToString());
	Result->SetNumberField(TEXT("total_emitters"), System->GetNumEmitters());
	return Result;
}

// ---------------------------------------------------------------------------
// FRenameNiagaraEmittersAction::ExecuteInternal
// ---------------------------------------------------------------------------
FString FRenameNiagaraEmittersAction::GetActionName() const
{
	return TEXT("rename_niagara_emitter");
}

bool FRenameNiagaraEmittersAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("new_name"), Unused, OutError))
	{
		return false;
	}

	FString NewName;
	if (!Params->TryGetStringField(TEXT("new_name"), NewName) || NewName.IsEmpty())
	{
		OutError = TEXT("Required parameter 'new_name' is missing");
		return false;
	}

	return true;
}

TSharedPtr<FJsonObject> FRenameNiagaraEmittersAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	FString EmitterName;
	FString NewName;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath) ||
		!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) ||
		!Params->TryGetStringField(TEXT("new_name"), NewName))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing required parameters: system_path, emitter_name, new_name"));
	}

	if (NewName.IsEmpty())
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters: new_name"));
	}

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 EmitterIndex = INDEX_NONE;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, EmitterIndex, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	const FString OldName = Handle->GetName().ToString();

	System->Modify();
	Handle->SetName(FName(*NewName), *System);

	NiagaraCommonUtils::CompileAndSync(System);
	UEditorAssetLibrary::SaveAsset(SystemPath);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("old_name"), OldName);
	Result->SetStringField(TEXT("new_name"), Handle->GetName().ToString());
	Result->SetStringField(TEXT("emitter_id"), Handle->GetId().ToString());
	Result->SetNumberField(TEXT("emitter_index"), EmitterIndex);
	return Result;
}

// ---------------------------------------------------------------------------
// FReorderNiagaraEmitterAction::ExecuteInternal
// ---------------------------------------------------------------------------
FString FReorderNiagaraEmitterAction::GetActionName() const
{
	return TEXT("reorder_niagara_emitter");
}

bool FReorderNiagaraEmitterAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	double NewIndex = 0.0;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError))
	{
		return false;
	}
	if (!Params.IsValid() || !Params->TryGetNumberField(TEXT("new_index"), NewIndex))
	{
		OutError = TEXT("Required parameter 'new_index' is missing");
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FReorderNiagaraEmitterAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	FString EmitterName;
	double NewIndexDouble = 0.0;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath) ||
		!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) ||
		!Params->TryGetNumberField(TEXT("new_index"), NewIndexDouble))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing required parameters: system_path, emitter_name, new_index"));
	}

	const int32 NewIndex = static_cast<int32>(NewIndexDouble);

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 CurrentIndex = INDEX_NONE;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, CurrentIndex, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
	if (NewIndex < 0 || NewIndex >= Handles.Num())
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("new_index %d out of range [0, %d)"), NewIndex, Handles.Num()));
	}

	if (CurrentIndex != NewIndex)
	{
		System->Modify();
		const FNiagaraEmitterHandle MovedHandle = Handles[CurrentIndex];
		Handles.RemoveAt(CurrentIndex);
		Handles.Insert(MovedHandle, NewIndex);
		NiagaraCommonUtils::CompileAndSync(System);
		UEditorAssetLibrary::SaveAsset(SystemPath);
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("emitter_name"), EmitterName);
	Result->SetNumberField(TEXT("old_index"), CurrentIndex);
	Result->SetNumberField(TEXT("new_index"), NewIndex);
	return Result;
}

// ---------------------------------------------------------------------------
// FAddNiagaraEventHandlerAction::ExecuteInternal
// ---------------------------------------------------------------------------
FString FAddNiagaraEventHandlerAction::GetActionName() const
{
	return TEXT("add_niagara_event_handler");
}

bool FAddNiagaraEventHandlerAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError);
}

TSharedPtr<FJsonObject> FAddNiagaraEventHandlerAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadSystemFromParams(Params, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString EmitterName;
	Params->TryGetStringField(TEXT("emitter_name"), EmitterName);

	FString SourceEmitterName;
	Params->TryGetStringField(TEXT("source_emitter"), SourceEmitterName);

	FString EventName = TEXT("CollisionEvent");
	Params->TryGetStringField(TEXT("event_name"), EventName);

	FString ExecutionMode = TEXT("spawned_particles");
	Params->TryGetStringField(TEXT("execution_mode"), ExecutionMode);

	int32 MaxEventsPerFrame = 0;
	Params->TryGetNumberField(TEXT("max_events_per_frame"), MaxEventsPerFrame);

	bool bRandomSpawnNumber = false;
	Params->TryGetBoolField(TEXT("random_spawn_number"), bRandomSpawnNumber);

	int32 SpawnNumber = 1;
	Params->TryGetNumberField(TEXT("spawn_number"), SpawnNumber);

	int32 EmitterIndex = INDEX_NONE;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, EmitterIndex, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	FNiagaraEventScriptProperties EventProps;
	EventProps.Script = NewObject<UNiagaraScript>(Handle->GetInstance().Emitter, FName(TEXT("EventScript")), RF_Transactional);
	EventProps.Script->SetUsage(ENiagaraScriptUsage::ParticleEventScript);
	EventProps.SourceEventName = FName(*EventName);

	const FString LowerMode = ExecutionMode.ToLower();
	if (LowerMode == TEXT("spawned_particles") || LowerMode == TEXT("spawn"))
	{
		EventProps.ExecutionMode = EScriptExecutionMode::SpawnedParticles;
	}
	else if (LowerMode == TEXT("every_particle") || LowerMode == TEXT("all"))
	{
		EventProps.ExecutionMode = EScriptExecutionMode::EveryParticle;
	}
	else if (LowerMode == TEXT("single_particle") || LowerMode == TEXT("single"))
	{
		EventProps.ExecutionMode = EScriptExecutionMode::SingleParticle;
	}
	else
	{
		EventProps.ExecutionMode = EScriptExecutionMode::SpawnedParticles;
	}

	EventProps.MaxEventsPerFrame = static_cast<uint32>(MaxEventsPerFrame);
	EventProps.bRandomSpawnNumber = bRandomSpawnNumber;
	EventProps.SpawnNumber = static_cast<uint32>(SpawnNumber);

	if (!SourceEmitterName.IsEmpty())
	{
		int32 SourceIndex = INDEX_NONE;
		FNiagaraEmitterHandle* SourceHandle = NiagaraCommonUtils::FindEmitterHandle(System, SourceEmitterName, SourceIndex, Error);
		if (SourceHandle)
		{
			EventProps.SourceEmitterID = SourceHandle->GetId();
		}
	}

	EmitterData->EventHandlerScriptProps.Add(EventProps);
	NiagaraCommonUtils::CompileAndSync(System);

	const int32 NewIndex = EmitterData->EventHandlerScriptProps.Num() - 1;

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("emitter_name"), EmitterName);
	Result->SetStringField(TEXT("event_name"), EventName);
	Result->SetStringField(TEXT("execution_mode"), ExecutionMode);
	Result->SetNumberField(TEXT("event_handler_index"), NewIndex);
	Result->SetNumberField(TEXT("total_handlers"), EmitterData->EventHandlerScriptProps.Num());
	return Result;
}

// ---------------------------------------------------------------------------
// FAddNiagaraSimulationStageAction::ExecuteInternal
// ---------------------------------------------------------------------------
FString FAddNiagaraSimulationStageAction::GetActionName() const
{
	return TEXT("add_niagara_simulation_stage");
}

bool FAddNiagaraSimulationStageAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError);
}

TSharedPtr<FJsonObject> FAddNiagaraSimulationStageAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadSystemFromParams(Params, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString EmitterName;
	Params->TryGetStringField(TEXT("emitter_name"), EmitterName);

	FString StageName = TEXT("SimulationStage");
	Params->TryGetStringField(TEXT("stage_name"), StageName);

	FString IterationSource = TEXT("particles");
	Params->TryGetStringField(TEXT("iteration_source"), IterationSource);

	int32 NumIterations = 1;
	Params->TryGetNumberField(TEXT("num_iterations"), NumIterations);

	bool bEnabled = true;
	Params->TryGetBoolField(TEXT("enabled"), bEnabled);

	int32 EmitterIndex = INDEX_NONE;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, EmitterIndex, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	UNiagaraEmitter* Emitter = Handle->GetInstance().Emitter;
	if (!Emitter)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	UNiagaraSimulationStageGeneric* NewStage = NewObject<UNiagaraSimulationStageGeneric>(Emitter, FName(*StageName));
	if (!NewStage)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create simulation stage"));
	}

	NewStage->SimulationStageName = FName(*StageName);
	NewStage->bEnabled = bEnabled;
	{
		const FNiagaraTypeDefinition IntDefinition = FNiagaraTypeDefinition::GetIntDef();
		const FNiagaraVariableBase IntVariable(IntDefinition, FName(TEXT("NumIterations")));
		NewStage->NumIterations.SetDefaultParameter(IntVariable, NumIterations);
	}

	const FString LowerSource = IterationSource.ToLower();
	if (LowerSource == TEXT("particles") || LowerSource == TEXT("particle"))
	{
		NewStage->IterationSource = ENiagaraIterationSource::Particles;
	}
	else if (LowerSource == TEXT("data_interface") || LowerSource == TEXT("datainterface"))
	{
		NewStage->IterationSource = ENiagaraIterationSource::DataInterface;
	}
	else
	{
		NewStage->IterationSource = ENiagaraIterationSource::Particles;
	}

	Emitter->AddSimulationStage(NewStage, EmitterData->Version.VersionGuid);
	NiagaraCommonUtils::CompileAndSync(System);

	const int32 NewIndex = EmitterData->GetSimulationStages().Num() - 1;

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("emitter_name"), EmitterName);
	Result->SetStringField(TEXT("stage_name"), StageName);
	Result->SetStringField(TEXT("iteration_source"), IterationSource);
	Result->SetNumberField(TEXT("num_iterations"), NumIterations);
	Result->SetNumberField(TEXT("stage_index"), NewIndex);
	Result->SetNumberField(TEXT("total_stages"), EmitterData->GetSimulationStages().Num());
	return Result;
}

// ---------------------------------------------------------------------------
// FGetNiagaraEventHandlersAction::ExecuteInternal
// ---------------------------------------------------------------------------
FString FGetNiagaraEventHandlersAction::GetActionName() const
{
	return TEXT("get_niagara_event_handlers");
}

bool FGetNiagaraEventHandlersAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError);
}

TSharedPtr<FJsonObject> FGetNiagaraEventHandlersAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadSystemFromParams(Params, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString EmitterName;
	Params->TryGetStringField(TEXT("emitter_name"), EmitterName);

	int32 EmitterIndex = INDEX_NONE;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, EmitterIndex, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	TArray<TSharedPtr<FJsonValue>> HandlersArr;
	for (int32 Index = 0; Index < EmitterData->EventHandlerScriptProps.Num(); ++Index)
	{
		const FNiagaraEventScriptProperties& EventProps = EmitterData->EventHandlerScriptProps[Index];

		TSharedPtr<FJsonObject> HandlerObj = MakeShared<FJsonObject>();
		HandlerObj->SetNumberField(TEXT("index"), Index);
		HandlerObj->SetStringField(TEXT("event_name"), EventProps.SourceEventName.ToString());

		FString ModeName;
		switch (EventProps.ExecutionMode)
		{
		case EScriptExecutionMode::SpawnedParticles:
			ModeName = TEXT("spawned_particles");
			break;
		case EScriptExecutionMode::EveryParticle:
			ModeName = TEXT("every_particle");
			break;
		case EScriptExecutionMode::SingleParticle:
			ModeName = TEXT("single_particle");
			break;
		default:
			ModeName = TEXT("unknown");
			break;
		}
		HandlerObj->SetStringField(TEXT("execution_mode"), ModeName);
		HandlerObj->SetNumberField(TEXT("max_events_per_frame"), static_cast<int32>(EventProps.MaxEventsPerFrame));
		HandlerObj->SetNumberField(TEXT("spawn_number"), static_cast<int32>(EventProps.SpawnNumber));
		HandlerObj->SetBoolField(TEXT("random_spawn_number"), EventProps.bRandomSpawnNumber);

		FString SourceEmitterString = EventProps.SourceEmitterID.IsValid()
			                              ? EventProps.SourceEmitterID.ToString()
			                              : TEXT("");
		if (EventProps.SourceEmitterID.IsValid())
		{
			for (const FNiagaraEmitterHandle& OtherHandle : System->GetEmitterHandles())
			{
				if (OtherHandle.GetId() == EventProps.SourceEmitterID)
				{
					SourceEmitterString = OtherHandle.GetName().ToString();
					break;
				}
			}
		}
		HandlerObj->SetStringField(TEXT("source_emitter"), SourceEmitterString);
		HandlersArr.Add(MakeShared<FJsonValueObject>(HandlerObj));
	}

	TArray<TSharedPtr<FJsonValue>> StagesArr;
	const TArray<UNiagaraSimulationStageBase*>& SimulationStages = EmitterData->GetSimulationStages();
	for (int32 Index = 0; Index < SimulationStages.Num(); ++Index)
	{
		UNiagaraSimulationStageBase* Stage = SimulationStages[Index];
		if (!Stage)
		{
			continue;
		}

		TSharedPtr<FJsonObject> StageObj = MakeShared<FJsonObject>();
		StageObj->SetNumberField(TEXT("index"), Index);
		StageObj->SetStringField(TEXT("name"), Stage->GetName());
		StageObj->SetBoolField(TEXT("enabled"), Stage->bEnabled);

		UNiagaraSimulationStageGeneric* GenericStage = Cast<UNiagaraSimulationStageGeneric>(Stage);
		if (GenericStage)
		{
			StageObj->SetStringField(TEXT("simulation_stage_name"), GenericStage->SimulationStageName.ToString());
			StageObj->SetNumberField(TEXT("num_iterations"), GenericStage->NumIterations.GetDefaultValue<int32>());

			FString IterationSourceString;
			switch (GenericStage->IterationSource)
			{
			case ENiagaraIterationSource::Particles:
				IterationSourceString = TEXT("particles");
				break;
			case ENiagaraIterationSource::DataInterface:
				IterationSourceString = TEXT("data_interface");
				break;
			default:
				IterationSourceString = TEXT("unknown");
				break;
			}
			StageObj->SetStringField(TEXT("iteration_source"), IterationSourceString);
		}

		StagesArr.Add(MakeShared<FJsonValueObject>(StageObj));
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("emitter_name"), EmitterName);
	Result->SetArrayField(TEXT("event_handlers"), HandlersArr);
	Result->SetNumberField(TEXT("event_handler_count"), HandlersArr.Num());
	Result->SetArrayField(TEXT("simulation_stages"), StagesArr);
	Result->SetNumberField(TEXT("simulation_stage_count"), StagesArr.Num());
	return Result;
}

// ---------------------------------------------------------------------------
// FListNiagaraEmitterTemplatesAction::ExecuteInternal
// ---------------------------------------------------------------------------
FString FListNiagaraEmitterTemplatesAction::GetActionName() const
{
	return TEXT("list_niagara_emitter_templates");
}

bool FListNiagaraEmitterTemplatesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Params;
	(void)Context;
	(void)OutError;
	return true;
}

TSharedPtr<FJsonObject> FListNiagaraEmitterTemplatesAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Category = TEXT("all");
	Params->TryGetStringField(TEXT("category"), Category);

	struct FTemplateEntry
	{
		const TCHAR* Path;
		const TCHAR* Name;
		const TCHAR* Category;
		const TCHAR* Description;
	};

	static const FTemplateEntry Templates[] =
	{
		{
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst"), TEXT("Simple Sprite Burst"), TEXT("sprites"),
			TEXT("A simple burst of sprites")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/Fountain"), TEXT("Fountain"), TEXT("sprites"),
			TEXT("A continuous fountain of particles")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/OmnidirectionalBurst"), TEXT("Omnidirectional Burst"), TEXT("sprites"),
			TEXT("Particles burst in all directions")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/DirectionalBurst"), TEXT("Directional Burst"), TEXT("sprites"),
			TEXT("Particles burst in a specific direction")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/ConfettiBurst"), TEXT("Confetti Burst"), TEXT("sprites"),
			TEXT("Confetti-style particle burst")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/HangingParticulates"), TEXT("Hanging Particulates"), TEXT("sprites"),
			TEXT("Floating dust-like particles")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/BlowingParticles"), TEXT("Blowing Particles"), TEXT("sprites"),
			TEXT("Particles blown by wind")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/SingleLoopingParticle"), TEXT("Single Looping Particle"), TEXT("sprites"),
			TEXT("A single particle that loops")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/Minimal"), TEXT("Minimal"), TEXT("sprites"),
			TEXT("Minimal emitter setup for customization")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/RecycleParticlesInView"), TEXT("Recycle Particles In View"), TEXT("sprites"),
			TEXT("Particles recycled when in camera view")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/UpwardMeshBurst"), TEXT("Upward Mesh Burst"), TEXT("meshes"),
			TEXT("Mesh particles burst upward")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/LocationBasedRibbon"), TEXT("Location Based Ribbon"), TEXT("ribbons"),
			TEXT("Ribbon that follows a path")
		},
		{TEXT("/Niagara/DefaultAssets/Templates/Emitters/StaticBeam"), TEXT("Static Beam"), TEXT("beams"), TEXT("A static beam effect")},
		{
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/DynamicBeam"), TEXT("Dynamic Beam"), TEXT("beams"),
			TEXT("A dynamic, animated beam")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/BehaviorExamples/GridLocation"), TEXT("Grid Location"), TEXT("behaviors"),
			TEXT("Particles spawned in a grid pattern")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/BehaviorExamples/MeshOrientation"), TEXT("Mesh Orientation"), TEXT("behaviors"),
			TEXT("Example of mesh particle orientation")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/BehaviorExamples/MeshRotationForce"), TEXT("Mesh Rotation Force"), TEXT("behaviors"),
			TEXT("Mesh particles with rotation")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/BehaviorExamples/SpriteFacingAndAlignment"), TEXT("Sprite Facing And Alignment"),
			TEXT("behaviors"), TEXT("Sprite billboard and alignment options")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/BehaviorExamples/SubUVAnimation"), TEXT("SubUV Animation"), TEXT("behaviors"),
			TEXT("Sprite sheet animation")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/BehaviorExamples/RibbonID"), TEXT("Ribbon ID"), TEXT("behaviors"),
			TEXT("Multiple ribbon trails example")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/BehaviorExamples/RibbonShapes"), TEXT("Ribbon Shapes"), TEXT("behaviors"),
			TEXT("Different ribbon shape modes")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/BehaviorExamples/PlayAudio"), TEXT("Play Audio"), TEXT("behaviors"),
			TEXT("Audio playback with particles")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/BehaviorExamples/KillParticles"), TEXT("Kill Particles"), TEXT("behaviors"),
			TEXT("Particle death conditions")
		},
		{
			TEXT("/Niagara/DefaultAssets/Templates/BehaviorExamples/SpawnGroups"), TEXT("Spawn Groups"), TEXT("behaviors"),
			TEXT("Grouped particle spawning")
		},
	};

	TArray<TSharedPtr<FJsonValue>> TemplatesArr;
	for (const FTemplateEntry& Entry : Templates)
	{
		if (!Category.Equals(TEXT("all"), ESearchCase::IgnoreCase) &&
			!FString(Entry.Category).Equals(Category, ESearchCase::IgnoreCase))
		{
			continue;
		}

		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("path"), Entry.Path);
		Obj->SetStringField(TEXT("name"), Entry.Name);
		Obj->SetStringField(TEXT("category"), Entry.Category);
		Obj->SetStringField(TEXT("description"), Entry.Description);
		TemplatesArr.Add(MakeShared<FJsonValueObject>(Obj));
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("category_filter"), Category);
	Result->SetArrayField(TEXT("templates"), TemplatesArr);
	Result->SetNumberField(TEXT("count"), TemplatesArr.Num());
	return Result;
}

// ---------------------------------------------------------------------------
// FGetNiagaraEmitterAttributesAction::ExecuteInternal
// ---------------------------------------------------------------------------
FString FGetNiagaraEmitterAttributesAction::GetActionName() const
{
	return TEXT("get_niagara_emitter_attributes");
}

bool FGetNiagaraEmitterAttributesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError);
}

TSharedPtr<FJsonObject> FGetNiagaraEmitterAttributesAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadSystemFromParams(Params, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString EmitterName;
	Params->TryGetStringField(TEXT("emitter_name"), EmitterName);

	int32 EmitterIndex = INDEX_NONE;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, EmitterIndex, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	struct FScopeInfo
	{
		const TCHAR* ScopeName;
		ENiagaraScriptUsage Usage;
	};

	static constexpr FScopeInfo Scopes[] =
	{
		{TEXT("particle_spawn"), ENiagaraScriptUsage::ParticleSpawnScript},
		{TEXT("particle_update"), ENiagaraScriptUsage::ParticleUpdateScript},
		{TEXT("emitter_spawn"), ENiagaraScriptUsage::EmitterSpawnScript},
		{TEXT("emitter_update"), ENiagaraScriptUsage::EmitterUpdateScript},
	};

	TArray<TSharedPtr<FJsonValue>> AttributesArr;

	for (const FScopeInfo& ScopeInfo : Scopes)
	{
		UNiagaraScript* Script = nullptr;
		switch (ScopeInfo.Usage)
		{
		case ENiagaraScriptUsage::ParticleSpawnScript:
			Script = EmitterData->SpawnScriptProps.Script;
			break;
		case ENiagaraScriptUsage::ParticleUpdateScript:
			Script = EmitterData->UpdateScriptProps.Script;
			break;
		case ENiagaraScriptUsage::EmitterSpawnScript:
			Script = EmitterData->EmitterSpawnScriptProps.Script;
			break;
		case ENiagaraScriptUsage::EmitterUpdateScript:
			Script = EmitterData->EmitterUpdateScriptProps.Script;
			break;
		default:
			break;
		}

		if (!Script)
		{
			continue;
		}

		TArrayView<const FNiagaraVariableWithOffset> RapidIterParams =
			Script->RapidIterationParameters.ReadParameterVariables();
		for (const FNiagaraVariableWithOffset& Variable : RapidIterParams)
		{
			const FString AttributeName = Variable.GetName().ToString();
			if (!Filter.IsEmpty() && !AttributeName.Contains(Filter, ESearchCase::IgnoreCase))
			{
				continue;
			}

			TSharedPtr<FJsonObject> AttrObj = MakeShared<FJsonObject>();
			AttrObj->SetStringField(TEXT("name"), AttributeName);
			AttrObj->SetStringField(TEXT("type"), Variable.GetType().GetName());
			AttrObj->SetStringField(TEXT("scope"), ScopeInfo.ScopeName);
			AttrObj->SetStringField(TEXT("source"), TEXT("rapid_iteration"));
			AttributesArr.Add(MakeShared<FJsonValueObject>(AttrObj));
		}
	}

	struct FWellKnownAttr
	{
		const TCHAR* Name;
		const TCHAR* Type;
	};

	static const FWellKnownAttr WellKnown[] =
	{
		{TEXT("Particles.Position"), TEXT("Vector")},
		{TEXT("Particles.Velocity"), TEXT("Vector")},
		{TEXT("Particles.Color"), TEXT("LinearColor")},
		{TEXT("Particles.SpriteSize"), TEXT("Vector2D")},
		{TEXT("Particles.SpriteRotation"), TEXT("float")},
		{TEXT("Particles.Scale"), TEXT("Vector")},
		{TEXT("Particles.Lifetime"), TEXT("float")},
		{TEXT("Particles.Age"), TEXT("float")},
		{TEXT("Particles.NormalizedAge"), TEXT("float")},
		{TEXT("Particles.Mass"), TEXT("float")},
		{TEXT("Particles.MeshOrientation"), TEXT("Quat")},
		{TEXT("Particles.UniqueID"), TEXT("int32")},
		{TEXT("Particles.RibbonID"), TEXT("NiagaraID")},
		{TEXT("Particles.RibbonWidth"), TEXT("float")},
		{TEXT("Particles.RibbonFacing"), TEXT("Vector")},
		{TEXT("Particles.RibbonTwist"), TEXT("float")},
		{TEXT("Particles.MaterialRandom"), TEXT("float")},
		{TEXT("Particles.DynamicMaterialParameter"), TEXT("Vector4")},
		{TEXT("Particles.DynamicMaterialParameter1"), TEXT("Vector4")},
		{TEXT("Particles.SubImageIndex"), TEXT("float")},
		{TEXT("Particles.CameraOffset"), TEXT("float")},
		{TEXT("Particles.LightRadius"), TEXT("float")},
		{TEXT("Particles.LightExponent"), TEXT("float")},
		{TEXT("Particles.LightVolumetricScattering"), TEXT("float")},
	};

	for (const FWellKnownAttr& Attr : WellKnown)
	{
		if (!Filter.IsEmpty() && !FString(Attr.Name).Contains(Filter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		TSharedPtr<FJsonObject> AttrObj = MakeShared<FJsonObject>();
		AttrObj->SetStringField(TEXT("name"), Attr.Name);
		AttrObj->SetStringField(TEXT("type"), Attr.Type);
		AttrObj->SetStringField(TEXT("scope"), TEXT("particle"));
		AttrObj->SetStringField(TEXT("source"), TEXT("well_known"));
		AttributesArr.Add(MakeShared<FJsonValueObject>(AttrObj));
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("emitter_name"), EmitterName);
	Result->SetArrayField(TEXT("attributes"), AttributesArr);
	Result->SetNumberField(TEXT("count"), AttributesArr.Num());
	return Result;
}

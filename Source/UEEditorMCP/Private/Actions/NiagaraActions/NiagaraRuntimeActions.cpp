#include "Actions/NiagaraActions/NiagaraRuntimeActions.h"

#include "MCPCommonUtils.h"
#include "Actions/NiagaraActions/NiagaraCommonUtils.h"

#include "NiagaraSystem.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/SceneComponent.h"

TSharedPtr<FJsonObject> FSpawnNiagaraEffectAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	(void)Context;
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString Error;
	UNiagaraSystem* NiagaraSystem = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!NiagaraSystem)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"), TEXT("no_world"));
	}

	FVector Location = FMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
	FRotator Rotation = FMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
	FVector Scale = FVector::OneVector;
	if (Params->HasField(TEXT("scale")))
	{
		Scale = FMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
	}

	bool bAutoActivate = true;
	Params->TryGetBoolField(TEXT("auto_activate"), bAutoActivate);

	bool bForceSolo = false;
	Params->TryGetBoolField(TEXT("force_solo"), bForceSolo);

	int32 WarmupTicks = 0;
	if (Params->HasField(TEXT("warmup_ticks")))
	{
		WarmupTicks = static_cast<int32>(Params->GetNumberField(TEXT("warmup_ticks")));
	}

	double WarmupDeltaTime = 1.0 / 60.0;
	if (Params->HasField(TEXT("warmup_delta_time")))
	{
		WarmupDeltaTime = Params->GetNumberField(TEXT("warmup_delta_time"));
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ANiagaraActor* NiagaraActor = World->SpawnActor<ANiagaraActor>(Location, Rotation, SpawnParams);
	if (!NiagaraActor)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn Niagara actor"));
	}

	UNiagaraComponent* NiagaraComp = NiagaraActor->GetNiagaraComponent();
	if (NiagaraComp)
	{
		NiagaraComp->SetAsset(NiagaraSystem);
		NiagaraComp->SetAutoActivate(bAutoActivate);
		NiagaraComp->SetForceSolo(bForceSolo);
		NiagaraActor->SetActorScale3D(Scale);

		if (bAutoActivate)
		{
			NiagaraComp->Activate(true);
		}

		if (WarmupTicks > 0)
		{
			NiagaraComp->AdvanceSimulation(WarmupTicks, static_cast<float>(WarmupDeltaTime));
		}
	}

	FString ActorName;
	if (Params->TryGetStringField(TEXT("name"), ActorName))
	{
		NiagaraActor->SetActorLabel(*ActorName);
	}

	FString FolderPath;
	if (Params->TryGetStringField(TEXT("folder"), FolderPath))
	{
		NiagaraActor->SetFolderPath(FName(*FolderPath));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("actor_name"), NiagaraActor->GetActorLabel());
	Result->SetStringField(TEXT("internal_name"), NiagaraActor->GetName());
	Result->SetStringField(TEXT("system_path"), SystemPath);
	Result->SetObjectField(TEXT("location"), NiagaraCommonUtils::VectorToJson(NiagaraActor->GetActorLocation()));
	Result->SetObjectField(TEXT("rotation"), NiagaraCommonUtils::RotatorToJson(NiagaraActor->GetActorRotation()));
	Result->SetObjectField(TEXT("scale"), NiagaraCommonUtils::VectorToJson(NiagaraActor->GetActorScale3D()));
	Result->SetBoolField(TEXT("auto_activate"), bAutoActivate);
	Result->SetBoolField(TEXT("force_solo"), bForceSolo);
	Result->SetNumberField(TEXT("warmup_ticks"), WarmupTicks);
	Result->SetNumberField(TEXT("warmup_delta_time"), WarmupDeltaTime);
	return Result;
}

TSharedPtr<FJsonObject> FControlNiagaraEffectAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	(void)Context;
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	FString Action;
	if (!Params->TryGetStringField(TEXT("action"), Action))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action' parameter"));
	}

	FString Error;
	ANiagaraActor* NiagaraActor = NiagaraCommonUtils::FindNiagaraActorByName(ActorName, Error);
	if (!NiagaraActor)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	UNiagaraComponent* NiagaraComp = NiagaraActor->GetNiagaraComponent();
	if (!NiagaraComp)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Actor has no Niagara component"));
	}

	const FString LowerAction = Action.ToLower();

	if (LowerAction == TEXT("activate"))
	{
		NiagaraComp->Activate(true);
	}
	else if (LowerAction == TEXT("deactivate"))
	{
		NiagaraComp->Deactivate();
	}
	else if (LowerAction == TEXT("reset"))
	{
		NiagaraComp->ResetSystem();
	}
	else if (LowerAction == TEXT("reinitialize") || LowerAction == TEXT("reset_system"))
	{
		NiagaraComp->ReinitializeSystem();
	}
	else
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unknown action '%s'. Use: activate, deactivate, reset, reinitialize"), *Action));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("actor_name"), ActorName);
	Result->SetStringField(TEXT("action"), Action);
	Result->SetBoolField(TEXT("is_active"), NiagaraComp->IsActive());
	return Result;
}

TSharedPtr<FJsonObject> FAddNiagaraComponentAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	(void)Context;
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"), TEXT("no_world"));
	}

	AActor* Actor = NiagaraCommonUtils::FindActorByName(World, ActorName);
	if (!Actor)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Actor '%s' not found in level"), *ActorName));
	}

	FString Error;
	UNiagaraSystem* NiagaraSystem = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!NiagaraSystem)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString ComponentName = TEXT("NiagaraComponent");
	Params->TryGetStringField(TEXT("component_name"), ComponentName);

	UNiagaraComponent* NiagaraComp = NewObject<UNiagaraComponent>(Actor, FName(*ComponentName));
	if (!NiagaraComp)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Niagara component"));
	}

	NiagaraComp->SetAsset(NiagaraSystem);

	if (Params->HasField(TEXT("relative_location")))
	{
		FVector RelLoc = FMCPCommonUtils::GetVectorFromJson(Params, TEXT("relative_location"));
		NiagaraComp->SetRelativeLocation(RelLoc);
	}

	if (Params->HasField(TEXT("relative_rotation")))
	{
		FRotator RelRot = FMCPCommonUtils::GetRotatorFromJson(Params, TEXT("relative_rotation"));
		NiagaraComp->SetRelativeRotation(RelRot);
	}

	bool bAutoActivate = true;
	Params->TryGetBoolField(TEXT("auto_activate"), bAutoActivate);
	NiagaraComp->SetAutoActivate(bAutoActivate);

	if (USceneComponent* Root = Actor->GetRootComponent())
	{
		NiagaraComp->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
	}

	NiagaraComp->RegisterComponent();

	if (bAutoActivate)
	{
		NiagaraComp->Activate(true);
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("actor_name"), ActorName);
	Result->SetStringField(TEXT("component_name"), ComponentName);
	Result->SetStringField(TEXT("system_path"), SystemPath);
	Result->SetBoolField(TEXT("auto_activate"), bAutoActivate);
	return Result;
}

TSharedPtr<FJsonObject> FGetNiagaraActorsAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return CreateErrorResponse(TEXT("No editor world available"), TEXT("no_world"));
	}

	FString NameFilter;
	Params->TryGetStringField(TEXT("name_filter"), NameFilter);

	FString SystemFilter;
	Params->TryGetStringField(TEXT("system_filter"), SystemFilter);

	TArray<TSharedPtr<FJsonValue>> ActorsArr;

	for (TActorIterator<ANiagaraActor> It(World); It; ++It)
	{
		ANiagaraActor* Actor = *It;

		if (!NameFilter.IsEmpty())
		{
			const FString Label = Actor->GetActorLabel();
			const FString InternalName = Actor->GetName();
			if (!Label.Contains(NameFilter, ESearchCase::IgnoreCase) &&
				!InternalName.Contains(NameFilter, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}

		UNiagaraComponent* NiagaraComp = Actor->GetNiagaraComponent();
		FString SystemName;
		FString SystemAssetPath;

		if (NiagaraComp && NiagaraComp->GetAsset())
		{
			SystemName = NiagaraComp->GetAsset()->GetName();
			SystemAssetPath = NiagaraComp->GetAsset()->GetPathName();

			if (!SystemFilter.IsEmpty() &&
				!SystemName.Contains(SystemFilter, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}
		else if (!SystemFilter.IsEmpty())
		{
			continue;
		}

		auto ActorObj = MakeShared<FJsonObject>();
		ActorObj->SetStringField(TEXT("name"), Actor->GetActorLabel());
		ActorObj->SetStringField(TEXT("internal_name"), Actor->GetName());
		ActorObj->SetStringField(TEXT("system_name"), SystemName);
		ActorObj->SetStringField(TEXT("system_path"), SystemAssetPath);
		ActorObj->SetObjectField(TEXT("location"), NiagaraCommonUtils::VectorToJson(Actor->GetActorLocation()));
		ActorObj->SetObjectField(TEXT("rotation"), NiagaraCommonUtils::RotatorToJson(Actor->GetActorRotation()));
		ActorObj->SetObjectField(TEXT("scale"), NiagaraCommonUtils::VectorToJson(Actor->GetActorScale3D()));

		if (NiagaraComp)
		{
			ActorObj->SetBoolField(TEXT("is_active"), NiagaraComp->IsActive());
			ActorObj->SetBoolField(TEXT("auto_activate"), NiagaraComp->bAutoActivate);
		}

		const FName FolderPath = Actor->GetFolderPath();
		if (!FolderPath.IsNone())
		{
			ActorObj->SetStringField(TEXT("folder"), FolderPath.ToString());
		}

		ActorsArr.Add(MakeShared<FJsonValueObject>(ActorObj));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("actors"), ActorsArr);
	Result->SetNumberField(TEXT("count"), ActorsArr.Num());
	return Result;
}

// ---------------------------------------------------------------------------
// Action metadata
// ---------------------------------------------------------------------------

FString FSpawnNiagaraEffectAction::GetActionName() const
{
	return TEXT("spawn_niagara_effect");
}

bool FSpawnNiagaraEffectAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError);
}

FString FControlNiagaraEffectAction::GetActionName() const
{
	return TEXT("control_niagara_effect");
}

bool FControlNiagaraEffectAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("actor_name"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("action"), Unused, OutError);
}

FString FAddNiagaraComponentAction::GetActionName() const
{
	return TEXT("add_niagara_component");
}

bool FAddNiagaraComponentAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("actor_name"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError);
}

FString FGetNiagaraActorsAction::GetActionName() const
{
	return TEXT("get_niagara_actors");
}

bool FGetNiagaraActorsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Params;
	(void)Context;
	(void)OutError;
	return true;
}

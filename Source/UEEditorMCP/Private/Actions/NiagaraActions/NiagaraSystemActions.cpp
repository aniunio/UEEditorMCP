#include "Actions/NiagaraActions/NiagaraSystemActions.h"

#include "MCPCommonUtils.h"
#include "Actions/NiagaraActions/NiagaraCommonUtils.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NiagaraActor.h"
#include "NiagaraCommon.h"
#include "NiagaraComponent.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraSystemInstanceController.h"

#include "UpgradeNiagaraScriptResults.h"
#include "NiagaraSystemEditorData.h"
#include "NiagaraSystemFactoryNew.h"

FString FCreateNiagaraSystemAction::GetActionName() const
{
	return TEXT("create_niagara_system");
}

bool FCreateNiagaraSystemAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Unused;
	return GetRequiredString(Params, TEXT("asset_path"), Unused, OutError);
}

TSharedPtr<FJsonObject> FCreateNiagaraSystemAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context)
{
	FString AssetPath;
	Params->TryGetStringField(TEXT("asset_path"), AssetPath);

	FString TemplateName = TEXT("empty");
	Params->TryGetStringField(TEXT("template"), TemplateName);

	if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Asset already exists: %s"), *AssetPath));
	}

	FString PackagePath;
	FString AssetName;
	FString AssetError;

	if (!TemplateName.Equals(TEXT("empty"), ESearchCase::IgnoreCase))
	{
		const FString TemplatePath = NiagaraCommonUtils::ResolveTemplateReference(TemplateName);
		if (UNiagaraSystem* TemplateSystem = Cast<UNiagaraSystem>(UEditorAssetLibrary::LoadAsset(TemplatePath)))
		{
			UNiagaraSystem* System = Cast<UNiagaraSystem>(
				FMCPCommonUtils::DuplicateEditorAsset(TemplateSystem, AssetPath, PackagePath, AssetName, AssetError));
			if (!System)
			{
				return FMCPCommonUtils::CreateErrorResponse(
					FString::Printf(TEXT("Failed to duplicate Niagara system template '%s': %s"), *TemplatePath, *AssetError));
			}

			NiagaraCommonUtils::CompileAndSync(System, true);

			FString SaveError;
			const bool bSaved = FMCPCommonUtils::SavePackageSafely(System->GetOutermost(), System, &SaveError);
			if (!bSaved)
			{
				return FMCPCommonUtils::CreateErrorResponse(
					FString::Printf(TEXT("Duplicated Niagara system but failed to save '%s': %s"), *AssetPath, *SaveError));
			}

			TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
			Result->SetBoolField(TEXT("success"), true);
			Result->SetStringField(TEXT("asset_name"), AssetName);
			Result->SetStringField(TEXT("asset_path"), AssetPath);
			Result->SetStringField(TEXT("package_path"), PackagePath);
			Result->SetStringField(TEXT("template"), TemplateName);
			Result->SetStringField(TEXT("template_copy_mode"), TEXT("duplicate_asset"));
			Result->SetBoolField(TEXT("template_applied"), true);
			Result->SetBoolField(TEXT("saved"), bSaved);
			Result->SetNumberField(TEXT("emitter_count"), System->GetNumEmitters());
			return Result;
		}
	}

	UNiagaraSystemFactoryNew* Factory = NewObject<UNiagaraSystemFactoryNew>();
	UNiagaraSystem* System = Cast<UNiagaraSystem>(
		FMCPCommonUtils::CreateEditorAsset(
			AssetPath,
			UNiagaraSystem::StaticClass(),
			Factory,
			PackagePath,
			AssetName,
			AssetError));
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(AssetError);
	}

	const bool bTemplateApplied = NiagaraCommonUtils::TryCopyTemplateToSystem(*System, TemplateName);
	NiagaraCommonUtils::CompileAndSync(System);

	FString SaveError;
	const bool bSaved = FMCPCommonUtils::SavePackageSafely(System->GetOutermost(), System, &SaveError);
	if (!bSaved)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Created Niagara system but failed to save '%s': %s"), *AssetPath, *SaveError));
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("asset_name"), AssetName);
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetStringField(TEXT("package_path"), PackagePath);
	Result->SetStringField(TEXT("template"), TemplateName);
	Result->SetBoolField(TEXT("template_applied"), bTemplateApplied);
	Result->SetBoolField(TEXT("saved"), bSaved);
	Result->SetNumberField(TEXT("emitter_count"), System->GetNumEmitters());
	return Result;
}

bool FGetNiagaraSystemInfoAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError);
}

bool FListNiagaraSystemsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	return true;
}

bool FDeleteNiagaraSystemAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError);
}

bool FCompileNiagaraSystemAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError);
}

bool FSetNiagaraSystemPropertyAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	if (!GetRequiredString(Params, TEXT("property"), Unused, OutError))
	{
		return false;
	}
	if (!GetRequiredString(Params, TEXT("value"), Unused, OutError))
	{
		return false;
	}
	return true;
}

bool FGetNiagaraSystemErrorsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError);
}

bool FGetNiagaraParticleStatsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError);
}

bool FSetNiagaraPlaybackRangeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	double RangeStart = 0.0;
	double RangeEnd = 0.0;
	double FrameRate = 0.0;
	const bool bHasRangeStart = Params.IsValid() && Params->TryGetNumberField(TEXT("range_start"), RangeStart);

	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	if (!Params.IsValid() || !Params->TryGetNumberField(TEXT("range_end"), RangeEnd))
	{
		OutError = TEXT("Required parameter 'range_end' is missing");
		return false;
	}
	if (RangeEnd <= (bHasRangeStart ? RangeStart : 0.0))
	{
		OutError = TEXT("range_end must be greater than range_start");
		return false;
	}
	if (Params.IsValid() && Params->HasField(TEXT("frame_rate")))
	{
		if (!Params->TryGetNumberField(TEXT("frame_rate"), FrameRate))
		{
			OutError = TEXT("Optional parameter 'frame_rate' must be numeric");
			return false;
		}
		if (FrameRate <= 0.0)
		{
			OutError = TEXT("frame_rate must be positive");
			return false;
		}
	}
	return true;
}

bool FGetNiagaraPlaybackRangeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError);
}

bool FGetNiagaraModuleVersionsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	if (!GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError))
	{
		return false;
	}
	return true;
}

bool FUpgradeNiagaraModuleVersionAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	if (!GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError))
	{
		return false;
	}
	if (!GetRequiredString(Params, TEXT("module_name"), Unused, OutError))
	{
		return false;
	}
	if (!GetRequiredString(Params, TEXT("script_usage"), Unused, OutError))
	{
		return false;
	}
	return true;
}


TSharedPtr<FJsonObject> FGetNiagaraSystemInfoAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	Params->TryGetStringField(TEXT("system_path"), SystemPath);

	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	FString IncludeValue = TEXT("all");
	Params->TryGetStringField(TEXT("include"), IncludeValue);

	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	bool bIncludeEmitters = false;
	bool bIncludeParameters = false;
	bool bIncludeCompilation = false;
	{
		FString Normalized = IncludeValue;
		if (Normalized.IsEmpty())
		{
			Normalized = TEXT("all");
		}

		FString TokenSource = Normalized;
		TokenSource.ReplaceInline(TEXT(","), TEXT(" "));

		TArray<FString> Tokens;
		TokenSource.ParseIntoArrayWS(Tokens);

		const auto HasInclude = [&Tokens, &Normalized](const TCHAR* Name)
		{
			for (const FString& Token : Tokens)
			{
				if (Token.Equals(Name, ESearchCase::IgnoreCase))
				{
					return true;
				}
			}

			return Normalized.Contains(Name, ESearchCase::IgnoreCase);
		};

		const bool bIncludeAll = HasInclude(TEXT("all"));
		bIncludeEmitters = bIncludeAll || HasInclude(TEXT("emitters"));
		bIncludeParameters = bIncludeAll || HasInclude(TEXT("parameters"));
		bIncludeCompilation = bIncludeAll || HasInclude(TEXT("compilation"));
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("system_name"), System->GetName());
	Result->SetStringField(TEXT("asset_path"), SystemPath);
	Result->SetNumberField(TEXT("emitter_count"), System->GetNumEmitters());

	if (bIncludeEmitters)
	{
		Result->SetArrayField(TEXT("emitters"), NiagaraCommonUtils::BuildEmitterInfoArray(System, Filter));
	}

	if (bIncludeParameters)
	{
		Result->SetArrayField(TEXT("parameters"), NiagaraCommonUtils::BuildParameterInfoArray(System, Filter));
	}

	if (bIncludeCompilation)
	{
		TSharedPtr<FJsonObject> CompilationObject = MakeShared<FJsonObject>();
		CompilationObject->SetBoolField(TEXT("is_valid"), System->IsValid());
		CompilationObject->SetBoolField(TEXT("is_ready_to_run"), System->IsReadyToRun());
		Result->SetObjectField(TEXT("compilation"), CompilationObject);
	}

	return Result;
}

TSharedPtr<FJsonObject> FListNiagaraSystemsAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SearchPath = TEXT("/Game");
	Params->TryGetStringField(TEXT("path"), SearchPath);

	FString NameFilter;
	Params->TryGetStringField(TEXT("name_filter"), NameFilter);

	double MaxResultsAsNumber = 100.0;
	Params->TryGetNumberField(TEXT("max_results"), MaxResultsAsNumber);
	const int32 MaxResults = FMath::Max(1, static_cast<int32>(MaxResultsAsNumber));

	IAssetRegistry& Registry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(FTopLevelAssetPath(TEXT("/Script/Niagara.NiagaraSystem")), Assets, true);

	TArray<TSharedPtr<FJsonValue>> Systems;
	Systems.Reserve(FMath::Min(MaxResults, Assets.Num()));

	for (const FAssetData& Asset : Assets)
	{
		const FString PackageName = Asset.PackageName.ToString();
		if (!SearchPath.IsEmpty() && !PackageName.StartsWith(SearchPath))
		{
			continue;
		}
		if (!NameFilter.IsEmpty() && !Asset.AssetName.ToString().Contains(NameFilter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		TSharedPtr<FJsonObject> SystemObject = MakeShared<FJsonObject>();
		SystemObject->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		SystemObject->SetStringField(TEXT("path"), Asset.GetObjectPathString());
		Systems.Add(MakeShared<FJsonValueObject>(SystemObject));

		if (Systems.Num() >= MaxResults)
		{
			break;
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("systems"), Systems);
	Result->SetNumberField(TEXT("count"), Systems.Num());
	return Result;
}

TSharedPtr<FJsonObject> FDeleteNiagaraSystemAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	Params->TryGetStringField(TEXT("system_path"), SystemPath);

	bool bForce = false;
	Params->TryGetBoolField(TEXT("force"), bForce);

	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	FString CanonicalPath = UEditorAssetLibrary::GetPathNameForLoadedAsset(System);

	if (!bForce)
	{
		const TArray<TSharedPtr<FJsonValue>> Referencers = NiagaraCommonUtils::CollectExternalReferencers(CanonicalPath);
		if (Referencers.Num() > 0)
		{
			TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
			Result->SetBoolField(TEXT("success"), false);
			Result->SetStringField(TEXT("error"), TEXT("Asset has references. Pass force=true to delete."));
			Result->SetArrayField(TEXT("referencers"), Referencers);
			return Result;
		}
	}

	FString DeleteError;
	const bool bDeleted = FMCPCommonUtils::DeleteEditorAsset(System, CanonicalPath, DeleteError);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), bDeleted);
	Result->SetStringField(TEXT("system_path"), CanonicalPath.IsEmpty() ? SystemPath : CanonicalPath);
	if (!bDeleted)
	{
		Result->SetStringField(TEXT("error"), DeleteError);
	}

	return Result;
}

TSharedPtr<FJsonObject> FCompileNiagaraSystemAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	Params->TryGetStringField(TEXT("system_path"), SystemPath);

	bool bWaitForCompletion = true;
	Params->TryGetBoolField(TEXT("wait_for_completion"), bWaitForCompletion);

	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	System->RequestCompile(true);
	if (bWaitForCompletion)
	{
		System->WaitForCompilationComplete(false, false);
	}

	UEditorAssetLibrary::SaveAsset(SystemPath);

	TArray<TSharedPtr<FJsonValue>> ScriptStatuses;
	int32 ErrorCount = 0;
	NiagaraCommonUtils::CollectCompileStatuses(System, ScriptStatuses, ErrorCount);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetBoolField(TEXT("is_valid"), System->IsValid());
	Result->SetBoolField(TEXT("is_ready_to_run"), System->IsReadyToRun());
	Result->SetBoolField(TEXT("has_outstanding_compilations"), System->HasOutstandingCompilationRequests());
	Result->SetNumberField(TEXT("error_count"), ErrorCount);
	Result->SetArrayField(TEXT("script_statuses"), ScriptStatuses);
	return Result;
}

TSharedPtr<FJsonObject> FSetNiagaraSystemPropertyAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	FString RequestedProperty;
	Params->TryGetStringField(TEXT("system_path"), SystemPath);
	Params->TryGetStringField(TEXT("property"), RequestedProperty);

	FString ValueText;
	Params->TryGetStringField(TEXT("value"), ValueText);

	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	if (FProperty* Property = NiagaraCommonUtils::ResolveSystemProperty(RequestedProperty, ValueText))
	{
		System->Modify();
		Property->ImportText_Direct(*ValueText, Property->ContainerPtrToValuePtr<void>(System), System, PPF_None);
		NiagaraCommonUtils::CompileAndSync(System);
		UEditorAssetLibrary::SaveAsset(SystemPath);

		TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
		Result->SetBoolField(TEXT("success"), true);
		Result->SetStringField(TEXT("property"), Property->GetName());
		Result->SetStringField(TEXT("value"), ValueText);
		return Result;
	}

	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);
	return FMCPCommonUtils::CreateErrorResponse(
		NiagaraCommonUtils::BuildUnknownSystemPropertyError(RequestedProperty, Filter));
}

TSharedPtr<FJsonObject> FGetNiagaraSystemErrorsAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	Params->TryGetStringField(TEXT("system_path"), SystemPath);

	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	FString EmitterFilter;
	Params->TryGetStringField(TEXT("emitter_name"), EmitterFilter);

	FString SeverityFilter = TEXT("all");
	Params->TryGetStringField(TEXT("severity"), SeverityFilter);

	TArray<TSharedPtr<FJsonValue>> Issues;
	for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		const FString EmitterName = Handle.GetName().ToString();
		if (!NiagaraCommonUtils::MatchesNameFilter(EmitterName, EmitterFilter))
		{
			continue;
		}

		FVersionedNiagaraEmitterData* EmitterData =
			NiagaraCommonUtils::GetEmitterData(&Handle);
		if (!EmitterData)
		{
			continue;
		}

		NiagaraCommonUtils::AppendCompileIssues(*EmitterData, EmitterName, SeverityFilter, Issues);
		NiagaraCommonUtils::AppendRendererIssues(Handle, *EmitterData, EmitterName, SeverityFilter, Issues);
	}

	NiagaraCommonUtils::AppendSystemMessageIssues(System, EmitterFilter, SeverityFilter, Issues);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("system_path"), SystemPath);
	Result->SetNumberField(TEXT("issue_count"), Issues.Num());
	Result->SetArrayField(TEXT("issues"), Issues);
	return CreateSuccessResponse(Result);
}

TSharedPtr<FJsonObject> FGetNiagaraParticleStatsAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	Params->TryGetStringField(TEXT("system_path"), SystemPath);

	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	FString EmitterFilter;
	Params->TryGetStringField(TEXT("emitter_name"), EmitterFilter);

	FString ActorName;
	Params->TryGetStringField(TEXT("actor_name"), ActorName);

	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!EditorWorld)
	{
		return CreateErrorResponse(TEXT("No editor world available"), TEXT("no_world"));
	}

	UNiagaraComponent* NiagaraComponent = nullptr;
	FNiagaraSystemInstance* SystemInstance = nullptr;
	FString ResolvedActorName;

	auto TryBindComponentInstance = [&ResolvedActorName](
		UNiagaraComponent* CandidateComponent,
		UNiagaraComponent*& OutComponent,
		FNiagaraSystemInstance*& OutInstance) -> bool
	{
		if (!CandidateComponent)
		{
			return false;
		}

		const FNiagaraSystemInstanceControllerPtr Controller = CandidateComponent->GetSystemInstanceController();
		if (!Controller.IsValid() || !Controller->IsValid())
		{
			return false;
		}

		FNiagaraSystemInstance* CandidateInstance = Controller->GetSystemInstance_Unsafe();
		if (!CandidateInstance)
		{
			return false;
		}

		OutComponent = CandidateComponent;
		OutInstance = CandidateInstance;

		if (AActor* Owner = CandidateComponent->GetOwner())
		{
			ResolvedActorName = Owner->GetActorLabel();
		}

		return true;
	};

	bool bFoundInstance = false;
	if (!ActorName.IsEmpty())
	{
		FString FindError;
		if (ANiagaraActor* NiagaraActor = NiagaraCommonUtils::FindNiagaraActorByName(ActorName, FindError))
		{
			if (UNiagaraComponent* CandidateComponent = NiagaraActor->GetNiagaraComponent())
			{
				if (CandidateComponent->GetAsset() != System)
				{
					return FMCPCommonUtils::CreateErrorResponse(
						FString::Printf(
							TEXT("Actor '%s' is not using system '%s'"),
							*ActorName,
							*SystemPath));
				}

				bFoundInstance = TryBindComponentInstance(CandidateComponent, NiagaraComponent, SystemInstance);
			}
		}
		else
		{
			return FMCPCommonUtils::CreateErrorResponse(FindError);
		}
	}
	else
	{
		for (TActorIterator<ANiagaraActor> It(EditorWorld); It; ++It)
		{
			ANiagaraActor* CandidateActor = *It;
			UNiagaraComponent* CandidateComponent = CandidateActor ? CandidateActor->GetNiagaraComponent() : nullptr;
			if (!CandidateComponent || CandidateComponent->GetAsset() != System)
			{
				continue;
			}

			if (!CandidateComponent->IsActive())
			{
				continue;
			}

			if (TryBindComponentInstance(CandidateComponent, NiagaraComponent, SystemInstance))
			{
				bFoundInstance = true;
				break;
			}
		}

		if (!bFoundInstance)
		{
			bFoundInstance = NiagaraCommonUtils::TryFindPreviewInstance(System, NiagaraComponent, SystemInstance);
		}
	}

	TArray<TSharedPtr<FJsonValue>> Emitters;
	if (bFoundInstance && NiagaraComponent && SystemInstance)
	{
		const TSharedPtr<FJsonObject> BoundsObject = NiagaraCommonUtils::BuildBoundsObject(NiagaraComponent->Bounds.GetBox());
		for (const FNiagaraEmitterInstanceRef& EmitterInstance : SystemInstance->GetEmitters())
		{
			const FNiagaraEmitterHandle& Handle = EmitterInstance->GetEmitterHandle();
			const FString EmitterName = Handle.GetName().ToString();
			if (!NiagaraCommonUtils::MatchesNameFilter(EmitterName, EmitterFilter))
			{
				continue;
			}

			const ENiagaraExecutionState ExecutionState = EmitterInstance->GetExecutionState();
			TSharedPtr<FJsonObject> EmitterObject = MakeShared<FJsonObject>();
			EmitterObject->SetStringField(TEXT("name"), EmitterName);
			EmitterObject->SetNumberField(TEXT("num_particles"), EmitterInstance->GetNumParticles());
			EmitterObject->SetNumberField(TEXT("total_spawned"), EmitterInstance->GetTotalSpawnedParticles());
			EmitterObject->SetStringField(TEXT("execution_state"), NiagaraCommonUtils::ExecutionStateToString(ExecutionState));
			EmitterObject->SetBoolField(TEXT("is_enabled"), Handle.GetIsEnabled());
			EmitterObject->SetBoolField(TEXT("is_active"), ExecutionState == ENiagaraExecutionState::Active);
			if (BoundsObject.IsValid())
			{
				EmitterObject->SetObjectField(TEXT("bounds"), BoundsObject);
			}

			Emitters.Add(MakeShared<FJsonValueObject>(EmitterObject));
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("system_path"), SystemPath);
	if (!ResolvedActorName.IsEmpty())
	{
		Result->SetStringField(TEXT("actor_name"), ResolvedActorName);
	}
	if (!bFoundInstance)
	{
		Result->SetStringField(
			TEXT("message"),
			TEXT("No active Niagara instance found. Spawn or preview the system first."));
	}
	Result->SetNumberField(TEXT("emitter_count"), Emitters.Num());
	Result->SetArrayField(TEXT("emitters"), Emitters);
	return CreateSuccessResponse(Result);
}

TSharedPtr<FJsonObject> FSetNiagaraPlaybackRangeAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	Params->TryGetStringField(TEXT("system_path"), SystemPath);

	double RangeEnd = 0.0;
	Params->TryGetNumberField(TEXT("range_end"), RangeEnd);

	double RangeStart = 0.0;
	Params->TryGetNumberField(TEXT("range_start"), RangeStart);

	int32 FrameRateValue = 60;
	if (Params->HasField(TEXT("frame_rate")))
	{
		FrameRateValue = static_cast<int32>(Params->GetNumberField(TEXT("frame_rate")));
	}

	if (RangeEnd <= RangeStart)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("range_end must be greater than range_start"));
	}
	if (FrameRateValue <= 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("frame_rate must be positive"));
	}

	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	FString EditorDataError;
	UNiagaraSystemEditorData* EditorData = NiagaraCommonUtils::GetSystemEditorData(System, EditorDataError);
	if (!EditorData)
	{
		return FMCPCommonUtils::CreateErrorResponse(EditorDataError);
	}

	const float RangeStartFloat = static_cast<float>(RangeStart);
	const float RangeEndFloat = static_cast<float>(RangeEnd);
	EditorData->SetPlaybackRange(TRange<float>(RangeStartFloat, RangeEndFloat));
	NiagaraCommonUtils::ApplyPlaybackFrameRate(EditorData, FrameRateValue);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("system_path"), SystemPath);
	Result->SetNumberField(TEXT("range_start"), RangeStartFloat);
	Result->SetNumberField(TEXT("range_end"), RangeEndFloat);
	Result->SetNumberField(TEXT("frame_rate"), FrameRateValue);
	return CreateSuccessResponse(Result);
}

TSharedPtr<FJsonObject> FGetNiagaraPlaybackRangeAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	Params->TryGetStringField(TEXT("system_path"), SystemPath);

	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	FString EditorDataError;
	UNiagaraSystemEditorData* EditorData = NiagaraCommonUtils::GetSystemEditorData(System, EditorDataError);
	if (!EditorData)
	{
		return FMCPCommonUtils::CreateErrorResponse(EditorDataError);
	}

	const TRange<float> PlaybackRange = EditorData->GetPlaybackRange();
	const FFrameRate FrameRate = EditorData->GetPlaybackFrameRate();

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("system_path"), SystemPath);
	Result->SetNumberField(
		TEXT("range_start"),
		PlaybackRange.HasLowerBound() ? PlaybackRange.GetLowerBoundValue() : 0.0f);
	Result->SetNumberField(
		TEXT("range_end"),
		PlaybackRange.HasUpperBound() ? PlaybackRange.GetUpperBoundValue() : 0.0f);
	Result->SetNumberField(TEXT("frame_rate"), FrameRate.Numerator);
	Result->SetNumberField(TEXT("frame_rate_denominator"), FrameRate.Denominator);
	Result->SetBoolField(TEXT("is_locked"), EditorData->GetLockPlaybackFrameRate());
	return CreateSuccessResponse(Result);
}

TSharedPtr<FJsonObject> FGetNiagaraModuleVersionsAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	Params->TryGetStringField(TEXT("system_path"), SystemPath);

	FString EmitterName;
	Params->TryGetStringField(TEXT("emitter_name"), EmitterName);

	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	int32 EmitterIndex = INDEX_NONE;
	FString HandleError;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, EmitterIndex, HandleError);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(HandleError);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get emitter data"));
	}

	TArray<TSharedPtr<FJsonValue>> Modules;
	static constexpr ENiagaraScriptUsage Usages[] = {
		ENiagaraScriptUsage::EmitterSpawnScript,
		ENiagaraScriptUsage::EmitterUpdateScript,
		ENiagaraScriptUsage::ParticleSpawnScript,
		ENiagaraScriptUsage::ParticleUpdateScript,
	};

	for (ENiagaraScriptUsage Usage : Usages)
	{
		NiagaraCommonUtils::AppendModuleVersionsForUsage(*EmitterData, Usage, Filter, Modules);
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("system_path"), SystemPath);
	Result->SetStringField(TEXT("emitter_name"), EmitterName);
	Result->SetNumberField(TEXT("module_count"), Modules.Num());
	Result->SetArrayField(TEXT("modules"), Modules);
	return CreateSuccessResponse(Result);
}

TSharedPtr<FJsonObject> FUpgradeNiagaraModuleVersionAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	Params->TryGetStringField(TEXT("system_path"), SystemPath);

	FString EmitterName;
	Params->TryGetStringField(TEXT("emitter_name"), EmitterName);

	FString ModuleName;
	Params->TryGetStringField(TEXT("module_name"), ModuleName);

	FString UsageText;
	Params->TryGetStringField(TEXT("script_usage"), UsageText);

	FString TargetVersionText = TEXT("latest");
	Params->TryGetStringField(TEXT("target_version"), TargetVersionText);

	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	bool bUsageOk = false;
	const ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(UsageText, bUsageOk);
	if (!bUsageOk)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unsupported script_usage: '%s'"), *UsageText));
	}

	UNiagaraScript* UsageScript = nullptr;
	FString ResolveError;
	UNiagaraGraph* Graph = NiagaraCommonUtils::ResolveEmitterStackGraph(System, EmitterName, Usage, UsageScript, ResolveError);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(ResolveError);
	}

	UNiagaraNodeFunctionCall* ModuleCall = NiagaraCommonUtils::FindModuleFunctionCall(Graph, ModuleName);
	if (!ModuleCall)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Module '%s' not found in emitter '%s' usage '%s'"),
				*ModuleName,
				*EmitterName,
				*NiagaraCommonUtils::ScriptUsageToString(Usage)));
	}
	if (!ModuleCall->FunctionScript)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Module '%s' has no function script"), *ModuleName));
	}
	if (!ModuleCall->FunctionScript->IsVersioningEnabled())
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Module '%s' does not support versioning"), *ModuleName));
	}

	const TArray<FNiagaraAssetVersion> AvailableVersions = ModuleCall->FunctionScript->GetAllAvailableVersions();
	if (AvailableVersions.Num() == 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Module '%s' does not expose any versions"), *ModuleName));
	}

	const FNiagaraAssetVersion CurrentVersion = ModuleCall->FunctionScript->GetScriptData(ModuleCall->SelectedScriptVersion)
		? ModuleCall->FunctionScript->GetScriptData(ModuleCall->SelectedScriptVersion)->Version
		: ModuleCall->FunctionScript->GetExposedVersion();
	const FNiagaraAssetVersion LatestVersion = NiagaraCommonUtils::PickLatestVersion(AvailableVersions, CurrentVersion);

	FNiagaraAssetVersion TargetVersion = LatestVersion;
	FGuid TargetVersionGuid = LatestVersion.VersionGuid;
	if (!TargetVersionText.IsEmpty() && !TargetVersionText.Equals(TEXT("latest"), ESearchCase::IgnoreCase))
	{
		bool bFoundTarget = false;
		for (const FNiagaraAssetVersion& Candidate : AvailableVersions)
		{
			const FString CandidateText = NiagaraCommonUtils::FormatNiagaraAssetVersion(Candidate);
			if (CandidateText.Equals(TargetVersionText, ESearchCase::IgnoreCase))
			{
				TargetVersion = Candidate;
				TargetVersionGuid = Candidate.VersionGuid;
				bFoundTarget = true;
				break;
			}
		}

		if (!bFoundTarget)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Version '%s' not found for module '%s'"), *TargetVersionText, *ModuleName));
		}
	}

	const FGuid CurrentVersionGuid = ModuleCall->SelectedScriptVersion.IsValid()
		? ModuleCall->SelectedScriptVersion
		: ModuleCall->FunctionScript->GetExposedVersion().VersionGuid;

	ModuleCall->Modify();
	FNiagaraScriptVersionUpgradeContext UpgradeContext;
	UpgradeContext.bSkipPythonScript = false;
	ModuleCall->ChangeScriptVersion(TargetVersionGuid, UpgradeContext, false, false);

	// We intentionally do not keep the temporary "upgrade note" UI state.
	ModuleCall->PreviousScriptVersion = ModuleCall->SelectedScriptVersion;
	ModuleCall->PythonUpgradeScriptWarnings.Empty();

	NiagaraCommonUtils::CompileAndSync(System, true);
	UEditorAssetLibrary::SaveAsset(SystemPath);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("system_path"), SystemPath);
	Result->SetStringField(TEXT("emitter_name"), EmitterName);
	Result->SetStringField(TEXT("module_name"), ModuleName);
	Result->SetStringField(TEXT("script_usage"), NiagaraCommonUtils::ScriptUsageToString(Usage));
	Result->SetStringField(TEXT("previous_version"), NiagaraCommonUtils::FormatNiagaraAssetVersion(CurrentVersion));
	Result->SetStringField(TEXT("current_version"), NiagaraCommonUtils::FormatNiagaraAssetVersion(TargetVersion));
	Result->SetBoolField(TEXT("changed"), CurrentVersionGuid != TargetVersionGuid);
	return CreateSuccessResponse(Result);
}

// ---------------------------------------------------------------------------
// Action metadata
// ---------------------------------------------------------------------------


FString FGetNiagaraSystemInfoAction::GetActionName() const
{
	return TEXT("get_niagara_system_info");
}

FString FListNiagaraSystemsAction::GetActionName() const
{
	return TEXT("list_niagara_systems");
}

FString FDeleteNiagaraSystemAction::GetActionName() const
{
	return TEXT("delete_niagara_system");
}

FString FCompileNiagaraSystemAction::GetActionName() const
{
	return TEXT("compile_niagara_system");
}

FString FSetNiagaraSystemPropertyAction::GetActionName() const
{
	return TEXT("set_niagara_system_property");
}

FString FGetNiagaraSystemErrorsAction::GetActionName() const
{
	return TEXT("get_niagara_system_errors");
}

FString FGetNiagaraParticleStatsAction::GetActionName() const
{
	return TEXT("get_niagara_particle_stats");
}

FString FSetNiagaraPlaybackRangeAction::GetActionName() const
{
	return TEXT("set_niagara_playback_range");
}

FString FGetNiagaraPlaybackRangeAction::GetActionName() const
{
	return TEXT("get_niagara_playback_range");
}

FString FGetNiagaraModuleVersionsAction::GetActionName() const
{
	return TEXT("get_niagara_module_versions");
}

FString FUpgradeNiagaraModuleVersionAction::GetActionName() const
{
	return TEXT("upgrade_niagara_module_version");
}

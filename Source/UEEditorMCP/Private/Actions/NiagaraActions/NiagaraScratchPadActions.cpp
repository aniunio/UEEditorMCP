#include "Actions/NiagaraActions/NiagaraScratchPadActions.h"

#include "MCPCommonUtils.h"
#include "Actions/NiagaraActions/NiagaraCommonUtils.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraCommon.h"
#include "NiagaraTypes.h"

#include "NiagaraNodeCustomHlsl.h"
#include "ViewModels/NiagaraScratchPadScriptViewModel.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

static TSharedPtr<FJsonObject> CreateScratchPadNameConflictResponse(const FString& ModuleName)
{
	return FMCPCommonUtils::CreateErrorResponse(
		FString::Printf(TEXT("Scratch pad module '%s' already exists"), *ModuleName));
}

TSharedPtr<FJsonObject> FCreateNiagaraScratchPadModuleAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params,
                                                                              FMCPEditorContext& Context)
{
	(void)Context;
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString ModuleName = TEXT("ScratchPadModule");
	Params->TryGetStringField(TEXT("module_name"), ModuleName);
	if (ModuleName.IsEmpty())
	{
		ModuleName = TEXT("ScratchPadModule");
	}

	FString ModuleTypeStr = TEXT("module");
	Params->TryGetStringField(TEXT("module_type"), ModuleTypeStr);
	const ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScratchPadModuleType(ModuleTypeStr);

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	if (NiagaraCommonUtils::HasScratchPadScriptNameConflict(System, ModuleName))
	{
		return CreateScratchPadNameConflictResponse(ModuleName);
	}

	const FName ExactName(*ModuleName);

	System->Modify();

	UNiagaraScript* NewScript = nullptr;
	bool bUsedTemplate = false;

	if (UNiagaraScript* Template = NiagaraCommonUtils::LoadDefaultScratchPadTemplateScript(Usage))
	{
		NewScript = Cast<UNiagaraScript>(StaticDuplicateObject(Template, System, ExactName));
		if (NewScript)
		{
			NewScript->ClearFlags(RF_Public | RF_Standalone);
			NewScript->SetFlags(RF_Transactional);
			NewScript->SetUsage(Usage);
			bUsedTemplate = true;
		}
	}

	if (!NewScript)
	{
		NewScript = NewObject<UNiagaraScript>(System, ExactName, RF_Transactional);
		if (!NewScript)
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create scratch pad script"));
		}
		NewScript->SetUsage(Usage);

		UNiagaraScriptSource* ScriptSource = NewObject<UNiagaraScriptSource>(
			NewScript, TEXT("ScriptSource"), RF_Transactional);
		UNiagaraGraph* Graph = NewObject<UNiagaraGraph>(
			ScriptSource, TEXT("NiagaraGraph"), RF_Transactional);
		ScriptSource->NodeGraph = Graph;
		NewScript->SetLatestSource(ScriptSource);

		NiagaraCommonUtils::BuildMinimalScratchGraph(NewScript, Graph, Usage);
	}

	System->ScratchPadScripts.Add(NewScript);
	System->MarkPackageDirty();
	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, NewScript->GetName());

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("module_name"), NewScript->GetName());
	Result->SetStringField(TEXT("system_path"), SystemPath);
	Result->SetStringField(TEXT("module_type"), ModuleTypeStr);
	Result->SetBoolField(TEXT("used_template"), bUsedTemplate);
	Result->SetNumberField(TEXT("scratch_pad_count"), System->ScratchPadScripts.Num());
	return Result;
}

TSharedPtr<FJsonObject> FDuplicateNiagaraScratchPadModuleAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params,
                                                                                 FMCPEditorContext& Context)
{
	(void)Context;
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}
	FString SourceName;
	if (!Params->TryGetStringField(TEXT("module_name"), SourceName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name' parameter"));
	}

	FString NewName;
	Params->TryGetStringField(TEXT("new_name"), NewName);

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	UNiagaraScript* Source = NiagaraCommonUtils::FindScratchPadScript(System, SourceName);
	if (!Source)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Scratch pad module '%s' not found"), *SourceName));
	}

	if (NewName.IsEmpty())
	{
		NewName = SourceName + TEXT("_Copy");
	}

	if (NiagaraCommonUtils::HasScratchPadScriptNameConflict(System, NewName))
	{
		return CreateScratchPadNameConflictResponse(NewName);
	}

	const FName ExactName(*NewName);
	System->Modify();
	UNiagaraScript* Duplicate = Cast<UNiagaraScript>(StaticDuplicateObject(Source, System, ExactName));
	if (!Duplicate)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Duplication failed"));
	}
	Duplicate->ClearFlags(RF_Public | RF_Standalone);
	Duplicate->SetFlags(RF_Transactional);

	System->ScratchPadScripts.Add(Duplicate);
	System->MarkPackageDirty();
	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, Duplicate->GetName());

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("source_name"), SourceName);
	Result->SetStringField(TEXT("new_name"), Duplicate->GetName());
	Result->SetNumberField(TEXT("scratch_pad_count"), System->ScratchPadScripts.Num());
	return Result;
}

TSharedPtr<FJsonObject> FDeleteNiagaraScratchPadModuleAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params,
                                                                              FMCPEditorContext& Context)
{
	(void)Context;
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}
	FString ModuleName;
	if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name' parameter"));
	}

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 RemovedIndex = INDEX_NONE;
	System->Modify();
	for (int32 i = 0; i < System->ScratchPadScripts.Num(); ++i)
	{
		UNiagaraScript* Script = System->ScratchPadScripts[i];
		if (Script && Script->GetName().Equals(ModuleName, ESearchCase::IgnoreCase))
		{
			System->ScratchPadScripts.RemoveAt(i);
			RemovedIndex = i;
			break;
		}
	}

	if (RemovedIndex == INDEX_NONE)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Scratch pad module '%s' not found"), *ModuleName));
	}

	System->MarkPackageDirty();
	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, FString());

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("module_name"), ModuleName);
	Result->SetNumberField(TEXT("removed_index"), RemovedIndex);
	Result->SetNumberField(TEXT("scratch_pad_count"), System->ScratchPadScripts.Num());
	return Result;
}

TSharedPtr<FJsonObject> FRenameNiagaraScratchPadModuleAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params,
                                                                              FMCPEditorContext& Context)
{
	(void)Context;
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}
	FString OldName;
	if (!Params->TryGetStringField(TEXT("module_name"), OldName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name' parameter"));
	}
	FString NewName;
	if (!Params->TryGetStringField(TEXT("new_name"), NewName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'new_name' parameter"));
	}
	if (NewName.IsEmpty())
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'new_name' parameter"));
	}

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}
	UNiagaraScript* Script = NiagaraCommonUtils::FindScratchPadScript(System, OldName);
	if (!Script)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Scratch pad module '%s' not found"), *OldName));
	}

	if (!Script->GetName().Equals(NewName, ESearchCase::IgnoreCase) &&
		NiagaraCommonUtils::HasScratchPadScriptNameConflict(System, NewName))
	{
		return CreateScratchPadNameConflictResponse(NewName);
	}

	const FName ExactName(*NewName);
	Script->Modify();
	if (!Script->Rename(*ExactName.ToString(), System, REN_DontCreateRedirectors))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Rename failed"));
	}
	System->MarkPackageDirty();
	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, Script->GetName());

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("old_name"), OldName);
	Result->SetStringField(TEXT("new_name"), Script->GetName());
	return Result;
}

TSharedPtr<FJsonObject> FListNiagaraScratchPadModulesAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params,
                                                                             FMCPEditorContext& Context)
{
	(void)Context;
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	TArray<TSharedPtr<FJsonValue>> Scripts;
	for (int32 i = 0; i < System->ScratchPadScripts.Num(); ++i)
	{
		Scripts.Add(MakeShared<FJsonValueObject>(NiagaraCommonUtils::ScratchPadScriptToJson(System->ScratchPadScripts[i], i)));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("system_path"), SystemPath);
	Result->SetArrayField(TEXT("modules"), Scripts);
	Result->SetNumberField(TEXT("count"), Scripts.Num());
	return Result;
}

TSharedPtr<FJsonObject> FApplyNiagaraScratchPadAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	(void)Context;
	FString SystemPath;
	FString ModuleName;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath) ||
		!Params->TryGetStringField(TEXT("module_name"), ModuleName))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing 'system_path' or 'module_name'"));
	}
	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	TSharedPtr<FNiagaraScratchPadScriptViewModel> ScriptVM = NiagaraCommonUtils::GetScratchPadScriptViewModel(System, ModuleName);
	bool bUsedViewModel = ScriptVM.IsValid();
	if (!ScriptVM.IsValid())
	{
		if (!NiagaraCommonUtils::FindScratchPadScript(System, ModuleName))
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Scratch pad module '%s' not found on system"), *ModuleName));
		}
		NiagaraCommonUtils::CompileAndSync(System, true);
		System->MarkPackageDirty();
	}
	else
	{
		ScriptVM->ApplyChanges();
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("system_path"), SystemPath);
	Result->SetStringField(TEXT("module_name"), ModuleName);
	Result->SetStringField(TEXT("action"), TEXT("apply"));
	Result->SetBoolField(TEXT("used_view_model"), bUsedViewModel);
	Result->SetStringField(
		TEXT("apply_mode"),
		bUsedViewModel ? TEXT("editor_view_model") : TEXT("direct_asset_compile"));
	return Result;
}

TSharedPtr<FJsonObject> FApplyAndSaveNiagaraScratchPadAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params,
                                                                              FMCPEditorContext& Context)
{
	(void)Context;
	FString SystemPath;
	FString ModuleName;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath) ||
		!Params->TryGetStringField(TEXT("module_name"), ModuleName))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing 'system_path' or 'module_name'"));
	}
	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	TSharedPtr<FNiagaraScratchPadScriptViewModel> ScriptVM = NiagaraCommonUtils::GetScratchPadScriptViewModel(System, ModuleName);
	bool bUsedViewModel = ScriptVM.IsValid();
	if (!ScriptVM.IsValid())
	{
		UNiagaraScript* Script = NiagaraCommonUtils::FindScratchPadScript(System, ModuleName);
		if (!Script)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Scratch pad module '%s' not found on system"), *ModuleName));
		}

		NiagaraCommonUtils::CompileAndSync(System, true);
		System->MarkPackageDirty();

		UPackage* Package = System->GetOutermost();
		if (!Package)
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("Scratch pad system package is null"));
		}

		const FString PackageName = Package->GetName();
		FString SaveError;
		if (!FMCPCommonUtils::SavePackageSafely(Package, System, &SaveError))
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Failed to save package '%s': %s"), *PackageName, *SaveError));
		}
	}
	else
	{
		ScriptVM->ApplyChangesAndSave();
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("system_path"), SystemPath);
	Result->SetStringField(TEXT("module_name"), ModuleName);
	Result->SetStringField(TEXT("action"), TEXT("apply_and_save"));
	Result->SetBoolField(TEXT("used_view_model"), bUsedViewModel);
	Result->SetStringField(
		TEXT("apply_mode"),
		bUsedViewModel ? TEXT("editor_view_model") : TEXT("direct_asset_compile_and_save"));
	return Result;
}

TSharedPtr<FJsonObject> FFindNiagaraScratchPadUsageAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params,
                                                                           FMCPEditorContext& Context)
{
	(void)Context;
	FString SystemPath;
	FString ModuleName;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath) ||
		!Params->TryGetStringField(TEXT("module_name"), ModuleName))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing 'system_path' or 'module_name'"));
	}

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	UNiagaraScript* TargetScript = NiagaraCommonUtils::FindScratchPadScript(System, ModuleName);
	if (!TargetScript)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Scratch pad module '%s' not found on system"), *ModuleName));
	}
	const bool bTargetIsDynamicInput =
		TargetScript->GetUsage() == ENiagaraScriptUsage::DynamicInput;

	TArray<TSharedPtr<FJsonValue>> Sites;

	auto ScanGraph = [&](UNiagaraGraph* Graph, const FString& EmitterName, const FString& UsageLabel)
	{
		if (!Graph)
		{
			return;
		}
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UNiagaraNodeFunctionCall* Call = Cast<UNiagaraNodeFunctionCall>(Node);
			if (!Call || Call->FunctionScript != TargetScript)
			{
				continue;
			}

			auto Site = MakeShared<FJsonObject>();
			Site->SetStringField(TEXT("emitter"), EmitterName);
			Site->SetStringField(TEXT("script_usage"), UsageLabel);
			Site->SetStringField(TEXT("function_name"), Call->GetFunctionName());
			Site->SetStringField(TEXT("node_id"),
			                     Call->NodeGuid.ToString(EGuidFormats::DigitsWithHyphens));
			Site->SetBoolField(TEXT("is_dynamic_input"), bTargetIsDynamicInput);
			Sites.Add(MakeShared<FJsonValueObject>(Site));
		}
	};

	if (UNiagaraScript* SysSpawn = System->GetSystemSpawnScript())
	{
		if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(SysSpawn->GetLatestSource()))
		{
			ScanGraph(Source->NodeGraph, TEXT("<system>"), TEXT("system_spawn"));
		}
	}
	if (UNiagaraScript* SysUpdate = System->GetSystemUpdateScript())
	{
		if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(SysUpdate->GetLatestSource()))
		{
			ScanGraph(Source->NodeGraph, TEXT("<system>"), TEXT("system_update"));
		}
	}

	static constexpr ENiagaraScriptUsage Usages[] = {
		ENiagaraScriptUsage::EmitterSpawnScript,
		ENiagaraScriptUsage::EmitterUpdateScript,
		ENiagaraScriptUsage::ParticleSpawnScript,
		ENiagaraScriptUsage::ParticleUpdateScript,
	};

	for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		FVersionedNiagaraEmitterData* Data =
			Handle.GetEmitterData();
		if (!Data)
		{
			continue;
		}
		for (ENiagaraScriptUsage Usage : Usages)
		{
			if (UNiagaraScript* Script = Data->GetScript(Usage, FGuid()))
			{
				if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource()))
				{
					ScanGraph(Source->NodeGraph,
					          Handle.GetName().ToString(),
					          NiagaraCommonUtils::ScriptUsageToString(Usage));
				}
			}
		}
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("target_module"), ModuleName);
	Result->SetBoolField(TEXT("target_is_dynamic_input"), bTargetIsDynamicInput);
	Result->SetNumberField(TEXT("usage_count"), Sites.Num());
	Result->SetArrayField(TEXT("sites"), Sites);
	return Result;
}

TSharedPtr<FJsonObject> FCreateNiagaraModuleAssetAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	(void)Context;
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
	}

	FString ModuleTypeStr = TEXT("module");
	Params->TryGetStringField(TEXT("module_type"), ModuleTypeStr);
	const ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScratchPadModuleType(ModuleTypeStr);

	FString PackagePath;
	FString AssetName;
	int32 LastSlash = INDEX_NONE;
	if (!AssetPath.FindLastChar('/', LastSlash) || LastSlash <= 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Invalid asset_path format. Expected '/Game/Path/AssetName'"));
	}
	PackagePath = AssetPath.Left(LastSlash);
	AssetName = AssetPath.Mid(LastSlash + 1);
	if (AssetName.IsEmpty())
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Asset name is empty"));
	}

	const FString FullPackagePath = PackagePath / AssetName;
	UPackage* Package = CreatePackage(*FullPackagePath);
	if (!Package)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to create package at '%s'"), *FullPackagePath));
	}
	Package->FullyLoad();

	UNiagaraScript* NewScript = nullptr;
	bool bUsedTemplate = false;

	if (UNiagaraScript* Template = NiagaraCommonUtils::LoadDefaultScratchPadTemplateScript(Usage))
	{
		NewScript = Cast<UNiagaraScript>(StaticDuplicateObject(Template, Package, FName(*AssetName)));
		if (NewScript)
		{
			NewScript->SetFlags(RF_Public | RF_Standalone | RF_Transactional);
			NewScript->SetUsage(Usage);
			bUsedTemplate = true;
		}
	}

	if (!NewScript)
	{
		NewScript = NewObject<UNiagaraScript>(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
		if (!NewScript)
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Niagara script"));
		}
		NewScript->SetUsage(Usage);

		UNiagaraScriptSource* ScriptSource = NewObject<UNiagaraScriptSource>(
			NewScript, TEXT("ScriptSource"), RF_Transactional);
		UNiagaraGraph* Graph = NewObject<UNiagaraGraph>(
			ScriptSource, TEXT("NiagaraGraph"), RF_Transactional);
		ScriptSource->NodeGraph = Graph;
		NewScript->SetLatestSource(ScriptSource);

		NiagaraCommonUtils::BuildMinimalScratchGraph(NewScript, Graph, Usage);
	}

	FString DescriptionStr;
	if (Params->TryGetStringField(TEXT("description"), DescriptionStr))
	{
		FTextProperty* DescProp = CastField<FTextProperty>(
			UNiagaraScript::StaticClass()->FindPropertyByName(TEXT("Description")));
		if (DescProp)
		{
			DescProp->SetPropertyValue_InContainer(NewScript, FText::FromString(DescriptionStr));
		}
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewScript);

	FString SaveError;
	const bool bSaved = FMCPCommonUtils::SavePackageSafely(Package, NewScript, &SaveError);
	if (!bSaved)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Created module asset but failed to save '%s': %s"), *FullPackagePath, *SaveError));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("asset_path"), NewScript->GetPathName());
	Result->SetStringField(TEXT("asset_name"), AssetName);
	Result->SetStringField(TEXT("module_type"), ModuleTypeStr);
	Result->SetStringField(TEXT("package_path"), PackagePath);
	Result->SetBoolField(TEXT("used_template"), bUsedTemplate);
	Result->SetBoolField(TEXT("saved"), bSaved);
	return Result;
}

TSharedPtr<FJsonObject> FSetNiagaraScratchPadHlslAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	(void)Context;
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}
	FString ModuleName;
	if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name' parameter"));
	}
	FString HlslCode;
	if (!Params->TryGetStringField(TEXT("hlsl_code"), HlslCode))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'hlsl_code' parameter"));
	}
	bool bClearExistingPins = false;
	Params->TryGetBoolField(TEXT("clear_existing_pins"), bClearExistingPins);

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}
	TArray<UNiagaraNodeCustomHlsl*> HlslNodes;
	NiagaraCommonUtils::CollectScratchPadHlslNodes(System, ModuleName, HlslNodes);
	if (HlslNodes.Num() == 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Scratch pad module '%s' not found or has no valid graph"), *ModuleName));
	}

	struct FPinSpec
	{
		FName Name;
		FNiagaraTypeDefinition Type;
	};
	TArray<FPinSpec> InputSpecs;
	TArray<FPinSpec> OutputSpecs;

	auto CollectSpecs = [&](const TCHAR* FieldName, TArray<FPinSpec>& Out)
	{
		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (!Params->TryGetArrayField(FieldName, Arr))
		{
			return;
		}
		for (const TSharedPtr<FJsonValue>& Val : *Arr)
		{
			const TSharedPtr<FJsonObject>* Obj = nullptr;
			if (!Val->TryGetObject(Obj))
			{
				continue;
			}
			FString PinName;
			FString PinType = TEXT("float");
			(*Obj)->TryGetStringField(TEXT("name"), PinName);
			(*Obj)->TryGetStringField(TEXT("type"), PinType);
			if (PinName.IsEmpty())
			{
				continue;
			}
			FNiagaraTypeDefinition TypeDef;
			if (!NiagaraCommonUtils::ParseTypeDef(PinType, TypeDef))
			{
				continue;
			}
			Out.Add({FName(*PinName), TypeDef});
		}
	};
	CollectSpecs(TEXT("inputs"), InputSpecs);
	CollectSpecs(TEXT("outputs"), OutputSpecs);

	int32 InputsAdded = 0;
	int32 OutputsAdded = 0;

	for (UNiagaraNodeCustomHlsl* HlslNode : HlslNodes)
	{
		if (!HlslNode)
		{
			continue;
		}

		if (bClearExistingPins)
		{
			TArray<UEdGraphPin*> PinsToRemove;
			for (UEdGraphPin* Pin : HlslNode->Pins)
			{
				if (Pin && !NiagaraCommonUtils::IsAddPin(Pin))
				{
					PinsToRemove.Add(Pin);
				}
			}
			HlslNode->Modify();
			for (UEdGraphPin* Pin : PinsToRemove)
			{
				Pin->BreakAllPinLinks();
				HlslNode->RemovePin(Pin);
			}
		}

		for (const FPinSpec& Spec : InputSpecs)
		{
			if (NiagaraCommonUtils::AddTypedPin(HlslNode, EGPD_Input, Spec.Type, Spec.Name))
			{
				++InputsAdded;
			}
		}
		for (const FPinSpec& Spec : OutputSpecs)
		{
			if (NiagaraCommonUtils::AddTypedPin(HlslNode, EGPD_Output, Spec.Type, Spec.Name))
			{
				++OutputsAdded;
			}
		}

		NiagaraCommonUtils::SetCustomHlslViaReflection(HlslNode, HlslCode);

		if (UEdGraph* Graph = HlslNode->GetGraph())
		{
			Graph->NotifyGraphChanged();
		}
	}

	InputsAdded /= HlslNodes.Num();
	OutputsAdded /= HlslNodes.Num();

	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, ModuleName);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("module_name"), ModuleName);
	Result->SetNumberField(TEXT("hlsl_length"), HlslCode.Len());
	Result->SetNumberField(TEXT("inputs_added"), InputsAdded);
	Result->SetNumberField(TEXT("outputs_added"), OutputsAdded);
	Result->SetBoolField(TEXT("cleared_existing_pins"), bClearExistingPins);
	return Result;
}

TSharedPtr<FJsonObject> FAddNiagaraCustomHlslInputAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	(void)Context;
	FString PinName;
	if (!Params->TryGetStringField(TEXT("pin_name"), PinName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pin_name' parameter"));
	}
	FString PinType = TEXT("float");
	Params->TryGetStringField(TEXT("pin_type"), PinType);

	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	TArray<UNiagaraNodeCustomHlsl*> HlslNodes;
	FString Error;
	if (!NiagaraCommonUtils::ResolveScratchPadHlslNodes(Params, System, ModuleName, HlslNodes, Error))
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FNiagaraTypeDefinition TypeDef;
	if (!NiagaraCommonUtils::ParseTypeDef(PinType, TypeDef))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unknown pin type '%s'"), *PinType));
	}

	bool bAny = false;
	for (UNiagaraNodeCustomHlsl* HlslNode : HlslNodes)
	{
		if (NiagaraCommonUtils::AddTypedPin(HlslNode, EGPD_Input, TypeDef, FName(*PinName)))
		{
			bAny = true;
		}
	}
	if (!bAny)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to add pin"));
	}
	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, ModuleName);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("pin_name"), PinName);
	Result->SetStringField(TEXT("pin_type"), NiagaraCommonUtils::TypeDefToString(TypeDef));
	Result->SetNumberField(TEXT("updated_node_count"), HlslNodes.Num());
	return Result;
}

TSharedPtr<FJsonObject> FAddNiagaraCustomHlslOutputAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params,
                                                                           FMCPEditorContext& Context)
{
	(void)Context;
	FString PinName;
	if (!Params->TryGetStringField(TEXT("pin_name"), PinName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pin_name' parameter"));
	}
	FString PinType = TEXT("float");
	Params->TryGetStringField(TEXT("pin_type"), PinType);

	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	TArray<UNiagaraNodeCustomHlsl*> HlslNodes;
	FString Error;
	if (!NiagaraCommonUtils::ResolveScratchPadHlslNodes(Params, System, ModuleName, HlslNodes, Error))
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FNiagaraTypeDefinition TypeDef;
	if (!NiagaraCommonUtils::ParseTypeDef(PinType, TypeDef))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unknown pin type '%s'"), *PinType));
	}

	bool bAny = false;
	for (UNiagaraNodeCustomHlsl* HlslNode : HlslNodes)
	{
		if (NiagaraCommonUtils::AddTypedPin(HlslNode, EGPD_Output, TypeDef, FName(*PinName)))
		{
			bAny = true;
		}
	}
	if (!bAny)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to add pin"));
	}
	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, ModuleName);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("pin_name"), PinName);
	Result->SetStringField(TEXT("pin_type"), NiagaraCommonUtils::TypeDefToString(TypeDef));
	Result->SetNumberField(TEXT("updated_node_count"), HlslNodes.Num());
	return Result;
}

TSharedPtr<FJsonObject> FRenameNiagaraCustomHlslPinAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params,
                                                                           FMCPEditorContext& Context)
{
	(void)Context;
	FString OldName;
	FString NewName;
	if (!Params->TryGetStringField(TEXT("old_name"), OldName) ||
		!Params->TryGetStringField(TEXT("new_name"), NewName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'old_name' or 'new_name' parameter"));
	}

	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	TArray<UNiagaraNodeCustomHlsl*> HlslNodes;
	FString Error;
	if (!NiagaraCommonUtils::ResolveScratchPadHlslNodes(Params, System, ModuleName, HlslNodes, Error))
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	bool bAny = false;
	FString LastError;
	for (UNiagaraNodeCustomHlsl* HlslNode : HlslNodes)
	{
		FString RenameError;
		if (NiagaraCommonUtils::RenameDynamicPin(HlslNode, FName(*OldName), FName(*NewName), RenameError))
		{
			bAny = true;
		}
		else
		{
			LastError = RenameError;
		}
	}
	if (!bAny)
	{
		return FMCPCommonUtils::CreateErrorResponse(LastError.IsEmpty() ? TEXT("Rename failed") : LastError);
	}
	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, ModuleName);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("old_name"), OldName);
	Result->SetStringField(TEXT("new_name"), NewName);
	Result->SetNumberField(TEXT("updated_node_count"), HlslNodes.Num());
	return Result;
}

TSharedPtr<FJsonObject> FRemoveNiagaraCustomHlslPinAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params,
                                                                           FMCPEditorContext& Context)
{
	(void)Context;
	FString PinName;
	if (!Params->TryGetStringField(TEXT("pin_name"), PinName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pin_name' parameter"));
	}

	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	TArray<UNiagaraNodeCustomHlsl*> HlslNodes;
	FString Error;
	if (!NiagaraCommonUtils::ResolveScratchPadHlslNodes(Params, System, ModuleName, HlslNodes, Error))
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	bool bAny = false;
	FString LastError;
	for (UNiagaraNodeCustomHlsl* HlslNode : HlslNodes)
	{
		FString RemoveError;
		if (NiagaraCommonUtils::RemoveDynamicPinByName(HlslNode, FName(*PinName), RemoveError))
		{
			bAny = true;
		}
		else
		{
			LastError = RemoveError;
		}
	}
	if (!bAny)
	{
		return FMCPCommonUtils::CreateErrorResponse(LastError.IsEmpty() ? TEXT("Remove failed") : LastError);
	}
	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, ModuleName);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("pin_name"), PinName);
	Result->SetNumberField(TEXT("updated_node_count"), HlslNodes.Num());
	return Result;
}

// ---------------------------------------------------------------------------
// Action metadata
// ---------------------------------------------------------------------------

FString FCreateNiagaraScratchPadModuleAction::GetActionName() const
{
	return TEXT("create_niagara_scratch_pad_module");
}

bool FCreateNiagaraScratchPadModuleAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError);
}

FString FDuplicateNiagaraScratchPadModuleAction::GetActionName() const
{
	return TEXT("duplicate_niagara_scratch_pad_module");
}

bool FDuplicateNiagaraScratchPadModuleAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("module_name"), Unused, OutError);
}

FString FDeleteNiagaraScratchPadModuleAction::GetActionName() const
{
	return TEXT("delete_niagara_scratch_pad_module");
}

bool FDeleteNiagaraScratchPadModuleAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("module_name"), Unused, OutError);
}

FString FRenameNiagaraScratchPadModuleAction::GetActionName() const
{
	return TEXT("rename_niagara_scratch_pad_module");
}

bool FRenameNiagaraScratchPadModuleAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("module_name"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("new_name"), Unused, OutError);
}

FString FListNiagaraScratchPadModulesAction::GetActionName() const
{
	return TEXT("list_niagara_scratch_pad_modules");
}

bool FListNiagaraScratchPadModulesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError);
}

FString FApplyNiagaraScratchPadAction::GetActionName() const
{
	return TEXT("apply_niagara_scratch_pad");
}

bool FApplyNiagaraScratchPadAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("module_name"), Unused, OutError);
}

FString FApplyAndSaveNiagaraScratchPadAction::GetActionName() const
{
	return TEXT("apply_and_save_niagara_scratch_pad");
}

bool FApplyAndSaveNiagaraScratchPadAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("module_name"), Unused, OutError);
}

FString FFindNiagaraScratchPadUsageAction::GetActionName() const
{
	return TEXT("find_niagara_scratch_pad_usage");
}

bool FFindNiagaraScratchPadUsageAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("module_name"), Unused, OutError);
}

FString FCreateNiagaraModuleAssetAction::GetActionName() const
{
	return TEXT("create_niagara_module_asset");
}

bool FCreateNiagaraModuleAssetAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("asset_path"), Unused, OutError);
}

FString FSetNiagaraScratchPadHlslAction::GetActionName() const
{
	return TEXT("set_niagara_scratch_pad_hlsl");
}

bool FSetNiagaraScratchPadHlslAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("module_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("hlsl_code"), Unused, OutError))
	{
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>* Inputs = GetOptionalArray(Params, TEXT("inputs"));
	if (Params.IsValid() && Params->HasField(TEXT("inputs")) && Inputs == nullptr)
	{
		OutError = TEXT("Optional parameter 'inputs' must be an array");
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>* Outputs = GetOptionalArray(Params, TEXT("outputs"));
	if (Params.IsValid() && Params->HasField(TEXT("outputs")) && Outputs == nullptr)
	{
		OutError = TEXT("Optional parameter 'outputs' must be an array");
		return false;
	}
	return true;
}

FString FAddNiagaraCustomHlslInputAction::GetActionName() const
{
	return TEXT("add_niagara_custom_hlsl_input");
}

bool FAddNiagaraCustomHlslInputAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("module_name"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("pin_name"), Unused, OutError);
}

FString FAddNiagaraCustomHlslOutputAction::GetActionName() const
{
	return TEXT("add_niagara_custom_hlsl_output");
}

bool FAddNiagaraCustomHlslOutputAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("module_name"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("pin_name"), Unused, OutError);
}

FString FRenameNiagaraCustomHlslPinAction::GetActionName() const
{
	return TEXT("rename_niagara_custom_hlsl_pin");
}

bool FRenameNiagaraCustomHlslPinAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("module_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("old_name"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("new_name"), Unused, OutError);
}

FString FRemoveNiagaraCustomHlslPinAction::GetActionName() const
{
	return TEXT("remove_niagara_custom_hlsl_pin");
}

bool FRemoveNiagaraCustomHlslPinAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("module_name"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("pin_name"), Unused, OutError);
}

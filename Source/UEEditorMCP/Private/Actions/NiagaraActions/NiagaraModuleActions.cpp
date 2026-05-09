#include "Actions/NiagaraActions/NiagaraModuleActions.h"
#include "EditorAssetLibrary.h"
#include "MCPCommonUtils.h"
#include "Actions/EditorAction.h"
#include "Actions/NiagaraActions/NiagaraCommonUtils.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScript.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeCustomHlsl.h"
#include "NiagaraCommon.h"
#include "NiagaraTypes.h"

#include "EdGraph/EdGraphPin.h"
#include "ScopedTransaction.h"
#include "NiagaraEditorUtilities.h"
#include "AssetRegistry/AssetData.h"

#include "NiagaraEditorModule.h"
#include "EdGraphSchema_Niagara.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraStackFunctionInput.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "ViewModels/NiagaraSystemViewModel.h"
#include "ViewModels/NiagaraEmitterHandleViewModel.h"
#include "ViewModels/Stack/NiagaraStackViewModel.h"
#include "ViewModels/Stack/NiagaraStackModuleItem.h"
#include "AssetRegistry/ARFilter.h"
#include "NiagaraDataInterfaceCurve.h"
#include "NiagaraDataInterfaceVectorCurve.h"
#include "NiagaraDataInterfaceColorCurve.h"

#include "EdGraph/EdGraphPin.h"
#include "Curves/RichCurve.h"
#include "ScopedTransaction.h"

#include "Stateless/NiagaraStatelessDistribution.h"
#include "NiagaraParameterStore.h"
#include "NiagaraDataInterface.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "UObject/UObjectIterator.h"
#include "NiagaraNodeParameterMapSet.h"
#include "NiagaraNodeParameterMapGet.h"
#include "NiagaraNodeAssignment.h"
#include "NiagaraParameterMapHistory.h"
#include "NiagaraConstants.h"
#include "AssetRegistry/ARFilter.h"

#include "EdGraph/EdGraphPin.h"

FString FGetNiagaraModulesAction::GetActionName() const
{
	return TEXT("get_niagara_modules");
}

bool FGetNiagaraModulesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError);
}

FString FAddNiagaraModuleAction::GetActionName() const
{
	return TEXT("add_niagara_module");
}

bool FAddNiagaraModuleAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("module_path"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("script_usage"), Unused, OutError);
}

FString FRemoveNiagaraModuleAction::GetActionName() const
{
	return TEXT("remove_niagara_module");
}

bool FRemoveNiagaraModuleAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("module_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("script_usage"), Unused, OutError);
}

FString FSetNiagaraModuleEnabledAction::GetActionName() const
{
	return TEXT("set_niagara_module_enabled");
}

bool FSetNiagaraModuleEnabledAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("module_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("script_usage"), Unused, OutError);
}

FString FReorderNiagaraModuleAction::GetActionName() const
{
	return TEXT("reorder_niagara_module");
}

bool FReorderNiagaraModuleAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	double NewIndex = 0.0;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("module_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("script_usage"), Unused, OutError))
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

FString FGetNiagaraModuleInputsAction::GetActionName() const
{
	return TEXT("get_niagara_module_inputs");
}

bool FGetNiagaraModuleInputsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("module_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("script_usage"), Unused, OutError);
}

FString FSetNiagaraModuleInputAction::GetActionName() const
{
	return TEXT("set_niagara_module_input");
}

bool FSetNiagaraModuleInputAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("module_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("input_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("script_usage"), Unused, OutError))
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

FString FSetNiagaraDynamicInputAction::GetActionName() const
{
	return TEXT("set_niagara_dynamic_input");
}

bool FSetNiagaraDynamicInputAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("module_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("input_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("script_usage"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("dynamic_input_type"), Unused, OutError);
}

FString FSetNiagaraCurveAction::GetActionName() const
{
	return TEXT("set_niagara_curve");
}

bool FSetNiagaraCurveAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	const TArray<TSharedPtr<FJsonValue>>* Keys = nullptr;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("module_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("input_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("script_usage"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("curve_type"), Unused, OutError))
	{
		return false;
	}
	if (!Params.IsValid() || !Params->TryGetArrayField(TEXT("keys"), Keys) || Keys == nullptr || Keys->Num() == 0)
	{
		OutError = TEXT("Required parameter 'keys' is missing or empty");
		return false;
	}
	return true;
}

FString FGetNiagaraRapidIterationParametersAction::GetActionName() const
{
	return TEXT("get_niagara_rapid_iteration_parameters");
}

bool FGetNiagaraRapidIterationParametersAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context,
                                                         FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError);
}

FString FSetNiagaraRapidIterationParameterAction::GetActionName() const
{
	return TEXT("set_niagara_rapid_iteration_parameter");
}

bool FSetNiagaraRapidIterationParameterAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context,
                                                        FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("module_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("input_name"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("script_usage"), Unused, OutError))
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

FString FListNiagaraModulesAction::GetActionName() const
{
	return TEXT("list_niagara_modules");
}

bool FListNiagaraModulesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Params;
	(void)Context;
	(void)OutError;
	return true;
}

FString FGetNiagaraModuleInputBindingAction::GetActionName() const
{
	return TEXT("get_niagara_module_input_binding");
}

bool FGetNiagaraModuleInputBindingAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("module_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("script_usage"), Unused, OutError);
}

FString FClearNiagaraModuleInputAction::GetActionName() const
{
	return TEXT("clear_niagara_module_input");
}

bool FClearNiagaraModuleInputAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("module_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("script_usage"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("input_name"), Unused, OutError);
}

FString FListNiagaraInputSourceMenuAction::GetActionName() const
{
	return TEXT("list_niagara_input_source_menu");
}

bool FListNiagaraInputSourceMenuAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("module_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("script_usage"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("input_name"), Unused, OutError);
}

FString FResolveNiagaraBuiltInDynamicInputAction::GetActionName() const
{
	return TEXT("resolve_niagara_built_in_dynamic_input");
}

bool FResolveNiagaraBuiltInDynamicInputAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context,
                                                        FString& OutError)
{
	(void)Params;
	(void)Context;
	(void)OutError;
	return true;
}

// ---------------------------------------------------------------------------
// FGetNiagaraModulesAction::ExecuteInternal
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FGetNiagaraModulesAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'emitter_name' parameter"));
	}

	FString ScriptUsageFilter = TEXT("all");
	Params->TryGetStringField(TEXT("script_usage"), ScriptUsageFilter);

	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	bool bIncludeInputs = true;
	Params->TryGetBoolField(TEXT("include_inputs"), bIncludeInputs);

	// Load the system.
	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	// Find the emitter.
	int32 EmitterIdx = INDEX_NONE;
	FString EmitterError;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(
		System, EmitterName, EmitterIdx, EmitterError);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(EmitterError);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	// Resolve the script usages to query.
	TArray<ENiagaraScriptUsage> Usages;
	if (ScriptUsageFilter.Equals(TEXT("all"), ESearchCase::IgnoreCase))
	{
		Usages.Add(ENiagaraScriptUsage::EmitterSpawnScript);
		Usages.Add(ENiagaraScriptUsage::EmitterUpdateScript);
		Usages.Add(ENiagaraScriptUsage::ParticleSpawnScript);
		Usages.Add(ENiagaraScriptUsage::ParticleUpdateScript);
	}
	else
	{
		bool bOk = false;
		ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageFilter, bOk);
		if (!bOk)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Invalid script_usage: '%s'"), *ScriptUsageFilter));
		}
		Usages.Add(Usage);
	}

	TArray<TSharedPtr<FJsonValue>> ModulesArray;

	for (ENiagaraScriptUsage Usage : Usages)
	{
		UNiagaraGraph* Graph = NiagaraCommonUtils::GetGraphForUsage(EmitterData, Usage);
		if (!Graph)
		{
			continue;
		}

		UNiagaraNodeOutput* OutputNode = NiagaraCommonUtils::GetOutputNodeForUsage(Graph, Usage);
		if (!OutputNode)
		{
			continue;
		}

		// Walk the chain in execution order.
		TArray<UNiagaraNodeFunctionCall*> Chain;
		NiagaraCommonUtils::CollectModuleChain(OutputNode, Chain);

		for (int32 i = 0; i < Chain.Num(); ++i)
		{
			if (!Filter.IsEmpty() &&
				!Chain[i]->GetFunctionName().Contains(Filter, ESearchCase::IgnoreCase))
			{
				continue;
			}
			TSharedPtr<FJsonObject> ModuleJson = NiagaraCommonUtils::ModuleNodeToJson(
				Chain[i], i, Usage, bIncludeInputs);
			ModulesArray.Add(MakeShared<FJsonValueObject>(ModuleJson));
		}
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("system_path"), SystemPath);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetArrayField(TEXT("modules"), ModulesArray);
	Data->SetNumberField(TEXT("count"), ModulesArray.Num());

	return CreateSuccessResponse(Data);
}

// ---------------------------------------------------------------------------
// FAddNiagaraModuleAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FAddNiagaraModuleAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'emitter_name' parameter"));
	}

	FString ModulePath;
	if (!Params->TryGetStringField(TEXT("module_path"), ModulePath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_path' parameter"));
	}

	FString ScriptUsageStr;
	if (!Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'script_usage' parameter"));
	}

	int32 TargetIndex = INDEX_NONE;
	{
		double IndexD = -1;
		if (Params->TryGetNumberField(TEXT("index"), IndexD))
		{
			TargetIndex = static_cast<int32>(IndexD);
		}
	}

	// Parse the script usage.
	bool bUsageOk = false;
	ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bUsageOk);
	if (!bUsageOk)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Invalid script_usage: '%s'"), *ScriptUsageStr));
	}

	// Load the system.
	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	// Find the emitter.
	int32 EmitterIdx = INDEX_NONE;
	FString EmitterError;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(
		System, EmitterName, EmitterIdx, EmitterError);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(EmitterError);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	// Load the module script.
	UNiagaraScript* ModuleScript = LoadObject<UNiagaraScript>(nullptr, *ModulePath);
	if (!ModuleScript)
	{
		// Retry with an explicit asset object suffix.
		FString FullPath = ModulePath;
		if (!FullPath.Contains(TEXT(".")))
		{
			FString AssetName = FPaths::GetBaseFilename(FullPath);
			FullPath = FullPath + TEXT(".") + AssetName;
		}
		ModuleScript = LoadObject<UNiagaraScript>(nullptr, *FullPath);
	}
	if (!ModuleScript)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to load module script: %s"), *ModulePath));
	}

	// Resolve the graph and output node.
	UNiagaraGraph* Graph = NiagaraCommonUtils::GetGraphForUsage(EmitterData, Usage);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("No graph for the given script usage"));
	}

	UNiagaraNodeOutput* OutputNode = NiagaraCommonUtils::GetOutputNodeForUsage(Graph, Usage);
	if (!OutputNode)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("No output node for usage: %s"), *ScriptUsageStr));
	}

	FScopedTransaction Transaction(
		NSLOCTEXT("UnrealMCPBridge", "AddNiagaraModule", "Add Niagara Module"));

	// Use the same helper the Niagara editor uses for stack insertion.
	// It creates the function-call node, resolves the full module signature,
	// inserts the node at the requested stack position, and keeps Module.*
	// inputs visible to later input-setting actions.
	FString SuggestedName;
	Params->TryGetStringField(TEXT("suggested_name"), SuggestedName);
	UNiagaraNodeFunctionCall* NewNode = FNiagaraStackGraphUtilities::AddScriptModuleToStack(
		ModuleScript,
		*OutputNode,
		TargetIndex,
		SuggestedName,
		FGuid());

	if (!NewNode)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("FNiagaraStackGraphUtilities::AddScriptModuleToStack returned null"));
	}

	Graph->NotifyGraphChanged();
	NiagaraCommonUtils::CompileAndSync(System);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("system_path"), SystemPath);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetStringField(TEXT("module_name"), NewNode->GetFunctionName());
	Data->SetStringField(TEXT("module_path"), ModulePath);
	Data->SetStringField(TEXT("script_usage"), ScriptUsageStr);

	// Report the exposed input pins so callers can verify the inserted module
	// is ready for follow-up input configuration.
	TArray<TSharedPtr<FJsonValue>> InputPinNames;
	for (UEdGraphPin* Pin : NewNode->Pins)
	{
		if (!Pin || Pin->Direction != EGPD_Input)
		{
			continue;
		}
		if (Pin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryMisc)
		{
			continue;
		}
		InputPinNames.Add(MakeShared<FJsonValueString>(Pin->PinName.ToString()));
	}
	Data->SetArrayField(TEXT("input_pins"), InputPinNames);

	return CreateSuccessResponse(Data);
}

// ---------------------------------------------------------------------------
// FRemoveNiagaraModuleAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FRemoveNiagaraModuleAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'emitter_name' parameter"));
	}

	FString ModuleName;
	if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name' parameter"));
	}

	FString ScriptUsageStr;
	if (!Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'script_usage' parameter"));
	}

	bool bUsageOk = false;
	ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bUsageOk);
	if (!bUsageOk)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Invalid script_usage: '%s'"), *ScriptUsageStr));
	}

	// Load the system.
	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	// Find the emitter.
	int32 EmitterIdx = INDEX_NONE;
	FString EmitterError;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(
		System, EmitterName, EmitterIdx, EmitterError);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(EmitterError);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	UNiagaraGraph* Graph = NiagaraCommonUtils::GetGraphForUsage(EmitterData, Usage);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("No graph for the given script usage"));
	}

	// Find the module node.
	FString FindError;
	UNiagaraNodeFunctionCall* ModuleNode = NiagaraCommonUtils::FindModuleNode(
		Graph, Usage, ModuleName, FindError);
	if (!ModuleNode)
	{
		return FMCPCommonUtils::CreateErrorResponse(FindError);
	}

	FScopedTransaction Transaction(
		NSLOCTEXT("UnrealMCPBridge", "RemoveNiagaraModule", "Remove Niagara Module"));

	// Splice the chain around the removed node to keep the pipeline intact.
	UEdGraphPin* NodeInput = NiagaraCommonUtils::FindFirstPin(ModuleNode, EGPD_Input);
	UEdGraphPin* NodeOutput = NiagaraCommonUtils::FindFirstPin(ModuleNode, EGPD_Output);

	UEdGraphPin* UpstreamOutput =
		(NodeInput && NodeInput->LinkedTo.Num() > 0) ? NodeInput->LinkedTo[0] : nullptr;
	UEdGraphPin* DownstreamInput =
		(NodeOutput && NodeOutput->LinkedTo.Num() > 0) ? NodeOutput->LinkedTo[0] : nullptr;

	// Break all links on the node.
	for (UEdGraphPin* Pin : ModuleNode->Pins)
	{
		if (Pin)
		{
			Pin->BreakAllPinLinks();
		}
	}

	// Reconnect the chain around the removed node.
	if (UpstreamOutput && DownstreamInput)
	{
		UpstreamOutput->MakeLinkTo(DownstreamInput);
	}

	Graph->RemoveNode(ModuleNode);
	Graph->NotifyGraphChanged();
	NiagaraCommonUtils::CompileAndSync(System);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("removed_module"), ModuleName);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetStringField(TEXT("script_usage"), ScriptUsageStr);

	return CreateSuccessResponse(Data);
}

// ---------------------------------------------------------------------------
// FSetNiagaraModuleEnabledAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FSetNiagaraModuleEnabledAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'emitter_name' parameter"));
	}

	FString ModuleName;
	if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name' parameter"));
	}

	FString ScriptUsageStr;
	if (!Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'script_usage' parameter"));
	}

	bool bEnabled = true;
	Params->TryGetBoolField(TEXT("enabled"), bEnabled);

	bool bUsageOk = false;
	ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bUsageOk);
	if (!bUsageOk)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Invalid script_usage: '%s'"), *ScriptUsageStr));
	}

	// Load the system.
	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	// Find the emitter.
	int32 EmitterIdx = INDEX_NONE;
	FString EmitterError;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(
		System, EmitterName, EmitterIdx, EmitterError);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(EmitterError);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	UNiagaraGraph* Graph = NiagaraCommonUtils::GetGraphForUsage(EmitterData, Usage);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("No graph for the given script usage"));
	}

	FString FindError;
	UNiagaraNodeFunctionCall* ModuleNode = NiagaraCommonUtils::FindModuleNode(
		Graph, Usage, ModuleName, FindError);
	if (!ModuleNode)
	{
		return FMCPCommonUtils::CreateErrorResponse(FindError);
	}

	FScopedTransaction Transaction(
		NSLOCTEXT("UnrealMCPBridge", "SetNiagaraModuleEnabled", "Set Module Enabled"));

	ModuleNode->SetEnabledState(
		bEnabled ? ENodeEnabledState::Enabled : ENodeEnabledState::Disabled,
		false);
	ModuleNode->MarkNodeRequiresSynchronization(TEXT("Module enabled state changed"), true);

	Graph->NotifyGraphChanged();
	NiagaraCommonUtils::CompileAndSync(System);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("module_name"), ModuleName);
	Data->SetBoolField(TEXT("enabled"), bEnabled);

	return CreateSuccessResponse(Data);
}

// ---------------------------------------------------------------------------
// FReorderNiagaraModuleAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FReorderNiagaraModuleAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'emitter_name' parameter"));
	}

	FString ModuleName;
	if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name' parameter"));
	}

	FString ScriptUsageStr;
	if (!Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'script_usage' parameter"));
	}

	double NewIndexD = 0;
	if (!Params->TryGetNumberField(TEXT("new_index"), NewIndexD))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'new_index' parameter"));
	}
	int32 NewIndex = static_cast<int32>(NewIndexD);

	bool bUsageOk = false;
	ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bUsageOk);
	if (!bUsageOk)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Invalid script_usage: '%s'"), *ScriptUsageStr));
	}

	// Load the system.
	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	// Find the emitter.
	int32 EmitterIdx = INDEX_NONE;
	FString EmitterError;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(
		System, EmitterName, EmitterIdx, EmitterError);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(EmitterError);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	UNiagaraGraph* Graph = NiagaraCommonUtils::GetGraphForUsage(EmitterData, Usage);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("No graph for the given script usage"));
	}

	UNiagaraNodeOutput* OutputNode = NiagaraCommonUtils::GetOutputNodeForUsage(Graph, Usage);
	if (!OutputNode)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("No output node found"));
	}

	// Collect the current chain.
	TArray<UNiagaraNodeFunctionCall*> Chain;
	NiagaraCommonUtils::CollectModuleChain(OutputNode, Chain);

	if (Chain.Num() < 2)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Not enough modules in chain to reorder"));
	}

	// Find the node to move.
	int32 OldIndex = INDEX_NONE;
	UNiagaraNodeFunctionCall* TargetNode = nullptr;
	for (int32 i = 0; i < Chain.Num(); ++i)
	{
		FString NodeName = Chain[i]->GetFunctionName();
		FText DisplayName = Chain[i]->GetNodeTitle(ENodeTitleType::ListView);

		if (NodeName.Equals(ModuleName, ESearchCase::IgnoreCase) ||
			DisplayName.ToString().Equals(ModuleName, ESearchCase::IgnoreCase))
		{
			OldIndex = i;
			TargetNode = Chain[i];
			break;
		}
	}

	if (!TargetNode)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Module '%s' not found in chain"), *ModuleName));
	}

	int32 ClampedIndex = FMath::Clamp(NewIndex, 0, Chain.Num() - 1);
	if (ClampedIndex == OldIndex)
	{
		TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("module_name"), ModuleName);
		Data->SetNumberField(TEXT("index"), OldIndex);
		Data->SetStringField(TEXT("note"), TEXT("Module already at requested index"));
		return CreateSuccessResponse(Data);
	}

	FScopedTransaction Transaction(
		NSLOCTEXT("UnrealMCPBridge", "ReorderNiagaraModule", "Reorder Niagara Module"));

	// Disconnect all chain links before rebuilding the chain.
	for (UNiagaraNodeFunctionCall* Node : Chain)
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction == EGPD_Input || Pin->Direction == EGPD_Output)
			{
				Pin->BreakAllPinLinks();
			}
		}
	}

	// Disconnect the output-node input pin.
	UEdGraphPin* OutputNodeInputPin = NiagaraCommonUtils::FindFirstPin(OutputNode, EGPD_Input);
	if (OutputNodeInputPin)
	{
		OutputNodeInputPin->BreakAllPinLinks();
	}

	// Reorder the chain array.
	Chain.RemoveAt(OldIndex);
	Chain.Insert(TargetNode, ClampedIndex);

	// Rebuild the chain from the reordered array.
	for (int32 i = 1; i < Chain.Num(); ++i)
	{
		UEdGraphPin* PrevOutput = NiagaraCommonUtils::FindFirstPin(Chain[i - 1], EGPD_Output);
		UEdGraphPin* CurrInput = NiagaraCommonUtils::FindFirstPin(Chain[i], EGPD_Input);
		if (PrevOutput && CurrInput)
		{
			PrevOutput->MakeLinkTo(CurrInput);
		}
	}

	// Connect the last node to the output node.
	if (Chain.Num() > 0 && OutputNodeInputPin)
	{
		UEdGraphPin* LastOutput = NiagaraCommonUtils::FindFirstPin(Chain.Last(), EGPD_Output);
		if (LastOutput)
		{
			LastOutput->MakeLinkTo(OutputNodeInputPin);
		}
	}

	Graph->NotifyGraphChanged();
	NiagaraCommonUtils::CompileAndSync(System);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("module_name"), ModuleName);
	Data->SetNumberField(TEXT("old_index"), OldIndex);
	Data->SetNumberField(TEXT("new_index"), ClampedIndex);

	return CreateSuccessResponse(Data);
}

// ---------------------------------------------------------------------------
// FGetNiagaraModuleInputsAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FGetNiagaraModuleInputsAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'emitter_name' parameter"));
	}

	FString ModuleName;
	if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name' parameter"));
	}

	FString ScriptUsageStr;
	if (!Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'script_usage' parameter"));
	}

	FString InputFilter;
	Params->TryGetStringField(TEXT("input_filter"), InputFilter);

	bool bIncludeSchema = false;
	Params->TryGetBoolField(TEXT("include_schema"), bIncludeSchema);

	bool bUsageOk = false;
	ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bUsageOk);
	if (!bUsageOk)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Invalid script_usage: '%s'"), *ScriptUsageStr));
	}

	// Load the system.
	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	// Find the emitter.
	int32 EmitterIdx = INDEX_NONE;
	FString EmitterError;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(
		System, EmitterName, EmitterIdx, EmitterError);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(EmitterError);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	UNiagaraGraph* Graph = NiagaraCommonUtils::GetGraphForUsage(EmitterData, Usage);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("No graph for the given script usage"));
	}

	FString FindError;
	UNiagaraNodeFunctionCall* ModuleNode = NiagaraCommonUtils::FindModuleNode(
		Graph, Usage, ModuleName, FindError);
	if (!ModuleNode)
	{
		return FMCPCommonUtils::CreateErrorResponse(FindError);
	}

	TArray<TSharedPtr<FJsonValue>> InputsArray;

	for (UEdGraphPin* Pin : ModuleNode->Pins)
	{
		if (Pin->Direction != EGPD_Input)
		{
			continue;
		}

		// Skip internal parameter-map pins.
		FString PinCategory = Pin->PinType.PinCategory.ToString();
		if (PinCategory.Equals(TEXT("Misc"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		FString PinName = Pin->PinName.ToString();

		// Apply the optional name filter.
		if (!InputFilter.IsEmpty() &&
			!PinName.Contains(InputFilter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		auto InputObj = MakeShared<FJsonObject>();
		InputObj->SetStringField(TEXT("name"), PinName);
		InputObj->SetStringField(TEXT("type"), PinCategory);
		InputObj->SetStringField(TEXT("default_value"), Pin->DefaultValue);
		InputObj->SetBoolField(TEXT("is_connected"), Pin->LinkedTo.Num() > 0);
		InputObj->SetStringField(TEXT("source"), TEXT("function_call_pin"));

		if (Pin->PinType.PinSubCategoryObject.IsValid())
		{
			UObject* SubObj = Pin->PinType.PinSubCategoryObject.Get();
			InputObj->SetStringField(TEXT("sub_type"), SubObj->GetName());
			InputObj->SetStringField(TEXT("sub_type_path"), SubObj->GetPathName());

			// Attach type schema metadata with the shared Niagara serializers.
			if (UEnum* EnumPtr = Cast<UEnum>(SubObj))
			{
				if (bIncludeSchema)
				{
					InputObj->SetObjectField(TEXT("type_schema"),
					                         NiagaraCommonUtils::SerializeEnum(EnumPtr));
				}

				if (!Pin->DefaultValue.IsEmpty())
				{
					const int64 CurrentVal = EnumPtr->GetValueByName(FName(*Pin->DefaultValue));
					if (CurrentVal != INDEX_NONE)
					{
						InputObj->SetStringField(TEXT("default_value_display"),
						                         EnumPtr->GetDisplayNameTextByValue(CurrentVal).ToString());
					}
				}
			}
			else if (UScriptStruct* StructPtr = Cast<UScriptStruct>(SubObj))
			{
				if (bIncludeSchema)
				{
					InputObj->SetObjectField(TEXT("type_schema"),
					                         NiagaraCommonUtils::SerializeStructFields(StructPtr));
				}
			}
			else if (UClass* ClassPtr = Cast<UClass>(SubObj))
			{
				if (bIncludeSchema && ClassPtr->IsChildOf(UNiagaraDataInterface::StaticClass()))
				{
					InputObj->SetObjectField(TEXT("type_schema"),
					                         NiagaraCommonUtils::SerializeDataInterfaceClass(ClassPtr));
				}
			}
		}

		InputsArray.Add(MakeShared<FJsonValueObject>(InputObj));
	}

	// Second pass: parameter-map-backed module inputs.
	// Direct function-call pins only cover legacy modules. Scratch-pad and
	// other parameter-map-style modules expose Module.* inputs through stack
	// graph reads, so enumerate them with GetStackFunctionInputs just like the
	// editor stack UI does.
	{
		TArray<FNiagaraVariable> StackInputs;
		FCompileConstantResolver ConstantResolver(System, Usage);
		FNiagaraStackGraphUtilities::GetStackFunctionInputs(
			*ModuleNode,
			StackInputs,
			ConstantResolver,
			FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly);

		const FString ModulePrefix(TEXT("Module."));
		for (const FNiagaraVariable& Var : StackInputs)
		{
			const FString FullName = Var.GetName().ToString();
			FString DisplayName = FullName;
			if (DisplayName.StartsWith(ModulePrefix))
			{
				DisplayName.RightChopInline(ModulePrefix.Len());
			}

			if (!InputFilter.IsEmpty() &&
				!DisplayName.Contains(InputFilter, ESearchCase::IgnoreCase) &&
				!FullName.Contains(InputFilter, ESearchCase::IgnoreCase))
			{
				continue;
			}

			auto InputObj = MakeShared<FJsonObject>();
			InputObj->SetStringField(TEXT("name"), DisplayName);
			InputObj->SetStringField(TEXT("aliased_name"), FullName);
			InputObj->SetStringField(TEXT("type"), Var.GetType().GetName());
			InputObj->SetStringField(TEXT("source"), TEXT("parameter_map_input"));

			if (bIncludeSchema)
			{
				InputObj->SetObjectField(TEXT("type_schema"),
				                         NiagaraCommonUtils::SerializeNiagaraType(Var.GetType()));
			}

			InputsArray.Add(MakeShared<FJsonValueObject>(InputObj));
		}
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("module_name"), ModuleName);
	Data->SetStringField(TEXT("script_usage"), ScriptUsageStr);
	Data->SetArrayField(TEXT("inputs"), InputsArray);
	Data->SetNumberField(TEXT("count"), InputsArray.Num());

	return CreateSuccessResponse(Data);
}

// ---------------------------------------------------------------------------
// FSetNiagaraModuleInputAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FSetNiagaraModuleInputAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'emitter_name' parameter"));
	}

	FString ModuleName;
	if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name' parameter"));
	}

	FString InputName;
	if (!Params->TryGetStringField(TEXT("input_name"), InputName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'input_name' parameter"));
	}

	FString ScriptUsageStr;
	if (!Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'script_usage' parameter"));
	}

	if (!Params->HasField(TEXT("value")))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
	}

	bool bUsageOk = false;
	ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bUsageOk);
	if (!bUsageOk)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Invalid script_usage: '%s'"), *ScriptUsageStr));
	}

	// Load the system.
	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	// Find the emitter.
	int32 EmitterIdx = INDEX_NONE;
	FString EmitterError;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(
		System, EmitterName, EmitterIdx, EmitterError);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(EmitterError);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	UNiagaraGraph* Graph = NiagaraCommonUtils::GetGraphForUsage(EmitterData, Usage);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("No graph for the given script usage"));
	}

	FString FindError;
	UNiagaraNodeFunctionCall* ModuleNode = NiagaraCommonUtils::FindModuleNode(
		Graph, Usage, ModuleName, FindError);
	if (!ModuleNode)
	{
		return FMCPCommonUtils::CreateErrorResponse(FindError);
	}

	// Nested input paths cannot use the top-level rapid-iteration aliasing.
	// Resolve the leaf dynamic-input call and write the literal directly to its
	// override pin, creating that override if needed.
	if (InputName.Contains(TEXT(".")))
	{
		FString LeafName, DescentError;
		UNiagaraNodeFunctionCall* LeafCall =
			NiagaraCommonUtils::DescendNestedPath(*ModuleNode, InputName, LeafName, DescentError);
		if (!LeafCall)
		{
			return FMCPCommonUtils::CreateErrorResponse(DescentError);
		}

		// Resolve the leaf input type from the stack-function input metadata.
		TArray<FNiagaraVariable> LeafInputs;
		TSet<FNiagaraVariable> LeafHidden;
		FCompileConstantResolver Resolver;
		FNiagaraStackGraphUtilities::GetStackFunctionInputs(
			*LeafCall, LeafInputs, LeafHidden, Resolver,
			FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::AllInputs,
			/*bIgnoreDisabled=*/true);

		FNiagaraTypeDefinition LeafType;
		const FString ModulePrefixed = FString::Printf(TEXT("Module.%s"), *LeafName);
		for (const FNiagaraVariable& V : LeafInputs)
		{
			if (V.GetName().ToString().Equals(ModulePrefixed, ESearchCase::IgnoreCase))
			{
				LeafType = V.GetType();
				break;
			}
		}
		if (!LeafType.IsValid())
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Leaf input '%s' not found on dynamic input"), *LeafName));
		}

		const FNiagaraParameterHandle Aliased =
			FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
				FNiagaraParameterHandle(FName(*ModulePrefixed)), LeafCall);

		// Create the override pin on demand so the nested literal is persisted.
		UEdGraphPin& OverridePin =
			FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
				*LeafCall, Aliased, LeafType, FGuid(), FGuid());

		const TSharedPtr<FJsonValue> RawVal = Params->TryGetField(TEXT("value"));
		FString ValueStr;
		FString ValueError;
		if (!NiagaraCommonUtils::JsonValueToPinDefaultString(RawVal, LeafType, ValueStr, ValueError))
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Could not serialize nested input '%s': %s"), *InputName, *ValueError));
		}

		LeafCall->Modify();
		OverridePin.GetOwningNode()->Modify();
		OverridePin.Modify();
		OverridePin.BreakAllPinLinks();
		OverridePin.DefaultValue = ValueStr;
		LeafCall->GetGraph()->NotifyGraphChanged();
		if (UNiagaraScript* OwningScript = EmitterData->GetScript(Usage, FGuid()))
		{
			OwningScript->MarkPackageDirty();
			OwningScript->PostEditChange();
		}
		NiagaraCommonUtils::CompileAndSync(System, false);

		auto R = MakeShared<FJsonObject>();
		R->SetBoolField(TEXT("success"), true);
		R->SetStringField(TEXT("input_name"), InputName);
		R->SetStringField(TEXT("leaf_name"), LeafName);
		R->SetStringField(TEXT("value"), ValueStr);
		R->SetStringField(TEXT("type"), LeafType.GetName());
		R->SetStringField(TEXT("path"), TEXT("nested_override_pin_default"));
		return R;
	}

	const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
	UEdGraphPin* InputPin = nullptr;
	FNiagaraTypeDefinition InputType;
	FString ResolvedInputName = InputName;
	FString InputSource;
	for (UEdGraphPin* Pin : ModuleNode->Pins)
	{
		if (Pin->Direction == EGPD_Input &&
			Pin->PinName.ToString().Equals(InputName, ESearchCase::IgnoreCase))
		{
			InputPin = Pin;
			InputType = NiagaraSchema->PinToTypeDefinition(Pin);
			ResolvedInputName = Pin->PinName.ToString();
			InputSource = TEXT("function_call_pin");
			break;
		}
	}

	if (!InputType.IsValid())
	{
		TArray<FNiagaraVariable> StackInputs;
		FCompileConstantResolver ConstantResolver(System, Usage);
		FNiagaraStackGraphUtilities::GetStackFunctionInputs(
			*ModuleNode,
			StackInputs,
			ConstantResolver,
			FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly);

		const FString ModulePrefix(TEXT("Module."));
		for (const FNiagaraVariable& Var : StackInputs)
		{
			FString DisplayName = Var.GetName().ToString();
			if (DisplayName.StartsWith(ModulePrefix))
			{
				DisplayName.RightChopInline(ModulePrefix.Len());
			}
			if (DisplayName.Equals(InputName, ESearchCase::IgnoreCase))
			{
				InputType = Var.GetType();
				ResolvedInputName = DisplayName;
				InputSource = TEXT("parameter_map_input");
				break;
			}
		}
	}

	if (!InputType.IsValid())
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Input '%s' not found on module '%s'"), *InputName, *ModuleName));
	}

	FScopedTransaction Transaction(
		NSLOCTEXT("UnrealMCPBridge", "SetNiagaraModuleInput", "Set Module Input"));

	const TSharedPtr<FJsonValue> JsonValue = Params->TryGetField(TEXT("value"));
	FString ValueStr;
	FString ValueError;
	if (!NiagaraCommonUtils::JsonValueToPinDefaultString(JsonValue, InputType, ValueStr, ValueError))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Could not serialize value for input '%s': %s"), *ResolvedInputName, *ValueError));
	}

	// Validate enum literals before assignment. Niagara will happily store an
	// unknown string on the pin, but the graph will ignore that value later.
	//
	// Accept three forms for caller convenience:
	//   - internal short name ("NewEnumerator0")
	//   - full name           ("ENiagara_EmitterStateOptions::NewEnumerator0")
	//   - display name        ("Infinite")
	UEnum* EnumPtr = nullptr;
	if (InputPin)
	{
		EnumPtr = Cast<UEnum>(InputPin->PinType.PinSubCategoryObject.Get());
	}
	if (!EnumPtr && InputType.IsEnum())
	{
		EnumPtr = InputType.GetEnum();
	}
	if (EnumPtr)
	{
		int64 ResolvedValue = EnumPtr->GetValueByNameString(ValueStr);
		if (ResolvedValue == INDEX_NONE)
		{
			// Fall back to the display name.
			for (int32 i = 0; i < EnumPtr->NumEnums(); ++i)
			{
				if (EnumPtr->GetDisplayNameTextByIndex(i).ToString().Equals(ValueStr, ESearchCase::IgnoreCase))
				{
					ResolvedValue = EnumPtr->GetValueByIndex(i);
					break;
				}
			}
		}
		if (ResolvedValue == INDEX_NONE)
		{
			TArray<FString> ValidEntries;
			for (int32 i = 0; i < EnumPtr->NumEnums(); ++i)
			{
				const FString ShortName = EnumPtr->GetNameStringByIndex(i);
				if (ShortName.EndsWith(TEXT("_MAX")))
				{
					continue;
				}
				if (EnumPtr->HasMetaData(TEXT("Hidden"), i))
				{
					continue;
				}
				const FString DisplayName = EnumPtr->GetDisplayNameTextByIndex(i).ToString();
				ValidEntries.Add(FString::Printf(TEXT("%s ('%s')"), *ShortName, *DisplayName));
			}
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(
					TEXT("Enum value '%s' is not valid for input '%s' (enum: %s). Valid entries: %s"),
					*ValueStr, *InputName, *EnumPtr->GetName(),
					*FString::Join(ValidEntries, TEXT(", "))));
		}
		// Niagara stores the enum default value as the short entry name.
		ValueStr = EnumPtr->GetNameStringByValue(ResolvedValue);
	}

	ModuleNode->Modify();
	Graph->Modify();
	if (InputSource == TEXT("function_call_pin"))
	{
		InputPin->Modify();
		InputPin->DefaultValue = ValueStr;
	}
	else
	{
		const FNiagaraParameterHandle InputHandle =
			FNiagaraParameterHandle::CreateModuleParameterHandle(FName(*ResolvedInputName));
		const FNiagaraParameterHandle AliasedHandle =
			FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(InputHandle, ModuleNode);

		UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
			*ModuleNode, AliasedHandle, InputType, FGuid(), FGuid());
		OverridePin.Modify();
		if (OverridePin.LinkedTo.Num() > 0)
		{
			NiagaraCommonUtils::RemoveOverridePinConnections(OverridePin, Graph);
		}
		OverridePin.DefaultValue = ValueStr;
	}
	ModuleNode->MarkNodeRequiresSynchronization(TEXT("Module input changed"), true);

	Graph->NotifyGraphChanged();
	NiagaraCommonUtils::CompileAndSync(System);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("module_name"), ModuleName);
	Data->SetStringField(TEXT("input_name"), ResolvedInputName);
	Data->SetStringField(TEXT("input_source"), InputSource);
	Data->SetStringField(TEXT("input_type"), InputType.GetName());
	Data->SetStringField(TEXT("value"), ValueStr);

	return CreateSuccessResponse(Data);
}

// ---------------------------------------------------------------------------
// FSetNiagaraDynamicInputAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FSetNiagaraDynamicInputAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'emitter_name' parameter"));
	}

	FString ModuleName;
	if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name' parameter"));
	}

	FString InputName;
	if (!Params->TryGetStringField(TEXT("input_name"), InputName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'input_name' parameter"));
	}

	FString ScriptUsageStr;
	if (!Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'script_usage' parameter"));
	}

	FString DynamicInputType;
	if (!Params->TryGetStringField(TEXT("dynamic_input_type"), DynamicInputType))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'dynamic_input_type' parameter"));
	}

	bool bUsageOk = false;
	ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bUsageOk);
	if (!bUsageOk)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Invalid script_usage: '%s'"), *ScriptUsageStr));
	}

	// Load the system.
	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	// Find the emitter.
	int32 EmitterIdx = INDEX_NONE;
	FString EmitterError;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(
		System, EmitterName, EmitterIdx, EmitterError);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(EmitterError);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	UNiagaraGraph* Graph = NiagaraCommonUtils::GetGraphForUsage(EmitterData, Usage);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("No graph for the given script usage"));
	}

	FString FindError;
	UNiagaraNodeFunctionCall* ModuleNode = NiagaraCommonUtils::FindModuleNode(
		Graph, Usage, ModuleName, FindError);
	if (!ModuleNode)
	{
		return FMCPCommonUtils::CreateErrorResponse(FindError);
	}

	// Resolve nested input paths through dynamic-input chains so the binding is
	// applied to the leaf call, not the outer module input.
	FString LeafInputName;
	FString DescentError;
	UNiagaraNodeFunctionCall* TargetCall =
		NiagaraCommonUtils::DescendNestedPath(*ModuleNode, InputName, LeafInputName, DescentError);
	if (!TargetCall)
	{
		return FMCPCommonUtils::CreateErrorResponse(DescentError);
	}

	// Build the handle for the resolved leaf input.
	FNiagaraParameterHandle InputHandle =
		FNiagaraParameterHandle::CreateModuleParameterHandle(FName(*LeafInputName));
	FNiagaraParameterHandle AliasedHandle =
		FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(InputHandle, TargetCall);
	ModuleNode = TargetCall; // remainder of handler operates on the leaf call

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("module_name"), ModuleName);
	Data->SetStringField(TEXT("input_name"), InputName);
	Data->SetStringField(TEXT("dynamic_input_type"), DynamicInputType);

	FScopedTransaction Transaction(
		NSLOCTEXT("UnrealMCPBridge", "SetNiagaraDynamicInput", "Set Dynamic Input"));

	FString LowerType = DynamicInputType.ToLower();

	// Handle Random Range / Uniform Random.
	if (LowerType == TEXT("random_range") || LowerType == TEXT("uniform_random"))
	{
		// Infer float versus vector from the provided values.
		bool bIsVector = false;
		FVector MinVec = FVector::ZeroVector;
		FVector MaxVec = FVector::OneVector;
		double MinFloat = 0.0;
		double MaxFloat = 1.0;

		TSharedPtr<FJsonValue> MinVal = Params->TryGetField(TEXT("min_value"));
		TSharedPtr<FJsonValue> MaxVal = Params->TryGetField(TEXT("max_value"));

		if (MinVal.IsValid() && MinVal->Type == EJson::Object)
		{
			bIsVector = true;
			TSharedPtr<FJsonObject> MinObj = MinVal->AsObject();
			MinVec.X = MinObj->GetNumberField(TEXT("x"));
			MinVec.Y = MinObj->GetNumberField(TEXT("y"));
			MinVec.Z = MinObj->GetNumberField(TEXT("z"));

			if (MaxVal.IsValid() && MaxVal->Type == EJson::Object)
			{
				TSharedPtr<FJsonObject> MaxObj = MaxVal->AsObject();
				MaxVec.X = MaxObj->GetNumberField(TEXT("x"));
				MaxVec.Y = MaxObj->GetNumberField(TEXT("y"));
				MaxVec.Z = MaxObj->GetNumberField(TEXT("z"));
			}
		}
		else
		{
			if (MinVal.IsValid())
			{
				MinFloat = MinVal->AsNumber();
			}
			if (MaxVal.IsValid())
			{
				MaxFloat = MaxVal->AsNumber();
			}
		}

		// Prefer the canonical engine asset path, then fall back to an
		// AssetRegistry lookup because these template paths can vary by UE
		// version or plugin layout.
		FString RandomScriptPath = bIsVector
			                           ? TEXT("/Niagara/Modules/DynamicInputs/UniformRangedVector.UniformRangedVector")
			                           : TEXT("/Niagara/Modules/DynamicInputs/UniformRangedFloat.UniformRangedFloat");

		UNiagaraScript* DynamicInputScript = LoadObject<UNiagaraScript>(nullptr, *RandomScriptPath);
		if (!DynamicInputScript)
		{
			FNiagaraEditorUtilities::FGetFilteredScriptAssetsOptions Opts;
			Opts.ScriptUsageToInclude = ENiagaraScriptUsage::DynamicInput;
			Opts.bIncludeDeprecatedScripts = false;

			TArray<FAssetData> DIAssets;
			FNiagaraEditorUtilities::GetFilteredScriptAssets(Opts, DIAssets);

			const TCHAR* WantSubstr = bIsVector ? TEXT("UniformRangedVector") : TEXT("UniformRangedFloat");
			const TCHAR* FallbackSubstr = bIsVector ? TEXT("RangedVector") : TEXT("RangedFloat");

			auto TryResolve = [&](const TCHAR* Substr) -> UNiagaraScript*
			{
				for (const FAssetData& Asset : DIAssets)
				{
					if (Asset.AssetName.ToString().Contains(Substr, ESearchCase::IgnoreCase))
					{
						if (UNiagaraScript* Loaded = Cast<UNiagaraScript>(Asset.GetAsset()))
						{
							return Loaded;
						}
					}
				}
				return nullptr;
			};

			DynamicInputScript = TryResolve(WantSubstr);
			if (!DynamicInputScript)
			{
				DynamicInputScript = TryResolve(FallbackSubstr);
			}

			if (!DynamicInputScript)
			{
				return FMCPCommonUtils::CreateErrorResponse(FString::Printf(
					TEXT(
						"No dynamic-input template found for %s. Use resolve_niagara_built_in_dynamic_input with name_filter='Random' to discover available templates."),
					WantSubstr));
			}
		}

		FNiagaraTypeDefinition InputType = bIsVector
			                                   ? FNiagaraTypeDefinition::GetVec3Def()
			                                   : FNiagaraTypeDefinition::GetFloatDef();

		UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
			*ModuleNode, AliasedHandle, InputType, FGuid(), FGuid());

		// Remove any existing override connections.
		if (OverridePin.LinkedTo.Num() > 0)
		{
			NiagaraCommonUtils::RemoveOverridePinConnections(OverridePin, Graph);
		}

		UNiagaraNodeFunctionCall* DynamicInputNode = nullptr;
		FNiagaraStackGraphUtilities::SetDynamicInputForFunctionInput(
			OverridePin, DynamicInputScript, DynamicInputNode, FGuid(), TEXT("Random"), FGuid());

		if (DynamicInputNode)
		{
			// Assign min/max values on the dynamic input pins.
			for (UEdGraphPin* DIPin : DynamicInputNode->Pins)
			{
				if (DIPin->Direction != EGPD_Input)
				{
					continue;
				}

				FString PinNameLower = DIPin->PinName.ToString().ToLower();
				if (PinNameLower.Contains(TEXT("min")))
				{
					if (bIsVector)
					{
						DIPin->DefaultValue = FString::Printf(
							TEXT("%f,%f,%f"), MinVec.X, MinVec.Y, MinVec.Z);
					}
					else
					{
						DIPin->DefaultValue = FString::SanitizeFloat(MinFloat);
					}
				}
				else if (PinNameLower.Contains(TEXT("max")))
				{
					if (bIsVector)
					{
						DIPin->DefaultValue = FString::Printf(
							TEXT("%f,%f,%f"), MaxVec.X, MaxVec.Y, MaxVec.Z);
					}
					else
					{
						DIPin->DefaultValue = FString::SanitizeFloat(MaxFloat);
					}
				}
			}
		}

		Data->SetStringField(TEXT("script_path"), RandomScriptPath);
		Data->SetBoolField(TEXT("dynamic_input_created"), DynamicInputNode != nullptr);
	}
	// Handle Parameter Link.
	else if (LowerType == TEXT("parameter_link"))
	{
		FString ParameterName;
		if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Missing 'parameter_name' for parameter_link dynamic input"));
		}

		const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();

		// Resolve the target input type from the module pin.
		FNiagaraTypeDefinition TargetType = FNiagaraTypeDefinition::GetFloatDef();
		for (UEdGraphPin* Pin : ModuleNode->Pins)
		{
			if (Pin->Direction == EGPD_Input &&
				Pin->PinName.ToString().Equals(InputName, ESearchCase::IgnoreCase))
			{
				TargetType = NiagaraSchema->PinToTypeDefinition(Pin);
				break;
			}
		}

		// Resolve the source parameter type from exposed parameters.
		FNiagaraTypeDefinition SourceType;
		bool bFoundSourceParam = false;
		TArrayView<const FNiagaraVariableWithOffset> ExposedVars =
			System->GetExposedParameters().ReadParameterVariables();
		for (const FNiagaraVariableWithOffset& Var : ExposedVars)
		{
			FString VarName = Var.GetName().ToString();
			if (VarName.Equals(ParameterName, ESearchCase::IgnoreCase) ||
				VarName.Contains(ParameterName, ESearchCase::IgnoreCase))
			{
				SourceType = Var.GetType();
				bFoundSourceParam = true;
				break;
			}
		}

		if (!bFoundSourceParam)
		{
			SourceType = FNiagaraTypeDefinition::GetVec3Def();
		}

		// Return the resolved source/target types for debugging.
		Data->SetStringField(TEXT("source_type"), SourceType.GetName());
		Data->SetStringField(TEXT("target_type"), TargetType.GetName());
		Data->SetBoolField(TEXT("types_differ"), SourceType != TargetType);

		// Reuse the live Niagara editor ViewModel instead of creating a new one.
		// Creating a fresh ViewModel here can destabilize MessageManager state.
		TSharedPtr<FNiagaraSystemViewModel> SystemViewModel =
			FNiagaraEditorModule::Get().GetExistingViewModelForSystem(System);

		if (!SystemViewModel.IsValid())
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("No active Niagara editor found for this system. "
					"Open the system in the Niagara editor first, then retry the parameter_link command."));
		}

		// Find the emitter handle ViewModel for this emitter.
		TSharedPtr<FNiagaraEmitterHandleViewModel> EmitterHandleVM;
		for (const TSharedRef<FNiagaraEmitterHandleViewModel>& HandleVM : SystemViewModel->GetEmitterHandleViewModels())
		{
			if (HandleVM->GetName().ToString().Equals(EmitterName, ESearchCase::IgnoreCase))
			{
				EmitterHandleVM = HandleVM;
				break;
			}
		}

		if (!EmitterHandleVM.IsValid())
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Could not find emitter '%s' in SystemViewModel"), *EmitterName));
		}

		// Resolve the emitter stack ViewModel and locate the target module/input.
		UNiagaraStackViewModel* StackVM = EmitterHandleVM->GetEmitterStackViewModel();
		if (!StackVM)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Failed to get emitter stack ViewModel"));
		}

		UNiagaraStackEntry* RootEntry = StackVM->GetRootEntry();
		if (!RootEntry)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Failed to get stack root entry"));
		}

		// Find the stack module item by display name or function-call name.
		TArray<UNiagaraStackModuleItem*> ModuleItems;
		RootEntry->GetFilteredChildrenOfType<UNiagaraStackModuleItem>(ModuleItems, true);

		UNiagaraStackModuleItem* TargetModuleItem = nullptr;
		for (UNiagaraStackModuleItem* Item : ModuleItems)
		{
			FString DisplayName = Item->GetDisplayName().ToString();
			if (DisplayName.Equals(ModuleName, ESearchCase::IgnoreCase))
			{
				TargetModuleItem = Item;
				break;
			}

			// Fall back to the underlying function-call node name.
			UNiagaraNodeFunctionCall& FuncNode = Item->GetModuleNode();
			FString FuncName = FuncNode.GetFunctionName();
			if (FuncName.Equals(ModuleName, ESearchCase::IgnoreCase))
			{
				TargetModuleItem = Item;
				break;
			}
		}

		if (!TargetModuleItem)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(
					TEXT("Could not find module '%s' in the stack ViewModel (found %d modules)"),
					*ModuleName, ModuleItems.Num()));
		}

		// Collect all inputs recursively, including nested child inputs.
		TArray<UNiagaraStackFunctionInput*> ParameterInputs;
		TargetModuleItem->GetUnfilteredChildrenOfType<UNiagaraStackFunctionInput>(
			ParameterInputs, true);

		UNiagaraStackFunctionInput* TargetInput = nullptr;
		for (UNiagaraStackFunctionInput* Input : ParameterInputs)
		{
			FString InputDisplayName = Input->GetDisplayName().ToString();
			FString InputParamName = Input->GetInputParameterHandle().GetName().ToString();

			// Prefer exact matches first.
			if (InputDisplayName.Equals(InputName, ESearchCase::IgnoreCase) ||
				InputParamName.Equals(InputName, ESearchCase::IgnoreCase))
			{
				TargetInput = Input;
				break;
			}

			// Fall back to a contains match if no exact name matched.
			if (InputDisplayName.Contains(InputName, ESearchCase::IgnoreCase) ||
				InputParamName.Contains(InputName, ESearchCase::IgnoreCase))
			{
				TargetInput = Input;
				// Keep searching in case an exact match appears later.
			}
		}

		if (!TargetInput)
		{
			// Return a trimmed list of available inputs to help callers recover.
			TArray<FString> AvailableInputs;
			for (UNiagaraStackFunctionInput* Input : ParameterInputs)
			{
				AvailableInputs.Add(Input->GetDisplayName().ToString());
			}
			// Cap the list to keep the response readable.
			if (AvailableInputs.Num() > 20)
			{
				AvailableInputs.SetNum(20);
				AvailableInputs.Add(TEXT("..."));
			}

			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(
					TEXT("Could not find input '%s' on module '%s' (found %d inputs). Available: %s"),
					*InputName, *ModuleName, ParameterInputs.Num(),
					*FString::Join(AvailableInputs, TEXT(", "))));
		}

		// Use the ViewModel-reported input type rather than inferring from pins.
		FNiagaraTypeDefinition ActualTargetType = TargetInput->GetInputType();

		// Reset existing linked/dynamic state through the ViewModel before
		// applying a new link. This is safer than editing inconsistent graph
		// nodes directly.
		auto CurrentMode = TargetInput->GetValueMode();
		if (CurrentMode != UNiagaraStackFunctionInput::EValueMode::Local &&
			CurrentMode != UNiagaraStackFunctionInput::EValueMode::DefaultFunction &&
			TargetInput->CanReset())
		{
			TargetInput->Reset();
			// Refresh the stack so later operations see the updated state.
			TargetInput->RefreshChildren();
		}

		// Build the linked parameter variable with the resolved source type.
		FNiagaraVariable LinkedParam(SourceType, FName(*ParameterName));

		// Include the final target type in the response for diagnostics.
		Data->SetStringField(TEXT("actual_target_type"), ActualTargetType.GetName());

		// Insert a conversion script when source and target types differ.
		bool bTypeConversionUsed = false;
		FString ConversionScriptPath;

		if (SourceType != ActualTargetType)
		{
			TArray<UNiagaraScript*> ConversionScripts =
				UNiagaraStackFunctionInput::GetPossibleConversionScripts(SourceType, ActualTargetType);

			if (ConversionScripts.Num() > 0)
			{
				TargetInput->SetLinkedParameterValueViaConversionScript(
					LinkedParam, *ConversionScripts[0]);

				bTypeConversionUsed = true;
				ConversionScriptPath = ConversionScripts[0]->GetPathName();
			}
			else
			{
				// Fall back to a direct link when no conversion script exists.
				TargetInput->SetLinkedParameterValue(LinkedParam);
			}
		}
		else
		{
			// Matching types can be linked directly.
			TargetInput->SetLinkedParameterValue(LinkedParam);
		}

		Data->SetStringField(TEXT("linked_parameter"), ParameterName);
		Data->SetBoolField(TEXT("type_conversion_used"), bTypeConversionUsed);
		Data->SetBoolField(TEXT("dynamic_input_created"), true);
		if (bTypeConversionUsed)
		{
			Data->SetStringField(TEXT("conversion_script"), ConversionScriptPath);
		}

	}
	// Handle Custom Expression.
	else if (LowerType == TEXT("custom_expression"))
	{
		FString Expression;
		if (!Params->TryGetStringField(TEXT("expression"), Expression))
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Missing 'expression' for custom_expression dynamic input"));
		}

		// Create a custom HLSL node and wire it as a dynamic input.
		UNiagaraNodeCustomHlsl* CustomNode = NewObject<UNiagaraNodeCustomHlsl>(Graph);
		CustomNode->CreateNewGuid();
		CustomNode->PostPlacedNewNode();
		CustomNode->AllocateDefaultPins();
		// SetCustomHlsl is not exported from NiagaraEditor, so use reflection.
		FStrProperty* HlslProp = CastField<FStrProperty>(
			UNiagaraNodeCustomHlsl::StaticClass()->FindPropertyByName(TEXT("CustomHlsl")));
		if (HlslProp)
		{
			HlslProp->SetPropertyValue_InContainer(CustomNode, Expression);
		}
		CustomNode->MarkNodeRequiresSynchronization(TEXT("HLSL set via MCP"), true);
		Graph->AddNode(CustomNode, false, false);

		// Wire the custom-node output to the module override pin.
		FNiagaraTypeDefinition InputType = FNiagaraTypeDefinition::GetFloatDef();
		UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
			*ModuleNode, AliasedHandle, InputType, FGuid(), FGuid());

		if (OverridePin.LinkedTo.Num() > 0)
		{
			NiagaraCommonUtils::RemoveOverridePinConnections(OverridePin, Graph);
		}

		UEdGraphPin* CustomOutputPin = NiagaraCommonUtils::FindFirstPin(CustomNode, EGPD_Output);
		if (CustomOutputPin)
		{
			CustomOutputPin->MakeLinkTo(&OverridePin);
		}

		Data->SetStringField(TEXT("expression"), Expression);
		Data->SetBoolField(TEXT("dynamic_input_created"), true);
	}
	// Handle generic script-backed dynamic inputs.
	// Accept an arbitrary dynamic-input script asset path for cases not covered
	// by the dedicated helpers above.
	else if (LowerType == TEXT("script") || LowerType == TEXT("asset"))
	{
		FString ScriptPath;
		if (!Params->TryGetStringField(TEXT("dynamic_input_script_path"), ScriptPath))
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Missing 'dynamic_input_script_path' for script-backed dynamic input"));
		}

		UNiagaraScript* DynamicInputScript = LoadObject<UNiagaraScript>(nullptr, *ScriptPath);
		if (!DynamicInputScript)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Failed to load dynamic input script '%s'"), *ScriptPath));
		}
		if (DynamicInputScript->GetUsage() != ENiagaraScriptUsage::DynamicInput)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Script is not a DynamicInput usage script"));
		}

		// Resolve the declared input type from the module pin.
		const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
		FNiagaraTypeDefinition InputType = FNiagaraTypeDefinition::GetFloatDef();
		for (UEdGraphPin* Pin : ModuleNode->Pins)
		{
			if (Pin->Direction == EGPD_Input &&
				Pin->PinName.ToString().Equals(InputName, ESearchCase::IgnoreCase))
			{
				InputType = NiagaraSchema->PinToTypeDefinition(Pin);
				break;
			}
		}

		UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
			*ModuleNode, AliasedHandle, InputType, FGuid(), FGuid());
		if (OverridePin.LinkedTo.Num() > 0)
		{
			NiagaraCommonUtils::RemoveOverridePinConnections(OverridePin, Graph);
		}

		UNiagaraNodeFunctionCall* DynamicInputNode = nullptr;
		FString SuggestedName;
		Params->TryGetStringField(TEXT("suggested_name"), SuggestedName);
		FNiagaraStackGraphUtilities::SetDynamicInputForFunctionInput(
			OverridePin, DynamicInputScript, DynamicInputNode, FGuid(), SuggestedName, FGuid());

		// Optionally apply literal defaults to the dynamic-input pins.
		// Shape: { "pin_name": "value_as_string", ... }
		const TSharedPtr<FJsonObject>* DefaultsObj = nullptr;
		if (DynamicInputNode && Params->TryGetObjectField(TEXT("pin_defaults"), DefaultsObj))
		{
			for (UEdGraphPin* DIPin : DynamicInputNode->Pins)
			{
				if (!DIPin || DIPin->Direction != EGPD_Input)
				{
					continue;
				}
				FString DefaultStr;
				if ((*DefaultsObj)->TryGetStringField(DIPin->PinName.ToString(), DefaultStr))
				{
					DIPin->DefaultValue = DefaultStr;
				}
			}
		}

		Data->SetStringField(TEXT("script_path"), DynamicInputScript->GetPathName());
		Data->SetStringField(TEXT("input_type"), InputType.GetName());
		Data->SetBoolField(TEXT("dynamic_input_created"), DynamicInputNode != nullptr);
	}
	else
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(
				TEXT(
					"Unknown dynamic_input_type: '%s'. Supported: random_range, uniform_random, parameter_link, custom_expression, script"),
				*DynamicInputType));
	}

	Graph->NotifyGraphChanged();
	NiagaraCommonUtils::CompileAndSync(System);

	return CreateSuccessResponse(Data);
}

// Rapid-iteration parameter tools live in NiagaraRapidIterationOps.cpp.


// ---------------------------------------------------------------------------
// Local helper functions.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// FSetNiagaraCurveAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FSetNiagaraCurveAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'emitter_name' parameter"));
	}

	FString ModuleName;
	if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name' parameter"));
	}

	FString InputName;
	if (!Params->TryGetStringField(TEXT("input_name"), InputName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'input_name' parameter"));
	}

	FString ScriptUsageStr;
	if (!Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'script_usage' parameter"));
	}

	FString CurveType;
	if (!Params->TryGetStringField(TEXT("curve_type"), CurveType))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'curve_type' parameter"));
	}

	const TArray<TSharedPtr<FJsonValue>>* KeysArrayPtr = nullptr;
	if (!Params->TryGetArrayField(TEXT("keys"), KeysArrayPtr) ||
		!KeysArrayPtr ||
		KeysArrayPtr->Num() == 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing or empty 'keys' array"));
	}

	bool bUsageOk = false;
	ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bUsageOk);
	if (!bUsageOk)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Invalid script_usage: '%s'"), *ScriptUsageStr));
	}

	// Load the system.
	FString LoadError;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(LoadError);
	}

	// Find the emitter.
	int32 EmitterIdx = INDEX_NONE;
	FString EmitterError;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(
		System, EmitterName, EmitterIdx, EmitterError);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(EmitterError);
	}

	// Handle the stateless-emitter path.
	if (NiagaraCommonUtils::IsStatelessEmitterHandle(Handle))
	{
		UObject* StatelessEmitter = reinterpret_cast<UObject*>(Handle->GetStatelessEmitter());
		if (!StatelessEmitter)
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("Stateless emitter pointer is null"));
		}

		FArrayProperty* ModulesArrayProp = CastField<FArrayProperty>(
			StatelessEmitter->GetClass()->FindPropertyByName(TEXT("Modules")));
		if (!ModulesArrayProp)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Could not find Modules property on Stateless emitter"));
		}

		FScriptArrayHelper ModulesArray(
			ModulesArrayProp, ModulesArrayProp->ContainerPtrToValuePtr<void>(StatelessEmitter));

		UObject* TargetModule = nullptr;
		for (int32 i = 0; i < ModulesArray.Num(); ++i)
		{
			UObject** ModulePtr = reinterpret_cast<UObject**>(ModulesArray.GetRawPtr(i));
			if (ModulePtr && *ModulePtr)
			{
				UObject* Module = *ModulePtr;
				if (Module->GetClass()->GetName().Contains(ModuleName, ESearchCase::IgnoreCase))
				{
					TargetModule = Module;
					break;
				}
			}
		}

		if (!TargetModule)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Stateless module not found: %s"), *ModuleName));
		}

		FProperty* DistributionProp = nullptr;
		FString FoundPropName;
		for (TFieldIterator<FProperty> PropIt(TargetModule->GetClass()); PropIt; ++PropIt)
		{
			FProperty* Prop = *PropIt;
			FString PropName = Prop->GetName();

			FStructProperty* StructProp = CastField<FStructProperty>(Prop);
			if (!StructProp || !PropName.Contains(TEXT("Distribution"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			FString PropNameWithoutDist = PropName.Replace(TEXT("Distribution"), TEXT(""));
			if (InputName.Contains(PropNameWithoutDist, ESearchCase::IgnoreCase) ||
				PropNameWithoutDist.Contains(InputName, ESearchCase::IgnoreCase))
			{
				DistributionProp = Prop;
				FoundPropName = PropName;
				break;
			}
		}

		if (!DistributionProp)
		{
			FString AvailableProps;
			for (TFieldIterator<FProperty> PropIt(TargetModule->GetClass()); PropIt; ++PropIt)
			{
				if (!AvailableProps.IsEmpty())
				{
					AvailableProps += TEXT(", ");
				}
				AvailableProps += (*PropIt)->GetName();
			}
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(
					TEXT("Distribution property not found for input '%s'. Available: %s"),
					*InputName, *AvailableProps));
		}

		if (CurveType.Equals(TEXT("color"), ESearchCase::IgnoreCase))
		{
			FStructProperty* StructProp = CastField<FStructProperty>(DistributionProp);
			if (StructProp && StructProp->Struct->GetFName() == TEXT("NiagaraDistributionColor"))
			{
				FNiagaraDistributionColor* ColorDist =
					StructProp->ContainerPtrToValuePtr<FNiagaraDistributionColor>(TargetModule);
				if (!ColorDist)
				{
					return FMCPCommonUtils::CreateErrorResponse(
						TEXT("Failed to get Distribution color pointer"));
				}

				ColorDist->Mode = ENiagaraDistributionMode::UniformCurve;

				FNiagaraDistributionBase* BaseDistribution =
					ColorDist;
				BaseDistribution->ChannelCurves.SetNum(4);
				for (int32 i = 0; i < 4; ++i)
				{
					BaseDistribution->ChannelCurves[i].Reset();
				}

				int32 KeyCount = 0;
				for (const TSharedPtr<FJsonValue>& KeyVal : *KeysArrayPtr)
				{
					TSharedPtr<FJsonObject> KeyObj = KeyVal->AsObject();
					if (!KeyObj)
					{
						continue;
					}

					float Time = static_cast<float>(KeyObj->GetNumberField(TEXT("time")));
					float R = static_cast<float>(KeyObj->GetNumberField(TEXT("r")));
					float G = static_cast<float>(KeyObj->GetNumberField(TEXT("g")));
					float B = static_cast<float>(KeyObj->GetNumberField(TEXT("b")));

					double A = 1.0;
					KeyObj->TryGetNumberField(TEXT("a"), A);

					BaseDistribution->ChannelCurves[0].AddKey(Time, R);
					BaseDistribution->ChannelCurves[1].AddKey(Time, G);
					BaseDistribution->ChannelCurves[2].AddKey(Time, B);
					BaseDistribution->ChannelCurves[3].AddKey(Time, static_cast<float>(A));
					++KeyCount;
				}

				ColorDist->ValuesTimeRange = FVector2f(0.0f, 1.0f);
				ColorDist->UpdateValuesFromDistribution();

				TargetModule->Modify();
				StatelessEmitter->Modify();
				System->MarkPackageDirty();
				System->PostEditChange();
				System->RequestCompile(true);
				System->WaitForCompilationComplete();

				TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
				Data->SetStringField(TEXT("module_name"), ModuleName);
				Data->SetStringField(TEXT("input_name"), InputName);
				Data->SetStringField(TEXT("property_name"), FoundPropName);
				Data->SetStringField(TEXT("curve_type"), CurveType);
				Data->SetNumberField(TEXT("key_count"), KeyCount);
				Data->SetBoolField(TEXT("stateless_emitter"), true);

				return CreateSuccessResponse(Data);
			}
		}

		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unsupported curve type for Stateless emitter: %s"), *CurveType));
	}

	// Handle the standard emitter path.
	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	UNiagaraGraph* Graph = NiagaraCommonUtils::GetGraphForUsage(EmitterData, Usage);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("No graph for the given script usage"));
	}

	FString FindError;
	UNiagaraNodeFunctionCall* ModuleNode = NiagaraCommonUtils::FindModuleNode(
		Graph, Usage, ModuleName, FindError);
	if (!ModuleNode)
	{
		return FMCPCommonUtils::CreateErrorResponse(FindError);
	}

	// Resolve the target input the same way the stack UI does: direct module
	// pins first, then parameter-map-backed stack inputs exposed as Module.*.
	const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
	FNiagaraTypeDefinition ResolvedInputType;
	FString ResolvedInputName = InputName;
	bool bIsDataInterfaceInput = false;
	for (UEdGraphPin* Pin : ModuleNode->Pins)
	{
		if (Pin->Direction == EGPD_Input &&
			Pin->PinName.ToString().Equals(InputName, ESearchCase::IgnoreCase))
		{
			ResolvedInputType = NiagaraSchema->PinToTypeDefinition(Pin);
			ResolvedInputName = Pin->PinName.ToString();
			const FString PinCategory = Pin->PinType.PinCategory.ToString();
			bIsDataInterfaceInput =
				PinCategory.Equals(TEXT("Class"), ESearchCase::IgnoreCase) ||
				ResolvedInputType.IsDataInterface();
			break;
		}
	}

	if (!ResolvedInputType.IsValid())
	{
		TArray<FNiagaraVariable> StackInputs;
		FCompileConstantResolver ConstantResolver(System, Usage);
		FNiagaraStackGraphUtilities::GetStackFunctionInputs(
			*ModuleNode,
			StackInputs,
			ConstantResolver,
			FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly);

		const FString ModulePrefix(TEXT("Module."));
		for (const FNiagaraVariable& Var : StackInputs)
		{
			FString DisplayName = Var.GetName().ToString();
			if (DisplayName.StartsWith(ModulePrefix))
			{
				DisplayName.RightChopInline(ModulePrefix.Len());
			}
			if (DisplayName.Equals(InputName, ESearchCase::IgnoreCase))
			{
				ResolvedInputType = Var.GetType();
				ResolvedInputName = DisplayName;
				bIsDataInterfaceInput = ResolvedInputType.IsDataInterface();
				break;
			}
		}
	}

	if (!ResolvedInputType.IsValid())
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Input '%s' not found on module '%s'"), *InputName, *ModuleName));
	}

	if (bIsDataInterfaceInput)
	{
		UClass* CurveClass = nullptr;
		if (CurveType.Equals(TEXT("float"), ESearchCase::IgnoreCase))
		{
			CurveClass = UNiagaraDataInterfaceCurve::StaticClass();
		}
		else if (CurveType.Equals(TEXT("vector"), ESearchCase::IgnoreCase))
		{
			CurveClass = UNiagaraDataInterfaceVectorCurve::StaticClass();
		}
		else if (CurveType.Equals(TEXT("color"), ESearchCase::IgnoreCase))
		{
			CurveClass = UNiagaraDataInterfaceColorCurve::StaticClass();
		}
		else
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Unknown curve type: '%s'"), *CurveType));
		}

		FNiagaraParameterHandle InputHandle =
			FNiagaraParameterHandle::CreateModuleParameterHandle(FName(*ResolvedInputName));
		FNiagaraParameterHandle AliasedHandle =
			FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(InputHandle, ModuleNode);

		FNiagaraTypeDefinition CurveTypeDef(CurveClass);
		UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
			*ModuleNode, AliasedHandle, CurveTypeDef, FGuid(), FGuid());

		if (OverridePin.LinkedTo.Num() > 0)
		{
			NiagaraCommonUtils::RemoveOverridePinConnections(OverridePin, Graph);
		}

		UNiagaraDataInterface* CurveDataInterface = nullptr;
		FString AliasedInputName = AliasedHandle.GetParameterHandleString().ToString();
		FNiagaraStackGraphUtilities::SetDataInterfaceValueForFunctionInput(
			OverridePin, CurveClass, AliasedInputName, CurveDataInterface, FGuid());

		if (!CurveDataInterface)
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create curve data interface"));
		}

		FScopedTransaction Transaction(
			NSLOCTEXT("UnrealMCPBridge", "SetNiagaraCurveDirect", "Set Niagara Curve Direct"));
		CurveDataInterface->Modify();

		int32 KeyCount = 0;
		if (CurveType.Equals(TEXT("float"), ESearchCase::IgnoreCase))
		{
			KeyCount = NiagaraCommonUtils::PopulateFloatCurveDataInterface(
				Cast<UNiagaraDataInterfaceCurve>(CurveDataInterface), *KeysArrayPtr);
		}
		else if (CurveType.Equals(TEXT("vector"), ESearchCase::IgnoreCase))
		{
			KeyCount = NiagaraCommonUtils::PopulateVectorCurveDataInterface(
				Cast<UNiagaraDataInterfaceVectorCurve>(CurveDataInterface), *KeysArrayPtr);
		}
		else if (CurveType.Equals(TEXT("color"), ESearchCase::IgnoreCase))
		{
			KeyCount = NiagaraCommonUtils::PopulateColorCurveDataInterface(
				Cast<UNiagaraDataInterfaceColorCurve>(CurveDataInterface), *KeysArrayPtr);
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UNiagaraNodeInput* InputNode = Cast<UNiagaraNodeInput>(Node);
			if (!InputNode)
			{
				continue;
			}

			FObjectProperty* DIProp = CastField<FObjectProperty>(
				InputNode->GetClass()->FindPropertyByName(TEXT("DataInterface")));
			if (!DIProp)
			{
				continue;
			}

			UNiagaraDataInterface* NodeDI = Cast<UNiagaraDataInterface>(
				DIProp->GetObjectPropertyValue_InContainer(InputNode));
			if (NodeDI == CurveDataInterface)
			{
				InputNode->MarkNodeRequiresSynchronization(
					TEXT("Curve data interface modified"), true);
				break;
			}
		}

		Graph->NotifyGraphChanged();
		ModuleNode->MarkNodeRequiresSynchronization(TEXT("Curve set"), true);

		System->MarkPackageDirty();
		System->PostEditChange();
		System->RequestCompile(true);
		System->WaitForCompilationComplete();

		TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("module_name"), ModuleName);
		Data->SetStringField(TEXT("input_name"), InputName);
		Data->SetStringField(TEXT("curve_type"), CurveType);
		Data->SetNumberField(TEXT("key_count"), KeyCount);
		Data->SetBoolField(TEXT("direct_data_interface"), true);
		Data->SetBoolField(TEXT("stateless_emitter"), false);

		return CreateSuccessResponse(Data);
	}
	FString DynamicInputPath;
	FNiagaraTypeDefinition InputType;
	UClass* CurveClass = nullptr;

	if (CurveType.Equals(TEXT("float"), ESearchCase::IgnoreCase))
	{
		DynamicInputPath = TEXT("/Niagara/DynamicInputs/ValueFromCurve/FloatFromCurve.FloatFromCurve");
		InputType = FNiagaraTypeDefinition::GetFloatDef();
		CurveClass = UNiagaraDataInterfaceCurve::StaticClass();
	}
	else if (CurveType.Equals(TEXT("vector"), ESearchCase::IgnoreCase))
	{
		DynamicInputPath = TEXT("/Niagara/DynamicInputs/ValueFromCurve/VectorFromCurve.VectorFromCurve");
		InputType = FNiagaraTypeDefinition::GetVec3Def();
		CurveClass = UNiagaraDataInterfaceVectorCurve::StaticClass();
	}
	else if (CurveType.Equals(TEXT("color"), ESearchCase::IgnoreCase))
	{
		DynamicInputPath = TEXT("/Niagara/DynamicInputs/ValueFromCurve/ColorFromCurve.ColorFromCurve");
		InputType = FNiagaraTypeDefinition::GetColorDef();
		CurveClass = UNiagaraDataInterfaceColorCurve::StaticClass();
	}
	else
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(
				TEXT("Unknown curve type: '%s'. Supported: float, vector, color"), *CurveType));
	}

	UNiagaraScript* DynamicInputScript = LoadObject<UNiagaraScript>(nullptr, *DynamicInputPath);
	if (!DynamicInputScript)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to load Dynamic Input script: %s"), *DynamicInputPath));
	}

	FNiagaraParameterHandle InputHandle =
		FNiagaraParameterHandle::CreateModuleParameterHandle(FName(*ResolvedInputName));
	FNiagaraParameterHandle AliasedHandle =
		FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(InputHandle, ModuleNode);

	UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
		*ModuleNode, AliasedHandle, InputType, FGuid(), FGuid());

	if (OverridePin.LinkedTo.Num() > 0)
	{
		NiagaraCommonUtils::RemoveOverridePinConnections(OverridePin, Graph);
	}

	UNiagaraNodeFunctionCall* DynamicInputNode = nullptr;
	FNiagaraStackGraphUtilities::SetDynamicInputForFunctionInput(
		OverridePin, DynamicInputScript, DynamicInputNode, FGuid(),
		CurveType + TEXT("FromCurve"), FGuid());

	if (!DynamicInputNode)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Failed to create Dynamic Input node"));
	}

	FString CurveInputName;
	{
		TArray<FNiagaraVariable> DynamicInputVars;
		TSet<FNiagaraVariable> DynamicInputHidden;
		FCompileConstantResolver Resolver;
		FNiagaraStackGraphUtilities::GetStackFunctionInputs(
			*DynamicInputNode,
			DynamicInputVars,
			DynamicInputHidden,
			Resolver,
			FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::AllInputs,
			/*bIgnoreDisabled=*/true);

		const FString ModulePrefix(TEXT("Module."));
		for (const FNiagaraVariable& Var : DynamicInputVars)
		{
			if (!Var.GetType().IsDataInterface())
			{
				continue;
			}

			FString DisplayName = Var.GetName().ToString();
			if (DisplayName.StartsWith(ModulePrefix))
			{
				DisplayName.RightChopInline(ModulePrefix.Len());
			}

			if (Var.GetType().GetClass() == CurveClass ||
				DisplayName.Contains(TEXT("Curve"), ESearchCase::IgnoreCase))
			{
				CurveInputName = DisplayName;
				break;
			}
		}
	}
	if (CurveInputName.IsEmpty())
	{
		CurveInputName = TEXT("DefaultCurve");
	}
	FNiagaraParameterHandle CurveInputHandle =
		FNiagaraParameterHandle::CreateModuleParameterHandle(FName(*CurveInputName));
	FNiagaraParameterHandle AliasedCurveHandle =
		FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(CurveInputHandle, DynamicInputNode);

	FNiagaraTypeDefinition CurveInputType(CurveClass);
	UEdGraphPin& CurveOverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
		*DynamicInputNode, AliasedCurveHandle, CurveInputType, FGuid(), FGuid());

	if (CurveOverridePin.LinkedTo.Num() > 0)
	{
		NiagaraCommonUtils::RemoveOverridePinConnections(CurveOverridePin, Graph);
	}

	UNiagaraDataInterface* CurveDataInterface = nullptr;
	FString AliasedCurveInputName = AliasedCurveHandle.GetParameterHandleString().ToString();
	FNiagaraStackGraphUtilities::SetDataInterfaceValueForFunctionInput(
		CurveOverridePin, CurveClass, AliasedCurveInputName, CurveDataInterface, FGuid());

	if (!CurveDataInterface)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Failed to create curve data interface on dynamic input"));
	}

	FScopedTransaction Transaction(
		NSLOCTEXT("UnrealMCPBridge", "SetNiagaraCurveDI", "Set Niagara Curve"));
	CurveDataInterface->Modify();

	int32 KeyCount = 0;
	if (CurveType.Equals(TEXT("float"), ESearchCase::IgnoreCase))
	{
		KeyCount = NiagaraCommonUtils::PopulateFloatCurveDataInterface(
			Cast<UNiagaraDataInterfaceCurve>(CurveDataInterface), *KeysArrayPtr);
	}
	else if (CurveType.Equals(TEXT("vector"), ESearchCase::IgnoreCase))
	{
		KeyCount = NiagaraCommonUtils::PopulateVectorCurveDataInterface(
			Cast<UNiagaraDataInterfaceVectorCurve>(CurveDataInterface), *KeysArrayPtr);
	}
	else if (CurveType.Equals(TEXT("color"), ESearchCase::IgnoreCase))
	{
		KeyCount = NiagaraCommonUtils::PopulateColorCurveDataInterface(
			Cast<UNiagaraDataInterfaceColorCurve>(CurveDataInterface), *KeysArrayPtr);
	}

	Graph->NotifyGraphChanged();
	DynamicInputNode->MarkNodeRequiresSynchronization(TEXT("Curve set"), true);
	ModuleNode->MarkNodeRequiresSynchronization(TEXT("Input overridden"), true);

	System->MarkPackageDirty();
	System->PostEditChange();
	System->RequestCompile(true);
	System->WaitForCompilationComplete();

	for (UEdGraphPin* Pin : ModuleNode->Pins)
	{
		if (Pin->Direction != EGPD_Input)
		{
			continue;
		}
		if (!Pin->PinName.ToString().Contains(TEXT("Scale Mode")))
		{
			continue;
		}
		if (!Pin->PinType.PinSubCategoryObject.IsValid())
		{
			continue;
		}

		UEnum* EnumType = Cast<UEnum>(Pin->PinType.PinSubCategoryObject.Get());
		if (!EnumType)
		{
			continue;
		}

		FString BestModeValue;
		for (int32 i = 0; i < EnumType->NumEnums() - 1; ++i)
		{
			FString DisplayName = EnumType->GetDisplayNameTextByIndex(i).ToString();
			if (DisplayName.Contains(TEXT("Curve"), ESearchCase::IgnoreCase))
			{
				BestModeValue = EnumType->GetNameStringByIndex(i);
				break;
			}
		}

		if (!BestModeValue.IsEmpty() && !Pin->DefaultValue.Equals(BestModeValue))
		{
			Pin->DefaultValue = BestModeValue;
			System->RequestCompile(true);
			System->WaitForCompilationComplete();
		}
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("module_name"), ModuleName);
	Data->SetStringField(TEXT("input_name"), ResolvedInputName);
	Data->SetStringField(TEXT("curve_type"), CurveType);
	Data->SetStringField(TEXT("curve_input_name"), CurveInputName);
	Data->SetNumberField(TEXT("key_count"), KeyCount);
	Data->SetBoolField(TEXT("dynamic_input_used"), true);
	Data->SetBoolField(TEXT("stateless_emitter"), false);

	return CreateSuccessResponse(Data);
}



// ---------------------------------------------------------------------------
// FGetNiagaraRapidIterationParametersAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FGetNiagaraRapidIterationParametersAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath, EmitterName;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath) ||
		!Params->TryGetStringField(TEXT("emitter_name"), EmitterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing required: system_path, emitter_name"));
	}

	FString ScriptUsageStr = TEXT("all");
	Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr);

	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 EmitterIdx;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(
		System, EmitterName, EmitterIdx, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FVersionedNiagaraEmitterData* Data = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!Data)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));
	}

	// Resolve the script usages to query.
	TArray<ENiagaraScriptUsage> Usages;
	if (ScriptUsageStr.Equals(TEXT("all"), ESearchCase::IgnoreCase))
	{
		Usages.Add(ENiagaraScriptUsage::EmitterSpawnScript);
		Usages.Add(ENiagaraScriptUsage::EmitterUpdateScript);
		Usages.Add(ENiagaraScriptUsage::ParticleSpawnScript);
		Usages.Add(ENiagaraScriptUsage::ParticleUpdateScript);
	}
	else
	{
		bool bOk = false;
		ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bOk);
		if (!bOk)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Invalid script_usage: '%s'"), *ScriptUsageStr));
		}
		Usages.Add(Usage);
	}

	TArray<TSharedPtr<FJsonValue>> ParamsArr;

	for (ENiagaraScriptUsage Usage : Usages)
	{
		UNiagaraScript* Script = NiagaraCommonUtils::GetScriptForUsage(Data, Usage);
		if (!Script)
		{
			continue;
		}

		const FNiagaraParameterStore& Store = Script->RapidIterationParameters;
		TArrayView<const FNiagaraVariableWithOffset> Vars = Store.ReadParameterVariables();

		for (const FNiagaraVariableWithOffset& Var : Vars)
		{
			FString ParamName = Var.GetName().ToString();

			// Apply the optional name filter.
			if (!Filter.IsEmpty() &&
				!ParamName.Contains(Filter, ESearchCase::IgnoreCase))
			{
				continue;
			}

			auto ParamObj = MakeShared<FJsonObject>();
			ParamObj->SetStringField(TEXT("name"), ParamName);
			ParamObj->SetStringField(TEXT("type"), Var.GetType().GetName());
			ParamObj->SetStringField(TEXT("script_usage"),
			                         NiagaraCommonUtils::ScriptUsageToString(Usage));

			// Parse module_name and input_name from the RI parameter name.
			// Format: "Constants.EmitterName.ModuleName.InputName"
			TArray<FString> Parts;
			ParamName.ParseIntoArray(Parts, TEXT("."));
			if (Parts.Num() >= 4)
			{
				ParamObj->SetStringField(TEXT("module_name"), Parts[2]);
				FString InputName;
				for (int32 i = 3; i < Parts.Num(); ++i)
				{
					if (i > 3)
					{
						InputName += TEXT(".");
					}
					InputName += Parts[i];
				}
				ParamObj->SetStringField(TEXT("input_name"), InputName);
			}

			NiagaraCommonUtils::SerializeRapidIterationValue(Store, Var, ParamObj);
			ParamsArr.Add(MakeShared<FJsonValueObject>(ParamObj));
		}
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("emitter_name"), EmitterName);
	Result->SetArrayField(TEXT("parameters"), ParamsArr);
	Result->SetNumberField(TEXT("count"), ParamsArr.Num());
	return Result;
}

// ---------------------------------------------------------------------------
// FSetNiagaraRapidIterationParameterAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FSetNiagaraRapidIterationParameterAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath, EmitterName, ModuleName, InputName, ScriptUsageStr;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath) ||
		!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) ||
		!Params->TryGetStringField(TEXT("module_name"), ModuleName) ||
		!Params->TryGetStringField(TEXT("input_name"), InputName) ||
		!Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing required: system_path, emitter_name, module_name, input_name, script_usage"));
	}

	bool bOk = false;
	ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bOk);
	if (!bOk)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Invalid script_usage"));
	}

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 EmitterIdx;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(
		System, EmitterName, EmitterIdx, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("No emitter data"));
	}

	UNiagaraScript* Script = NiagaraCommonUtils::GetScriptForUsage(EmitterData, Usage);
	if (!Script)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("No script for usage: %s"), *ScriptUsageStr));
	}

	// Build the RI parameter name.
	// Format: "Constants.EmitterName.ModuleName.InputName"
	FString RIParamName = FString::Printf(TEXT("Constants.%s.%s.%s"),
	                                      *EmitterName, *ModuleName, *InputName);

	// Look up the parameter in the store.
	FNiagaraParameterStore& Store = Script->RapidIterationParameters;
	TArrayView<const FNiagaraVariableWithOffset> AllVars = Store.ReadParameterVariables();

	// Prefer an exact name match first.
	const FNiagaraVariableWithOffset* FoundVar = nullptr;
	for (const FNiagaraVariableWithOffset& Var : AllVars)
	{
		if (Var.GetName().ToString().Equals(RIParamName, ESearchCase::IgnoreCase))
		{
			FoundVar = &Var;
			break;
		}
	}

	// Fall back to a partial match on module and input names.
	if (!FoundVar)
	{
		for (const FNiagaraVariableWithOffset& Var : AllVars)
		{
			FString VarName = Var.GetName().ToString();
			if (VarName.Contains(ModuleName, ESearchCase::IgnoreCase) &&
				VarName.Contains(InputName, ESearchCase::IgnoreCase))
			{
				FoundVar = &Var;
				break;
			}
		}
	}

	if (!FoundVar)
	{
		// Return nearby candidates to aid recovery.
		TArray<FString> Available;
		for (const FNiagaraVariableWithOffset& Var : AllVars)
		{
			FString VarName = Var.GetName().ToString();
			if (VarName.Contains(ModuleName, ESearchCase::IgnoreCase))
			{
				Available.Add(VarName);
			}
		}

		FString AvailableStr = Available.Num() > 0
			                       ? FString::Join(Available, TEXT("\n  "))
			                       : TEXT("(none found for this module)");

		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Parameter not found: '%s'\nAvailable for '%s':\n  %s"),
			                *RIParamName, *ModuleName, *AvailableStr));
	}

	// Read the incoming JSON value.
	const TSharedPtr<FJsonValue>* ValueField = Params->Values.Find(TEXT("value"));
	if (!ValueField || !ValueField->IsValid())
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
	}

	FNiagaraVariable MutableVar(FoundVar->GetType(), FoundVar->GetName());
	FString TypeName = FoundVar->GetType().GetName();
	FString ValueStr;
	bool bSuccess = false;

	Script->Modify();

	// Apply the value based on the resolved parameter type.
	if (TypeName.Contains(TEXT("Float")) || TypeName == TEXT("float"))
	{
		float Val = static_cast<float>((*ValueField)->AsNumber());
		bSuccess = Store.SetParameterValue<float>(Val, MutableVar, true);
		ValueStr = FString::SanitizeFloat(Val);
	}
	else if (TypeName.Contains(TEXT("Int32")) || TypeName == TEXT("int32"))
	{
		int32 Val = static_cast<int32>((*ValueField)->AsNumber());
		bSuccess = Store.SetParameterValue<int32>(Val, MutableVar, true);
		ValueStr = FString::FromInt(Val);
	}
	else if (TypeName.Contains(TEXT("Bool")) || TypeName == TEXT("bool"))
	{
		int32 BoolVal = (*ValueField)->AsBool() ? 1 : 0;
		bSuccess = Store.SetParameterValue<int32>(BoolVal, MutableVar, true);
		ValueStr = BoolVal ? TEXT("true") : TEXT("false");
	}
	else if (TypeName.Contains(TEXT("Vector3f")) || TypeName.Contains(TEXT("Position")))
	{
		FVector3f Vec(0, 0, 0);
		if ((*ValueField)->Type == EJson::Object)
		{
			auto Obj = (*ValueField)->AsObject();
			Vec.X = static_cast<float>(Obj->GetNumberField(TEXT("x")));
			Vec.Y = static_cast<float>(Obj->GetNumberField(TEXT("y")));
			Vec.Z = static_cast<float>(Obj->GetNumberField(TEXT("z")));
		}
		else if ((*ValueField)->Type == EJson::Array)
		{
			const auto& Arr = (*ValueField)->AsArray();
			if (Arr.Num() >= 3)
			{
				Vec.X = static_cast<float>(Arr[0]->AsNumber());
				Vec.Y = static_cast<float>(Arr[1]->AsNumber());
				Vec.Z = static_cast<float>(Arr[2]->AsNumber());
			}
		}
		bSuccess = Store.SetParameterValue<FVector3f>(Vec, MutableVar, true);
		ValueStr = FString::Printf(TEXT("(%f, %f, %f)"), Vec.X, Vec.Y, Vec.Z);
	}
	else if (TypeName.Contains(TEXT("Vector2f")))
	{
		FVector2f Vec(0, 0);
		if ((*ValueField)->Type == EJson::Object)
		{
			auto Obj = (*ValueField)->AsObject();
			Vec.X = static_cast<float>(Obj->GetNumberField(TEXT("x")));
			Vec.Y = static_cast<float>(Obj->GetNumberField(TEXT("y")));
		}
		else if ((*ValueField)->Type == EJson::Array)
		{
			const auto& Arr = (*ValueField)->AsArray();
			if (Arr.Num() >= 2)
			{
				Vec.X = static_cast<float>(Arr[0]->AsNumber());
				Vec.Y = static_cast<float>(Arr[1]->AsNumber());
			}
		}
		bSuccess = Store.SetParameterValue<FVector2f>(Vec, MutableVar, true);
		ValueStr = FString::Printf(TEXT("(%f, %f)"), Vec.X, Vec.Y);
	}
	else if (TypeName.Contains(TEXT("Vector4f")))
	{
		FVector4f Vec(0, 0, 0, 0);
		if ((*ValueField)->Type == EJson::Object)
		{
			auto Obj = (*ValueField)->AsObject();
			Vec.X = static_cast<float>(Obj->GetNumberField(TEXT("x")));
			Vec.Y = static_cast<float>(Obj->GetNumberField(TEXT("y")));
			Vec.Z = static_cast<float>(Obj->GetNumberField(TEXT("z")));
			Vec.W = static_cast<float>(Obj->GetNumberField(TEXT("w")));
		}
		else if ((*ValueField)->Type == EJson::Array)
		{
			const auto& Arr = (*ValueField)->AsArray();
			if (Arr.Num() >= 4)
			{
				Vec.X = static_cast<float>(Arr[0]->AsNumber());
				Vec.Y = static_cast<float>(Arr[1]->AsNumber());
				Vec.Z = static_cast<float>(Arr[2]->AsNumber());
				Vec.W = static_cast<float>(Arr[3]->AsNumber());
			}
		}
		bSuccess = Store.SetParameterValue<FVector4f>(Vec, MutableVar, true);
		ValueStr = FString::Printf(TEXT("(%f, %f, %f, %f)"), Vec.X, Vec.Y, Vec.Z, Vec.W);
	}
	else if (TypeName.Contains(TEXT("LinearColor")))
	{
		FLinearColor Color(0, 0, 0, 1);
		if ((*ValueField)->Type == EJson::Object)
		{
			auto Obj = (*ValueField)->AsObject();
			Color.R = static_cast<float>(Obj->GetNumberField(TEXT("r")));
			Color.G = static_cast<float>(Obj->GetNumberField(TEXT("g")));
			Color.B = static_cast<float>(Obj->GetNumberField(TEXT("b")));
			double A = 1.0;
			Obj->TryGetNumberField(TEXT("a"), A);
			Color.A = static_cast<float>(A);
		}
		else if ((*ValueField)->Type == EJson::Array)
		{
			const auto& Arr = (*ValueField)->AsArray();
			if (Arr.Num() >= 3)
			{
				Color.R = static_cast<float>(Arr[0]->AsNumber());
				Color.G = static_cast<float>(Arr[1]->AsNumber());
				Color.B = static_cast<float>(Arr[2]->AsNumber());
				if (Arr.Num() >= 4)
				{
					Color.A = static_cast<float>(Arr[3]->AsNumber());
				}
			}
		}
		bSuccess = Store.SetParameterValue<FLinearColor>(Color, MutableVar, true);
		ValueStr = FString::Printf(TEXT("(R=%f, G=%f, B=%f, A=%f)"),
		                           Color.R, Color.G, Color.B, Color.A);
	}
	else if (TypeName.Contains(TEXT("Quat")))
	{
		FQuat4f Quat(0, 0, 0, 1);
		if ((*ValueField)->Type == EJson::Object)
		{
			auto Obj = (*ValueField)->AsObject();
			Quat.X = static_cast<float>(Obj->GetNumberField(TEXT("x")));
			Quat.Y = static_cast<float>(Obj->GetNumberField(TEXT("y")));
			Quat.Z = static_cast<float>(Obj->GetNumberField(TEXT("z")));
			Quat.W = static_cast<float>(Obj->GetNumberField(TEXT("w")));
		}
		bSuccess = Store.SetParameterValue<FQuat4f>(Quat, MutableVar, true);
		ValueStr = FString::Printf(TEXT("(%f, %f, %f, %f)"),
		                           Quat.X, Quat.Y, Quat.Z, Quat.W);
	}
	else
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unsupported parameter type: '%s'"), *TypeName));
	}

	if (!bSuccess)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Failed to set parameter value in the store"));
	}

	// Recompile to apply the change.
	NiagaraCommonUtils::CompileAndSync(System);
	UEditorAssetLibrary::SaveAsset(SystemPath);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("parameter_name"), FoundVar->GetName().ToString());
	Result->SetStringField(TEXT("module_name"), ModuleName);
	Result->SetStringField(TEXT("input_name"), InputName);
	Result->SetStringField(TEXT("value"), ValueStr);
	Result->SetStringField(TEXT("type"), TypeName);
	Result->SetStringField(TEXT("script_usage"), ScriptUsageStr);
	return Result;
}




// ---------------------------------------------------------------------------
// FListNiagaraModulesAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FListNiagaraModulesAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Category = TEXT("all");
	Params->TryGetStringField(TEXT("category"), Category);

	FString Search;
	Params->TryGetStringField(TEXT("search"), Search);

	// Hardcode a catalog of common built-in Niagara modules.
	struct FModuleEntry
	{
		const TCHAR* Path;
		const TCHAR* Category;
		const TCHAR* Name;
		const TCHAR* Description;
	};

	static const FModuleEntry BuiltInModules[] =
	{
		// Emitter lifecycle.
		{
			TEXT("/Niagara/Modules/Emitter/EmitterState.EmitterState"), TEXT("emitter"), TEXT("Emitter State"),
			TEXT("Controls emitter lifecycle state")
		},
		{TEXT("/Niagara/Modules/Emitter/SpawnRate.SpawnRate"), TEXT("emitter"), TEXT("Spawn Rate"), TEXT("Sets particle spawn rate")},
		{
			TEXT("/Niagara/Modules/Emitter/SpawnBurstInstantaneous.SpawnBurstInstantaneous"), TEXT("emitter"),
			TEXT("Spawn Burst Instantaneous"), TEXT("Spawns particles in a single burst")
		},
		{
			TEXT("/Niagara/Modules/Emitter/SpawnPerUnit.SpawnPerUnit"), TEXT("emitter"), TEXT("Spawn Per Unit"),
			TEXT("Spawns particles based on distance traveled")
		},

		// Spawn - Location.
		{
			TEXT("/Niagara/Modules/Spawn/Location/SystemLocation.SystemLocation"), TEXT("spawn"), TEXT("System Location"),
			TEXT("Sets particle spawn location to system location")
		},
		{
			TEXT("/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation"), TEXT("spawn"), TEXT("Shape Location"),
			TEXT("Random location using configurable shapes: Sphere, Box, Cylinder, Torus, Cone, Ring, Disc, Grid")
		},

		// Spawn - Velocity.
		{
			TEXT("/Niagara/Modules/Spawn/Velocity/AddVelocity.AddVelocity"), TEXT("spawn"), TEXT("Add Velocity"),
			TEXT("Adds velocity to particles")
		},
		{
			TEXT("/Niagara/Modules/Spawn/Velocity/AddVelocityInCone.AddVelocityInCone"), TEXT("spawn"), TEXT("Add Velocity In Cone"),
			TEXT("Random velocity within cone")
		},
		{
			TEXT("/Niagara/Modules/Spawn/Velocity/AddVelocityFromPoint.AddVelocityFromPoint"), TEXT("spawn"),
			TEXT("Add Velocity From Point"), TEXT("Velocity away from point")
		},
		{
			TEXT("/Niagara/Modules/Spawn/Velocity/InheritVelocity.InheritVelocity"), TEXT("spawn"), TEXT("Inherit Velocity"),
			TEXT("Inherit velocity from parent")
		},

		// Spawn - Initialize.
		{
			TEXT("/Niagara/Modules/Spawn/Initialize/InitializeParticle.InitializeParticle"), TEXT("spawn"), TEXT("Initialize Particle"),
			TEXT("Sets initial particle properties")
		},
		{
			TEXT("/Niagara/Modules/Spawn/Size/InitialSize.InitialSize"), TEXT("spawn"), TEXT("Initial Size"),
			TEXT("Sets initial particle size")
		},
		{
			TEXT("/Niagara/Modules/Spawn/Mass/InitialMass.InitialMass"), TEXT("spawn"), TEXT("Initial Mass"),
			TEXT("Sets initial particle mass")
		},
		{
			TEXT("/Niagara/Modules/Spawn/Lifetime/SetLifetime.SetLifetime"), TEXT("spawn"), TEXT("Set Lifetime"),
			TEXT("Sets particle lifetime")
		},

		// Spawn - Rotation.
		{
			TEXT("/Niagara/Modules/Spawn/Rotation/InitialMeshOrientation.InitialMeshOrientation"), TEXT("spawn"),
			TEXT("Initial Mesh Orientation"), TEXT("Sets initial mesh rotation")
		},

		// Update.
		{TEXT("/Niagara/Modules/Update/Lifetime/UpdateAge.UpdateAge"), TEXT("update"), TEXT("Update Age"), TEXT("Updates particle age")},
		{TEXT("/Niagara/Modules/Update/Color/Color.Color"), TEXT("update"), TEXT("Color"), TEXT("Sets particle color over lifetime")},
		{TEXT("/Niagara/Modules/Update/Color/ScaleColor.ScaleColor"), TEXT("update"), TEXT("Scale Color"), TEXT("Scales particle color")},
		{
			TEXT("/Niagara/Modules/Update/Size/ScaleSize.ScaleSize"), TEXT("update"), TEXT("Scale Size"),
			TEXT("Scales particle size over lifetime")
		},
		{
			TEXT("/Niagara/Modules/Update/Size/ScaleSizeBySpeed.ScaleSizeBySpeed"), TEXT("update"), TEXT("Scale Size By Speed"),
			TEXT("Scales size based on velocity")
		},
		{
			TEXT("/Niagara/Modules/Update/Rotation/UpdateMeshOrientation.UpdateMeshOrientation"), TEXT("update"),
			TEXT("Update Mesh Orientation"), TEXT("Updates mesh rotation")
		},
		{
			TEXT("/Niagara/Modules/Update/Orientation/SpriteFacingAndAlignment.SpriteFacingAndAlignment"), TEXT("update"),
			TEXT("Sprite Facing And Alignment"), TEXT("Controls sprite billboard mode")
		},

		// Forces.
		{TEXT("/Niagara/Modules/Forces/Gravity.Gravity"), TEXT("forces"), TEXT("Gravity"), TEXT("Applies gravity force")},
		{TEXT("/Niagara/Modules/Forces/Drag.Drag"), TEXT("forces"), TEXT("Drag"), TEXT("Applies drag force")},
		{TEXT("/Niagara/Modules/Forces/Wind.Wind"), TEXT("forces"), TEXT("Wind"), TEXT("Applies wind force")},
		{TEXT("/Niagara/Modules/Forces/PointForce.PointForce"), TEXT("forces"), TEXT("Point Force"), TEXT("Force from a point")},
		{TEXT("/Niagara/Modules/Forces/VortexForce.VortexForce"), TEXT("forces"), TEXT("Vortex Force"), TEXT("Rotating vortex force")},
		{
			TEXT("/Niagara/Modules/Forces/CurlNoiseForce.CurlNoiseForce"), TEXT("forces"), TEXT("Curl Noise Force"),
			TEXT("Curl noise turbulence")
		},

		// Solver.
		{
			TEXT("/Niagara/Modules/Solvers/SolveForcesAndVelocity.SolveForcesAndVelocity"), TEXT("update"),
			TEXT("Solve Forces And Velocity"), TEXT("Integrates forces to update position")
		},

		// Collision.
		{
			TEXT("/Niagara/Modules/Collision/CollisionQuery.CollisionQuery"), TEXT("update"), TEXT("Collision Query"),
			TEXT("Performs collision detection")
		},
	};

	TArray<TSharedPtr<FJsonValue>> ModulesArr;

	FString LowerCategory = Category.ToLower();
	FString LowerSearch = Search.ToLower();

	for (const FModuleEntry& Module : BuiltInModules)
	{
		// Apply the category filter.
		if (LowerCategory != TEXT("all"))
		{
			FString ModCat = Module.Category;
			if (!ModCat.Equals(Category, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}

		// Apply the search filter.
		if (!LowerSearch.IsEmpty())
		{
			FString ModName = Module.Name;
			FString ModPath = Module.Path;
			FString ModDescription = Module.Description;
			if (!ModName.Contains(Search, ESearchCase::IgnoreCase) &&
				!ModPath.Contains(Search, ESearchCase::IgnoreCase) &&
				!ModDescription.Contains(Search, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}

		auto ModObj = MakeShared<FJsonObject>();
		ModObj->SetStringField(TEXT("name"), Module.Name);
		ModObj->SetStringField(TEXT("path"), Module.Path);
		ModObj->SetStringField(TEXT("category"), Module.Category);
		ModObj->SetStringField(TEXT("description"), Module.Description);
		ModulesArr.Add(MakeShared<FJsonValueObject>(ModObj));
	}

	// Also search the Asset Registry for project-local NiagaraScript modules.
	if (!Search.IsEmpty())
	{
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		FARFilter Filter;
		Filter.ClassPaths.Add(UNiagaraScript::StaticClass()->GetClassPathName());
		Filter.bRecursivePaths = true;
		Filter.PackagePaths.Add(FName(TEXT("/Game")));

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssets(Filter, Assets);

		for (const FAssetData& Asset : Assets)
		{
			FString AssetName = Asset.AssetName.ToString();
			FString AssetPath = Asset.GetObjectPathString();

			if (AssetName.Contains(Search, ESearchCase::IgnoreCase))
			{
				auto ModObj = MakeShared<FJsonObject>();
				ModObj->SetStringField(TEXT("name"), AssetName);
				ModObj->SetStringField(TEXT("path"), AssetPath);
				ModObj->SetStringField(TEXT("category"), TEXT("project"));
				ModObj->SetStringField(TEXT("description"), TEXT("Project module script"));
				ModulesArr.Add(MakeShared<FJsonValueObject>(ModObj));
			}
		}
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("category_filter"), Category);
	Result->SetStringField(TEXT("search_filter"), Search);
	Result->SetArrayField(TEXT("modules"), ModulesArr);
	Result->SetNumberField(TEXT("count"), ModulesArr.Num());
	return Result;
}

// ---------------------------------------------------------------------------
// Niagara stack-input introspection and mutation helpers.
//
// These helpers operate on module inputs inside an emitter stack. Some of the
// engine's stack-override helpers are not exported, so this file reproduces
// the needed behavior with exported Niagara APIs plus direct graph inspection.
// Exported APIs used here include:
//   - FNiagaraStackGraphUtilities::GetStackFunctionInputs                   (h:134)
//   - FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput  (h:227)
//   - FNiagaraStackGraphUtilities::SetDynamicInputForFunctionInput          (h:233)
//   - UNiagaraStackFunctionInput::GetAvailableParameters                    (h:184)
//   - FNiagaraEditorUtilities::GetFilteredScriptAssets                      - Dynamic-input discovery
//   - FVersionedNiagaraEmitterData::GetScript(Usage, UsageId)               (Emitter.h:384)
// ---------------------------------------------------------------------------

struct FBindingInfo
{
	FString Mode; // One of: "default", "local", "linked", "dynamic", "function_call", "data", or "unknown".
	FString LinkedParameter; // Valid for linked/data bindings.
	FString DynamicInputScriptPath; // Asset path for dynamic/function-call bindings.
	FString DynamicInputFunctionName; // Friendly function name for responses.
	UNiagaraNodeFunctionCall* DynamicInputCall = nullptr; // Used for nested traversal.
	FString LocalValue; // Literal override value for local bindings.
};

static FBindingInfo ClassifyOverride(UEdGraphPin* OverridePin)
	{
		FBindingInfo Info;
		if (!OverridePin)
		{
			Info.Mode = TEXT("default");
			return Info;
		}

		// No link means a local literal, which may still carry an explicit default string.
		if (OverridePin->LinkedTo.Num() == 0)
		{
			Info.Mode = TEXT("local");
			Info.LocalValue = OverridePin->DefaultValue.IsEmpty()
				                  ? OverridePin->AutogeneratedDefaultValue
				                  : OverridePin->DefaultValue;
			return Info;
		}

		UEdGraphPin* SourcePin = OverridePin->LinkedTo[0];
		if (!SourcePin || !SourcePin->GetOwningNodeUnchecked())
		{
			Info.Mode = TEXT("unknown");
			return Info;
		}
		UEdGraphNode* SourceNode = SourcePin->GetOwningNode();

		if (UNiagaraNodeFunctionCall* FnCall = Cast<UNiagaraNodeFunctionCall>(SourceNode))
		{
			// Dynamic inputs are function-call nodes whose script usage is DynamicInput.
			if (FnCall->FunctionScript &&
				FnCall->FunctionScript->GetUsage() == ENiagaraScriptUsage::DynamicInput)
			{
				Info.Mode = TEXT("dynamic");
				Info.DynamicInputScriptPath = FnCall->FunctionScript->GetPathName();
				Info.DynamicInputFunctionName = FnCall->GetFunctionName();
				Info.DynamicInputCall = FnCall;
				return Info;
			}
			// Keep non-dynamic function calls visible to the caller for debugging.
			Info.Mode = TEXT("function_call");
			if (FnCall->FunctionScript)
			{
				Info.DynamicInputScriptPath = FnCall->FunctionScript->GetPathName();
			}
			Info.DynamicInputFunctionName = FnCall->GetFunctionName();
			return Info;
		}

		if (Cast<UNiagaraNodeParameterMapGet>(SourceNode))
		{
			Info.Mode = TEXT("linked");
			Info.LinkedParameter = SourcePin->PinName.ToString();
			return Info;
		}

		if (UNiagaraNodeInput* InputNode = Cast<UNiagaraNodeInput>(SourceNode))
		{
			// Data-interface locals are represented by an input node.
			Info.Mode = TEXT("data");
			Info.LinkedParameter = InputNode->Input.GetName().ToString();
			return Info;
		}

		if (Cast<UNiagaraNodeCustomHlsl>(SourceNode))
		{
			Info.Mode = TEXT("expression");
			return Info;
		}

		Info.Mode = TEXT("unknown");
		return Info;
	}

// Serialize one resolved binding to JSON and optionally recurse into the
// dynamic input child bindings.
static TSharedPtr<FJsonObject> SerializeBinding(
		const FString& InputName,
		const FNiagaraVariable& InputVar,
		const FBindingInfo& Info,
		int32 RemainingDepth)
	{
		auto J = MakeShared<FJsonObject>();
		J->SetStringField(TEXT("name"), InputName);
		J->SetStringField(TEXT("type"), InputVar.GetType().GetName());
		J->SetStringField(TEXT("mode"), Info.Mode);

		if (Info.Mode == TEXT("local") && !Info.LocalValue.IsEmpty())
		{
			J->SetStringField(TEXT("value"), Info.LocalValue);
		}
		if (Info.Mode == TEXT("linked") || Info.Mode == TEXT("data"))
		{
			J->SetStringField(TEXT("linked_parameter"), Info.LinkedParameter);
		}
		if (Info.Mode == TEXT("dynamic") || Info.Mode == TEXT("function_call"))
		{
			if (!Info.DynamicInputScriptPath.IsEmpty())
			{
				J->SetStringField(TEXT("script_path"), Info.DynamicInputScriptPath);
			}
			if (!Info.DynamicInputFunctionName.IsEmpty())
			{
				J->SetStringField(TEXT("function_name"), Info.DynamicInputFunctionName);
			}

			// Recurse into child inputs so nested dynamic-input bindings remain visible.
			if (Info.Mode == TEXT("dynamic") && Info.DynamicInputCall && RemainingDepth > 0)
			{
				TArray<FNiagaraVariable> ChildVars;
				TSet<FNiagaraVariable> ChildHidden;
				FCompileConstantResolver Resolver;
				FNiagaraStackGraphUtilities::GetStackFunctionInputs(
					*Info.DynamicInputCall,
					ChildVars,
					ChildHidden,
					Resolver,
					FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::AllInputs,
					/*bIgnoreDisabled=*/true);

				TArray<TSharedPtr<FJsonValue>> ChildArr;
				for (const FNiagaraVariable& ChildVar : ChildVars)
				{
					const FNiagaraParameterHandle ChildHandle =
						FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
							FNiagaraParameterHandle(ChildVar.GetName()), Info.DynamicInputCall);
					UEdGraphPin* ChildPin = NiagaraCommonUtils::FindModuleInputOverridePin(*Info.DynamicInputCall, ChildHandle);
					FBindingInfo ChildInfo = ClassifyOverride(ChildPin);

					// Strip the Module. prefix to match stack-UI display names.
					FString ChildName = ChildVar.GetName().ToString();
					ChildName.RemoveFromStart(TEXT("Module."));

					ChildArr.Add(MakeShared<FJsonValueObject>(
						SerializeBinding(ChildName, ChildVar, ChildInfo, RemainingDepth - 1)));
				}
				J->SetArrayField(TEXT("children"), ChildArr);
			}
		}
		return J;
	}


// ---------------------------------------------------------------------------
// FGetNiagaraModuleInputBindingAction::ExecuteInternal
//
// Params:
//   system_path, emitter_name, module_name, script_usage  (required)
//   input_filter    - Optional substring match on input name.
//   max_depth       - Nested recursion cap. Default is 3.
//
// Returns per input: name, type, mode, value/linked_parameter/script_path,
// and recursive children when bound to a dynamic input.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FGetNiagaraModuleInputBindingAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath, EmitterName, ModuleName, ScriptUsageStr;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath) ||
		!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) ||
		!Params->TryGetStringField(TEXT("module_name"), ModuleName) ||
		!Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing required: system_path, emitter_name, module_name, script_usage"));
	}

	bool bUsageParsed = false;
	ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bUsageParsed);
	if (!bUsageParsed)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unknown script_usage '%s'"), *ScriptUsageStr));
	}

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	UNiagaraScript* Script = nullptr;
	UNiagaraGraph* Graph = NiagaraCommonUtils::ResolveEmitterStackGraph(System, EmitterName, Usage, Script, Error);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	UNiagaraNodeFunctionCall* ModuleCall = NiagaraCommonUtils::FindModuleFunctionCall(Graph, ModuleName);
	if (!ModuleCall)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Module '%s' not found in %s stack"),
			                *ModuleName, *ScriptUsageStr));
	}

	FString InputFilter;
	Params->TryGetStringField(TEXT("input_filter"), InputFilter);
	int32 MaxDepth = 3;
	Params->TryGetNumberField(TEXT("max_depth"), MaxDepth);

	TArray<FNiagaraVariable> InputVars;
	TSet<FNiagaraVariable> HiddenVars;
	FCompileConstantResolver Resolver;
	FNiagaraStackGraphUtilities::GetStackFunctionInputs(
		*ModuleCall, InputVars, HiddenVars, Resolver,
		FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::AllInputs,
		/*bIgnoreDisabled=*/true);

	TArray<TSharedPtr<FJsonValue>> BindingArr;
	for (const FNiagaraVariable& Var : InputVars)
	{
		FString InputName = Var.GetName().ToString();
		InputName.RemoveFromStart(TEXT("Module."));

		if (!InputFilter.IsEmpty() &&
			!InputName.Contains(InputFilter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		const FNiagaraParameterHandle Aliased =
			FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
				FNiagaraParameterHandle(Var.GetName()), ModuleCall);
		UEdGraphPin* OverridePin = NiagaraCommonUtils::FindModuleInputOverridePin(*ModuleCall, Aliased);
		FBindingInfo Info = ClassifyOverride(OverridePin);
		BindingArr.Add(MakeShared<FJsonValueObject>(
			SerializeBinding(InputName, Var, Info, MaxDepth)));
	}

	auto R = MakeShared<FJsonObject>();
	R->SetBoolField(TEXT("success"), true);
	R->SetStringField(TEXT("system_path"), SystemPath);
	R->SetStringField(TEXT("emitter_name"), EmitterName);
	R->SetStringField(TEXT("module_name"), ModuleName);
	R->SetStringField(TEXT("script_usage"), ScriptUsageStr);
	R->SetNumberField(TEXT("input_count"), BindingArr.Num());
	R->SetArrayField(TEXT("inputs"), BindingArr);
	return R;
}


// ---------------------------------------------------------------------------
// FClearNiagaraModuleInputAction::ExecuteInternal
//
// Mirrors the stack UI's "Reset to Default". It clears the override pin,
// removes orphaned feeder nodes, and then removes the override pin itself
// from the parameter-map set node.
//
// Params: system_path + emitter_name + module_name + script_usage + input_name.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FClearNiagaraModuleInputAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath, EmitterName, ModuleName, ScriptUsageStr, InputName;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath) ||
		!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) ||
		!Params->TryGetStringField(TEXT("module_name"), ModuleName) ||
		!Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr) ||
		!Params->TryGetStringField(TEXT("input_name"), InputName))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing required: system_path, emitter_name, module_name, script_usage, input_name"));
	}

	bool bUsageParsed = false;
	ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bUsageParsed);
	if (!bUsageParsed)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unknown script_usage '%s'"), *ScriptUsageStr));
	}

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	UNiagaraScript* Script = nullptr;
	UNiagaraGraph* Graph = NiagaraCommonUtils::ResolveEmitterStackGraph(System, EmitterName, Usage, Script, Error);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	UNiagaraNodeFunctionCall* ModuleCall = NiagaraCommonUtils::FindModuleFunctionCall(Graph, ModuleName);
	if (!ModuleCall)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Module '%s' not found in %s stack"),
			                *ModuleName, *ScriptUsageStr));
	}

	// Nested input paths walk from the module input into chained dynamic inputs.
	TArray<FString> Segments;
	InputName.ParseIntoArray(Segments, TEXT("."));
	if (Segments.Num() == 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Empty input_name"));
	}

	UNiagaraNodeFunctionCall* CurrentCall = ModuleCall;
	UEdGraphPin* TargetOverridePin = nullptr;
	for (int32 i = 0; i < Segments.Num(); ++i)
	{
		const FString ModulePrefixed = FString::Printf(TEXT("Module.%s"), *Segments[i]);
		const FNiagaraParameterHandle Aliased =
			FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
				FNiagaraParameterHandle(FName(*ModulePrefixed)), CurrentCall);

		UEdGraphPin* Pin = NiagaraCommonUtils::FindModuleInputOverridePin(*CurrentCall, Aliased);
		if (i == Segments.Num() - 1)
		{
			TargetOverridePin = Pin;
			break;
		}
		if (!Pin || Pin->LinkedTo.Num() == 0)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Path segment '%s' has no dynamic input to descend into"),
				                *Segments[i]));
		}
		UNiagaraNodeFunctionCall* Next =
			Cast<UNiagaraNodeFunctionCall>(Pin->LinkedTo[0]->GetOwningNode());
		if (!Next)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Path segment '%s' is not a dynamic input"),
				                *Segments[i]));
		}
		CurrentCall = Next;
	}

	if (!TargetOverridePin)
	{
		auto R = MakeShared<FJsonObject>();
		R->SetBoolField(TEXT("success"), true);
		R->SetBoolField(TEXT("was_overridden"), false);
		R->SetStringField(TEXT("message"), TEXT("Input already at default - no override pin exists"));
		return R;
	}

	// Remove linked feeder nodes after collecting them up front. This avoids
	// mutating LinkedTo while iterating it.
	TSet<UEdGraphNode*> NodesToRemove;
	for (UEdGraphPin* Linked : TargetOverridePin->LinkedTo)
	{
		if (Linked && Linked->GetOwningNodeUnchecked())
		{
			NodesToRemove.Add(Linked->GetOwningNode());
		}
	}

	CurrentCall->GetGraph()->Modify();
	TargetOverridePin->BreakAllPinLinks();

	for (UEdGraphNode* Node : NodesToRemove)
	{
		if (!Node)
		{
			continue;
		}
		// Only remove nodes that no longer feed anything after the disconnect.
		bool bStillUsed = false;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0)
			{
				bStillUsed = true;
				break;
			}
		}
		if (!bStillUsed)
		{
			Node->Modify();
			Node->BreakAllNodeLinks();
			CurrentCall->GetGraph()->RemoveNode(Node);
		}
	}

	// Remove the override pin from the map-set node so the input fully returns
	// to its default, editor-equivalent state.
	UNiagaraNodeParameterMapSet* OverrideNode = NiagaraCommonUtils::FindModuleInputOverrideNode(*CurrentCall);
	if (OverrideNode)
	{
		OverrideNode->Modify();
		OverrideNode->RemovePin(TargetOverridePin);
	}

	CurrentCall->GetGraph()->NotifyGraphChanged();
	Script->MarkPackageDirty();
	Script->PostEditChange();
	NiagaraCommonUtils::CompileAndSync(System, false);

	auto R = MakeShared<FJsonObject>();
	R->SetBoolField(TEXT("success"), true);
	R->SetBoolField(TEXT("was_overridden"), true);
	R->SetStringField(TEXT("input_name"), InputName);
	R->SetNumberField(TEXT("removed_node_count"), NodesToRemove.Num());
	return R;
}


// ---------------------------------------------------------------------------
// FListNiagaraInputSourceMenuAction::ExecuteInternal
//
// Returns three sections matching the editor menu:
//   1. dynamic_inputs   - Every UNiagaraScript with Usage=DynamicInput whose
//                          output type matches the input type (when provided).
//                          Built via FNiagaraEditorUtilities::GetFilteredScriptAssets.
//   2. link_parameters  - Namespace-grouped parameters (Engine/Emitter/Particles/User/System/StackContext)
//                          that are type-compatible. Built via parameter map history.
//   3. make             - "Read from new <scope> parameter" actions for each
//                          writable namespace.
//
// Params: system_path, emitter_name, module_name, script_usage, input_name,
//         type_filter (optional - limit to a specific type), name_filter (optional).
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FListNiagaraInputSourceMenuAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath, EmitterName, ModuleName, ScriptUsageStr, InputName;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath) ||
		!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) ||
		!Params->TryGetStringField(TEXT("module_name"), ModuleName) ||
		!Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr) ||
		!Params->TryGetStringField(TEXT("input_name"), InputName))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing required: system_path, emitter_name, module_name, script_usage, input_name"));
	}

	FString NameFilter;
	Params->TryGetStringField(TEXT("name_filter"), NameFilter);

	bool bUsageParsed = false;
	ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bUsageParsed);
	if (!bUsageParsed)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unknown script_usage '%s'"), *ScriptUsageStr));
	}

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	UNiagaraScript* Script = nullptr;
	UNiagaraGraph* Graph = NiagaraCommonUtils::ResolveEmitterStackGraph(System, EmitterName, Usage, Script, Error);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	UNiagaraNodeFunctionCall* ModuleCall = NiagaraCommonUtils::FindModuleFunctionCall(Graph, ModuleName);
	if (!ModuleCall)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Module '%s' not found"), *ModuleName));
	}

	// Resolve the target input type so dynamic inputs can be filtered by the
	// same output-type compatibility rules as the editor UI.
	TArray<FNiagaraVariable> InputVars;
	TSet<FNiagaraVariable> HiddenVars;
	FCompileConstantResolver Resolver;
	FNiagaraStackGraphUtilities::GetStackFunctionInputs(
		*ModuleCall, InputVars, HiddenVars, Resolver,
		FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::AllInputs,
		/*bIgnoreDisabled=*/true);

	FNiagaraTypeDefinition TargetType;
	const FString ModulePrefixed = FString::Printf(TEXT("Module.%s"), *InputName);
	for (const FNiagaraVariable& V : InputVars)
	{
		if (V.GetName().ToString().Equals(ModulePrefixed, ESearchCase::IgnoreCase))
		{
			TargetType = V.GetType();
			break;
		}
	}

	// Section 1: dynamic inputs.
	TArray<TSharedPtr<FJsonValue>> DynamicInputArr;
	{
		FNiagaraEditorUtilities::FGetFilteredScriptAssetsOptions DIOpts;
		DIOpts.ScriptUsageToInclude = ENiagaraScriptUsage::DynamicInput;
		DIOpts.bIncludeDeprecatedScripts = false;

		TArray<FAssetData> DynamicInputAssets;
		FNiagaraEditorUtilities::GetFilteredScriptAssets(DIOpts, DynamicInputAssets);

		for (const FAssetData& AssetData : DynamicInputAssets)
		{
			const FString AssetName = AssetData.AssetName.ToString();
			if (!NameFilter.IsEmpty() &&
				!AssetName.Contains(NameFilter, ESearchCase::IgnoreCase))
			{
				continue;
			}

			auto Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("name"), AssetName);
			Entry->SetStringField(TEXT("script_path"), AssetData.GetObjectPathString());
			Entry->SetStringField(TEXT("source"), TEXT("asset_registry"));
			// Category metadata is optional.
			FString Category;
			if (AssetData.GetTagValue<FString>(TEXT("Category"), Category) && !Category.IsEmpty())
			{
				Entry->SetStringField(TEXT("category"), Category);
			}
			DynamicInputArr.Add(MakeShared<FJsonValueObject>(Entry));
		}

		// Also include scratch-pad dynamic-input scripts. They are valid stack
		// choices, but they do not appear in AssetRegistry results because they
		// are sub-objects on the Niagara system.
		for (UNiagaraScript* ScratchScript : System->ScratchPadScripts)
		{
			if (!ScratchScript)
			{
				continue;
			}
			if (ScratchScript->GetUsage() != ENiagaraScriptUsage::DynamicInput)
			{
				continue;
			}

			const FString AssetName = ScratchScript->GetName();
			if (!NameFilter.IsEmpty() &&
				!AssetName.Contains(NameFilter, ESearchCase::IgnoreCase))
			{
				continue;
			}

			auto Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("name"), AssetName);
			Entry->SetStringField(TEXT("script_path"), ScratchScript->GetPathName());
			Entry->SetStringField(TEXT("source"), TEXT("scratch_pad"));
			Entry->SetStringField(TEXT("category"), TEXT("Scratch Pad"));
			DynamicInputArr.Add(MakeShared<FJsonValueObject>(Entry));
		}
	}

	// Section 2: link parameters.
	// Enumerate well-known Niagara constants plus the system's user parameters.
	TArray<TSharedPtr<FJsonValue>> LinkParamArr;
	auto AddLinkEntry = [&](const FString& Name, const FString& Type, const FString& Namespace)
	{
		if (!NameFilter.IsEmpty() && !Name.Contains(NameFilter, ESearchCase::IgnoreCase))
		{
			return;
		}
		auto E = MakeShared<FJsonObject>();
		E->SetStringField(TEXT("name"), Name);
		E->SetStringField(TEXT("type"), Type);
		E->SetStringField(TEXT("namespace"), Namespace);
		LinkParamArr.Add(MakeShared<FJsonValueObject>(E));
	};

	for (const FNiagaraVariable& EngineVar : FNiagaraConstants::GetEngineConstants())
	{
		AddLinkEntry(EngineVar.GetName().ToString(), EngineVar.GetType().GetName(), TEXT("Engine"));
	}
	for (const FNiagaraVariable& SysVar : FNiagaraConstants::GetCommonParticleAttributes())
	{
		AddLinkEntry(SysVar.GetName().ToString(), SysVar.GetType().GetName(), TEXT("Particles"));
	}

	// Add user-exposed parameters from this system.
	TArray<FNiagaraVariable> UserVars;
	System->GetExposedParameters().GetParameters(UserVars);
	for (const FNiagaraVariable& V : UserVars)
	{
		AddLinkEntry(V.GetName().ToString(), V.GetType().GetName(), TEXT("User"));
	}

	auto R = MakeShared<FJsonObject>();
	R->SetBoolField(TEXT("success"), true);
	R->SetStringField(TEXT("input_name"), InputName);
	if (TargetType.IsValid())
	{
		R->SetStringField(TEXT("input_type"), TargetType.GetName());
	}
	R->SetNumberField(TEXT("dynamic_input_count"), DynamicInputArr.Num());
	R->SetArrayField(TEXT("dynamic_inputs"), DynamicInputArr);
	R->SetNumberField(TEXT("link_parameter_count"), LinkParamArr.Num());
	R->SetArrayField(TEXT("link_parameters"), LinkParamArr);
	return R;
}


// ---------------------------------------------------------------------------
// FResolveNiagaraBuiltInDynamicInputAction::ExecuteInternal
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FResolveNiagaraBuiltInDynamicInputAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString NameFilter, ExactName;
	Params->TryGetStringField(TEXT("name_filter"), NameFilter);
	Params->TryGetStringField(TEXT("exact_name"), ExactName);
	int32 MaxResults = 20;
	Params->TryGetNumberField(TEXT("max_results"), MaxResults);

	FNiagaraEditorUtilities::FGetFilteredScriptAssetsOptions Opts;
	Opts.ScriptUsageToInclude = ENiagaraScriptUsage::DynamicInput;
	Opts.bIncludeDeprecatedScripts = false;

	TArray<FAssetData> DIAssets;
	FNiagaraEditorUtilities::GetFilteredScriptAssets(Opts, DIAssets);

	TArray<TSharedPtr<FJsonValue>> Matches;
	for (const FAssetData& Asset : DIAssets)
	{
		if (Matches.Num() >= MaxResults)
		{
			break;
		}
		const FString AssetName = Asset.AssetName.ToString();
		if (!ExactName.IsEmpty())
		{
			if (!AssetName.Equals(ExactName, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}
		else if (!NameFilter.IsEmpty())
		{
			if (!AssetName.Contains(NameFilter, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}

		auto Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("name"), AssetName);
		Entry->SetStringField(TEXT("path"), Asset.GetObjectPathString());
		Entry->SetStringField(TEXT("package"), Asset.PackageName.ToString());
		Matches.Add(MakeShared<FJsonValueObject>(Entry));
	}

	auto R = MakeShared<FJsonObject>();
	R->SetBoolField(TEXT("success"), true);
	R->SetNumberField(TEXT("total_scanned"), DIAssets.Num());
	R->SetNumberField(TEXT("match_count"), Matches.Num());
	R->SetArrayField(TEXT("matches"), Matches);
	return R;
}

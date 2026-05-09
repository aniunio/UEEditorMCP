#include "Actions/NiagaraActions/NiagaraCommonUtils.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "EditorAssetLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeOp.h"
#include "NiagaraCommon.h"
#include "NiagaraParameterStore.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraLightRendererProperties.h"
#include "NiagaraComponentRendererProperties.h"
#include "NiagaraMessageDataBase.h"
#include "NiagaraMessageStore.h"
#include "NiagaraScriptVariable.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraSystemInstanceController.h"
#include "EdGraphSchema_Niagara.h"

#include "NiagaraSystemEditorData.h"
#include "NiagaraEditorModule.h"
#include "ViewModels/NiagaraSystemViewModel.h"
#include "ViewModels/NiagaraScratchPadViewModel.h"
#include "ViewModels/NiagaraScratchPadScriptViewModel.h"

#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterfaceColorCurve.h"
#include "NiagaraDataInterfaceCurve.h"
#include "NiagaraDataInterfaceVectorCurve.h"
#include "NiagaraTypeRegistry.h"
#include "NiagaraNode.h"
#include "NiagaraNodeCustomHlsl.h"
#include "NiagaraNodeParameterMapGet.h"
#include "NiagaraNodeParameterMapSet.h"
#include "NiagaraEditorSettings.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphNode.h"

#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/MetaData.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"
#include "Templates/SharedPointer.h"

static bool GetRequiredString(const TSharedPtr<FJsonObject>& Params, const FString& ParamName, FString& OutValue,
                              FString& OutError)
{
	if (!Params.IsValid() || !Params->TryGetStringField(ParamName, OutValue) || OutValue.IsEmpty())
	{
		OutError = FString::Printf(TEXT("Required parameter '%s' is missing or empty"), *ParamName);
		return false;
	}
	return true;
}

static TSharedPtr<FJsonObject> SerializeProperty(const FProperty* Property, const void* Container = nullptr, int32 Depth = 0);
static void RebuildSignatureFromPins(UNiagaraNodeFunctionCall* FunctionCallNode);
static FString PinTypeDescription(const UEdGraphPin* Pin);

// ---------------------------------------------------------------------------
// Asset loading helpers.
// ---------------------------------------------------------------------------
template <typename AssetType>
static AssetType* LoadNiagaraAssetWithFallbacks(
	const FString& AssetPath,
	const TCHAR* AssetLabel,
	FString& OutError)
{
	if (AssetPath.IsEmpty())
	{
		OutError = FString::Printf(TEXT("%s path is empty"), AssetLabel);
		return nullptr;
	}

	TArray<FString> AttemptedPaths;
	auto AddAttempt = [&AttemptedPaths](const FString& Path)
	{
		if (!Path.IsEmpty())
		{
			AttemptedPaths.AddUnique(Path);
		}
	};

	FString PackagePath = AssetPath;
	int32 DotIndex = INDEX_NONE;
	if (PackagePath.FindLastChar(TEXT('.'), DotIndex))
	{
		PackagePath = PackagePath.Left(DotIndex);
	}
	FString ObjectPath = AssetPath;
	if (!AssetPath.Contains(TEXT(".")))
	{
		const FString AssetName = FPaths::GetBaseFilename(AssetPath);
		ObjectPath = FString::Printf(TEXT("%s.%s"), *AssetPath, *AssetName);
	}

	AddAttempt(AssetPath);
	AddAttempt(ObjectPath);
	AddAttempt(PackagePath);

	for (const FString& Candidate : AttemptedPaths)
	{
		if (AssetType* Loaded = LoadObject<AssetType>(nullptr, *Candidate))
		{
			return Loaded;
		}
		if (AssetType* Loaded = Cast<AssetType>(UEditorAssetLibrary::LoadAsset(Candidate)))
		{
			return Loaded;
		}
	}

	IAssetRegistry& Registry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	TArray<FAssetData> Assets;
	Registry.GetAssetsByPackageName(FName(*PackagePath), Assets);

	for (const FAssetData& AssetData : Assets)
	{
		AddAttempt(AssetData.GetObjectPathString());
		if (AssetType* Loaded = Cast<AssetType>(AssetData.GetAsset()))
		{
			return Loaded;
		}
	}

	OutError = FString::Printf(
		TEXT("%s asset not found: '%s'. Tried: %s"),
		AssetLabel,
		*AssetPath,
		*FString::Join(AttemptedPaths, TEXT(", ")));
	return nullptr;
}

UNiagaraSystem* NiagaraCommonUtils::LoadNiagaraSystem(const FString& AssetPath, FString& OutError)
{
	return LoadNiagaraAssetWithFallbacks<UNiagaraSystem>(AssetPath, TEXT("Niagara System"), OutError);
}

// ---------------------------------------------------------------------------
// Emitter handle lookup helpers.
// ---------------------------------------------------------------------------
FNiagaraEmitterHandle* NiagaraCommonUtils::FindEmitterHandle(
	UNiagaraSystem* NiagaraSystem,
	const FString& EmitterName,
	int32& OutIndex,
	FString& OutError)
{
	if (!NiagaraSystem)
	{
		OutError = TEXT("NiagaraSystem is null");
		return nullptr;
	}

	TArray<FNiagaraEmitterHandle>& Handles = NiagaraSystem->GetEmitterHandles();

	// Support "Emitter[N]" index notation
	if (EmitterName.StartsWith(TEXT("Emitter[")) && EmitterName.EndsWith(TEXT("]")))
	{
		FString IndexStr = EmitterName.Mid(8, EmitterName.Len() - 9);
		int32 Idx = FCString::Atoi(*IndexStr);
		if (Idx >= 0 && Idx < Handles.Num())
		{
			OutIndex = Idx;
			return &Handles[Idx];
		}
		OutError = FString::Printf(
			TEXT("Emitter index %d out of range (system has %d emitters)"),
			Idx, Handles.Num());
		return nullptr;
	}

	// Case-insensitive name search
	for (int32 i = 0; i < Handles.Num(); ++i)
	{
		if (Handles[i].GetName().ToString().Equals(EmitterName, ESearchCase::IgnoreCase)
			|| Handles[i].GetUniqueInstanceName().Equals(EmitterName, ESearchCase::IgnoreCase))
		{
			OutIndex = i;
			return &Handles[i];
		}
	}

	OutError = FString::Printf(
		TEXT("Emitter '%s' not found in system (has %d emitters)"),
		*EmitterName, Handles.Num());
	return nullptr;
}

FVersionedNiagaraEmitterData* NiagaraCommonUtils::GetEmitterData(const FNiagaraEmitterHandle* Handle)
{
	if (!Handle)
	{
		return nullptr;
	}
	return Handle->GetEmitterData();
}

// ---------------------------------------------------------------------------
// Script usage conversion helpers.
// ---------------------------------------------------------------------------
ENiagaraScriptUsage NiagaraCommonUtils::ParseScriptUsage(const FString& Usage, bool& bOutSuccess)
{
	bOutSuccess = true;

	if (Usage.Equals(TEXT("emitter_spawn"), ESearchCase::IgnoreCase) || Usage.Equals(TEXT("EmitterSpawn"), ESearchCase::IgnoreCase))
	{
		return ENiagaraScriptUsage::EmitterSpawnScript;
	}
	if (Usage.Equals(TEXT("emitter_update"), ESearchCase::IgnoreCase) || Usage.Equals(TEXT("EmitterUpdate"), ESearchCase::IgnoreCase))
	{
		return ENiagaraScriptUsage::EmitterUpdateScript;
	}
	if (Usage.Equals(TEXT("particle_spawn"), ESearchCase::IgnoreCase) || Usage.Equals(TEXT("ParticleSpawn"), ESearchCase::IgnoreCase))
	{
		return ENiagaraScriptUsage::ParticleSpawnScript;
	}
	if (Usage.Equals(TEXT("particle_update"), ESearchCase::IgnoreCase) || Usage.Equals(TEXT("ParticleUpdate"), ESearchCase::IgnoreCase))
	{
		return ENiagaraScriptUsage::ParticleUpdateScript;
	}
	if (Usage.Equals(TEXT("system_spawn"), ESearchCase::IgnoreCase) || Usage.Equals(TEXT("SystemSpawn"), ESearchCase::IgnoreCase))
	{
		return ENiagaraScriptUsage::SystemSpawnScript;
	}
	if (Usage.Equals(TEXT("system_update"), ESearchCase::IgnoreCase) || Usage.Equals(TEXT("SystemUpdate"), ESearchCase::IgnoreCase))
	{
		return ENiagaraScriptUsage::SystemUpdateScript;
	}
	if (Usage.Equals(TEXT("module"), ESearchCase::IgnoreCase))
	{
		return ENiagaraScriptUsage::Module;
	}
	if (Usage.Equals(TEXT("dynamic_input"), ESearchCase::IgnoreCase) || Usage.Equals(TEXT("DynamicInput"), ESearchCase::IgnoreCase))
	{
		return ENiagaraScriptUsage::DynamicInput;
	}
	if (Usage.Equals(TEXT("function"), ESearchCase::IgnoreCase))
	{
		return ENiagaraScriptUsage::Function;
	}

	bOutSuccess = false;
	return ENiagaraScriptUsage::ParticleUpdateScript;
}

FString NiagaraCommonUtils::ScriptUsageToString(const ENiagaraScriptUsage& Usage)
{
	switch (Usage)
	{
	case ENiagaraScriptUsage::EmitterSpawnScript:
		return TEXT("emitter_spawn");
	case ENiagaraScriptUsage::EmitterUpdateScript:
		return TEXT("emitter_update");
	case ENiagaraScriptUsage::ParticleSpawnScript:
		return TEXT("particle_spawn");
	case ENiagaraScriptUsage::ParticleUpdateScript:
		return TEXT("particle_update");
	case ENiagaraScriptUsage::SystemSpawnScript:
		return TEXT("system_spawn");
	case ENiagaraScriptUsage::SystemUpdateScript:
		return TEXT("system_update");
	case ENiagaraScriptUsage::Module:
		return TEXT("module");
	case ENiagaraScriptUsage::DynamicInput:
		return TEXT("dynamic_input");
	case ENiagaraScriptUsage::Function:
		return TEXT("function");
	default:
		return TEXT("unknown");
	}
}

// ---------------------------------------------------------------------------
// Graph access helpers.
// ---------------------------------------------------------------------------
UNiagaraGraph* NiagaraCommonUtils::GetGraphForUsage(const FVersionedNiagaraEmitterData* EmitterData, const ENiagaraScriptUsage& Usage)
{
	if (!EmitterData)
	{
		return nullptr;
	}

	UNiagaraScript* Script = nullptr;

	switch (Usage)
	{
	case ENiagaraScriptUsage::EmitterSpawnScript:
		Script = EmitterData->EmitterSpawnScriptProps.Script;
		break;
	case ENiagaraScriptUsage::EmitterUpdateScript:
		Script = EmitterData->EmitterUpdateScriptProps.Script;
		break;
	case ENiagaraScriptUsage::ParticleSpawnScript:
		Script = EmitterData->SpawnScriptProps.Script;
		break;
	case ENiagaraScriptUsage::ParticleUpdateScript:
		Script = EmitterData->UpdateScriptProps.Script;
		break;
	default:
		return nullptr;
	}

	if (!Script)
	{
		return nullptr;
	}

	if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource()))
	{
		return Source->NodeGraph;
	}

	return nullptr;
}

UNiagaraNodeOutput* NiagaraCommonUtils::GetOutputNodeForUsage(const UNiagaraGraph* Graph, const ENiagaraScriptUsage& Usage)
{
	if (!Graph)
	{
		return nullptr;
	}

	TArray<UNiagaraNodeOutput*> OutputNodes;
	Graph->GetNodesOfClass<UNiagaraNodeOutput>(OutputNodes);

	for (UNiagaraNodeOutput* OutputNode : OutputNodes)
	{
		if (OutputNode->GetUsage() == Usage)
		{
			return OutputNode;
		}
	}

	return nullptr;
}

UNiagaraNodeFunctionCall* NiagaraCommonUtils::FindModuleNode(const UNiagaraGraph* Graph, const FString& ModuleName, FString& OutError)
{
	if (!Graph)
	{
		OutError = TEXT("Graph is null");
		return nullptr;
	}

	TArray<UNiagaraNodeFunctionCall*> FunctionNodes;
	Graph->GetNodesOfClass<UNiagaraNodeFunctionCall>(FunctionNodes);

	for (UNiagaraNodeFunctionCall* Node : FunctionNodes)
	{
		if (Node->GetFunctionName().Equals(ModuleName, ESearchCase::IgnoreCase)
			|| Node->GetNodeTitle(ENodeTitleType::ListView).ToString().Equals(ModuleName, ESearchCase::IgnoreCase))
		{
			return Node;
		}
	}

	OutError = FString::Printf(TEXT("Module '%s' not found in graph"), *ModuleName);

	return nullptr;
}

UNiagaraNodeFunctionCall* NiagaraCommonUtils::FindModuleNode(
	const UNiagaraGraph* Graph,
	ENiagaraScriptUsage Usage,
	const FString& ModuleName,
	FString& OutError)
{
	(void)Usage;
	return FindModuleNode(Graph, ModuleName, OutError);
}

ANiagaraActor* NiagaraCommonUtils::FindNiagaraActorByName(const FString& Name, FString& OutError)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		OutError = TEXT("No editor world available");
		return nullptr;
	}

	for (TActorIterator<ANiagaraActor> It(World); It; ++It)
	{
		ANiagaraActor* NiagaraActor = *It;
		if (NiagaraActor->GetActorLabel().Equals(Name, ESearchCase::IgnoreCase) ||
			NiagaraActor->GetName().Equals(Name, ESearchCase::IgnoreCase))
		{
			return NiagaraActor;
		}
	}

	OutError = FString::Printf(TEXT("Actor not found: '%s'"), *Name);
	return nullptr;
}

// ---------------------------------------------------------------------------
// Compilation & Editor Sync
// ---------------------------------------------------------------------------
void NiagaraCommonUtils::CompileAndSync(UNiagaraSystem* NiagaraSystem, bool bForce)
{
	if (!NiagaraSystem)
	{
		return;
	}

	if (UNiagaraSystemEditorData* EditorData = Cast<UNiagaraSystemEditorData>(NiagaraSystem->GetEditorData()))
	{
		EditorData->SynchronizeOverviewGraphWithSystem(*NiagaraSystem);
	}

	(void)NiagaraSystem->MarkPackageDirty();
	NiagaraSystem->RequestCompile(bForce);

	if (bForce)
	{
		NiagaraSystem->WaitForCompilationComplete();
	}

	NiagaraSystem->PostEditChange();
}

// ---------------------------------------------------------------------------
// Scratch pad transient-proxy helpers.
// When a system is open, the Niagara editor duplicates each scratch pad script
// into a transient edit copy. Users interact with that edit copy, so writes to
// the asset script alone do not appear live until the system is reopened.
// To keep MCP edits visible immediately, update both the asset script and the
// transient edit copy when the editor is open.
// ---------------------------------------------------------------------------
UNiagaraScript* NiagaraCommonUtils::FindScratchPadScript(UNiagaraSystem* System, const FString& ModuleName)
{
	if (!System)
	{
		return nullptr;
	}
	for (UNiagaraScript* Script : System->ScratchPadScripts)
	{
		if (Script && Script->GetName().Equals(ModuleName, ESearchCase::IgnoreCase))
		{
			return Script;
		}
	}
	return nullptr;
}

bool NiagaraCommonUtils::HasScratchPadScriptNameConflict(UNiagaraSystem* System, const FString& ModuleName)
{
	if (!System)
	{
		return false;
	}

	if (FindObject<UNiagaraScript>(System, *ModuleName))
	{
		return true;
	}

	for (UNiagaraScript* Script : System->ScratchPadScripts)
	{
		if (Script && Script->GetName().Equals(ModuleName, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

static void GetScratchPadScriptPair(
	UNiagaraSystem* System,
	const FString& ModuleName,
	TArray<UNiagaraScript*>& OutScripts)
{
	OutScripts.Reset();

	UNiagaraScript* Original = NiagaraCommonUtils::FindScratchPadScript(System, ModuleName);
	if (!Original)
	{
		return;
	}
	OutScripts.Add(Original);

	FNiagaraEditorModule& NiagaraEditor = FNiagaraEditorModule::Get();
	TSharedPtr<FNiagaraSystemViewModel> SystemVM = NiagaraEditor.GetExistingViewModelForSystem(System);
	if (!SystemVM.IsValid())
	{
		return; // Editor not open - only the asset script needs updating.
	}

	UNiagaraScratchPadViewModel* ScratchVM = SystemVM->GetScriptScratchPadViewModel();
	if (!ScratchVM)
	{
		return;
	}

	// Look up the script VM by the ORIGINAL script pointer first, then fall back
	// to name lookup (matches GetViewModelForScript(FName) overload).
	TSharedPtr<FNiagaraScratchPadScriptViewModel> ScriptVM = ScratchVM->GetViewModelForScript(Original);
	if (!ScriptVM.IsValid())
	{
		ScriptVM = ScratchVM->GetViewModelForScript(FName(*ModuleName));
	}
	if (!ScriptVM.IsValid())
	{
		return;
	}

	const FVersionedNiagaraScript& EditRef = ScriptVM->GetEditScript();
	if (UNiagaraScript* EditCopy = EditRef.Script)
	{
		if (EditCopy != Original)
		{
			OutScripts.Add(EditCopy);
		}
	}
}

void NiagaraCommonUtils::NotifyScratchPadScriptChanged(UNiagaraSystem* System, const FString& ModuleName)
{
	if (!System)
	{
		return;
	}

	FNiagaraEditorModule& NiagaraEditor = FNiagaraEditorModule::Get();
	TSharedPtr<FNiagaraSystemViewModel> SystemVM = NiagaraEditor.GetExistingViewModelForSystem(System);
	if (!SystemVM.IsValid())
	{
		return;
	}

	UNiagaraScratchPadViewModel* ScratchVM = SystemVM->GetScriptScratchPadViewModel();
	if (!ScratchVM)
	{
		return;
	}

	// Refresh the scratch pad panel so name / pin list visuals rebuild.
	ScratchVM->RefreshScriptViewModels();

	// CRITICAL: ApplyScratchPadChanges is what the editor's "Apply Scratch"
	// toolbar button calls. It copies every script's EditScript content back
	// to its OriginalScript (System->ScratchPadScripts[i]) and recompiles
	// every emitter that references the scratch pad script. Without this,
	// new Module.* inputs added via MCP commands stay isolated in the edit
	// copy and the emitter stack never sees them - even after our compile +
	// dirty + Modify chain runs.
	ScratchVM->ApplyScratchPadChanges();

	// Tell the system view model to refresh its stack / viewport so the
	// compiled change is visible without requiring close+reopen.
	SystemVM->RefreshAll();
}

// ---------------------------------------------------------------------------
// JSON serialization helpers.
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> NiagaraCommonUtils::EmitterHandleToJson(
	const FNiagaraEmitterHandle& Handle,
	int32 Index)
{
	auto Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("name"), Handle.GetName().ToString());
	Obj->SetStringField(TEXT("unique_name"), Handle.GetUniqueInstanceName());
	Obj->SetStringField(TEXT("id"), Handle.GetId().ToString());
	Obj->SetNumberField(TEXT("index"), Index);
	Obj->SetBoolField(TEXT("enabled"), Handle.GetIsEnabled());

	FVersionedNiagaraEmitterData* Data = Handle.GetEmitterData();
	if (Data)
	{
		FString SimTarget = (Data->SimTarget == ENiagaraSimTarget::GPUComputeSim)
			                    ? TEXT("gpu")
			                    : TEXT("cpu");
		Obj->SetStringField(TEXT("sim_target"), SimTarget);
		Obj->SetBoolField(TEXT("local_space"), Data->bLocalSpace);
		Obj->SetBoolField(TEXT("determinism"), Data->bDeterminism);
		Obj->SetNumberField(TEXT("renderer_count"), Data->GetRenderers().Num());
	}

	return Obj;
}

TSharedPtr<FJsonObject> NiagaraCommonUtils::ModuleNodeToJson(
	UNiagaraNodeFunctionCall* Node,
	int32 Index,
	ENiagaraScriptUsage Usage,
	bool bIncludeInputs)
{
	auto Obj = MakeShared<FJsonObject>();
	if (!Node)
	{
		return Obj;
	}

	Obj->SetStringField(TEXT("name"), Node->GetFunctionName());
	Obj->SetNumberField(TEXT("index"), Index);
	Obj->SetStringField(TEXT("script_usage"), ScriptUsageToString(Usage));

	FText DisplayName = Node->GetNodeTitle(ENodeTitleType::ListView);
	Obj->SetStringField(TEXT("display_name"), DisplayName.ToString());

	if (Node->FunctionScript)
	{
		Obj->SetStringField(TEXT("script_path"), Node->FunctionScript->GetPathName());
	}

	if (bIncludeInputs)
	{
		TArray<TSharedPtr<FJsonValue>> InputsArray;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction != EGPD_Input)
			{
				continue;
			}

			// Skip parameter-map pins. They are internal wiring.
			if (Pin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryMisc)
			{
				continue;
			}

			auto InputObj = MakeShared<FJsonObject>();
			InputObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
			InputObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
			InputObj->SetStringField(TEXT("default_value"), Pin->DefaultValue);
			InputObj->SetBoolField(TEXT("is_connected"), Pin->LinkedTo.Num() > 0);

			InputsArray.Add(MakeShared<FJsonValueObject>(InputObj));
		}
		Obj->SetArrayField(TEXT("inputs"), InputsArray);
	}

	return Obj;
}

// ---------------------------------------------------------------------------
// Property introspection helpers.
// ---------------------------------------------------------------------------
static constexpr int32 MaxIntrospectionDepth = 6;

// ---------------------------------------------------------------------------
// Local property-introspection helpers.
// ---------------------------------------------------------------------------

static void StampMetadata(const FProperty* Property, FJsonObject& Out)
{
	if (!Property)
	{
		return;
	}
	const FString DisplayName = Property->GetDisplayNameText().ToString();
	if (!DisplayName.IsEmpty() && DisplayName != Property->GetName())
	{
		Out.SetStringField(TEXT("display_name"), DisplayName);
	}
	if (Property->HasMetaData(TEXT("ToolTip")))
	{
		Out.SetStringField(TEXT("tooltip"), Property->GetMetaData(TEXT("ToolTip")));
	}
	if (Property->HasMetaData(TEXT("Category")))
	{
		Out.SetStringField(TEXT("category"), Property->GetMetaData(TEXT("Category")));
	}
	if (Property->HasMetaData(TEXT("ClampMin")))
	{
		Out.SetStringField(TEXT("clamp_min"), Property->GetMetaData(TEXT("ClampMin")));
	}
	if (Property->HasMetaData(TEXT("ClampMax")))
	{
		Out.SetStringField(TEXT("clamp_max"), Property->GetMetaData(TEXT("ClampMax")));
	}
	if (Property->HasMetaData(TEXT("UIMin")))
	{
		Out.SetStringField(TEXT("ui_min"), Property->GetMetaData(TEXT("UIMin")));
	}
	if (Property->HasMetaData(TEXT("UIMax")))
	{
		Out.SetStringField(TEXT("ui_max"), Property->GetMetaData(TEXT("UIMax")));
	}
	if (Property->HasMetaData(TEXT("Units")))
	{
		Out.SetStringField(TEXT("units"), Property->GetMetaData(TEXT("Units")));
	}
}

static TSharedPtr<FJsonValue> SerializePrimitiveValue(const FProperty* Property, const void* ValuePtr)
{
	if (!Property || !ValuePtr)
	{
		return MakeShared<FJsonValueNull>();
	}

	if (const FBoolProperty* P = CastField<FBoolProperty>(Property))
	{
		return MakeShared<FJsonValueBoolean>(P->GetPropertyValue(ValuePtr));
	}
	if (const FByteProperty* P = CastField<FByteProperty>(Property))
	{
		const uint8 Val = P->GetPropertyValue(ValuePtr);
		if (P->Enum)
		{
			return MakeShared<FJsonValueString>(P->Enum->GetNameStringByValue(Val));
		}
		return MakeShared<FJsonValueNumber>(static_cast<double>(Val));
	}
	if (const FInt8Property* P = CastField<FInt8Property>(Property))
	{
		return MakeShared<FJsonValueNumber>(static_cast<double>(P->GetPropertyValue(ValuePtr)));
	}
	if (const FInt16Property* P = CastField<FInt16Property>(Property))
	{
		return MakeShared<FJsonValueNumber>(static_cast<double>(P->GetPropertyValue(ValuePtr)));
	}
	if (const FIntProperty* P = CastField<FIntProperty>(Property))
	{
		return MakeShared<FJsonValueNumber>(static_cast<double>(P->GetPropertyValue(ValuePtr)));
	}
	if (const FInt64Property* P = CastField<FInt64Property>(Property))
	{
		return MakeShared<FJsonValueNumber>(static_cast<double>(P->GetPropertyValue(ValuePtr)));
	}
	if (const FUInt16Property* P = CastField<FUInt16Property>(Property))
	{
		return MakeShared<FJsonValueNumber>(static_cast<double>(P->GetPropertyValue(ValuePtr)));
	}
	if (const FUInt32Property* P = CastField<FUInt32Property>(Property))
	{
		return MakeShared<FJsonValueNumber>(static_cast<double>(P->GetPropertyValue(ValuePtr)));
	}
	if (const FUInt64Property* P = CastField<FUInt64Property>(Property))
	{
		return MakeShared<FJsonValueNumber>(static_cast<double>(P->GetPropertyValue(ValuePtr)));
	}
	if (const FFloatProperty* P = CastField<FFloatProperty>(Property))
	{
		return MakeShared<FJsonValueNumber>(static_cast<double>(P->GetPropertyValue(ValuePtr)));
	}
	if (const FDoubleProperty* P = CastField<FDoubleProperty>(Property))
	{
		return MakeShared<FJsonValueNumber>(P->GetPropertyValue(ValuePtr));
	}
	if (const FStrProperty* P = CastField<FStrProperty>(Property))
	{
		return MakeShared<FJsonValueString>(P->GetPropertyValue(ValuePtr));
	}
	if (const FNameProperty* P = CastField<FNameProperty>(Property))
	{
		return MakeShared<FJsonValueString>(P->GetPropertyValue(ValuePtr).ToString());
	}
	if (const FTextProperty* P = CastField<FTextProperty>(Property))
	{
		return MakeShared<FJsonValueString>(P->GetPropertyValue(ValuePtr).ToString());
	}
	if (const FEnumProperty* P = CastField<FEnumProperty>(Property))
	{
		const UEnum* Enum = P->GetEnum();
		const int64 Val = P->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
		return MakeShared<FJsonValueString>(Enum ? Enum->GetNameStringByValue(Val) : FString::Printf(TEXT("%lld"), Val));
	}
	return MakeShared<FJsonValueNull>();
}

// ---------------------------------------------------------------------------
// Enum serialization helpers.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> NiagaraCommonUtils::SerializeEnum(const UEnum* Enum)
{
	auto Out = MakeShared<FJsonObject>();
	if (!Enum)
	{
		Out->SetStringField(TEXT("kind"), TEXT("invalid_enum"));
		return Out;
	}
	Out->SetStringField(TEXT("kind"), TEXT("enum"));
	Out->SetStringField(TEXT("name"), Enum->GetName());
	Out->SetStringField(TEXT("path"), Enum->GetPathName());

	TArray<TSharedPtr<FJsonValue>> Entries;
	const int32 NumEntries = Enum->NumEnums();
	for (int32 i = 0; i < NumEntries; ++i)
	{
		const FString FullName = Enum->GetNameByIndex(i).ToString();
		const FString ShortName = Enum->GetNameStringByIndex(i);
		if (ShortName.EndsWith(TEXT("_MAX")))
		{
			continue;
		}
		if (Enum->HasMetaData(TEXT("Hidden"), i))
		{
			continue;
		}
		auto Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("name"), ShortName);
		Entry->SetStringField(TEXT("full_name"), FullName);
		Entry->SetStringField(TEXT("display_name"),
		                      Enum->GetDisplayNameTextByIndex(i).ToString());
		Entry->SetNumberField(TEXT("value"), static_cast<double>(Enum->GetValueByIndex(i)));
		if (Enum->HasMetaData(TEXT("ToolTip"), i))
		{
			Entry->SetStringField(TEXT("tooltip"), Enum->GetMetaData(TEXT("ToolTip"), i));
		}
		Entries.Add(MakeShared<FJsonValueObject>(Entry));
	}
	Out->SetArrayField(TEXT("values"), Entries);
	return Out;
}

// ---------------------------------------------------------------------------
// SerializeProperty (recursive workhorse)
// ---------------------------------------------------------------------------

static TSharedPtr<FJsonObject> SerializeProperty(const FProperty* Property, const void* Container, int32 Depth)
{
	auto Out = MakeShared<FJsonObject>();
	if (!Property)
	{
		Out->SetStringField(TEXT("kind"), TEXT("null"));
		return Out;
	}
	if (Depth > MaxIntrospectionDepth)
	{
		Out->SetStringField(TEXT("kind"), TEXT("max_depth"));
		Out->SetStringField(TEXT("name"), Property->GetName());
		return Out;
	}

	Out->SetStringField(TEXT("name"), Property->GetName());
	Out->SetStringField(TEXT("cpp_type"), Property->GetCPPType());
	StampMetadata(Property, *Out);

	const void* ValuePtr = Container ? Property->ContainerPtrToValuePtr<void>(Container) : nullptr;

	// ---- Primitive scalars ----
	if (const FBoolProperty* P = CastField<FBoolProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("bool"));
		if (ValuePtr)
		{
			Out->SetBoolField(TEXT("value"), P->GetPropertyValue(ValuePtr));
		}
		return Out;
	}

	// Byte property: distinguishes plain uint8 from byte-backed enum
	if (const FByteProperty* P = CastField<FByteProperty>(Property))
	{
		if (P->Enum)
		{
			Out->SetStringField(TEXT("kind"), TEXT("enum"));
			Out->SetObjectField(TEXT("enum"), NiagaraCommonUtils::SerializeEnum(P->Enum));
			if (ValuePtr)
			{
				const uint8 Val = P->GetPropertyValue(ValuePtr);
				Out->SetStringField(TEXT("value"), P->Enum->GetNameStringByValue(Val));
				Out->SetStringField(TEXT("value_display"),
				                    P->Enum->GetDisplayNameTextByValue(Val).ToString());
			}
			return Out;
		}
		Out->SetStringField(TEXT("kind"), TEXT("int"));
		Out->SetStringField(TEXT("sub"), TEXT("uint8"));
		if (ValuePtr)
		{
			Out->SetNumberField(TEXT("value"), P->GetPropertyValue(ValuePtr));
		}
		return Out;
	}

	if (const FEnumProperty* P = CastField<FEnumProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("enum"));
		Out->SetObjectField(TEXT("enum"), NiagaraCommonUtils::SerializeEnum(P->GetEnum()));
		if (ValuePtr && P->GetEnum())
		{
			const int64 Val = P->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
			Out->SetStringField(TEXT("value"), P->GetEnum()->GetNameStringByValue(Val));
			Out->SetStringField(TEXT("value_display"),
			                    P->GetEnum()->GetDisplayNameTextByValue(Val).ToString());
		}
		return Out;
	}

	auto IntKind = [&](const TCHAR* Sub, int64 Value)
	{
		Out->SetStringField(TEXT("kind"), TEXT("int"));
		Out->SetStringField(TEXT("sub"), Sub);
		if (ValuePtr)
		{
			Out->SetNumberField(TEXT("value"), static_cast<double>(Value));
		}
	};
	if (const FInt8Property* P = CastField<FInt8Property>(Property))
	{
		IntKind(TEXT("int8"), ValuePtr ? P->GetPropertyValue(ValuePtr) : 0);
		return Out;
	}
	if (const FInt16Property* P = CastField<FInt16Property>(Property))
	{
		IntKind(TEXT("int16"), ValuePtr ? P->GetPropertyValue(ValuePtr) : 0);
		return Out;
	}
	if (const FIntProperty* P = CastField<FIntProperty>(Property))
	{
		IntKind(TEXT("int32"), ValuePtr ? P->GetPropertyValue(ValuePtr) : 0);
		return Out;
	}
	if (const FInt64Property* P = CastField<FInt64Property>(Property))
	{
		IntKind(TEXT("int64"), ValuePtr ? P->GetPropertyValue(ValuePtr) : 0);
		return Out;
	}
	if (const FUInt16Property* P = CastField<FUInt16Property>(Property))
	{
		IntKind(TEXT("uint16"), ValuePtr ? static_cast<int64>(P->GetPropertyValue(ValuePtr)) : 0);
		return Out;
	}
	if (const FUInt32Property* P = CastField<FUInt32Property>(Property))
	{
		IntKind(TEXT("uint32"), ValuePtr ? static_cast<int64>(P->GetPropertyValue(ValuePtr)) : 0);
		return Out;
	}
	if (const FUInt64Property* P = CastField<FUInt64Property>(Property))
	{
		IntKind(TEXT("uint64"), ValuePtr ? static_cast<int64>(P->GetPropertyValue(ValuePtr)) : 0);
		return Out;
	}

	if (const FFloatProperty* P = CastField<FFloatProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("float"));
		if (ValuePtr)
		{
			Out->SetNumberField(TEXT("value"), P->GetPropertyValue(ValuePtr));
		}
		return Out;
	}
	if (const FDoubleProperty* P = CastField<FDoubleProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("double"));
		if (ValuePtr)
		{
			Out->SetNumberField(TEXT("value"), P->GetPropertyValue(ValuePtr));
		}
		return Out;
	}
	if (const FStrProperty* P = CastField<FStrProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("string"));
		if (ValuePtr)
		{
			Out->SetStringField(TEXT("value"), P->GetPropertyValue(ValuePtr));
		}
		return Out;
	}
	if (const FNameProperty* P = CastField<FNameProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("name"));
		if (ValuePtr)
		{
			Out->SetStringField(TEXT("value"), P->GetPropertyValue(ValuePtr).ToString());
		}
		return Out;
	}
	if (const FTextProperty* P = CastField<FTextProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("text"));
		if (ValuePtr)
		{
			Out->SetStringField(TEXT("value"), P->GetPropertyValue(ValuePtr).ToString());
		}
		return Out;
	}

	// ---- Struct ----
	if (const FStructProperty* P = CastField<FStructProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("struct"));
		if (P->Struct)
		{
			Out->SetStringField(TEXT("struct"), P->Struct->GetName());
			Out->SetStringField(TEXT("struct_path"), P->Struct->GetPathName());
			Out->SetObjectField(TEXT("schema"),
			                    NiagaraCommonUtils::SerializeStructFields(P->Struct, ValuePtr, FString(), true, Depth + 1));
		}
		return Out;
	}

	// ---- Array ----
	if (const FArrayProperty* P = CastField<FArrayProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("array"));
		Out->SetObjectField(TEXT("inner"), SerializeProperty(P->Inner, nullptr, Depth + 1));
		if (ValuePtr)
		{
			FScriptArrayHelper Helper(P, ValuePtr);
			const int32 Num = Helper.Num();
			Out->SetNumberField(TEXT("count"), Num);

			TArray<TSharedPtr<FJsonValue>> Items;
			const int32 MaxItems = FMath::Min(Num, 64); // cap to keep responses small
			for (int32 i = 0; i < MaxItems; ++i)
			{
				Items.Add(SerializePrimitiveValue(P->Inner, Helper.GetRawPtr(i)));
			}
			Out->SetArrayField(TEXT("items"), Items);
			if (Num > MaxItems)
			{
				Out->SetBoolField(TEXT("truncated"), true);
			}
		}
		return Out;
	}

	// ---- Map ----
	if (const FMapProperty* P = CastField<FMapProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("map"));
		Out->SetObjectField(TEXT("key"), SerializeProperty(P->KeyProp, nullptr, Depth + 1));
		Out->SetObjectField(TEXT("value"), SerializeProperty(P->ValueProp, nullptr, Depth + 1));
		if (ValuePtr)
		{
			FScriptMapHelper Helper(P, ValuePtr);
			Out->SetNumberField(TEXT("count"), Helper.Num());
		}
		return Out;
	}

	// ---- Set ----
	if (const FSetProperty* P = CastField<FSetProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("set"));
		Out->SetObjectField(TEXT("element"), SerializeProperty(P->ElementProp, nullptr, Depth + 1));
		if (ValuePtr)
		{
			FScriptSetHelper Helper(P, ValuePtr);
			Out->SetNumberField(TEXT("count"), Helper.Num());
		}
		return Out;
	}

	// ---- Object refs ----
	if (const FObjectProperty* P = CastField<FObjectProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("object"));
		if (P->PropertyClass)
		{
			Out->SetStringField(TEXT("class"), P->PropertyClass->GetName());
			Out->SetStringField(TEXT("class_path"), P->PropertyClass->GetPathName());
		}
		if (ValuePtr)
		{
			UObject* Obj = P->GetObjectPropertyValue(ValuePtr);
			Out->SetStringField(TEXT("value"), Obj ? Obj->GetPathName() : FString(TEXT("None")));
		}
		return Out;
	}
	if (const FClassProperty* P = CastField<FClassProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("class"));
		if (P->MetaClass)
		{
			Out->SetStringField(TEXT("base_class"), P->MetaClass->GetName());
			Out->SetStringField(TEXT("base_class_path"), P->MetaClass->GetPathName());
		}
		if (ValuePtr)
		{
			UObject* Obj = P->GetObjectPropertyValue(ValuePtr);
			Out->SetStringField(TEXT("value"), Obj ? Obj->GetPathName() : FString(TEXT("None")));
		}
		return Out;
	}
	if (const FSoftObjectProperty* P = CastField<FSoftObjectProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("soft_object"));
		if (P->PropertyClass)
		{
			Out->SetStringField(TEXT("class"), P->PropertyClass->GetName());
		}
		if (ValuePtr)
		{
			Out->SetStringField(TEXT("value"), P->GetPropertyValue(ValuePtr).ToString());
		}
		return Out;
	}
	if (const FSoftClassProperty* P = CastField<FSoftClassProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("soft_class"));
		if (P->MetaClass)
		{
			Out->SetStringField(TEXT("base_class"), P->MetaClass->GetName());
		}
		if (ValuePtr)
		{
			Out->SetStringField(TEXT("value"), P->GetPropertyValue(ValuePtr).ToString());
		}
		return Out;
	}
	if (const FInterfaceProperty* P = CastField<FInterfaceProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("interface"));
		if (P->InterfaceClass)
		{
			Out->SetStringField(TEXT("interface"), P->InterfaceClass->GetName());
		}
		return Out;
	}
	if (CastField<FDelegateProperty>(Property) || CastField<FMulticastDelegateProperty>(Property))
	{
		Out->SetStringField(TEXT("kind"), TEXT("delegate"));
		return Out;
	}

	Out->SetStringField(TEXT("kind"), TEXT("unknown"));
	return Out;
}

// ---------------------------------------------------------------------------
// Struct-field serialization helpers.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> NiagaraCommonUtils::SerializeStructFields(
	const UStruct* Struct,
	const void* Container,
	const FString& Filter,
	bool bIncludeInherited,
	int32 Depth)
{
	auto Out = MakeShared<FJsonObject>();
	if (!Struct)
	{
		Out->SetStringField(TEXT("error"), TEXT("null struct"));
		return Out;
	}
	Out->SetStringField(TEXT("name"), Struct->GetName());
	Out->SetStringField(TEXT("path"), Struct->GetPathName());

	TArray<TSharedPtr<FJsonValue>> Fields;
	for (TFieldIterator<FProperty> PropIt(
		     Struct,
		     bIncludeInherited ? EFieldIterationFlags::IncludeSuper : EFieldIterationFlags::None);
	     PropIt; ++PropIt)
	{
		FProperty* Prop = *PropIt;
		if (!Prop)
		{
			continue;
		}

		// Only include user-editable and blueprint-visible properties. Skip
		// transient, internal, and hidden state.
		const bool bIsEditable = Prop->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible);
		if (!bIsEditable)
		{
			continue;
		}
		if (Prop->HasAnyPropertyFlags(CPF_Transient | CPF_DisableEditOnInstance))
		{
			continue;
		}

		if (!Filter.IsEmpty() && !Prop->GetName().Contains(Filter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		Fields.Add(MakeShared<FJsonValueObject>(SerializeProperty(Prop, Container, Depth + 1)));
	}
	Out->SetArrayField(TEXT("fields"), Fields);
	Out->SetNumberField(TEXT("field_count"), Fields.Num());
	return Out;
}

// ---------------------------------------------------------------------------
// Data-interface schema serialization helpers.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> NiagaraCommonUtils::SerializeDataInterfaceClass(UClass* DIClass)
{
	auto Out = MakeShared<FJsonObject>();
	if (!DIClass)
	{
		Out->SetStringField(TEXT("error"), TEXT("null class"));
		return Out;
	}
	if (!DIClass->IsChildOf(UNiagaraDataInterface::StaticClass()))
	{
		Out->SetStringField(TEXT("error"), TEXT("class is not a UNiagaraDataInterface subclass"));
		Out->SetStringField(TEXT("class"), DIClass->GetName());
		return Out;
	}

	Out->SetStringField(TEXT("kind"), TEXT("data_interface"));
	Out->SetStringField(TEXT("class"), DIClass->GetName());
	Out->SetStringField(TEXT("class_path"), DIClass->GetPathName());
	Out->SetStringField(TEXT("display_name"), DIClass->GetDisplayNameText().ToString());
	if (DIClass->HasMetaData(TEXT("ToolTip")))
	{
		Out->SetStringField(TEXT("tooltip"), DIClass->GetMetaData(TEXT("ToolTip")));
	}
	if (DIClass->HasMetaData(TEXT("Category")))
	{
		Out->SetStringField(TEXT("category"), DIClass->GetMetaData(TEXT("Category")));
	}

	UObject* CDO = DIClass->GetDefaultObject();
	Out->SetObjectField(TEXT("schema"),
	                    SerializeStructFields(DIClass, CDO, FString(), true, 0));
	return Out;
}

// ---------------------------------------------------------------------------
// Niagara-type serialization helpers.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> NiagaraCommonUtils::SerializeNiagaraType(const FNiagaraTypeDefinition& TypeDef)
{
	auto Out = MakeShared<FJsonObject>();
	if (!TypeDef.IsValid())
	{
		Out->SetStringField(TEXT("kind"), TEXT("invalid"));
		return Out;
	}
	Out->SetStringField(TEXT("name"), TypeDef.GetName());
	Out->SetNumberField(TEXT("size_bytes"), TypeDef.GetSize());

	if (TypeDef.IsDataInterface())
	{
		Out->SetStringField(TEXT("kind"), TEXT("data_interface"));
		Out->SetObjectField(TEXT("schema"), SerializeDataInterfaceClass(TypeDef.GetClass()));
		return Out;
	}
	if (TypeDef.IsEnum())
	{
		Out->SetStringField(TEXT("kind"), TEXT("enum"));
		Out->SetObjectField(TEXT("enum"), SerializeEnum(TypeDef.GetEnum()));
		return Out;
	}
	if (UScriptStruct* Struct = TypeDef.GetScriptStruct())
	{
		Out->SetStringField(TEXT("kind"), TEXT("struct"));
		Out->SetObjectField(TEXT("schema"),
		                    SerializeStructFields(Struct, nullptr, FString(), true, 0));
		return Out;
	}
	Out->SetStringField(TEXT("kind"), TEXT("primitive"));
	return Out;
}

// ---------------------------------------------------------------------------
// Niagara-type name resolution helpers.
// ---------------------------------------------------------------------------

bool NiagaraCommonUtils::ResolveTypeName(const FString& TypeName, FNiagaraTypeDefinition& OutType)
{
	// Built-ins via shared helper first
	if (ParseTypeDef(TypeName, OutType))
	{
		return true;
	}

	// Lookup as enum
	if (UEnum* Enum = FindObject<UEnum>(nullptr, *TypeName))
	{
		OutType = FNiagaraTypeDefinition(Enum);
		return true;
	}
	// Lookup as struct
	if (UScriptStruct* Struct = FindObject<UScriptStruct>(nullptr, *TypeName))
	{
		OutType = FNiagaraTypeDefinition(Struct);
		return true;
	}
	// Lookup as a class (data interface). Normalize the name first because
	// UClass::GetName() returns UObject short names WITHOUT the leading "U"
	FString Normalized = TypeName;
	if (Normalized.StartsWith(TEXT("U"), ESearchCase::CaseSensitive) &&
		Normalized.Len() > 1 &&
		FChar::IsUpper(Normalized[1]))
	{
		Normalized.RightChopInline(1);
	}

	if (UClass* Cls = FindObject<UClass>(nullptr, *TypeName))
	{
		if (Cls->IsChildOf(UNiagaraDataInterface::StaticClass()))
		{
			OutType = FNiagaraTypeDefinition(Cls);
			return true;
		}
	}
	if (UClass* Cls = FindObject<UClass>(nullptr, *Normalized))
	{
		if (Cls->IsChildOf(UNiagaraDataInterface::StaticClass()))
		{
			OutType = FNiagaraTypeDefinition(Cls);
			return true;
		}
	}
	const FString WithPrefix = FString::Printf(TEXT("NiagaraDataInterface%s"), *Normalized);
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (!It->IsChildOf(UNiagaraDataInterface::StaticClass()))
		{
			continue;
		}
		const FString ClsName = It->GetName();
		if (ClsName.Equals(Normalized, ESearchCase::IgnoreCase) ||
			ClsName.Equals(WithPrefix, ESearchCase::IgnoreCase))
		{
			OutType = FNiagaraTypeDefinition(*It);
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// JSON-to-pin default conversion helpers.
// ---------------------------------------------------------------------------

bool NiagaraCommonUtils::JsonValueToPinDefaultString(
	const TSharedPtr<FJsonValue>& JsonValue,
	const FNiagaraTypeDefinition& TypeDef,
	FString& OutDefaultValue,
	FString& OutError)
{
	if (!JsonValue.IsValid())
	{
		OutError = TEXT("Null JSON value");
		return false;
	}

	const EJson Type = JsonValue->Type;

	if (Type == EJson::Number)
	{
		OutDefaultValue = FString::SanitizeFloat(JsonValue->AsNumber());
		return true;
	}
	if (Type == EJson::Boolean)
	{
		OutDefaultValue = JsonValue->AsBool() ? TEXT("true") : TEXT("false");
		return true;
	}
	if (Type == EJson::String)
	{
		OutDefaultValue = JsonValue->AsString();
		return true;
	}
	if (Type == EJson::Object)
	{
		const TSharedPtr<FJsonObject> Obj = JsonValue->AsObject();
		if (Obj->HasField(TEXT("x")))
		{
			double X = 0, Y = 0, Z = 0, W = 0;
			Obj->TryGetNumberField(TEXT("x"), X);
			Obj->TryGetNumberField(TEXT("y"), Y);
			Obj->TryGetNumberField(TEXT("z"), Z);
			if (Obj->HasField(TEXT("w")))
			{
				Obj->TryGetNumberField(TEXT("w"), W);
				OutDefaultValue = FString::Printf(TEXT("%f,%f,%f,%f"), X, Y, Z, W);
			}
			else
			{
				OutDefaultValue = FString::Printf(TEXT("%f,%f,%f"), X, Y, Z);
			}
			return true;
		}
		if (Obj->HasField(TEXT("r")))
		{
			double R = 0, G = 0, B = 0, A = 1;
			Obj->TryGetNumberField(TEXT("r"), R);
			Obj->TryGetNumberField(TEXT("g"), G);
			Obj->TryGetNumberField(TEXT("b"), B);
			Obj->TryGetNumberField(TEXT("a"), A);
			OutDefaultValue = FString::Printf(TEXT("%f,%f,%f,%f"), R, G, B, A);
			return true;
		}
		OutError = TEXT("Object must have {x,y,z[,w]} or {r,g,b[,a]} keys");
		return false;
	}
	if (Type == EJson::Array)
	{
		const TArray<TSharedPtr<FJsonValue>>& Arr = JsonValue->AsArray();
		TArray<FString> Parts;
		for (const TSharedPtr<FJsonValue>& V : Arr)
		{
			Parts.Add(FString::SanitizeFloat(V->AsNumber()));
		}
		OutDefaultValue = FString::Join(Parts, TEXT(","));
		return true;
	}

	OutError = TEXT("Unsupported JSON value kind");
	return false;
}

// ---------------------------------------------------------------------------
// Niagara-type helpers.
// ---------------------------------------------------------------------------
bool NiagaraCommonUtils::ParseTypeDef(const FString& TypeString, FNiagaraTypeDefinition& OutType)
{
	const FString Lower = TypeString.ToLower().Replace(TEXT(" "), TEXT(""));

	// Built-in fast paths
	if (Lower == TEXT("float") || Lower == TEXT("scalar"))
	{
		OutType = FNiagaraTypeDefinition::GetFloatDef();
		return true;
	}
	if (Lower == TEXT("int") || Lower == TEXT("int32") || Lower == TEXT("integer"))
	{
		OutType = FNiagaraTypeDefinition::GetIntDef();
		return true;
	}
	if (Lower == TEXT("bool") || Lower == TEXT("boolean"))
	{
		OutType = FNiagaraTypeDefinition::GetBoolDef();
		return true;
	}
	if (Lower == TEXT("vec2") || Lower == TEXT("vector2") || Lower == TEXT("vector2d"))
	{
		OutType = FNiagaraTypeDefinition::GetVec2Def();
		return true;
	}
	if (Lower == TEXT("vec3") || Lower == TEXT("vector") || Lower == TEXT("vector3"))
	{
		OutType = FNiagaraTypeDefinition::GetVec3Def();
		return true;
	}
	if (Lower == TEXT("vec4") || Lower == TEXT("vector4"))
	{
		OutType = FNiagaraTypeDefinition::GetVec4Def();
		return true;
	}
	if (Lower == TEXT("color") || Lower == TEXT("linearcolor"))
	{
		OutType = FNiagaraTypeDefinition::GetColorDef();
		return true;
	}
	if (Lower == TEXT("quat") || Lower == TEXT("quaternion"))
	{
		OutType = FNiagaraTypeDefinition::GetQuatDef();
		return true;
	}
	if (Lower == TEXT("matrix") || Lower == TEXT("matrix4") || Lower == TEXT("mat4"))
	{
		OutType = FNiagaraTypeDefinition::GetMatrix4Def();
		return true;
	}
	if (Lower == TEXT("position"))
	{
		OutType = FNiagaraTypeDefinition::GetPositionDef();
		return true;
	}
	if (Lower == TEXT("parametermap") || Lower == TEXT("paramap") || Lower == TEXT("map"))
	{
		OutType = FNiagaraTypeDefinition::GetParameterMapDef();
		return true;
	}
	if (Lower == TEXT("id") || Lower == TEXT("niagaraid"))
	{
		OutType = FNiagaraTypeDefinition::GetIDDef();
		return true;
	}

	// Fallback: lookup in the registered type registry by name
	TOptional<FNiagaraTypeDefinition> Found = FNiagaraTypeRegistry::GetRegisteredTypeByName(FName(*TypeString));
	if (Found.IsSet() && Found.GetValue().IsValid())
	{
		OutType = Found.GetValue();
		return true;
	}

	return false;
}

FString NiagaraCommonUtils::TypeDefToString(const FNiagaraTypeDefinition& TypeDef)
{
	if (!TypeDef.IsValid())
	{
		return TEXT("Invalid");
	}
	return TypeDef.GetName();
}

// ---------------------------------------------------------------------------
// Dynamic-pin helpers.
// ---------------------------------------------------------------------------

// Mirror UNiagaraNodeWithDynamicPins::AddPinSubCategory. The engine helper is
// not exported from NiagaraEditor.
// See Engine/Plugins/FX/Niagara/Source/NiagaraEditor/Private/NiagaraNodeWithDynamicPins.cpp
static const FName NiagaraAddPinSubCategory(TEXT("DynamicAddPin"));

bool NiagaraCommonUtils::IsAddPin(const UEdGraphPin* Pin)
{
	return Pin &&
		Pin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryMisc &&
		Pin->PinType.PinSubCategory == NiagaraAddPinSubCategory;
}

// Local alias for in-file readability
static bool IsNiagaraAddPin(const UEdGraphPin* Pin) { return NiagaraCommonUtils::IsAddPin(Pin); }

static UEdGraphPin* FindAddPin(UNiagaraNode* Node, EEdGraphPinDirection Direction)
{
	if (!Node)
	{
		return nullptr;
	}
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->Direction == Direction && IsNiagaraAddPin(Pin))
		{
			return Pin;
		}
	}
	return nullptr;
}

static UEdGraphPin* CreateReplacementAddPin(UNiagaraNode* Node, EEdGraphPinDirection Direction)
{
	if (!Node)
	{
		return nullptr;
	}
	const FEdGraphPinType AddPinType(
		UEdGraphSchema_Niagara::PinCategoryMisc,
		NiagaraAddPinSubCategory,
		nullptr,
		EPinContainerType::None,
		false,
		FEdGraphTerminalType());
	return Node->CreatePin(Direction, AddPinType, TEXT("Add"));
}

UEdGraphPin* NiagaraCommonUtils::AddTypedPin(
	UNiagaraNode* Node,
	EEdGraphPinDirection Direction,
	const FNiagaraTypeDefinition& Type,
	const FName& InName,
	bool bRebuildSignature)
{
	if (!Node || !Type.IsValid())
	{
		return nullptr;
	}

	Node->Modify();
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
	if (!Schema)
	{
		return nullptr;
	}

	// Look for an existing "Add" pin to convert. This matches the engine's
	// RequestNewTypedPin behavior, which reuses the Add pin and spawns a
	// replacement pin.
	UEdGraphPin* Target = FindAddPin(Node, Direction);
	if (Target)
	{
		Target->Modify();
		Target->PinType = Schema->TypeDefinitionToPinType(Type);
		Target->PinName = InName;
		// Restore the Add pin so the UI keeps a "+" affordance.
		CreateReplacementAddPin(Node, Direction);
	}
	else
	{
		// Node has no Add pin yet (e.g. freshly created scratch pad custom hlsl
		// node we just constructed). Create the typed pin directly.
		const FEdGraphPinType PinType = Schema->TypeDefinitionToPinType(Type);
		Target = Node->CreatePin(Direction, PinType, InName);
	}

	if (bRebuildSignature)
	{
		if (UNiagaraNodeFunctionCall* FuncCall = Cast<UNiagaraNodeFunctionCall>(Node))
		{
			RebuildSignatureFromPins(FuncCall);
		}
	}

	Node->MarkNodeRequiresSynchronization(__FUNCTION__, true);
	if (UEdGraph* Graph = Node->GetGraph())
	{
		Graph->NotifyGraphChanged();
	}
	return Target;
}

static void RebuildSignatureFromPins(UNiagaraNodeFunctionCall* FunctionCallNode)
{
	if (!FunctionCallNode)
	{
		return;
	}
	FunctionCallNode->Modify();

	const UEdGraphSchema_Niagara* Schema = Cast<UEdGraphSchema_Niagara>(FunctionCallNode->GetSchema());
	if (!Schema)
	{
		Schema = GetDefault<UEdGraphSchema_Niagara>();
	}
	if (!Schema)
	{
		return;
	}

	FNiagaraFunctionSignature Sig = FunctionCallNode->Signature;
	Sig.Inputs.Empty();
	Sig.Outputs.Empty();

	for (UEdGraphPin* Pin : FunctionCallNode->Pins)
	{
		if (!Pin || IsNiagaraAddPin(Pin))
		{
			continue;
		}
		if (Pin->Direction == EGPD_Input)
		{
			Sig.Inputs.Add(Schema->PinToNiagaraVariable(Pin, true));
		}
		else
		{
			Sig.Outputs.Add(Schema->PinToNiagaraVariable(Pin, false));
		}
	}

	FunctionCallNode->Signature = Sig;
}

bool NiagaraCommonUtils::RenameDynamicPin(
	UNiagaraNode* Node,
	const FName& OldName,
	const FName& NewName,
	FString& OutError)
{
	if (!Node)
	{
		OutError = TEXT("Null node");
		return false;
	}
	if (OldName == NewName)
	{
		return true;
	}

	UEdGraphPin* Target = nullptr;
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->PinName == OldName && !IsNiagaraAddPin(Pin))
		{
			Target = Pin;
			break;
		}
	}
	if (!Target)
	{
		OutError = FString::Printf(TEXT("Pin '%s' not found on node"), *OldName.ToString());
		return false;
	}

	// Check for name collisions with other pins (excluding Add pins)
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin != Target && Pin && Pin->PinName == NewName && !IsNiagaraAddPin(Pin))
		{
			OutError = FString::Printf(TEXT("Pin name '%s' already exists"), *NewName.ToString());
			return false;
		}
	}

	Node->Modify();
	Target->Modify();
	const FString OldNameStr = OldName.ToString();
	Target->PinName = NewName;

	// CRITICAL ORDERING: rebuild the function signature from the new pin list
	// BEFORE touching CustomHlsl. PostEditChangeProperty on the CustomHlsl field
	// triggers RefreshFromExternalChanges, which re-reads pin state from the
	// signature - if the signature still has the old pin name, the refresh
	// rolls the rename back silently. Syncing the signature first avoids that.
	if (UNiagaraNodeFunctionCall* FuncCall = Cast<UNiagaraNodeFunctionCall>(Node))
	{
		RebuildSignatureFromPins(FuncCall);
	}

	// For custom HLSL, rewrite references to the old pin name inside the HLSL source
	// so the shader code keeps compiling. The Niagara translator accepts either
	// bare identifiers or {BracedForm} for pin references; we substitute the
	// braced form via whole-word replacement using direct string reflection
	// (GetCustomHlsl is not NIAGARAEDITOR_API exported).
	if (Node->IsA<UNiagaraNodeCustomHlsl>())
	{
		if (FProperty* HlslProp = UNiagaraNodeCustomHlsl::StaticClass()->FindPropertyByName(TEXT("CustomHlsl")))
		{
			if (FStrProperty* StrProp = CastField<FStrProperty>(HlslProp))
			{
				FString CurrentHlsl = StrProp->GetPropertyValue_InContainer(Node);
				if (!CurrentHlsl.IsEmpty())
				{
					const FString OldBraced = FString::Printf(TEXT("{%s}"), *OldNameStr);
					const FString NewBraced = FString::Printf(TEXT("{%s}"), *NewName.ToString());
					CurrentHlsl.ReplaceInline(*OldBraced, *NewBraced, ESearchCase::CaseSensitive);
					SetCustomHlslViaReflection(Node, CurrentHlsl);
				}
			}
		}
	}

	Node->MarkNodeRequiresSynchronization(__FUNCTION__, true);
	if (UEdGraph* Graph = Node->GetGraph())
	{
		Graph->NotifyGraphChanged();
	}
	return true;
}

bool NiagaraCommonUtils::RemoveDynamicPinByName(
	UNiagaraNode* Node,
	const FName& PinName,
	FString& OutError)
{
	if (!Node)
	{
		OutError = TEXT("Null node");
		return false;
	}

	UEdGraphPin* Target = nullptr;
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->PinName == PinName && !IsNiagaraAddPin(Pin))
		{
			Target = Pin;
			break;
		}
	}
	if (!Target)
	{
		OutError = FString::Printf(TEXT("Pin '%s' not found on node"), *PinName.ToString());
		return false;
	}

	Node->Modify();
	Target->Modify();
	Target->BreakAllPinLinks();
	Node->RemovePin(Target);

	if (UNiagaraNodeFunctionCall* FuncCall = Cast<UNiagaraNodeFunctionCall>(Node))
	{
		RebuildSignatureFromPins(FuncCall);
	}

	Node->MarkNodeRequiresSynchronization(__FUNCTION__, true);
	if (UEdGraph* Graph = Node->GetGraph())
	{
		Graph->NotifyGraphChanged();
	}
	return true;
}

bool NiagaraCommonUtils::SetCustomHlslViaReflection(UNiagaraNode* HlslNode, const FString& HlslCode)
{
	if (!HlslNode || !HlslNode->IsA<UNiagaraNodeCustomHlsl>())
	{
		return false;
	}

	FProperty* HlslProp = UNiagaraNodeCustomHlsl::StaticClass()->FindPropertyByName(TEXT("CustomHlsl"));
	FStrProperty* StrProp = CastField<FStrProperty>(HlslProp);
	if (!StrProp)
	{
		return false;
	}

	HlslNode->Modify();
	StrProp->SetPropertyValue_InContainer(HlslNode, HlslCode);

	// Mimic PostEditChangeProperty on the CustomHlsl field. The override in
	// UNiagaraNodeCustomHlsl calls RefreshFromExternalChanges + NotifyGraphNeedsRecompile.
	FPropertyChangedEvent ChangedEvent(HlslProp, EPropertyChangeType::ValueSet);
	HlslNode->PostEditChangeProperty(ChangedEvent);

	HlslNode->MarkNodeRequiresSynchronization(__FUNCTION__, true);
	return true;
}

// ---------------------------------------------------------------------------
// Scratch-pad helpers.
// ---------------------------------------------------------------------------
ENiagaraScriptUsage NiagaraCommonUtils::ParseScratchPadModuleType(const FString& ModuleType)
{
	const FString Lower = ModuleType.ToLower();
	if (Lower == TEXT("dynamic_input") || Lower == TEXT("dynamicinput"))
	{
		return ENiagaraScriptUsage::DynamicInput;
	}
	if (Lower == TEXT("function"))
	{
		return ENiagaraScriptUsage::Function;
	}
	return ENiagaraScriptUsage::Module;
}

UNiagaraScript* NiagaraCommonUtils::LoadDefaultScratchPadTemplateScript(ENiagaraScriptUsage Usage)
{
	const UNiagaraEditorSettings* Settings = GetDefault<UNiagaraEditorSettings>();
	if (!Settings)
	{
		return nullptr;
	}

	FSoftObjectPath TemplatePath;
	switch (Usage)
	{
	case ENiagaraScriptUsage::DynamicInput:
		TemplatePath = Settings->DefaultDynamicInputScript;
		break;
	case ENiagaraScriptUsage::Function:
		TemplatePath = Settings->DefaultFunctionScript;
		break;
	case ENiagaraScriptUsage::Module:
	default:
		TemplatePath = Settings->DefaultModuleScript;
		break;
	}

	if (!TemplatePath.IsValid())
	{
		return nullptr;
	}
	return Cast<UNiagaraScript>(TemplatePath.TryLoad());
}

void NiagaraCommonUtils::BuildMinimalScratchGraph(UNiagaraScript* Script, UNiagaraGraph* Graph, ENiagaraScriptUsage Usage)
{
	if (!Script || !Graph)
	{
		return;
	}

	FGraphNodeCreator<UNiagaraNodeOutput> OutputCreator(*Graph);
	UNiagaraNodeOutput* OutputNode = OutputCreator.CreateNode(false);
	OutputNode->SetUsage(Usage);
	OutputNode->NodePosX = 800;
	OutputNode->NodePosY = 0;
	FNiagaraVariable ParamMapOutput(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("Out"));
	OutputNode->Outputs.Add(ParamMapOutput);
	OutputCreator.Finalize();

	FGraphNodeCreator<UNiagaraNodeParameterMapSet> SetCreator(*Graph);
	UNiagaraNodeParameterMapSet* SetNode = SetCreator.CreateNode(false);
	SetNode->NodePosX = 500;
	SetNode->NodePosY = 0;
	SetCreator.Finalize();

	FGraphNodeCreator<UNiagaraNodeParameterMapGet> GetCreator(*Graph);
	UNiagaraNodeParameterMapGet* GetNode = GetCreator.CreateNode(false);
	GetNode->NodePosX = 200;
	GetNode->NodePosY = 0;
	GetCreator.Finalize();

	UEdGraphPin* GetMapOut = nullptr;
	for (UEdGraphPin* Pin : GetNode->Pins)
	{
		if (Pin && Pin->Direction == EGPD_Output && !IsAddPin(Pin))
		{
			GetMapOut = Pin;
			break;
		}
	}

	UEdGraphPin* SetMapIn = nullptr;
	UEdGraphPin* SetMapOut = nullptr;
	for (UEdGraphPin* Pin : SetNode->Pins)
	{
		if (!Pin || IsAddPin(Pin))
		{
			continue;
		}
		if (Pin->Direction == EGPD_Input && !SetMapIn)
		{
			SetMapIn = Pin;
		}
		else if (Pin->Direction == EGPD_Output && !SetMapOut)
		{
			SetMapOut = Pin;
		}
	}

	UEdGraphPin* OutputIn = OutputNode->Pins.Num() > 0 ? OutputNode->Pins[0] : nullptr;
	if (GetMapOut && SetMapIn)
	{
		GetMapOut->MakeLinkTo(SetMapIn);
	}
	if (SetMapOut && OutputIn)
	{
		SetMapOut->MakeLinkTo(OutputIn);
	}
}


static UNiagaraNodeCustomHlsl* FindOrCreateCustomHlslNode(UNiagaraGraph* Graph)
{
	if (!Graph)
	{
		return nullptr;
	}

	TArray<UNiagaraNodeCustomHlsl*> Existing;
	Graph->GetNodesOfClass<UNiagaraNodeCustomHlsl>(Existing);
	if (Existing.Num() > 0)
	{
		return Existing[0];
	}

	FGraphNodeCreator<UNiagaraNodeCustomHlsl> Creator(*Graph);
	UNiagaraNodeCustomHlsl* HlslNode = Creator.CreateNode(false);
	HlslNode->NodePosX = 300;
	HlslNode->NodePosY = 200;
	Creator.Finalize();
	return HlslNode;
}

static void CollectScratchPadGraphs(
	UNiagaraSystem* System,
	const FString& ModuleName,
	TArray<UNiagaraGraph*>& OutGraphs)
{
	OutGraphs.Reset();
	TArray<UNiagaraScript*> Scripts;
	GetScratchPadScriptPair(System, ModuleName, Scripts);
	for (UNiagaraScript* Script : Scripts)
	{
		if (!Script)
		{
			continue;
		}

		UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
		if (Source && Source->NodeGraph)
		{
			OutGraphs.Add(Source->NodeGraph);
		}
	}
}

void NiagaraCommonUtils::CollectScratchPadHlslNodes(
	UNiagaraSystem* System,
	const FString& ModuleName,
	TArray<UNiagaraNodeCustomHlsl*>& OutNodes)
{
	OutNodes.Reset();
	TArray<UNiagaraGraph*> Graphs;
	CollectScratchPadGraphs(System, ModuleName, Graphs);
	for (UNiagaraGraph* Graph : Graphs)
	{
		if (UNiagaraNodeCustomHlsl* Node = FindOrCreateCustomHlslNode(Graph))
		{
			OutNodes.Add(Node);
		}
	}
}

TSharedPtr<FJsonObject> NiagaraCommonUtils::ScratchPadScriptToJson(UNiagaraScript* Script, int32 Index)
{
	auto Json = MakeShared<FJsonObject>();
	Json->SetNumberField(TEXT("index"), Index);
	if (!Script)
	{
		Json->SetBoolField(TEXT("valid"), false);
		return Json;
	}

	Json->SetBoolField(TEXT("valid"), true);
	Json->SetStringField(TEXT("name"), Script->GetName());
	Json->SetStringField(TEXT("path"), Script->GetPathName());
	Json->SetStringField(TEXT("usage"), ScriptUsageToString(Script->GetUsage()));

	if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource()))
	{
		if (UNiagaraGraph* Graph = Source->NodeGraph)
		{
			Json->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());

			TArray<UNiagaraNodeCustomHlsl*> HlslNodes;
			Graph->GetNodesOfClass<UNiagaraNodeCustomHlsl>(HlslNodes);
			Json->SetNumberField(TEXT("custom_hlsl_nodes"), HlslNodes.Num());
		}
	}

	return Json;
}

TSharedPtr<FNiagaraScratchPadScriptViewModel> NiagaraCommonUtils::GetScratchPadScriptViewModel(
	UNiagaraSystem* System,
	const FString& ModuleName)
{
	if (!System)
	{
		return nullptr;
	}

	FNiagaraEditorModule& Module = FNiagaraEditorModule::Get();
	TSharedPtr<FNiagaraSystemViewModel> SystemVM = Module.GetExistingViewModelForSystem(System);
	if (!SystemVM.IsValid())
	{
		return nullptr;
	}

	UNiagaraScratchPadViewModel* ScratchVM = SystemVM->GetScriptScratchPadViewModel();
	if (!ScratchVM)
	{
		return nullptr;
	}

	return ScratchVM->GetViewModelForScript(FName(*ModuleName));
}

bool NiagaraCommonUtils::ResolveScratchPadHlslNodes(
	const TSharedPtr<FJsonObject>& Params,
	UNiagaraSystem*& OutSystem,
	FString& OutModuleName,
	TArray<UNiagaraNodeCustomHlsl*>& OutNodes,
	FString& OutError)
{
	OutSystem = nullptr;
	OutNodes.Reset();

	FString SystemPath;
	if (!GetRequiredString(Params, TEXT("system_path"), SystemPath, OutError))
	{
		return false;
	}
	if (!GetRequiredString(Params, TEXT("module_name"), OutModuleName, OutError))
	{
		return false;
	}

	OutSystem = LoadNiagaraSystem(SystemPath, OutError);
	if (!OutSystem)
	{
		return false;
	}

	CollectScratchPadHlslNodes(OutSystem, OutModuleName, OutNodes);
	if (OutNodes.Num() == 0)
	{
		OutError = FString::Printf(TEXT("Scratch pad module '%s' not found or has no valid graph"), *OutModuleName);
		return false;
	}
	return true;
}

UNiagaraNodeFunctionCall* NiagaraCommonUtils::DescendNestedPath(
	UNiagaraNodeFunctionCall& InitialCall,
	const FString& DotPath,
	FString& OutLeafInputName,
	FString& OutError)
{
	TArray<FString> Segments;
	DotPath.ParseIntoArray(Segments, TEXT("."));
	if (Segments.Num() == 0)
	{
		OutError = TEXT("Empty input path");
		return nullptr;
	}
	if (Segments.Num() == 1)
	{
		OutLeafInputName = Segments[0];
		return &InitialCall;
	}

	UNiagaraNodeFunctionCall* CurrentCall = &InitialCall;
	for (int32 i = 0; i < Segments.Num() - 1; ++i)
	{
		UNiagaraNodeParameterMapSet* OverrideNode = nullptr;
		for (UEdGraphPin* Pin : CurrentCall->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Input)
			{
				continue;
			}

			const FNiagaraTypeDefinition TypeDef =
				UEdGraphSchema_Niagara::PinTypeToTypeDefinition(Pin->PinType);
			if (TypeDef != FNiagaraTypeDefinition::GetParameterMapDef() || Pin->LinkedTo.Num() == 0)
			{
				continue;
			}

			OverrideNode = Cast<UNiagaraNodeParameterMapSet>(Pin->LinkedTo[0]->GetOwningNode());
			if (OverrideNode)
			{
				break;
			}
		}
		if (!OverrideNode)
		{
			OutError = FString::Printf(
				TEXT("Path segment '%s' has no override node - input is not currently overridden"),
				*Segments[i]);
			return nullptr;
		}

		const FString ModulePrefixed = FString::Printf(TEXT("Module.%s"), *Segments[i]);
		const FNiagaraParameterHandle Aliased =
			FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
				FNiagaraParameterHandle(FName(*ModulePrefixed)), CurrentCall);
		const FName TargetPinName = Aliased.GetParameterHandleString();

		UEdGraphPin* OverridePin = nullptr;
		for (UEdGraphPin* Pin : OverrideNode->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Input && Pin->PinName == TargetPinName)
			{
				OverridePin = Pin;
				break;
			}
		}
		if (!OverridePin || OverridePin->LinkedTo.Num() == 0)
		{
			OutError = FString::Printf(
				TEXT("Path segment '%s' has no dynamic input attached to descend into"),
				*Segments[i]);
			return nullptr;
		}

		UEdGraphPin* SourcePin = OverridePin->LinkedTo[0];
		if (!SourcePin || !SourcePin->GetOwningNodeUnchecked())
		{
			OutError = FString::Printf(TEXT("Path segment '%s' has invalid downstream node"), *Segments[i]);
			return nullptr;
		}

		UNiagaraNodeFunctionCall* NextCall = Cast<UNiagaraNodeFunctionCall>(SourcePin->GetOwningNode());
		if (!NextCall)
		{
			OutError = FString::Printf(
				TEXT(
					"Path segment '%s' is bound to a non-function-call node (e.g. linked parameter or expression) - cannot descend further"),
				*Segments[i]);
			return nullptr;
		}

		CurrentCall = NextCall;
	}

	OutLeafInputName = Segments.Last();
	return CurrentCall;
}

// ---------------------------------------------------------------------------
// Compile / Execution Status Helpers
// ---------------------------------------------------------------------------
static FString CompileStatusToString(ENiagaraScriptCompileStatus Status)
{
	switch (Status)
	{
	case ENiagaraScriptCompileStatus::NCS_Unknown:
		return TEXT("unknown");
	case ENiagaraScriptCompileStatus::NCS_Dirty:
		return TEXT("dirty");
	case ENiagaraScriptCompileStatus::NCS_Error:
		return TEXT("error");
	case ENiagaraScriptCompileStatus::NCS_UpToDate:
		return TEXT("up_to_date");
	case ENiagaraScriptCompileStatus::NCS_BeingCreated:
		return TEXT("being_created");
	case ENiagaraScriptCompileStatus::NCS_UpToDateWithWarnings:
		return TEXT("warning");
	case ENiagaraScriptCompileStatus::NCS_ComputeUpToDateWithWarnings:
		return TEXT("compute_warning");
	default:
		return TEXT("unknown");
	}
}

FString NiagaraCommonUtils::ExecutionStateToString(ENiagaraExecutionState State)
{
	switch (State)
	{
	case ENiagaraExecutionState::Active:
		return TEXT("active");
	case ENiagaraExecutionState::Inactive:
		return TEXT("inactive");
	case ENiagaraExecutionState::InactiveClear:
		return TEXT("inactive_clear");
	case ENiagaraExecutionState::Complete:
		return TEXT("complete");
	case ENiagaraExecutionState::Disabled:
		return TEXT("disabled");
	default:
		return TEXT("unknown");
	}
}

static FString CompileStatusToSeverity(ENiagaraScriptCompileStatus Status)
{
	switch (Status)
	{
	case ENiagaraScriptCompileStatus::NCS_Error:
		return TEXT("error");
	case ENiagaraScriptCompileStatus::NCS_UpToDateWithWarnings:
	case ENiagaraScriptCompileStatus::NCS_ComputeUpToDateWithWarnings:
		return TEXT("warning");
	case ENiagaraScriptCompileStatus::NCS_Dirty:
	case ENiagaraScriptCompileStatus::NCS_BeingCreated:
	case ENiagaraScriptCompileStatus::NCS_Unknown:
		return TEXT("info");
	default:
		return TEXT("info");
	}
}

static bool SeverityMatchesFilter(const FString& Severity, const FString& Filter)
{
	if (Filter.Equals(TEXT("all"), ESearchCase::IgnoreCase))
	{
		return true;
	}
	return Severity.Equals(Filter, ESearchCase::IgnoreCase);
}

UNiagaraSystem* NiagaraCommonUtils::LoadSystemFromParams(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
	FString SystemPath;
	if (!GetRequiredString(Params, TEXT("system_path"), SystemPath, OutError))
	{
		return nullptr;
	}

	return LoadNiagaraSystem(SystemPath, OutError);
}

static bool MatchesFilter(const FString& Candidate, const FString& Filter)
{
	return Filter.IsEmpty() || Candidate.Contains(Filter, ESearchCase::IgnoreCase);
}

bool NiagaraCommonUtils::MatchesNameFilter(const FString& Candidate, const FString& Filter)
{
	return Filter.IsEmpty() || Candidate.Equals(Filter, ESearchCase::IgnoreCase);
}

bool NiagaraCommonUtils::MatchesAnyFilter(const FString& Filter, const TArray<FString>& Fields)
{
	if (Filter.IsEmpty())
	{
		return true;
	}

	for (const FString& Field : Fields)
	{
		if (Field.Contains(Filter, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

AActor* NiagaraCommonUtils::FindActorByName(UWorld* World, const FString& Name)
{
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->GetActorLabel().Equals(Name, ESearchCase::IgnoreCase) ||
			Actor->GetName().Equals(Name, ESearchCase::IgnoreCase))
		{
			return Actor;
		}
	}

	return nullptr;
}

TSharedPtr<FJsonObject> NiagaraCommonUtils::VectorToJson(const FVector& Value)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetNumberField(TEXT("x"), Value.X);
	Obj->SetNumberField(TEXT("y"), Value.Y);
	Obj->SetNumberField(TEXT("z"), Value.Z);
	return Obj;
}

TSharedPtr<FJsonObject> NiagaraCommonUtils::RotatorToJson(const FRotator& Value)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetNumberField(TEXT("pitch"), Value.Pitch);
	Obj->SetNumberField(TEXT("yaw"), Value.Yaw);
	Obj->SetNumberField(TEXT("roll"), Value.Roll);
	return Obj;
}

TArray<TSharedPtr<FJsonValue>> NiagaraCommonUtils::BuildEmitterInfoArray(
	UNiagaraSystem* System,
	const FString& Filter)
{
	TArray<TSharedPtr<FJsonValue>> Emitters;
	if (!System)
	{
		return Emitters;
	}

	const TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
	Emitters.Reserve(Handles.Num());
	for (int32 HandleIndex = 0; HandleIndex < Handles.Num(); ++HandleIndex)
	{
		const FString EmitterName = Handles[HandleIndex].GetName().ToString();
		if (!MatchesFilter(EmitterName, Filter))
		{
			continue;
		}

		Emitters.Add(MakeShared<FJsonValueObject>(EmitterHandleToJson(Handles[HandleIndex], HandleIndex)));
	}

	return Emitters;
}

TArray<TSharedPtr<FJsonValue>> NiagaraCommonUtils::BuildParameterInfoArray(
	UNiagaraSystem* System,
	const FString& Filter)
{
	TArray<TSharedPtr<FJsonValue>> Parameters;
	if (!System)
	{
		return Parameters;
	}

	const FNiagaraUserRedirectionParameterStore& ParameterStore = System->GetExposedParameters();
	const TArrayView<const FNiagaraVariableWithOffset> Variables = ParameterStore.ReadParameterVariables();
	Parameters.Reserve(Variables.Num());

	for (const FNiagaraVariableWithOffset& Variable : Variables)
	{
		const FString VariableName = Variable.GetName().ToString();
		if (!MatchesFilter(VariableName, Filter))
		{
			continue;
		}

		TSharedPtr<FJsonObject> ParameterObject = MakeShared<FJsonObject>();
		ParameterObject->SetStringField(TEXT("name"), VariableName);
		ParameterObject->SetStringField(TEXT("type"), Variable.GetType().GetName());
		Parameters.Add(MakeShared<FJsonValueObject>(ParameterObject));
	}

	return Parameters;
}

static TSharedPtr<FJsonObject> NiagaraVariableToJson(const FNiagaraVariableBase& Var)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("name"), Var.GetName().ToString());
	Obj->SetStringField(TEXT("type"), Var.GetType().GetName());
	return Obj;
}

TSharedPtr<FJsonObject> NiagaraCommonUtils::ScriptParameterToJson(const FNiagaraVariable& Var, int32 Index)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetNumberField(TEXT("index"), Index);
	Obj->SetStringField(TEXT("name"), Var.GetName().ToString());
	Obj->SetStringField(TEXT("type"), Var.GetType().GetName());
	Obj->SetBoolField(TEXT("data_interface"), Var.IsDataInterface());
	return Obj;
}

TSharedPtr<FJsonObject> NiagaraCommonUtils::RendererToJson(UNiagaraRendererProperties* Renderer, int32 Index)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	if (!Renderer)
	{
		return Obj;
	}

	Obj->SetNumberField(TEXT("index"), Index);
	Obj->SetStringField(TEXT("name"), Renderer->GetName());
	Obj->SetBoolField(TEXT("enabled"), Renderer->GetIsEnabled());

	if (UNiagaraSpriteRendererProperties* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer))
	{
		Obj->SetStringField(TEXT("type"), TEXT("sprite"));
		if (Sprite->Material)
		{
			Obj->SetStringField(TEXT("material"), Sprite->Material->GetPathName());
		}
		Obj->SetStringField(
			TEXT("facing_mode"),
			StaticEnum<ENiagaraSpriteFacingMode>()->GetNameStringByValue(static_cast<int64>(Sprite->FacingMode)));
		Obj->SetStringField(
			TEXT("alignment"),
			StaticEnum<ENiagaraSpriteAlignment>()->GetNameStringByValue(static_cast<int64>(Sprite->Alignment)));
		Obj->SetNumberField(TEXT("sort_order"), Sprite->SortOrderHint);
	}
	else if (UNiagaraMeshRendererProperties* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer))
	{
		Obj->SetStringField(TEXT("type"), TEXT("mesh"));

		TArray<TSharedPtr<FJsonValue>> MeshesArr;
		for (const FNiagaraMeshRendererMeshProperties& MeshProp : Mesh->Meshes)
		{
			TSharedPtr<FJsonObject> MeshObj = MakeShared<FJsonObject>();
			if (MeshProp.Mesh)
			{
				MeshObj->SetStringField(TEXT("mesh_path"), MeshProp.Mesh->GetPathName());
			}
			MeshesArr.Add(MakeShared<FJsonValueObject>(MeshObj));
		}
		Obj->SetArrayField(TEXT("meshes"), MeshesArr);

		TArray<TSharedPtr<FJsonValue>> OverridesArr;
		for (const FNiagaraMeshMaterialOverride& Override : Mesh->OverrideMaterials)
		{
			TSharedPtr<FJsonObject> MatObj = MakeShared<FJsonObject>();
			if (Override.ExplicitMat)
			{
				MatObj->SetStringField(TEXT("material"), Override.ExplicitMat->GetPathName());
			}
			OverridesArr.Add(MakeShared<FJsonValueObject>(MatObj));
		}
		Obj->SetArrayField(TEXT("material_overrides"), OverridesArr);
		Obj->SetNumberField(TEXT("sort_order"), Mesh->SortOrderHint);
	}
	else if (UNiagaraRibbonRendererProperties* Ribbon = Cast<UNiagaraRibbonRendererProperties>(Renderer))
	{
		Obj->SetStringField(TEXT("type"), TEXT("ribbon"));
		if (Ribbon->Material)
		{
			Obj->SetStringField(TEXT("material"), Ribbon->Material->GetPathName());
		}
		Obj->SetNumberField(TEXT("sort_order"), Ribbon->SortOrderHint);
	}
	else if (Cast<UNiagaraLightRendererProperties>(Renderer))
	{
		Obj->SetStringField(TEXT("type"), TEXT("light"));
	}
	else if (Cast<UNiagaraComponentRendererProperties>(Renderer))
	{
		Obj->SetStringField(TEXT("type"), TEXT("component"));
	}
	else
	{
		Obj->SetStringField(TEXT("type"), Renderer->GetClass()->GetName());
	}

	TArray<TSharedPtr<FJsonValue>> BindingsArr;
	const TArray<const FNiagaraVariableAttributeBinding*>& AllBindings = Renderer->GetAttributeBindings();

	TMap<const FNiagaraVariableAttributeBinding*, FString> BindingNameMap;
	for (TFieldIterator<FProperty> It(Renderer->GetClass()); It; ++It)
	{
		FStructProperty* StructProperty = CastField<FStructProperty>(*It);
		if (!StructProperty || !StructProperty->Struct)
		{
			continue;
		}
		if (StructProperty->Struct->GetFName() != FName(TEXT("NiagaraVariableAttributeBinding")))
		{
			continue;
		}

		const FNiagaraVariableAttributeBinding* BindingPtr =
			StructProperty->ContainerPtrToValuePtr<FNiagaraVariableAttributeBinding>(Renderer);
		BindingNameMap.Add(BindingPtr, StructProperty->GetName());
	}

	for (const FNiagaraVariableAttributeBinding* Binding : AllBindings)
	{
		if (!Binding)
		{
			continue;
		}

		TSharedPtr<FJsonObject> BindingObj = MakeShared<FJsonObject>();
		const FNiagaraVariable BoundVar = Binding->GetParamMapBindableVariable();
		BindingObj->SetStringField(TEXT("bound_variable"), BoundVar.GetName().ToString());
		if (BoundVar.GetType().IsValid())
		{
			BindingObj->SetStringField(TEXT("bound_variable_type"), BoundVar.GetType().GetName());
		}
		if (const FString* Name = BindingNameMap.Find(Binding))
		{
			BindingObj->SetStringField(TEXT("binding_name"), *Name);
		}
		const FNiagaraVariableBase DataSetVar = Binding->GetDataSetBindableVariable();
		if (DataSetVar.GetName() != NAME_None)
		{
			BindingObj->SetStringField(TEXT("dataset_variable"), DataSetVar.GetName().ToString());
		}
		BindingsArr.Add(MakeShared<FJsonValueObject>(BindingObj));
	}

	Obj->SetArrayField(TEXT("bindings"), BindingsArr);
	return Obj;
}

TSharedPtr<FJsonObject> NiagaraCommonUtils::BuildBoundsObject(const FBox& Bounds)
{
	if (!Bounds.IsValid)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> BoundsObject = MakeShared<FJsonObject>();
	const FVector Center = Bounds.GetCenter();
	const FVector Extent = Bounds.GetExtent();
	BoundsObject->SetNumberField(TEXT("center_x"), Center.X);
	BoundsObject->SetNumberField(TEXT("center_y"), Center.Y);
	BoundsObject->SetNumberField(TEXT("center_z"), Center.Z);
	BoundsObject->SetNumberField(TEXT("extent_x"), Extent.X);
	BoundsObject->SetNumberField(TEXT("extent_y"), Extent.Y);
	BoundsObject->SetNumberField(TEXT("extent_z"), Extent.Z);
	return BoundsObject;
}

FNiagaraTypeDefinition NiagaraCommonUtils::ResolveNiagaraTypeOrDefault(const FString& TypeString)
{
	FNiagaraTypeDefinition Resolved;
	if (ResolveTypeName(TypeString, Resolved))
	{
		return Resolved;
	}
	return FNiagaraTypeDefinition();
}

bool NiagaraCommonUtils::ResolveScratchPadGraphs(
	const TSharedPtr<FJsonObject>& Params,
	UNiagaraSystem*& OutSystem,
	FString& OutModuleName,
	TArray<UNiagaraGraph*>& OutGraphs,
	FString& OutError)
{
	OutSystem = nullptr;
	OutGraphs.Reset();

	FString SystemPath;
	if (!GetRequiredString(Params, TEXT("system_path"), SystemPath, OutError))
	{
		return false;
	}
	if (!GetRequiredString(Params, TEXT("module_name"), OutModuleName, OutError))
	{
		return false;
	}

	OutSystem = LoadNiagaraSystem(SystemPath, OutError);
	if (!OutSystem)
	{
		return false;
	}

	TArray<UNiagaraScript*> Scripts;
	GetScratchPadScriptPair(OutSystem, OutModuleName, Scripts);
	for (UNiagaraScript* Script : Scripts)
	{
		if (!Script)
		{
			continue;
		}
		if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource()))
		{
			if (Source->NodeGraph)
			{
				OutGraphs.Add(Source->NodeGraph);
			}
		}
	}

	if (OutGraphs.Num() == 0)
	{
		OutError = FString::Printf(TEXT("Scratch pad module '%s' not found or has no valid graph"), *OutModuleName);
		return false;
	}

	return true;
}

UNiagaraGraph* NiagaraCommonUtils::ResolveScratchPadGraph(
	const TSharedPtr<FJsonObject>& Params,
	UNiagaraSystem*& OutSystem,
	FString& OutError)
{
	FString ModuleName;
	TArray<UNiagaraGraph*> Graphs;
	if (!ResolveScratchPadGraphs(Params, OutSystem, ModuleName, Graphs, OutError))
	{
		return nullptr;
	}
	return Graphs.Num() > 0 ? Graphs[0] : nullptr;
}

bool NiagaraCommonUtils::ResolveScratchPadScriptAndGraph(
	const TSharedPtr<FJsonObject>& Params,
	UNiagaraSystem*& OutSystem,
	FString& OutModuleName,
	UNiagaraScript*& OutScript,
	UNiagaraGraph*& OutGraph,
	FString& OutError)
{
	OutSystem = nullptr;
	OutScript = nullptr;
	OutGraph = nullptr;
	OutModuleName.Reset();

	FString SystemPath;
	FString ModuleName;
	if (Params.IsValid())
	{
		GetRequiredString(Params, TEXT("system_path"), SystemPath, OutError);
		GetRequiredString(Params, TEXT("module_name"), ModuleName, OutError);
	}

	if (!SystemPath.IsEmpty() && !ModuleName.IsEmpty())
	{
		OutSystem = LoadNiagaraSystem(SystemPath, OutError);
		if (!OutSystem)
		{
			return false;
		}

		OutModuleName = ModuleName;
		TArray<UNiagaraScript*> Scripts;
		GetScratchPadScriptPair(OutSystem, ModuleName, Scripts);
		if (Scripts.Num() == 0)
		{
			OutError = FString::Printf(TEXT("Scratch pad module '%s' not found"), *ModuleName);
			return false;
		}

		OutScript = Scripts.Last();
		if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(OutScript->GetLatestSource()))
		{
			OutGraph = Source->NodeGraph;
		}
		if (!OutGraph)
		{
			OutError = TEXT("Script has no graph source");
			return false;
		}
		return true;
	}

	FString ScriptPath;
	if (GetRequiredString(Params, TEXT("script_path"), ScriptPath, OutError))
	{
		OutScript = Cast<UNiagaraScript>(UEditorAssetLibrary::LoadAsset(ScriptPath));
		if (!OutScript)
		{
			OutError = FString::Printf(TEXT("Script asset not found: %s"), *ScriptPath);
			return false;
		}
		if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(OutScript->GetLatestSource()))
		{
			OutGraph = Source->NodeGraph;
		}
		if (!OutGraph)
		{
			OutError = FString::Printf(TEXT("Script has no graph: %s"), *ScriptPath);
			return false;
		}
		return true;
	}

	OutError = TEXT("Provide either (system_path + module_name) or script_path");
	return false;
}

TArray<UNiagaraGraph*> NiagaraCommonUtils::CollectMutationGraphs(
	UNiagaraSystem* System,
	const FString& ModuleName,
	UNiagaraScript* DirectScript)
{
	TArray<UNiagaraGraph*> Graphs;
	if (System && !ModuleName.IsEmpty())
	{
		TArray<UNiagaraScript*> Scripts;
		GetScratchPadScriptPair(System, ModuleName, Scripts);
		for (UNiagaraScript* Script : Scripts)
		{
			if (!Script)
			{
				continue;
			}
			if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource()))
			{
				if (Source->NodeGraph)
				{
					Graphs.AddUnique(Source->NodeGraph);
				}
			}
		}
	}
	else if (DirectScript)
	{
		if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(DirectScript->GetLatestSource()))
		{
			if (Source->NodeGraph)
			{
				Graphs.Add(Source->NodeGraph);
			}
		}
	}

	return Graphs;
}

UNiagaraNodeOutput* NiagaraCommonUtils::FindFirstOutputNode(UNiagaraGraph* Graph)
{
	if (!Graph)
	{
		return nullptr;
	}

	TArray<UNiagaraNodeOutput*> Outputs;
	Graph->GetNodesOfClass<UNiagaraNodeOutput>(Outputs);
	return Outputs.Num() > 0 ? Outputs[0] : nullptr;
}

void NiagaraCommonUtils::EnsureScriptVariableMetadata(
	UNiagaraGraph* Graph,
	const FNiagaraVariable& Var,
	ENiagaraDefaultMode DefaultMode)
{
	if (!Graph || !Var.GetType().IsValid())
	{
		return;
	}

	UNiagaraGraph::FScriptVariableMap& MetaMap = Graph->GetAllMetaData();
	if (MetaMap.Contains(Var))
	{
		return;
	}

	UNiagaraScriptVariable* ScriptVar = NewObject<UNiagaraScriptVariable>(Graph, NAME_None, RF_Transactional);
	if (!ScriptVar)
	{
		return;
	}

	FNiagaraVariableMetaData Meta;
	ScriptVar->Init(Var, Meta);
	ScriptVar->DefaultMode = DefaultMode;
	MetaMap.Add(Var, ScriptVar);
}

ENiagaraDefaultMode NiagaraCommonUtils::DefaultModeForNamespace(const FString& ParamName)
{
	if (ParamName.StartsWith(TEXT("Module."), ESearchCase::IgnoreCase) ||
		ParamName.StartsWith(TEXT("Input."), ESearchCase::IgnoreCase))
	{
		return ENiagaraDefaultMode::Value;
	}
	return ENiagaraDefaultMode::Binding;
}

bool NiagaraCommonUtils::ValidateReadableGraphParams(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
	FString SystemPath;
	FString ModuleName;
	FString EmitterName;
	FString ScriptUsage;
	FString ScriptPath;
	if (Params.IsValid())
	{
		GetRequiredString(Params, TEXT("system_path"), SystemPath, OutError);
		GetRequiredString(Params, TEXT("module_name"), ModuleName, OutError);
		GetRequiredString(Params, TEXT("emitter_name"), EmitterName, OutError);
		GetRequiredString(Params, TEXT("script_usage"), ScriptUsage, OutError);
		GetRequiredString(Params, TEXT("script_path"), ScriptPath, OutError);
	}

	if (!SystemPath.IsEmpty() && !ModuleName.IsEmpty())
	{
		return true;
	}
	if (!SystemPath.IsEmpty() && !EmitterName.IsEmpty() && !ScriptUsage.IsEmpty())
	{
		return true;
	}
	if (!ScriptPath.IsEmpty())
	{
		return true;
	}

	OutError = TEXT("Provide either (system_path + module_name), (system_path + emitter_name + script_usage), or script_path");
	return false;
}

bool NiagaraCommonUtils::ValidateMutationGraphParams(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
	FString SystemPath;
	FString ModuleName;
	FString ScriptPath;
	if (Params.IsValid())
	{
		GetRequiredString(Params, TEXT("system_path"), SystemPath, OutError);
		GetRequiredString(Params, TEXT("module_name"), ModuleName, OutError);
		GetRequiredString(Params, TEXT("script_path"), ScriptPath, OutError);
	}

	if (!SystemPath.IsEmpty() && !ModuleName.IsEmpty())
	{
		return true;
	}
	if (!ScriptPath.IsEmpty())
	{
		return true;
	}

	OutError = TEXT("Provide either (system_path + module_name) or script_path");
	return false;
}

bool NiagaraCommonUtils::ValidateScratchPadGraphParams(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
	FString SystemPath;
	FString ModuleName;
	if (!GetRequiredString(Params, TEXT("system_path"), SystemPath, OutError))
	{
		return false;
	}
	if (!GetRequiredString(Params, TEXT("module_name"), ModuleName, OutError))
	{
		return false;
	}

	return true;
}

bool NiagaraCommonUtils::ValidateNodeSelector(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
	int32 NodeIndex = INDEX_NONE;
	FString NodeClass;
	FString NodeId;
	const bool bHasIndex = Params.IsValid() && Params->TryGetNumberField(TEXT("node_index"), NodeIndex);
	const bool bHasClass = GetRequiredString(Params, TEXT("node_class"), NodeClass, OutError);
	const bool bHasId = GetRequiredString(Params, TEXT("node_id"), NodeId, OutError);
	if (bHasIndex || bHasClass || bHasId)
	{
		return true;
	}

	OutError = TEXT("Provide one of: node_index, node_class, node_id");
	return false;
}

bool NiagaraCommonUtils::ValidateSideNodeSelector(
	const TSharedPtr<FJsonObject>& Params,
	const TCHAR* Prefix,
	FString& OutError)
{
	int32 NodeIndex = INDEX_NONE;
	FString NodeClass;
	FString NodeId;
	const FString IndexKey = FString::Printf(TEXT("%s_node_index"), Prefix);
	const FString ClassKey = FString::Printf(TEXT("%s_node_class"), Prefix);
	const FString IdKey = FString::Printf(TEXT("%s_node_id"), Prefix);

	const bool bHasIndex = Params.IsValid() && Params->TryGetNumberField(IndexKey, NodeIndex);
	const bool bHasClass = GetRequiredString(Params, ClassKey, NodeClass, OutError);
	const bool bHasId = GetRequiredString(Params, IdKey, NodeId, OutError);
	if (bHasIndex || bHasClass || bHasId)
	{
		return true;
	}

	OutError = FString::Printf(TEXT("Provide one of: %s_node_index, %s_node_class, %s_node_id"), Prefix, Prefix, Prefix);
	return false;
}

UNiagaraGraph* NiagaraCommonUtils::ResolveReadableGraph(
	const TSharedPtr<FJsonObject>& Params,
	UNiagaraScript*& OutScript,
	FString& OutSourceDesc,
	FString& OutError)
{
	OutScript = nullptr;
	OutSourceDesc.Reset();

	FString SystemPath;
	FString ModuleName;
	if (Params.IsValid())
	{
		GetRequiredString(Params, TEXT("system_path"), SystemPath, OutError);
		GetRequiredString(Params, TEXT("module_name"), ModuleName, OutError);
	}

	if (!SystemPath.IsEmpty() && !ModuleName.IsEmpty())
	{
		UNiagaraSystem* System = LoadNiagaraSystem(SystemPath, OutError);
		if (!System)
		{
			return nullptr;
		}

		TArray<UNiagaraScript*> Scripts;
		GetScratchPadScriptPair(System, ModuleName, Scripts);
		for (int32 Index = Scripts.Num() - 1; Index >= 0; --Index)
		{
			if (UNiagaraScript* Script = Scripts[Index])
			{
				if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource()))
				{
					if (Source->NodeGraph)
					{
						OutScript = Script;
						OutSourceDesc = FString::Printf(TEXT("%s::%s"), *SystemPath, *ModuleName);
						return Source->NodeGraph;
					}
				}
			}
		}

		OutError = FString::Printf(
			TEXT("Scratch pad module '%s' not found on system '%s' or has no graph"),
			*ModuleName,
			*SystemPath);
		return nullptr;
	}

	FString EmitterName;
	FString ScriptUsageStr;

	if (!SystemPath.IsEmpty()
		&& GetRequiredString(Params, TEXT("emitter_name"), EmitterName, OutError)
		&& GetRequiredString(Params, TEXT("script_usage"), ScriptUsageStr, OutError))
	{
		UNiagaraSystem* System = LoadNiagaraSystem(SystemPath, OutError);
		if (!System)
		{
			return nullptr;
		}

		bool bParsed = false;
		const ENiagaraScriptUsage Usage = ParseScriptUsage(ScriptUsageStr, bParsed);
		if (!bParsed)
		{
			OutError = FString::Printf(TEXT("Unknown script_usage '%s'"), *ScriptUsageStr);
			return nullptr;
		}

		int32 EmitterIndex = INDEX_NONE;
		FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName, EmitterIndex, OutError);
		if (!Handle)
		{
			return nullptr;
		}

		FVersionedNiagaraEmitterData* Data = GetEmitterData(Handle);
		if (!Data)
		{
			OutError = TEXT("Failed to get emitter data");
			return nullptr;
		}

		UNiagaraScript* Script = Data->GetScript(Usage, FGuid());
		if (!Script)
		{
			OutError = FString::Printf(TEXT("Emitter '%s' has no script for usage '%s'"), *EmitterName, *ScriptUsageStr);
			return nullptr;
		}

		if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource()))
		{
			if (Source->NodeGraph)
			{
				OutScript = Script;
				OutSourceDesc = FString::Printf(TEXT("%s::%s::%s"), *SystemPath, *EmitterName, *ScriptUsageStr);
				return Source->NodeGraph;
			}
		}

		OutError = TEXT("Emitter script has no graph");
		return nullptr;
	}

	FString ScriptPath;
	if (GetRequiredString(Params, TEXT("script_path"), ScriptPath, OutError))
	{
		UNiagaraScript* Script = Cast<UNiagaraScript>(UEditorAssetLibrary::LoadAsset(ScriptPath));
		if (!Script)
		{
			OutError = FString::Printf(TEXT("Script asset not found: %s"), *ScriptPath);
			return nullptr;
		}

		UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
		if (!Source || !Source->NodeGraph)
		{
			OutError = FString::Printf(TEXT("Script has no graph source: %s"), *ScriptPath);
			return nullptr;
		}

		OutScript = Script;
		OutSourceDesc = ScriptPath;
		return Source->NodeGraph;
	}

	OutError = TEXT("Provide (system_path + module_name), (system_path + emitter_name + script_usage), or script_path");
	return nullptr;
}

UNiagaraNode* NiagaraCommonUtils::ResolveGraphNode(
	UNiagaraGraph* Graph,
	const TSharedPtr<FJsonObject>& Params,
	FString& OutError)
{
	if (!Graph)
	{
		OutError = TEXT("Null graph");
		return nullptr;
	}

	int32 NodeIndex = INDEX_NONE;
	if (Params.IsValid() && Params->TryGetNumberField(TEXT("node_index"), NodeIndex))
	{
		if (Graph->Nodes.IsValidIndex(NodeIndex))
		{
			if (UNiagaraNode* Node = Cast<UNiagaraNode>(Graph->Nodes[NodeIndex]))
			{
				return Node;
			}

			OutError = FString::Printf(TEXT("Node at index %d is not a Niagara node"), NodeIndex);
			return nullptr;
		}

		OutError = FString::Printf(TEXT("node_index %d out of range (have %d nodes)"), NodeIndex, Graph->Nodes.Num());
		return nullptr;
	}

	FString NodeClassName;
	if (GetRequiredString(Params, TEXT("node_class"), NodeClassName, OutError))
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			const FString ClassName = Node->GetClass()->GetName();
			if (ClassName.Equals(NodeClassName, ESearchCase::IgnoreCase) ||
				ClassName.Equals(FString::Printf(TEXT("NiagaraNode%s"), *NodeClassName), ESearchCase::IgnoreCase))
			{
				if (UNiagaraNode* NiagaraNode = Cast<UNiagaraNode>(Node))
				{
					return NiagaraNode;
				}
			}
		}
		return nullptr;
	}

	FString NodeIdStr;
	if (GetRequiredString(Params, TEXT("node_id"), NodeIdStr, OutError))
	{
		FGuid TargetGuid;
		if (FGuid::Parse(NodeIdStr, TargetGuid))
		{
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node && Node->NodeGuid == TargetGuid)
				{
					if (UNiagaraNode* NiagaraNode = Cast<UNiagaraNode>(Node))
					{
						return NiagaraNode;
					}
				}
			}
		}
		return nullptr;
	}

	OutError = TEXT("Must provide one of: node_index, node_class, node_id");
	return nullptr;
}

FString NiagaraCommonUtils::ShortNodeClassName(const UEdGraphNode* Node)
{
	if (!Node)
	{
		return TEXT("");
	}

	FString Name = Node->GetClass()->GetName();
	Name.RemoveFromStart(TEXT("NiagaraNode"));
	return Name;
}

FString NiagaraCommonUtils::ShortNodeTypeName(const UClass* Cls)
{
	if (!Cls)
	{
		return FString();
	}

	FString Name = Cls->GetName();
	if (Name.StartsWith(TEXT("NiagaraNode")))
	{
		Name = Name.RightChop(FString(TEXT("NiagaraNode")).Len());
	}
	return Name;
}

FString NiagaraCommonUtils::NodeDisplayTitle(UEdGraphNode* Node)
{
	return Node ? Node->GetNodeTitle(ENodeTitleType::ListView).ToString() : TEXT("");
}

static FString PinTypeDescription(const UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return TEXT("");
	}

	const FNiagaraTypeDefinition TypeDef = UEdGraphSchema_Niagara::PinTypeToTypeDefinition(Pin->PinType);
	if (TypeDef.IsValid())
	{
		return TypeDef.GetName();
	}
	if (!Pin->PinType.PinCategory.IsNone())
	{
		return Pin->PinType.PinCategory.ToString();
	}
	return TEXT("");
}

void NiagaraCommonUtils::BuildNodeIndexMap(UNiagaraGraph* Graph, TMap<UEdGraphNode*, int32>& OutMap)
{
	OutMap.Reset();
	if (!Graph)
	{
		return;
	}

	OutMap.Reserve(Graph->Nodes.Num());
	for (int32 NodeIndex = 0; NodeIndex < Graph->Nodes.Num(); ++NodeIndex)
	{
		if (UEdGraphNode* Node = Graph->Nodes[NodeIndex])
		{
			OutMap.Add(Node, NodeIndex);
		}
	}
}

void NiagaraCommonUtils::GatherClassMeta(
	UClass* Cls,
	FString& OutDisplayName,
	FString& OutTooltip,
	FString& OutCategory,
	FString& OutKeywords)
{
	if (!Cls)
	{
		return;
	}

	OutDisplayName = Cls->GetDisplayNameText().ToString();
	if (OutDisplayName.IsEmpty())
	{
		OutDisplayName = Cls->GetName();
	}
	if (Cls->HasMetaData(TEXT("ToolTip")))
	{
		OutTooltip = Cls->GetMetaData(TEXT("ToolTip"));
	}
	if (Cls->HasMetaData(TEXT("Category")))
	{
		OutCategory = Cls->GetMetaData(TEXT("Category"));
	}
	if (Cls->HasMetaData(TEXT("Keywords")))
	{
		OutKeywords = Cls->GetMetaData(TEXT("Keywords"));
	}
}

UClass* NiagaraCommonUtils::FindNiagaraNodeClass(const FString& ShortOrFullName)
{
	if (ShortOrFullName.IsEmpty())
	{
		return nullptr;
	}

	if (UClass* Exact = FindObject<UClass>(nullptr, *ShortOrFullName))
	{
		if (Exact->IsChildOf<UNiagaraNode>())
		{
			return Exact;
		}
	}

	const FString WithPrefix = FString::Printf(TEXT("NiagaraNode%s"), *ShortOrFullName);
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Cls = *It;
		if (!Cls->IsChildOf<UNiagaraNode>())
		{
			continue;
		}
		if (Cls->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
		{
			continue;
		}

		const FString Name = Cls->GetName();
		if (Name.Equals(ShortOrFullName, ESearchCase::IgnoreCase) ||
			Name.Equals(WithPrefix, ESearchCase::IgnoreCase))
		{
			return Cls;
		}
	}

	return nullptr;
}

TSharedPtr<FJsonObject> NiagaraCommonUtils::BuildCustomHlslSchema()
{
	TSharedPtr<FJsonObject> Schema = MakeShared<FJsonObject>();
	Schema->SetStringField(
		TEXT("description"),
		TEXT("Inserts raw HLSL into the Niagara translator. Pins are typed ports referenced in HLSL by name."));

	TArray<TSharedPtr<FJsonValue>> Fields;
	{
		TSharedPtr<FJsonObject> Field = MakeShared<FJsonObject>();
		Field->SetStringField(TEXT("name"), TEXT("hlsl_code"));
		Field->SetStringField(TEXT("type"), TEXT("string"));
		Field->SetStringField(
			TEXT("description"),
			TEXT("HLSL source. Reference pins by name (e.g. Output = Input * 2.0f;)."));
		Fields.Add(MakeShared<FJsonValueObject>(Field));
	}
	{
		TSharedPtr<FJsonObject> Field = MakeShared<FJsonObject>();
		Field->SetStringField(TEXT("name"), TEXT("inputs"));
		Field->SetStringField(TEXT("type"), TEXT("array<{name,type}>"));
		Field->SetStringField(
			TEXT("description"),
			TEXT(
				"Typed input pins. 'type' accepts float, int, bool, vec2/3/4, color, quat, matrix, position, ParameterMap, or any registered Niagara type."));
		Fields.Add(MakeShared<FJsonValueObject>(Field));
	}
	{
		TSharedPtr<FJsonObject> Field = MakeShared<FJsonObject>();
		Field->SetStringField(TEXT("name"), TEXT("outputs"));
		Field->SetStringField(TEXT("type"), TEXT("array<{name,type}>"));
		Field->SetStringField(
			TEXT("description"),
			TEXT("Typed output pins. Same type vocabulary as inputs."));
		Fields.Add(MakeShared<FJsonValueObject>(Field));
	}
	Schema->SetArrayField(TEXT("fields"), Fields);

	TSharedPtr<FJsonObject> Example = MakeShared<FJsonObject>();
	Example->SetStringField(TEXT("hlsl_code"), TEXT("Result = Value * Scale;"));

	TArray<TSharedPtr<FJsonValue>> ExampleInputs;
	{
		TSharedPtr<FJsonObject> Input = MakeShared<FJsonObject>();
		Input->SetStringField(TEXT("name"), TEXT("Value"));
		Input->SetStringField(TEXT("type"), TEXT("vec3"));
		ExampleInputs.Add(MakeShared<FJsonValueObject>(Input));
	}
	{
		TSharedPtr<FJsonObject> Input = MakeShared<FJsonObject>();
		Input->SetStringField(TEXT("name"), TEXT("Scale"));
		Input->SetStringField(TEXT("type"), TEXT("float"));
		ExampleInputs.Add(MakeShared<FJsonValueObject>(Input));
	}
	Example->SetArrayField(TEXT("inputs"), ExampleInputs);

	TArray<TSharedPtr<FJsonValue>> ExampleOutputs;
	{
		TSharedPtr<FJsonObject> Output = MakeShared<FJsonObject>();
		Output->SetStringField(TEXT("name"), TEXT("Result"));
		Output->SetStringField(TEXT("type"), TEXT("vec3"));
		ExampleOutputs.Add(MakeShared<FJsonValueObject>(Output));
	}
	Example->SetArrayField(TEXT("outputs"), ExampleOutputs);

	Schema->SetObjectField(TEXT("example"), Example);
	return Schema;
}

TSharedPtr<FJsonObject> NiagaraCommonUtils::SerializeNodePin(const UEdGraphPin* Pin)
{
	TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
	if (!Pin)
	{
		return PinObj;
	}

	PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());

	const FNiagaraTypeDefinition TypeDef = UEdGraphSchema_Niagara::PinTypeToTypeDefinition(Pin->PinType);
	PinObj->SetStringField(
		TEXT("type"),
		TypeDef.IsValid() ? TypeDefToString(TypeDef) : Pin->PinType.PinCategory.ToString());

	if (!Pin->PinToolTip.IsEmpty())
	{
		PinObj->SetStringField(TEXT("description"), Pin->PinToolTip);
	}
	if (!Pin->DefaultValue.IsEmpty())
	{
		PinObj->SetStringField(TEXT("default"), Pin->DefaultValue);
	}

	return PinObj;
}

static bool IsAddPinPlaceholder(const UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return true;
	}
	if (Pin->bOrphanedPin)
	{
		return true;
	}
	return Pin->PinName == NAME_None;
}

static TSharedPtr<FJsonObject> SerializeGraphLink(
	const UEdGraphPin* OtherPin,
	const TMap<UEdGraphNode*, int32>& NodeIndexMap)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	if (!OtherPin || !OtherPin->GetOwningNodeUnchecked())
	{
		Obj->SetNumberField(TEXT("to_node"), -1);
		return Obj;
	}

	UEdGraphNode* OwningNode = OtherPin->GetOwningNode();
	const int32* IndexPtr = NodeIndexMap.Find(OwningNode);
	Obj->SetNumberField(TEXT("to_node"), IndexPtr ? *IndexPtr : -1);
	Obj->SetStringField(TEXT("to_node_title"), NiagaraCommonUtils::NodeDisplayTitle(OwningNode));
	Obj->SetStringField(TEXT("to_node_class"), NiagaraCommonUtils::ShortNodeClassName(OwningNode));
	Obj->SetStringField(TEXT("to_pin"), OtherPin->PinName.ToString());
	Obj->SetStringField(
		TEXT("to_pin_direction"),
		OtherPin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
	return Obj;
}

static TSharedPtr<FJsonObject> SerializeGraphPin(
	UEdGraphPin* Pin,
	const TMap<UEdGraphNode*, int32>& NodeIndexMap,
	bool bIncludeLinks,
	bool bIncludeDefault)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("name"), Pin->PinName.ToString());
	Obj->SetStringField(TEXT("type"), PinTypeDescription(Pin));
	Obj->SetStringField(
		TEXT("direction"),
		Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
	Obj->SetBoolField(TEXT("connected"), Pin->LinkedTo.Num() > 0);
	Obj->SetNumberField(TEXT("link_count"), Pin->LinkedTo.Num());
	if (Pin->bHidden)
	{
		Obj->SetBoolField(TEXT("hidden"), true);
	}
	if (Pin->bOrphanedPin)
	{
		Obj->SetBoolField(TEXT("orphaned"), true);
	}

	if (bIncludeDefault && Pin->LinkedTo.Num() == 0 && !Pin->DefaultValue.IsEmpty())
	{
		Obj->SetStringField(TEXT("default_value"), Pin->DefaultValue);
	}

	if (bIncludeLinks && Pin->LinkedTo.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> Links;
		for (UEdGraphPin* Other : Pin->LinkedTo)
		{
			Links.Add(MakeShared<FJsonValueObject>(SerializeGraphLink(Other, NodeIndexMap)));
		}
		Obj->SetArrayField(TEXT("links"), Links);
	}

	return Obj;
}

TSharedPtr<FJsonObject> NiagaraCommonUtils::SerializeGraphNode(
	UEdGraphNode* Node,
	int32 NodeIndex,
	const TMap<UEdGraphNode*, int32>& NodeIndexMap,
	const FString& Verbosity)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetNumberField(TEXT("index"), NodeIndex);
	Obj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
	Obj->SetStringField(TEXT("type"), ShortNodeClassName(Node));
	Obj->SetStringField(TEXT("title"), NodeDisplayTitle(Node));
	Obj->SetStringField(TEXT("guid"), Node->NodeGuid.ToString(EGuidFormats::DigitsWithHyphens));

	TSharedPtr<FJsonObject> Pos = MakeShared<FJsonObject>();
	Pos->SetNumberField(TEXT("x"), Node->NodePosX);
	Pos->SetNumberField(TEXT("y"), Node->NodePosY);
	Obj->SetObjectField(TEXT("position"), Pos);

	const bool bFull = Verbosity.Equals(TEXT("full"), ESearchCase::IgnoreCase);
	const bool bConnections = bFull || Verbosity.Equals(TEXT("connections"), ESearchCase::IgnoreCase);

	int32 NumInputs = 0;
	int32 NumOutputs = 0;
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (IsAddPinPlaceholder(Pin))
		{
			continue;
		}
		if (Pin->Direction == EGPD_Input)
		{
			++NumInputs;
		}
		else
		{
			++NumOutputs;
		}
	}
	Obj->SetNumberField(TEXT("num_inputs"), NumInputs);
	Obj->SetNumberField(TEXT("num_outputs"), NumOutputs);

	if (Verbosity.Equals(TEXT("summary"), ESearchCase::IgnoreCase))
	{
		return Obj;
	}

	TArray<TSharedPtr<FJsonValue>> InputPins;
	TArray<TSharedPtr<FJsonValue>> OutputPins;
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (IsAddPinPlaceholder(Pin))
		{
			continue;
		}

		TSharedPtr<FJsonObject> PinObj = SerializeGraphPin(Pin, NodeIndexMap, bConnections, bFull);
		if (Pin->Direction == EGPD_Input)
		{
			InputPins.Add(MakeShared<FJsonValueObject>(PinObj));
		}
		else
		{
			OutputPins.Add(MakeShared<FJsonValueObject>(PinObj));
		}
	}
	Obj->SetArrayField(TEXT("inputs"), InputPins);
	Obj->SetArrayField(TEXT("outputs"), OutputPins);

	if (bFull)
	{
		if (UNiagaraNodeFunctionCall* FunctionCall = Cast<UNiagaraNodeFunctionCall>(Node))
		{
			Obj->SetStringField(TEXT("function_display_name"), FunctionCall->GetFunctionName());
			if (UNiagaraScript* Referenced = FunctionCall->FunctionScript)
			{
				Obj->SetStringField(TEXT("function_script"), Referenced->GetPathName());
			}
		}
		if (UNiagaraNodeOutput* OutputNode = Cast<UNiagaraNodeOutput>(Node))
		{
			Obj->SetStringField(TEXT("script_usage"), ScriptUsageToString(OutputNode->GetUsage()));
		}
		if (UNiagaraNodeInput* InputNode = Cast<UNiagaraNodeInput>(Node))
		{
			const FNiagaraVariable& Var = InputNode->Input;
			Obj->SetStringField(TEXT("input_name"), Var.GetName().ToString());
			Obj->SetStringField(TEXT("input_type"), Var.GetType().GetName());
		}
		if (UNiagaraNodeCustomHlsl* HlslNode = Cast<UNiagaraNodeCustomHlsl>(Node))
		{
			if (FProperty* HlslProp = UNiagaraNodeCustomHlsl::StaticClass()->FindPropertyByName(TEXT("CustomHlsl")))
			{
				if (FStrProperty* StrProp = CastField<FStrProperty>(HlslProp))
				{
					const FString Body = StrProp->GetPropertyValue_InContainer(HlslNode);
					Obj->SetNumberField(TEXT("hlsl_length"), Body.Len());
					Obj->SetStringField(
						TEXT("hlsl_preview"),
						Body.Len() <= 500 ? Body : (Body.Left(497) + TEXT("...")));
				}
			}
		}
		if (UNiagaraNodeOp* OpNode = Cast<UNiagaraNodeOp>(Node))
		{
			Obj->SetStringField(TEXT("op_name"), OpNode->OpName.ToString());
		}
	}

	return Obj;
}

UNiagaraGraph* NiagaraCommonUtils::CreateTemporaryGraphForUsage(ENiagaraScriptUsage Usage)
{
	UNiagaraGraph* TempGraph = NewObject<UNiagaraGraph>(GetTransientPackage(), NAME_None, RF_Transient);
	if (!TempGraph)
	{
		return nullptr;
	}

	FGraphNodeCreator<UNiagaraNodeOutput> OutputCreator(*TempGraph);
	UNiagaraNodeOutput* OutputNode = OutputCreator.CreateNode(false);
	OutputNode->SetUsage(Usage);
	OutputNode->Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("OutputMap")));
	OutputCreator.Finalize();

	return TempGraph;
}

UNiagaraNodeOp* NiagaraCommonUtils::CreateOpIntrospectionNode(UNiagaraGraph* Graph, const FName& OpName)
{
	if (!Graph || OpName.IsNone())
	{
		return nullptr;
	}

	FGraphNodeCreator<UNiagaraNodeOp> Creator(*Graph);
	UNiagaraNodeOp* OpNode = Creator.CreateNode(false);
	OpNode->OpName = OpName;
	Creator.Finalize();
	return OpNode;
}

void NiagaraCommonUtils::SerializeFunctionScriptPins(
	UNiagaraScript* Script,
	TArray<TSharedPtr<FJsonValue>>& OutInputs,
	TArray<TSharedPtr<FJsonValue>>& OutOutputs)
{
	if (!Script)
	{
		return;
	}

	UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
	if (!Source || !Source->NodeGraph)
	{
		return;
	}

	UNiagaraNodeOutput* OutputNode = Source->NodeGraph->FindEquivalentOutputNode(Script->GetUsage());
	if (!OutputNode)
	{
		return;
	}

	for (const FNiagaraVariable& Var : OutputNode->Outputs)
	{
		TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("name"), Var.GetName().ToString());
		Entry->SetStringField(TEXT("type"), TypeDefToString(Var.GetType()));
		OutOutputs.Add(MakeShared<FJsonValueObject>(Entry));
	}

	const UNiagaraGraph::FScriptVariableMap& MetaMap = Source->NodeGraph->GetAllMetaData();
	for (const TPair<FNiagaraVariable, TObjectPtr<UNiagaraScriptVariable>>& Pair : MetaMap)
	{
		const FNiagaraVariable& Var = Pair.Key;
		const FString NameStr = Var.GetName().ToString();
		if (!NameStr.StartsWith(TEXT("Module.")) && !NameStr.StartsWith(TEXT("Input.")))
		{
			continue;
		}

		TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("name"), NameStr);
		Entry->SetStringField(TEXT("type"), TypeDefToString(Var.GetType()));
		OutInputs.Add(MakeShared<FJsonValueObject>(Entry));
	}
}

bool NiagaraCommonUtils::ValidateRequiredStringParams(
	const TSharedPtr<FJsonObject>& Params,
	const TArray<FString>& RequiredParams,
	FString& OutError)
{
	for (const FString& ParamName : RequiredParams)
	{
		FString OutValue;
		if (!GetRequiredString(Params, ParamName, OutValue, OutError))
		{
			return false;
		}
	}
	return true;
}

void NiagaraCommonUtils::RemoveOverridePinConnections(UEdGraphPin& OverridePin, UNiagaraGraph* Graph)
{
	if (OverridePin.LinkedTo.Num() == 0)
	{
		return;
	}

	TArray<UEdGraphNode*> NodesToRemove;
	for (UEdGraphPin* LinkedPin : OverridePin.LinkedTo)
	{
		if (LinkedPin && LinkedPin->GetOwningNode())
		{
			NodesToRemove.AddUnique(LinkedPin->GetOwningNode());
		}
	}

	OverridePin.BreakAllPinLinks();

	for (UEdGraphNode* NodeToRemove : NodesToRemove)
	{
		if (!NodeToRemove || !Graph)
		{
			continue;
		}

		for (UEdGraphPin* Pin : NodeToRemove->Pins)
		{
			if (Pin)
			{
				Pin->BreakAllPinLinks();
			}
		}
		Graph->RemoveNode(NodeToRemove);
	}
}

int32 NiagaraCommonUtils::PopulateFloatCurveDataInterface(
	UNiagaraDataInterfaceCurve* CurveDI,
	const TArray<TSharedPtr<FJsonValue>>& Keys)
{
	if (!CurveDI)
	{
		return 0;
	}

	CurveDI->CurveAsset = nullptr;
	CurveDI->Curve.Reset();

	int32 Count = 0;
	for (const TSharedPtr<FJsonValue>& KeyVal : Keys)
	{
		const TSharedPtr<FJsonObject> KeyObj = KeyVal->AsObject();
		if (!KeyObj)
		{
			continue;
		}

		const float Time = static_cast<float>(KeyObj->GetNumberField(TEXT("time")));
		const float Value = static_cast<float>(KeyObj->GetNumberField(TEXT("value")));
		CurveDI->Curve.AddKey(Time, Value);
		++Count;
	}

	CurveDI->UpdateTimeRanges();
	CurveDI->UpdateLUT();
	CurveDI->MarkRenderDataDirty();
	return Count;
}

int32 NiagaraCommonUtils::PopulateVectorCurveDataInterface(
	UNiagaraDataInterfaceVectorCurve* CurveDI,
	const TArray<TSharedPtr<FJsonValue>>& Keys)
{
	if (!CurveDI)
	{
		return 0;
	}

	CurveDI->CurveAsset = nullptr;
	CurveDI->XCurve.Reset();
	CurveDI->YCurve.Reset();
	CurveDI->ZCurve.Reset();

	int32 Count = 0;
	for (const TSharedPtr<FJsonValue>& KeyVal : Keys)
	{
		const TSharedPtr<FJsonObject> KeyObj = KeyVal->AsObject();
		if (!KeyObj)
		{
			continue;
		}

		const float Time = static_cast<float>(KeyObj->GetNumberField(TEXT("time")));
		const float X = static_cast<float>(KeyObj->GetNumberField(TEXT("x")));
		const float Y = static_cast<float>(KeyObj->GetNumberField(TEXT("y")));
		const float Z = static_cast<float>(KeyObj->GetNumberField(TEXT("z")));
		CurveDI->XCurve.AddKey(Time, X);
		CurveDI->YCurve.AddKey(Time, Y);
		CurveDI->ZCurve.AddKey(Time, Z);
		++Count;
	}

	CurveDI->UpdateTimeRanges();
	CurveDI->UpdateLUT();
	CurveDI->MarkRenderDataDirty();
	return Count;
}

int32 NiagaraCommonUtils::PopulateColorCurveDataInterface(
	UNiagaraDataInterfaceColorCurve* CurveDI,
	const TArray<TSharedPtr<FJsonValue>>& Keys)
{
	if (!CurveDI)
	{
		return 0;
	}

	CurveDI->CurveAsset = nullptr;
	CurveDI->RedCurve.Reset();
	CurveDI->GreenCurve.Reset();
	CurveDI->BlueCurve.Reset();
	CurveDI->AlphaCurve.Reset();

	int32 Count = 0;
	for (const TSharedPtr<FJsonValue>& KeyVal : Keys)
	{
		const TSharedPtr<FJsonObject> KeyObj = KeyVal->AsObject();
		if (!KeyObj)
		{
			continue;
		}

		const float Time = static_cast<float>(KeyObj->GetNumberField(TEXT("time")));
		const float R = static_cast<float>(KeyObj->GetNumberField(TEXT("r")));
		const float G = static_cast<float>(KeyObj->GetNumberField(TEXT("g")));
		const float B = static_cast<float>(KeyObj->GetNumberField(TEXT("b")));
		double A = 1.0;
		KeyObj->TryGetNumberField(TEXT("a"), A);

		CurveDI->RedCurve.AddKey(Time, R);
		CurveDI->GreenCurve.AddKey(Time, G);
		CurveDI->BlueCurve.AddKey(Time, B);
		CurveDI->AlphaCurve.AddKey(Time, static_cast<float>(A));
		++Count;
	}

	CurveDI->UpdateTimeRanges();
	CurveDI->UpdateLUT();
	CurveDI->MarkRenderDataDirty();
	return Count;
}

bool NiagaraCommonUtils::IsStatelessEmitterHandle(const FNiagaraEmitterHandle* Handle)
{
	if (!Handle)
	{
		return false;
	}

	const UObject* StatelessEmitter = reinterpret_cast<const UObject*>(Handle->GetStatelessEmitter());
	return StatelessEmitter != nullptr && Handle->GetEmitterMode() == ENiagaraEmitterMode::Stateless;
}

UNiagaraNodeParameterMapSet* NiagaraCommonUtils::FindModuleInputOverrideNode(UNiagaraNodeFunctionCall& StackFunctionCall)
{
	for (UEdGraphPin* Pin : StackFunctionCall.Pins)
	{
		if (!Pin || Pin->Direction != EGPD_Input)
		{
			continue;
		}

		const FNiagaraTypeDefinition TypeDef = UEdGraphSchema_Niagara::PinTypeToTypeDefinition(Pin->PinType);
		if (TypeDef != FNiagaraTypeDefinition::GetParameterMapDef() || Pin->LinkedTo.Num() == 0)
		{
			continue;
		}

		UEdGraphPin* SourcePin = Pin->LinkedTo[0];
		if (!SourcePin || !SourcePin->GetOwningNodeUnchecked())
		{
			continue;
		}
		return Cast<UNiagaraNodeParameterMapSet>(SourcePin->GetOwningNode());
	}

	return nullptr;
}

UEdGraphPin* NiagaraCommonUtils::FindModuleInputOverridePin(
	UNiagaraNodeFunctionCall& StackFunctionCall,
	const FNiagaraParameterHandle& AliasedHandle)
{
	UNiagaraNodeParameterMapSet* OverrideNode = FindModuleInputOverrideNode(StackFunctionCall);
	if (!OverrideNode)
	{
		return nullptr;
	}

	const FName TargetName = AliasedHandle.GetParameterHandleString();
	for (UEdGraphPin* Pin : OverrideNode->Pins)
	{
		if (Pin && Pin->Direction == EGPD_Input && Pin->PinName == TargetName)
		{
			return Pin;
		}
	}
	return nullptr;
}

UNiagaraNodeFunctionCall* NiagaraCommonUtils::FindModuleFunctionCall(
	UNiagaraGraph* Graph,
	const FString& ModuleName)
{
	if (!Graph)
	{
		return nullptr;
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		UNiagaraNodeFunctionCall* Call = Cast<UNiagaraNodeFunctionCall>(Node);
		if (!Call)
		{
			continue;
		}

		const FString DisplayName = Call->GetFunctionName();
		if (DisplayName.Equals(ModuleName, ESearchCase::IgnoreCase))
		{
			return Call;
		}
		if (Call->FunctionScript &&
			Call->FunctionScript->GetName().Equals(ModuleName, ESearchCase::IgnoreCase))
		{
			return Call;
		}
	}

	return nullptr;
}

UNiagaraGraph* NiagaraCommonUtils::ResolveEmitterStackGraph(
	UNiagaraSystem* System,
	const FString& EmitterName,
	ENiagaraScriptUsage Usage,
	UNiagaraScript*& OutScript,
	FString& OutError)
{
	OutScript = nullptr;

	int32 Index = INDEX_NONE;
	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName, Index, OutError);
	if (!Handle)
	{
		return nullptr;
	}

	FVersionedNiagaraEmitterData* EmitterData = GetEmitterData(Handle);
	if (!EmitterData)
	{
		OutError = TEXT("Failed to get emitter data");
		return nullptr;
	}

	OutScript = EmitterData->GetScript(Usage, FGuid());
	if (!OutScript)
	{
		OutError = FString::Printf(
			TEXT("Emitter '%s' has no script for usage %s"),
			*EmitterName,
			*ScriptUsageToString(Usage));
		return nullptr;
	}

	if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(OutScript->GetLatestSource()))
	{
		if (Source->NodeGraph)
		{
			return Source->NodeGraph;
		}
	}

	OutError = TEXT("Emitter script has no graph source");
	return nullptr;
}

UNiagaraScript* NiagaraCommonUtils::GetScriptForUsage(
	FVersionedNiagaraEmitterData* EmitterData,
	ENiagaraScriptUsage Usage)
{
	if (!EmitterData)
	{
		return nullptr;
	}

	switch (Usage)
	{
	case ENiagaraScriptUsage::EmitterSpawnScript:
		return EmitterData->EmitterSpawnScriptProps.Script;
	case ENiagaraScriptUsage::EmitterUpdateScript:
		return EmitterData->EmitterUpdateScriptProps.Script;
	case ENiagaraScriptUsage::ParticleSpawnScript:
		return EmitterData->SpawnScriptProps.Script;
	case ENiagaraScriptUsage::ParticleUpdateScript:
		return EmitterData->UpdateScriptProps.Script;
	default:
		return nullptr;
	}
}

void NiagaraCommonUtils::SerializeRapidIterationValue(
	const FNiagaraParameterStore& Store,
	const FNiagaraVariableWithOffset& Var,
	const TSharedRef<FJsonObject>& OutObj)
{
	const FString TypeName = Var.GetType().GetName();
	const int32 Offset = Store.IndexOf(Var);
	if (Offset == INDEX_NONE)
	{
		OutObj->SetStringField(TEXT("value"), TEXT("(not found)"));
		return;
	}

	const uint8* DataPtr = Store.GetParameterData(Offset, Var.GetType().GetSize());
	if (!DataPtr)
	{
		OutObj->SetStringField(TEXT("value"), TEXT("(null)"));
		return;
	}

	if (TypeName.Contains(TEXT("Float")) || TypeName == TEXT("float"))
	{
		const float Val = *reinterpret_cast<const float*>(DataPtr);
		OutObj->SetNumberField(TEXT("value"), Val);
		OutObj->SetStringField(TEXT("value_type"), TEXT("float"));
	}
	else if (TypeName.Contains(TEXT("Int32")) || TypeName == TEXT("int32"))
	{
		const int32 Val = *reinterpret_cast<const int32*>(DataPtr);
		OutObj->SetNumberField(TEXT("value"), Val);
		OutObj->SetStringField(TEXT("value_type"), TEXT("int"));
	}
	else if (TypeName.Contains(TEXT("Bool")) || TypeName == TEXT("bool"))
	{
		const int32 BoolVal = *reinterpret_cast<const int32*>(DataPtr);
		OutObj->SetBoolField(TEXT("value"), BoolVal != 0);
		OutObj->SetStringField(TEXT("value_type"), TEXT("bool"));
	}
	else if (TypeName.Contains(TEXT("Vector3f")) || TypeName.Contains(TEXT("Position")))
	{
		const FVector3f* Vec = reinterpret_cast<const FVector3f*>(DataPtr);
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetNumberField(TEXT("x"), Vec->X);
		Obj->SetNumberField(TEXT("y"), Vec->Y);
		Obj->SetNumberField(TEXT("z"), Vec->Z);
		OutObj->SetObjectField(TEXT("value"), Obj);
		OutObj->SetStringField(TEXT("value_type"), TEXT("vector"));
	}
	else if (TypeName.Contains(TEXT("Vector2f")))
	{
		const FVector2f* Vec = reinterpret_cast<const FVector2f*>(DataPtr);
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetNumberField(TEXT("x"), Vec->X);
		Obj->SetNumberField(TEXT("y"), Vec->Y);
		OutObj->SetObjectField(TEXT("value"), Obj);
		OutObj->SetStringField(TEXT("value_type"), TEXT("vector2"));
	}
	else if (TypeName.Contains(TEXT("Vector4f")))
	{
		const FVector4f* Vec = reinterpret_cast<const FVector4f*>(DataPtr);
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetNumberField(TEXT("x"), Vec->X);
		Obj->SetNumberField(TEXT("y"), Vec->Y);
		Obj->SetNumberField(TEXT("z"), Vec->Z);
		Obj->SetNumberField(TEXT("w"), Vec->W);
		OutObj->SetObjectField(TEXT("value"), Obj);
		OutObj->SetStringField(TEXT("value_type"), TEXT("vector4"));
	}
	else if (TypeName.Contains(TEXT("LinearColor")))
	{
		const FLinearColor* Color = reinterpret_cast<const FLinearColor*>(DataPtr);
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetNumberField(TEXT("r"), Color->R);
		Obj->SetNumberField(TEXT("g"), Color->G);
		Obj->SetNumberField(TEXT("b"), Color->B);
		Obj->SetNumberField(TEXT("a"), Color->A);
		OutObj->SetObjectField(TEXT("value"), Obj);
		OutObj->SetStringField(TEXT("value_type"), TEXT("color"));
	}
	else if (TypeName.Contains(TEXT("Quat")))
	{
		const FQuat4f* Quat = reinterpret_cast<const FQuat4f*>(DataPtr);
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetNumberField(TEXT("x"), Quat->X);
		Obj->SetNumberField(TEXT("y"), Quat->Y);
		Obj->SetNumberField(TEXT("z"), Quat->Z);
		Obj->SetNumberField(TEXT("w"), Quat->W);
		OutObj->SetObjectField(TEXT("value"), Obj);
		OutObj->SetStringField(TEXT("value_type"), TEXT("quat"));
	}
	else
	{
		OutObj->SetStringField(TEXT("value"), TEXT("(unsupported type)"));
		OutObj->SetStringField(TEXT("value_type"), TypeName);
	}
}

void NiagaraCommonUtils::CollectModuleChain(
	UNiagaraNodeOutput* OutputNode,
	TArray<UNiagaraNodeFunctionCall*>& OutChain)
{
	if (!OutputNode)
	{
		return;
	}

	UEdGraphPin* CurrentInput = nullptr;
	for (UEdGraphPin* Pin : OutputNode->Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			CurrentInput = Pin;
			break;
		}
	}

	while (CurrentInput && CurrentInput->LinkedTo.Num() > 0)
	{
		UEdGraphPin* UpstreamPin = CurrentInput->LinkedTo[0];
		if (!UpstreamPin)
		{
			break;
		}

		UEdGraphNode* UpstreamNode = UpstreamPin->GetOwningNode();
		if (UNiagaraNodeFunctionCall* FuncNode = Cast<UNiagaraNodeFunctionCall>(UpstreamNode))
		{
			OutChain.Add(FuncNode);
		}

		CurrentInput = nullptr;
		if (UpstreamNode)
		{
			for (UEdGraphPin* Pin : UpstreamNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					CurrentInput = Pin;
					break;
				}
			}
		}
	}

	Algo::Reverse(OutChain);
}

UEdGraphPin* NiagaraCommonUtils::FindFirstPin(UEdGraphNode* Node, EEdGraphPinDirection Direction)
{
	if (!Node)
	{
		return nullptr;
	}

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin->Direction == Direction)
		{
			return Pin;
		}
	}

	return nullptr;
}

bool NiagaraCommonUtils::TrySplitAssetPath(
	const FString& AssetPath,
	FString& OutPackagePath,
	FString& OutAssetName,
	FString& OutError)
{
	int32 SlashIndex = INDEX_NONE;
	if (!AssetPath.FindLastChar(TEXT('/'), SlashIndex) || SlashIndex <= 0)
	{
		OutError = TEXT("Invalid asset_path format. Expected '/Game/Path/AssetName'");
		return false;
	}

	OutPackagePath = AssetPath.Left(SlashIndex);
	OutAssetName = AssetPath.Mid(SlashIndex + 1);
	if (OutAssetName.IsEmpty())
	{
		OutError = TEXT("Asset name is empty");
		return false;
	}

	return true;
}

FString NiagaraCommonUtils::ResolveTemplateReference(const FString& TemplateName)
{
	const FString LowerTemplate = TemplateName.ToLower();
	if (LowerTemplate == TEXT("simple_sprite") || LowerTemplate == TEXT("sprite"))
	{
		return TEXT("/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst.SimpleSpriteBurst");
	}
	if (LowerTemplate == TEXT("fountain") || LowerTemplate == TEXT("default"))
	{
		return TEXT("/Niagara/DefaultAssets/Templates/Emitters/Fountain.Fountain");
	}
	if (LowerTemplate == TEXT("mesh"))
	{
		return TEXT("/Niagara/DefaultAssets/Templates/Emitters/UpwardMeshBurst.UpwardMeshBurst");
	}
	if (LowerTemplate == TEXT("ribbon"))
	{
		return TEXT("/Niagara/DefaultAssets/Templates/Emitters/LocationBasedRibbon.LocationBasedRibbon");
	}
	if (LowerTemplate == TEXT("minimal"))
	{
		return TEXT("/Niagara/DefaultAssets/Templates/Emitters/Minimal.Minimal");
	}

	return TemplateName;
}

bool NiagaraCommonUtils::TryCopyTemplateToSystem(UNiagaraSystem& TargetSystem, const FString& TemplateName)
{
	if (TemplateName.Equals(TEXT("empty"), ESearchCase::IgnoreCase))
	{
		return false;
	}

	const FString TemplatePath = ResolveTemplateReference(TemplateName);
	if (UNiagaraSystem* TemplateSystem = Cast<UNiagaraSystem>(UEditorAssetLibrary::LoadAsset(TemplatePath)))
	{
		TargetSystem.Modify();

		TemplateSystem->GetExposedParameters().CopyParametersTo(
			TargetSystem.GetExposedParameters(),
			false,
			FNiagaraParameterStore::EDataInterfaceCopyMethod::Value);
		TargetSystem.bFixedBounds = TemplateSystem->bFixedBounds;
		TargetSystem.SetFixedBounds(TemplateSystem->GetFixedBounds());
		TargetSystem.SetWarmupTime(TemplateSystem->GetWarmupTime());
		TargetSystem.SetWarmupTickDelta(TemplateSystem->GetWarmupTickDelta());

		bool bAddedAnyEmitter = false;
		for (const FNiagaraEmitterHandle& TemplateHandle : TemplateSystem->GetEmitterHandles())
		{
			const FVersionedNiagaraEmitter TemplateInstance = TemplateHandle.GetInstance();
			if (!TemplateInstance.Emitter && !TemplateHandle.GetStatelessEmitter())
			{
				continue;
			}

			FName EmitterName = TemplateHandle.GetName();
			if (EmitterName.IsNone())
			{
				EmitterName = TemplateInstance.Emitter
					              ? TemplateInstance.Emitter->GetFName()
					              : FName(TEXT("Emitter"));
			}

			TargetSystem.DuplicateEmitterHandle(TemplateHandle, EmitterName);
			bAddedAnyEmitter = true;
		}

		return bAddedAnyEmitter;
	}

	if (UNiagaraEmitter* TemplateEmitter = Cast<UNiagaraEmitter>(UEditorAssetLibrary::LoadAsset(TemplatePath)))
	{
		TargetSystem.AddEmitterHandle(
			*TemplateEmitter,
			TemplateEmitter->GetFName(),
			TemplateEmitter->GetExposedVersion().VersionGuid);
		return true;
	}

	return false;
}

static FString ExtractPackageName(const FString& ObjectPath)
{
	int32 DotIndex = INDEX_NONE;
	if (ObjectPath.FindLastChar(TEXT('.'), DotIndex))
	{
		return ObjectPath.Left(DotIndex);
	}
	return ObjectPath;
}

static FString DescribeCompileStatusForResponse(ENiagaraScriptCompileStatus Status, bool& bOutIsError)
{
	bOutIsError = false;
	switch (Status)
	{
	case ENiagaraScriptCompileStatus::NCS_UpToDate:
		return TEXT("up_to_date");
	case ENiagaraScriptCompileStatus::NCS_Dirty:
		return TEXT("dirty");
	case ENiagaraScriptCompileStatus::NCS_Error:
		bOutIsError = true;
		return TEXT("error");
	default:
		return TEXT("unknown");
	}
}

static void AppendRendererFeedbackEntries(
	const TArray<FNiagaraRendererFeedback>& FeedbackEntries,
	const FString& EmitterName,
	UNiagaraRendererProperties* Renderer,
	int32 RendererIndex,
	const TCHAR* SeverityLabel,
	bool bIncludeFixDescription,
	const FString& SeverityFilter,
	TArray<TSharedPtr<FJsonValue>>& OutIssues)
{
	if (!SeverityMatchesFilter(SeverityLabel, SeverityFilter))
	{
		return;
	}

	for (const FNiagaraRendererFeedback& Feedback : FeedbackEntries)
	{
		TSharedPtr<FJsonObject> FeedbackObject = MakeShared<FJsonObject>();
		FeedbackObject->SetStringField(TEXT("emitter_name"), EmitterName);
		FeedbackObject->SetStringField(TEXT("severity"), SeverityLabel);
		FeedbackObject->SetStringField(TEXT("source"), TEXT("renderer"));
		FeedbackObject->SetStringField(TEXT("renderer_type"), Renderer->GetClass()->GetName());
		FeedbackObject->SetNumberField(TEXT("renderer_index"), RendererIndex);
		FeedbackObject->SetStringField(TEXT("description"), Feedback.GetDescriptionText().ToString());
		FeedbackObject->SetBoolField(TEXT("has_fix"), Feedback.IsFixable());
		if (bIncludeFixDescription && Feedback.IsFixable())
		{
			FeedbackObject->SetStringField(TEXT("fix_description"), Feedback.GetFixDescriptionText().ToString());
		}
		OutIssues.Add(MakeShared<FJsonValueObject>(FeedbackObject));
	}
}

FNiagaraAssetVersion NiagaraCommonUtils::PickLatestVersion(
	const TArray<FNiagaraAssetVersion>& Versions,
	const FNiagaraAssetVersion& Fallback)
{
	FNiagaraAssetVersion Latest = Fallback;
	for (const FNiagaraAssetVersion& Candidate : Versions)
	{
		const bool bIsNewerMajor = Candidate.MajorVersion > Latest.MajorVersion;
		const bool bIsNewerMinor =
			Candidate.MajorVersion == Latest.MajorVersion &&
			Candidate.MinorVersion > Latest.MinorVersion;
		if (bIsNewerMajor || bIsNewerMinor)
		{
			Latest = Candidate;
		}
	}
	return Latest;
}

TArray<TSharedPtr<FJsonValue>> NiagaraCommonUtils::CollectExternalReferencers(const FString& ObjectPath)
{
	IAssetRegistry& Registry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	TArray<FName> Referencers;
	Registry.GetReferencers(
		FName(*ExtractPackageName(ObjectPath)),
		Referencers,
		UE::AssetRegistry::EDependencyCategory::Package);

	TArray<TSharedPtr<FJsonValue>> Result;
	for (const FName& Referencer : Referencers)
	{
		const FString ReferencerName = Referencer.ToString();
		if (ReferencerName.StartsWith(TEXT("/Script/")))
		{
			continue;
		}

		Result.Add(MakeShared<FJsonValueString>(ReferencerName));
	}

	return Result;
}

void NiagaraCommonUtils::CollectCompileStatuses(
	UNiagaraSystem* System,
	TArray<TSharedPtr<FJsonValue>>& OutScriptStatuses,
	int32& OutErrorCount)
{
	OutScriptStatuses.Reset();
	OutErrorCount = 0;
	if (!System)
	{
		return;
	}

	auto AddCompileStatusEntry = [&OutScriptStatuses, &OutErrorCount](const FString& ScriptLabel, UNiagaraScript* Script)
	{
		if (!Script)
		{
			return;
		}

		bool bIsError = false;
		const FString StatusText = DescribeCompileStatusForResponse(Script->GetLastCompileStatus(), bIsError);

		TSharedPtr<FJsonObject> StatusObject = MakeShared<FJsonObject>();
		StatusObject->SetStringField(TEXT("script"), ScriptLabel);
		StatusObject->SetStringField(TEXT("status"), StatusText);
		OutScriptStatuses.Add(MakeShared<FJsonValueObject>(StatusObject));
		if (bIsError)
		{
			++OutErrorCount;
		}
	};

	AddCompileStatusEntry(TEXT("system_spawn"), System->GetSystemSpawnScript());
	AddCompileStatusEntry(TEXT("system_update"), System->GetSystemUpdateScript());

	for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		FVersionedNiagaraEmitterData* EmitterData = Handle.GetEmitterData();
		if (!EmitterData)
		{
			continue;
		}

		const FString Prefix = Handle.GetName().ToString();
		AddCompileStatusEntry(Prefix + TEXT(".emitter_spawn"), EmitterData->EmitterSpawnScriptProps.Script);
		AddCompileStatusEntry(Prefix + TEXT(".emitter_update"), EmitterData->EmitterUpdateScriptProps.Script);
		AddCompileStatusEntry(Prefix + TEXT(".particle_spawn"), EmitterData->SpawnScriptProps.Script);
		AddCompileStatusEntry(Prefix + TEXT(".particle_update"), EmitterData->UpdateScriptProps.Script);
	}
}

FProperty* NiagaraCommonUtils::ResolveSystemProperty(const FString& RequestedProperty, const FString& ValueText)
{
	if (FProperty* ExactProperty = UNiagaraSystem::StaticClass()->FindPropertyByName(FName(*RequestedProperty)))
	{
		return ExactProperty;
	}

	if (ValueText == TEXT("true") || ValueText == TEXT("false"))
	{
		const FString BoolProperty = TEXT("b") + RequestedProperty;
		return UNiagaraSystem::StaticClass()->FindPropertyByName(FName(*BoolProperty));
	}

	return nullptr;
}

FString NiagaraCommonUtils::BuildUnknownSystemPropertyError(
	const FString& RequestedProperty,
	const FString& Filter)
{
	TArray<FString> AvailableProperties;
	for (TFieldIterator<FProperty> PropertyIt(UNiagaraSystem::StaticClass()); PropertyIt; ++PropertyIt)
	{
		if (!PropertyIt->HasAnyPropertyFlags(CPF_Edit))
		{
			continue;
		}

		const FString PropertyName = PropertyIt->GetName();
		if (MatchesFilter(PropertyName, Filter))
		{
			AvailableProperties.Add(PropertyName);
		}
	}

	if (AvailableProperties.Num() > 20)
	{
		AvailableProperties.SetNum(20);
		AvailableProperties.Add(TEXT("..."));
	}

	return FString::Printf(
		TEXT("Property '%s' not found on UNiagaraSystem. Available: %s"),
		*RequestedProperty,
		*FString::Join(AvailableProperties, TEXT(", ")));
}

void NiagaraCommonUtils::AppendCompileIssues(
	FVersionedNiagaraEmitterData& EmitterData,
	const FString& EmitterName,
	const FString& SeverityFilter,
	TArray<TSharedPtr<FJsonValue>>& OutIssues)
{
	TArray<UNiagaraScript*> Scripts;
	EmitterData.GetScripts(Scripts, false, false);

	for (UNiagaraScript* Script : Scripts)
	{
		if (!Script)
		{
			continue;
		}

		const ENiagaraScriptCompileStatus CompileStatus = Script->GetLastCompileStatus();
		if (CompileStatus == ENiagaraScriptCompileStatus::NCS_UpToDate)
		{
			continue;
		}

		const FString Severity = CompileStatusToSeverity(CompileStatus);
		if (!SeverityMatchesFilter(Severity, SeverityFilter))
		{
			continue;
		}

		TSharedPtr<FJsonObject> IssueObject = MakeShared<FJsonObject>();
		IssueObject->SetStringField(TEXT("emitter_name"), EmitterName);
		IssueObject->SetStringField(TEXT("severity"), Severity);
		IssueObject->SetStringField(TEXT("compile_status"), CompileStatusToString(CompileStatus));
		IssueObject->SetStringField(TEXT("script_usage"), ScriptUsageToString(Script->GetUsage()));
		IssueObject->SetStringField(TEXT("source"), TEXT("compile_status"));
		IssueObject->SetStringField(
			TEXT("description"),
			FString::Printf(
				TEXT("Script '%s' in emitter '%s' has status: %s"),
				*ScriptUsageToString(Script->GetUsage()),
				*EmitterName,
				*CompileStatusToString(CompileStatus)));
		IssueObject->SetBoolField(TEXT("has_fix"), false);
		OutIssues.Add(MakeShared<FJsonValueObject>(IssueObject));
	}
}

void NiagaraCommonUtils::AppendRendererIssues(
	const FNiagaraEmitterHandle& Handle,
	FVersionedNiagaraEmitterData& EmitterData,
	const FString& EmitterName,
	const FString& SeverityFilter,
	TArray<TSharedPtr<FJsonValue>>& OutIssues)
{
	const FVersionedNiagaraEmitter VersionedEmitter = Handle.GetInstance();
	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData.GetRenderers();

	for (int32 RendererIndex = 0; RendererIndex < Renderers.Num(); ++RendererIndex)
	{
		UNiagaraRendererProperties* Renderer = Renderers[RendererIndex];
		if (!Renderer)
		{
			continue;
		}

		TArray<FNiagaraRendererFeedback> ErrorFeedback;
		TArray<FNiagaraRendererFeedback> WarningFeedback;
		TArray<FNiagaraRendererFeedback> InfoFeedback;
		Renderer->GetRendererFeedback(VersionedEmitter, ErrorFeedback, WarningFeedback, InfoFeedback);

		AppendRendererFeedbackEntries(
			ErrorFeedback,
			EmitterName,
			Renderer,
			RendererIndex,
			TEXT("error"),
			true,
			SeverityFilter,
			OutIssues);
		AppendRendererFeedbackEntries(
			WarningFeedback,
			EmitterName,
			Renderer,
			RendererIndex,
			TEXT("warning"),
			true,
			SeverityFilter,
			OutIssues);
		AppendRendererFeedbackEntries(
			InfoFeedback,
			EmitterName,
			Renderer,
			RendererIndex,
			TEXT("info"),
			false,
			SeverityFilter,
			OutIssues);
	}
}

void NiagaraCommonUtils::AppendSystemMessageIssues(
	UNiagaraSystem* System,
	const FString& EmitterFilter,
	const FString& SeverityFilter,
	TArray<TSharedPtr<FJsonValue>>& OutIssues)
{
	if (!System || !EmitterFilter.IsEmpty() || !SeverityMatchesFilter(TEXT("info"), SeverityFilter))
	{
		return;
	}

	FNiagaraMessageStore& MessageStore = System->GetMessageStore();
	for (const TPair<FGuid, TObjectPtr<UNiagaraMessageDataBase>>& Entry : MessageStore.GetMessages())
	{
		UNiagaraMessageDataBase* MessageData = Entry.Value;
		if (!MessageData)
		{
			continue;
		}

		TSharedPtr<FJsonObject> IssueObject = MakeShared<FJsonObject>();
		IssueObject->SetStringField(TEXT("emitter_name"), TEXT("System"));
		IssueObject->SetStringField(TEXT("severity"), TEXT("info"));
		IssueObject->SetStringField(
			TEXT("description"),
			FString::Printf(TEXT("System message: %s"), *MessageData->GetClass()->GetName()));
		IssueObject->SetStringField(TEXT("message_key"), Entry.Key.ToString());
		IssueObject->SetBoolField(TEXT("has_fix"), MessageData->GetAllowDismissal());
		OutIssues.Add(MakeShared<FJsonValueObject>(IssueObject));
	}
}

bool NiagaraCommonUtils::TryFindPreviewInstance(
	UNiagaraSystem* System,
	UNiagaraComponent*& OutComponent,
	FNiagaraSystemInstance*& OutSystemInstance)
{
	OutComponent = nullptr;
	OutSystemInstance = nullptr;

	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!EditorWorld)
	{
		return false;
	}

	for (TActorIterator<AActor> ActorIt(EditorWorld); ActorIt; ++ActorIt)
	{
		TArray<UNiagaraComponent*> Components;
		ActorIt->GetComponents<UNiagaraComponent>(Components);

		for (UNiagaraComponent* NiagaraComponent : Components)
		{
			if (!NiagaraComponent || NiagaraComponent->GetAsset() != System)
			{
				continue;
			}

			const FNiagaraSystemInstanceControllerPtr Controller = NiagaraComponent->GetSystemInstanceController();
			if (!Controller.IsValid() || !Controller->IsValid())
			{
				continue;
			}

			FNiagaraSystemInstance* Instance = Controller->GetSystemInstance_Unsafe();
			if (!Instance)
			{
				continue;
			}

			OutComponent = NiagaraComponent;
			OutSystemInstance = Instance;
			return true;
		}
	}

	return false;
}

UNiagaraSystemEditorData* NiagaraCommonUtils::GetSystemEditorData(UNiagaraSystem* System, FString& OutError)
{
	UNiagaraEditorDataBase* EditorDataBase = System ? System->GetEditorData() : nullptr;
	if (!EditorDataBase)
	{
		OutError = TEXT("System has no editor data");
		return nullptr;
	}

	UNiagaraSystemEditorData* EditorData = Cast<UNiagaraSystemEditorData>(EditorDataBase);
	if (!EditorData)
	{
		OutError = TEXT("Failed to cast editor data to UNiagaraSystemEditorData");
		return nullptr;
	}

	return EditorData;
}

void NiagaraCommonUtils::ApplyPlaybackFrameRate(UNiagaraSystemEditorData* EditorData, int32 FrameRateValue)
{
	FStructProperty* FrameRateProperty = CastField<FStructProperty>(
		UNiagaraSystemEditorData::StaticClass()->FindPropertyByName(TEXT("PlaybackFrameRate")));
	if (!FrameRateProperty)
	{
		return;
	}

	const FFrameRate DesiredRate(FrameRateValue, 1);
	FrameRateProperty->CopyCompleteValue(
		FrameRateProperty->ContainerPtrToValuePtr<void>(EditorData),
		&DesiredRate);
}

FString NiagaraCommonUtils::FormatNiagaraAssetVersion(const FNiagaraAssetVersion& Version)
{
	return FString::Printf(TEXT("%d.%d"), Version.MajorVersion, Version.MinorVersion);
}

static void PopulateModuleVersionFields(
	TSharedPtr<FJsonObject> ModuleObject,
	UNiagaraScript* FunctionScript)
{
	if (!FunctionScript)
	{
		ModuleObject->SetBoolField(TEXT("versioning_enabled"), false);
		ModuleObject->SetBoolField(TEXT("is_outdated"), false);
		ModuleObject->SetStringField(TEXT("current_version"), TEXT("N/A"));
		ModuleObject->SetStringField(TEXT("latest_version"), TEXT("N/A"));
		return;
	}

	const bool bVersioningEnabled = FunctionScript->IsVersioningEnabled();
	ModuleObject->SetBoolField(TEXT("versioning_enabled"), bVersioningEnabled);
	if (!bVersioningEnabled)
	{
		ModuleObject->SetBoolField(TEXT("is_outdated"), false);
		ModuleObject->SetStringField(TEXT("current_version"), TEXT("N/A"));
		ModuleObject->SetStringField(TEXT("latest_version"), TEXT("N/A"));
		return;
	}

	const FNiagaraAssetVersion CurrentVersion = FunctionScript->GetExposedVersion();
	const TArray<FNiagaraAssetVersion> AvailableVersions = FunctionScript->GetAllAvailableVersions();
	const FNiagaraAssetVersion LatestVersion = NiagaraCommonUtils::PickLatestVersion(AvailableVersions, CurrentVersion);

	ModuleObject->SetStringField(TEXT("current_version"), NiagaraCommonUtils::FormatNiagaraAssetVersion(CurrentVersion));
	ModuleObject->SetStringField(TEXT("latest_version"), NiagaraCommonUtils::FormatNiagaraAssetVersion(LatestVersion));
	ModuleObject->SetBoolField(
		TEXT("is_outdated"),
		CurrentVersion.MajorVersion != LatestVersion.MajorVersion ||
		CurrentVersion.MinorVersion != LatestVersion.MinorVersion);

	TArray<TSharedPtr<FJsonValue>> VersionValues;
	for (const FNiagaraAssetVersion& Version : AvailableVersions)
	{
		VersionValues.Add(MakeShared<FJsonValueString>(NiagaraCommonUtils::FormatNiagaraAssetVersion(Version)));
	}
	ModuleObject->SetArrayField(TEXT("all_versions"), VersionValues);
}

void NiagaraCommonUtils::AppendModuleVersionsForUsage(
	FVersionedNiagaraEmitterData& EmitterData,
	ENiagaraScriptUsage Usage,
	const FString& Filter,
	TArray<TSharedPtr<FJsonValue>>& OutModules)
{
	UNiagaraGraph* Graph = GetGraphForUsage(&EmitterData, Usage);
	if (!Graph)
	{
		return;
	}

	const FString UsageName = ScriptUsageToString(Usage);
	TArray<UNiagaraNodeFunctionCall*> FunctionNodes;
	Graph->GetNodesOfClass<UNiagaraNodeFunctionCall>(FunctionNodes);

	for (UNiagaraNodeFunctionCall* FunctionNode : FunctionNodes)
	{
		if (!FunctionNode)
		{
			continue;
		}

		const FString ModuleName = FunctionNode->GetFunctionName();
		if (!MatchesFilter(ModuleName, Filter))
		{
			continue;
		}

		TSharedPtr<FJsonObject> ModuleObject = MakeShared<FJsonObject>();
		ModuleObject->SetStringField(TEXT("name"), ModuleName);
		ModuleObject->SetStringField(TEXT("script_usage"), UsageName);
		PopulateModuleVersionFields(ModuleObject, FunctionNode->FunctionScript);
		OutModules.Add(MakeShared<FJsonValueObject>(ModuleObject));
	}
}

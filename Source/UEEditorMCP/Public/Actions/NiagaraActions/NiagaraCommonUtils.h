#pragma once

#include "CoreMinimal.h"
#include "NiagaraCommon.h"
#include "NiagaraTypes.h"
#include "EdGraph/EdGraphPin.h"

class AActor;
class ANiagaraActor;
class FProperty;
class FNiagaraSystemInstance;
class FNiagaraParameterHandle;
class FNiagaraScratchPadScriptViewModel;
class UClass;
class UEdGraphNode;
class UEnum;
class UNiagaraComponent;
class UNiagaraDataInterfaceColorCurve;
class UNiagaraDataInterfaceCurve;
class UNiagaraDataInterfaceVectorCurve;
class UNiagaraEmitter;
class UNiagaraGraph;
class UNiagaraNode;
class UNiagaraNodeCustomHlsl;
class UNiagaraNodeFunctionCall;
class UNiagaraNodeInput;
class UNiagaraNodeOp;
class UNiagaraNodeOutput;
class UNiagaraNodeParameterMapSet;
class UNiagaraRendererProperties;
class UNiagaraScript;
class UNiagaraScriptVariable;
class UNiagaraSystem;
class UNiagaraSystemEditorData;
class UWorld;
class UStruct;
struct FNiagaraEmitterHandle;
class FNiagaraRendererFeedback;
struct FNiagaraVariableWithOffset;
struct FVersionedNiagaraEmitterData;

/**
 * Static utility class providing common helpers for Niagara MCP actions.
 * All methods are static; construction is deleted.
 */
class NiagaraCommonUtils
{
public:
	NiagaraCommonUtils() = delete;

	/**
	 * Load a UNiagaraSystem asset by path.
	 * @param AssetPath  Asset path (with or without trailing ".AssetName").
	 * @param OutError   Receives an error message if the asset cannot be found.
	 * @return Pointer to the loaded system, or nullptr on failure.
	 */
	static UNiagaraSystem* LoadNiagaraSystem(const FString& AssetPath, FString& OutError);

	/**
	 * Load a UNiagaraSystem from a JSON params object that contains an "asset_path" field.
	 * @param Params    JSON object expected to contain "asset_path".
	 * @param OutError  Receives an error message on failure.
	 * @return Pointer to the loaded system, or nullptr on failure.
	 */
	static UNiagaraSystem* LoadSystemFromParams(const TSharedPtr<FJsonObject>& Params, FString& OutError);

	/**
	 * Find an emitter handle inside a Niagara system by name or "Emitter[N]" index notation.
	 * @param NiagaraSystem  The system to search.
	 * @param EmitterName    Emitter name or "Emitter[N]" index string.
	 * @param OutIndex       Receives the handle index on success.
	 * @param OutError       Receives an error message on failure.
	 * @return Pointer to the matching handle, or nullptr on failure.
	 */
	static FNiagaraEmitterHandle* FindEmitterHandle(
		UNiagaraSystem* NiagaraSystem,
		const FString& EmitterName,
		int32& OutIndex,
		FString& OutError);

	/**
	 * Get the versioned emitter data from an emitter handle.
	 * @param Handle  Pointer to the emitter handle.
	 * @return Pointer to the versioned data, or nullptr if Handle is null.
	 */
	static FVersionedNiagaraEmitterData* GetEmitterData(const FNiagaraEmitterHandle* Handle);

	/**
	 * Parse a script usage string (e.g. "ParticleUpdate") into the corresponding enum value.
	 * @param Usage        The usage string to parse.
	 * @param bOutSuccess  Set to true if parsing succeeded, false otherwise.
	 * @return The parsed ENiagaraScriptUsage value.
	 */
	static ENiagaraScriptUsage ParseScriptUsage(const FString& Usage, bool& bOutSuccess);

	/**
	 * Convert an ENiagaraScriptUsage enum value to its string representation.
	 * @param Usage  The enum value to convert.
	 * @return The string name of the usage.
	 */
	static FString ScriptUsageToString(const ENiagaraScriptUsage& Usage);

	/**
	 * Convert an ENiagaraExecutionState to a human-readable string.
	 * @param State  The execution state enum.
	 * @return String representation of the state.
	 */
	static FString ExecutionStateToString(ENiagaraExecutionState State);

	/**
	 * Check whether a candidate name matches a name filter (case-insensitive, wildcard-aware).
	 * @param Candidate  The name to test.
	 * @param Filter     The filter pattern.
	 * @return True if the name matches.
	 */
	static bool MatchesNameFilter(const FString& Candidate, const FString& Filter);

	/**
	 * Check whether a filter matches any of the provided field values.
	 * @param Filter  The filter pattern.
	 * @param Fields  Array of field values to test against.
	 * @return True if at least one field matches the filter.
	 */
	static bool MatchesAnyFilter(const FString& Filter, const TArray<FString>& Fields);

	/**
	 * Get the Niagara graph associated with a specific script usage from emitter data.
	 * @param EmitterData  The versioned emitter data.
	 * @param Usage        The script usage to look up.
	 * @return Pointer to the graph, or nullptr if not found.
	 */
	static UNiagaraGraph* GetGraphForUsage(const FVersionedNiagaraEmitterData* EmitterData, const ENiagaraScriptUsage& Usage);

	/**
	 * Find the output node for a given script usage within a graph.
	 * @param Graph  The Niagara graph to search.
	 * @param Usage  The script usage whose output node is needed.
	 * @return Pointer to the output node, or nullptr if not found.
	 */
	static UNiagaraNodeOutput* GetOutputNodeForUsage(const UNiagaraGraph* Graph, const ENiagaraScriptUsage& Usage);

	/**
	 * Find a module function-call node by name in the first available usage graph.
	 * @param Graph       The Niagara graph to search.
	 * @param ModuleName  Name of the module to find.
	 * @param OutError    Receives an error message on failure.
	 * @return Pointer to the function-call node, or nullptr on failure.
	 */
	static UNiagaraNodeFunctionCall* FindModuleNode(const UNiagaraGraph* Graph, const FString& ModuleName, FString& OutError);

	/**
	 * Find a module function-call node by name within a specific script usage graph.
	 * @param Graph       The Niagara graph to search.
	 * @param Usage       The script usage scope.
	 * @param ModuleName  Name of the module to find.
	 * @param OutError    Receives an error message on failure.
	 * @return Pointer to the function-call node, or nullptr on failure.
	 */
	static UNiagaraNodeFunctionCall* FindModuleNode(
		const UNiagaraGraph* Graph,
		ENiagaraScriptUsage Usage,
		const FString& ModuleName,
		FString& OutError);

	/**
	 * Find a Niagara actor in the current editor world by display name.
	 * @param Name      The actor name to search for.
	 * @param OutError  Receives an error message if not found.
	 * @return Pointer to the actor, or nullptr on failure.
	 */
	static ANiagaraActor* FindNiagaraActorByName(const FString& Name, FString& OutError);

	/**
	 * Find any actor in a world by name.
	 * @param World  The world to search.
	 * @param Name   The actor name to search for.
	 * @return Pointer to the actor, or nullptr if not found.
	 */
	static AActor* FindActorByName(UWorld* World, const FString& Name);

	/**
	 * Compile a Niagara system and synchronize its emitters.
	 * @param NiagaraSystem  The system to compile.
	 * @param bForce         If true, force recompilation even if already up-to-date.
	 */
	static void CompileAndSync(UNiagaraSystem* NiagaraSystem, bool bForce = false);

	/**
	 * Serialize an FVector to a JSON object with "x", "y", "z" fields.
	 * @param Value  The vector to serialize.
	 * @return JSON object representing the vector.
	 */
	static TSharedPtr<FJsonObject> VectorToJson(const FVector& Value);

	/**
	 * Serialize an FRotator to a JSON object with "pitch", "yaw", "roll" fields.
	 * @param Value  The rotator to serialize.
	 * @return JSON object representing the rotator.
	 */
	static TSharedPtr<FJsonObject> RotatorToJson(const FRotator& Value);

	/**
	 * Find a scratch-pad script by module name within a Niagara system.
	 * @param System      The Niagara system.
	 * @param ModuleName  The scratch-pad module name.
	 * @return Pointer to the script, or nullptr if not found.
	 */
	static UNiagaraScript* FindScratchPadScript(UNiagaraSystem* System, const FString& ModuleName);

	/**
	 * Check whether a scratch-pad script name already exists on the system.
	 * @param System      The Niagara system to inspect.
	 * @param ModuleName  The scratch-pad module name to check.
	 * @return True if the name conflicts with an existing scratch-pad script or child object.
	 */
	static bool HasScratchPadScriptNameConflict(UNiagaraSystem* System, const FString& ModuleName);

	/**
	 * Notify the system that a scratch-pad script has changed, triggering recompilation.
	 * @param System      The Niagara system.
	 * @param ModuleName  The scratch-pad module name.
	 */
	static void NotifyScratchPadScriptChanged(UNiagaraSystem* System, const FString& ModuleName);

	/**
	 * Convert an emitter handle and its index to a JSON object.
	 * @param Handle  The emitter handle.
	 * @param Index   The handle index in the system's handle array.
	 * @return JSON object with emitter metadata.
	 */
	static TSharedPtr<FJsonObject> EmitterHandleToJson(
		const FNiagaraEmitterHandle& Handle,
		int32 Index);

	/**
	 * Build a JSON array of emitter info objects for a system, optionally filtered by name.
	 * @param System  The Niagara system.
	 * @param Filter  Name filter; "all" or empty returns all emitters.
	 * @return Array of JSON emitter info objects.
	 */
	static TArray<TSharedPtr<FJsonValue>> BuildEmitterInfoArray(
		UNiagaraSystem* System,
		const FString& Filter);

	/**
	 * Build a JSON array of parameter info objects for a system, optionally filtered.
	 * @param System  The Niagara system.
	 * @param Filter  Name filter; "all" or empty returns all parameters.
	 * @return Array of JSON parameter info objects.
	 */
	static TArray<TSharedPtr<FJsonValue>> BuildParameterInfoArray(
		UNiagaraSystem* System,
		const FString& Filter);

	/**
	 * Serialize a module function-call node to a JSON object.
	 * @param Node            The function-call node.
	 * @param Index           Index of the node in its graph.
	 * @param Usage           The script usage context.
	 * @param bIncludeInputs  If true, include input pin details.
	 * @return JSON object representing the module node.
	 */
	static TSharedPtr<FJsonObject> ModuleNodeToJson(
		UNiagaraNodeFunctionCall* Node,
		int32 Index,
		ENiagaraScriptUsage Usage,
		bool bIncludeInputs);

	/**
	 * Serialize a script parameter (variable with index) to a JSON object.
	 * @param Var    The Niagara variable.
	 * @param Index  The parameter index.
	 * @return JSON object with parameter details.
	 */
	static TSharedPtr<FJsonObject> ScriptParameterToJson(const FNiagaraVariable& Var, int32 Index);

	/**
	 * Serialize a renderer properties object to a JSON object.
	 * @param Renderer  The renderer properties.
	 * @param Index     The renderer index.
	 * @return JSON object with renderer details.
	 */
	static TSharedPtr<FJsonObject> RendererToJson(UNiagaraRendererProperties* Renderer, int32 Index);

	/**
	 * Build a JSON object from an FBox with "min" and "max" vectors.
	 * @param Bounds  The bounding box.
	 * @return JSON object representing the bounds.
	 */
	static TSharedPtr<FJsonObject> BuildBoundsObject(const FBox& Bounds);

	/**
	 * Serialize all fields of a UStruct to a JSON object.
	 * @param Struct             The struct type to serialize.
	 * @param Container          Pointer to the struct instance (may be null).
	 * @param Filter             Optional name filter for fields; empty includes all.
	 * @param bIncludeInherited  If true, include fields from parent structs.
	 * @param Depth              Current recursion depth.
	 * @return JSON object with the struct's fields.
	 */
	static TSharedPtr<FJsonObject> SerializeStructFields(
		const UStruct* Struct,
		const void* Container = nullptr,
		const FString& Filter = FString(),
		bool bIncludeInherited = true,
		int32 Depth = 0);

	/**
	 * Serialize a UEnum to a JSON object with its name and enumerators.
	 * @param Enum  The enum type.
	 * @return JSON object representing the enum.
	 */
	static TSharedPtr<FJsonObject> SerializeEnum(const UEnum* Enum);

	/**
	 * Serialize a Niagara data interface class to a JSON object with its properties.
	 * @param DIClass  The data interface UClass.
	 * @return JSON object with class name and property details.
	 */
	static TSharedPtr<FJsonObject> SerializeDataInterfaceClass(UClass* DIClass);

	/**
	 * Serialize a FNiagaraTypeDefinition to a JSON object.
	 * @param TypeDef  The type definition.
	 * @return JSON object with type name and underlying struct info.
	 */
	static TSharedPtr<FJsonObject> SerializeNiagaraType(const FNiagaraTypeDefinition& TypeDef);

	/**
	 * Resolve a type name string to a FNiagaraTypeDefinition.
	 * @param TypeName  The type name to resolve (e.g. "float", "Vector3f").
	 * @param OutType   Receives the resolved type definition.
	 * @return True if the type name was resolved successfully.
	 */
	static bool ResolveTypeName(const FString& TypeName, FNiagaraTypeDefinition& OutType);

	/**
	 * Convert a JSON value to a pin default string suitable for a given type.
	 * @param JsonValue        The source JSON value.
	 * @param TypeDef          The Niagara type the value should conform to.
	 * @param OutDefaultValue  Receives the formatted default string.
	 * @param OutError         Receives an error message on failure.
	 * @return True if the conversion succeeded.
	 */
	static bool JsonValueToPinDefaultString(
		const TSharedPtr<FJsonValue>& JsonValue,
		const FNiagaraTypeDefinition& TypeDef,
		FString& OutDefaultValue,
		FString& OutError);

	/**
	 * Parse a type string (e.g. "float", "/Script/Niagara.NiagaraFloat") into a type definition.
	 * @param TypeString  The type string to parse.
	 * @param OutType     Receives the parsed type definition.
	 * @return True if parsing succeeded.
	 */
	static bool ParseTypeDef(const FString& TypeString, FNiagaraTypeDefinition& OutType);

	/**
	 * Convert a FNiagaraTypeDefinition to a human-readable string.
	 * @param TypeDef  The type definition.
	 * @return String representation of the type.
	 */
	static FString TypeDefToString(const FNiagaraTypeDefinition& TypeDef);

	/**
	 * Check whether a pin is an "Add Pin" placeholder used by dynamic-pin nodes.
	 * @param Pin  The pin to check.
	 * @return True if the pin is an add-pin placeholder.
	 */
	static bool IsAddPin(const UEdGraphPin* Pin);

	/**
	 * Resolve a type string to a NiagaraTypeDefinition, falling back to a default type.
	 * @param TypeString  The type string to resolve.
	 * @return The resolved type definition, or a default if unresolved.
	 */
	static FNiagaraTypeDefinition ResolveNiagaraTypeOrDefault(const FString& TypeString);

	/**
	 * Add a typed pin to a Niagara node.
	 * @param Node               The target node.
	 * @param Direction          Input or output.
	 * @param Type               The Niagara type for the pin.
	 * @param InName             The pin name.
	 * @param bRebuildSignature  If true, rebuild the node's signature after adding the pin.
	 * @return Pointer to the newly created pin.
	 */
	static UEdGraphPin* AddTypedPin(
		UNiagaraNode* Node,
		EEdGraphPinDirection Direction,
		const FNiagaraTypeDefinition& Type,
		const FName& InName,
		bool bRebuildSignature = true);

	/**
	 * Rename a dynamic pin on a Niagara node.
	 * @param Node      The node owning the pin.
	 * @param OldName   Current pin name.
	 * @param NewName   Desired new name.
	 * @param OutError  Receives an error message on failure.
	 * @return True if the rename succeeded.
	 */
	static bool RenameDynamicPin(
		UNiagaraNode* Node,
		const FName& OldName,
		const FName& NewName,
		FString& OutError);

	/**
	 * Remove a dynamic pin by name from a Niagara node.
	 * @param Node      The node owning the pin.
	 * @param PinName   Name of the pin to remove.
	 * @param OutError  Receives an error message on failure.
	 * @return True if the pin was removed.
	 */
	static bool RemoveDynamicPinByName(
		UNiagaraNode* Node,
		const FName& PinName,
		FString& OutError);

	/**
	 * Set custom HLSL code on a UNiagaraNodeCustomHlsl node via reflection.
	 * @param HlslNode  The custom HLSL node.
	 * @param HlslCode  The HLSL source code to set.
	 * @return True if the code was set successfully.
	 */
	static bool SetCustomHlslViaReflection(UNiagaraNode* HlslNode, const FString& HlslCode);

	/**
	 * Walk a dotted module path (e.g. "ModuleA.SubModule.InputName") through nested function-call nodes.
	 * @param InitialCall       The starting function-call node.
	 * @param DotPath           The dotted path to traverse.
	 * @param OutLeafInputName  Receives the final input name at the leaf.
	 * @param OutError          Receives an error message on failure.
	 * @return Pointer to the leaf function-call node, or nullptr on failure.
	 */
	static UNiagaraNodeFunctionCall* DescendNestedPath(
		UNiagaraNodeFunctionCall& InitialCall,
		const FString& DotPath,
		FString& OutLeafInputName,
		FString& OutError);

	/**
	 * Parse a scratch-pad module type string into an ENiagaraScriptUsage.
	 * @param ModuleType  The module type string (e.g. "ParticleUpdate").
	 * @return The corresponding script usage enum.
	 */
	static ENiagaraScriptUsage ParseScratchPadModuleType(const FString& ModuleType);

	/**
	 * Load the default scratch-pad template script for a given usage.
	 * @param Usage  The script usage.
	 * @return Pointer to the template script, or nullptr if not found.
	 */
	static UNiagaraScript* LoadDefaultScratchPadTemplateScript(ENiagaraScriptUsage Usage);

	/**
	 * Build a minimal scratch-pad graph structure on a script and graph.
	 * @param Script  The target script.
	 * @param Graph   The target graph.
	 * @param Usage   The script usage to set up.
	 */
	static void BuildMinimalScratchGraph(UNiagaraScript* Script, UNiagaraGraph* Graph, ENiagaraScriptUsage Usage);

	/**
	 * Collect all custom HLSL nodes from scratch-pad graphs for a given module.
	 * @param System      The Niagara system.
	 * @param ModuleName  The scratch-pad module name.
	 * @param OutNodes    Receives the collected HLSL nodes.
	 */
	static void CollectScratchPadHlslNodes(
		UNiagaraSystem* System,
		const FString& ModuleName,
		TArray<UNiagaraNodeCustomHlsl*>& OutNodes);

	/**
	 * Serialize a scratch-pad script to a JSON object with its graph and HLSL details.
	 * @param Script  The scratch-pad script.
	 * @param Index   The script index.
	 * @return JSON object representing the scratch-pad script.
	 */
	static TSharedPtr<FJsonObject> ScratchPadScriptToJson(UNiagaraScript* Script, int32 Index);

	/**
	 * Get the scratch-pad script view model for a module.
	 * @param System      The Niagara system.
	 * @param ModuleName  The scratch-pad module name.
	 * @return Shared pointer to the view model, or null if not found.
	 */
	static TSharedPtr<FNiagaraScratchPadScriptViewModel> GetScratchPadScriptViewModel(
		UNiagaraSystem* System,
		const FString& ModuleName);

	/**
	 * Resolve scratch-pad HLSL nodes from a JSON params object.
	 * @param Params         JSON params with system path and module name.
	 * @param OutSystem      Receives the resolved Niagara system.
	 * @param OutModuleName  Receives the resolved module name.
	 * @param OutNodes       Receives the resolved HLSL nodes.
	 * @param OutError       Receives an error message on failure.
	 * @return True if resolution succeeded.
	 */
	static bool ResolveScratchPadHlslNodes(
		const TSharedPtr<FJsonObject>& Params,
		UNiagaraSystem*& OutSystem,
		FString& OutModuleName,
		TArray<UNiagaraNodeCustomHlsl*>& OutNodes,
		FString& OutError);

	/**
	 * Resolve scratch-pad graphs from a JSON params object.
	 * @param Params         JSON params with system path and module name.
	 * @param OutSystem      Receives the resolved Niagara system.
	 * @param OutModuleName  Receives the resolved module name.
	 * @param OutGraphs      Receives the resolved graphs.
	 * @param OutError       Receives an error message on failure.
	 * @return True if resolution succeeded.
	 */
	static bool ResolveScratchPadGraphs(
		const TSharedPtr<FJsonObject>& Params,
		UNiagaraSystem*& OutSystem,
		FString& OutModuleName,
		TArray<UNiagaraGraph*>& OutGraphs,
		FString& OutError);

	/**
	 * Resolve a single scratch-pad graph from a JSON params object.
	 * @param Params     JSON params with system path.
	 * @param OutSystem  Receives the resolved Niagara system.
	 * @param OutError   Receives an error message on failure.
	 * @return Pointer to the resolved graph, or nullptr on failure.
	 */
	static UNiagaraGraph* ResolveScratchPadGraph(
		const TSharedPtr<FJsonObject>& Params,
		UNiagaraSystem*& OutSystem,
		FString& OutError);

	/**
	 * Resolve a scratch-pad script and its graph from a JSON params object.
	 * @param Params         JSON params with system path and module name.
	 * @param OutSystem      Receives the resolved Niagara system.
	 * @param OutModuleName  Receives the resolved module name.
	 * @param OutScript      Receives the resolved script.
	 * @param OutGraph       Receives the resolved graph.
	 * @param OutError       Receives an error message on failure.
	 * @return True if resolution succeeded.
	 */
	static bool ResolveScratchPadScriptAndGraph(
		const TSharedPtr<FJsonObject>& Params,
		UNiagaraSystem*& OutSystem,
		FString& OutModuleName,
		UNiagaraScript*& OutScript,
		UNiagaraGraph*& OutGraph,
		FString& OutError);

	/**
	 * Collect all mutation graphs for a module, including from the direct script.
	 * @param System        The Niagara system.
	 * @param ModuleName    The module name.
	 * @param DirectScript  An optional script to include directly.
	 * @return Array of collected mutation graphs.
	 */
	static TArray<UNiagaraGraph*> CollectMutationGraphs(
		UNiagaraSystem* System,
		const FString& ModuleName,
		UNiagaraScript* DirectScript);

	/**
	 * Find the first output node in a Niagara graph.
	 * @param Graph  The graph to search.
	 * @return Pointer to the first output node, or nullptr if none exists.
	 */
	static UNiagaraNodeOutput* FindFirstOutputNode(UNiagaraGraph* Graph);

	/**
	 * Ensure that a script variable has proper metadata in the graph's editor data.
	 * @param Graph        The Niagara graph.
	 * @param Var          The variable to ensure metadata for.
	 * @param DefaultMode  The default mode to set if metadata is missing.
	 */
	static void EnsureScriptVariableMetadata(
		UNiagaraGraph* Graph,
		const FNiagaraVariable& Var,
		ENiagaraDefaultMode DefaultMode = ENiagaraDefaultMode::Value);

	/**
	 * Determine the appropriate default mode based on a parameter name's namespace.
	 * @param ParamName  The fully-qualified parameter name.
	 * @return The recommended default mode.
	 */
	static ENiagaraDefaultMode DefaultModeForNamespace(const FString& ParamName);

	/**
	 * Validate that a JSON params object has all required fields for reading a graph.
	 * @param Params    JSON params to validate.
	 * @param OutError  Receives an error message on failure.
	 * @return True if all required fields are present.
	 */
	static bool ValidateReadableGraphParams(const TSharedPtr<FJsonObject>& Params, FString& OutError);

	/**
	 * Validate that a JSON params object has all required fields for mutation graph operations.
	 * @param Params    JSON params to validate.
	 * @param OutError  Receives an error message on failure.
	 * @return True if all required fields are present.
	 */
	static bool ValidateMutationGraphParams(const TSharedPtr<FJsonObject>& Params, FString& OutError);

	/**
	 * Validate that a JSON params object has all required fields for scratch-pad graph operations.
	 * @param Params    JSON params to validate.
	 * @param OutError  Receives an error message on failure.
	 * @return True if all required fields are present.
	 */
	static bool ValidateScratchPadGraphParams(const TSharedPtr<FJsonObject>& Params, FString& OutError);

	/**
	 * Validate a node selector (node_index or node_guid) in a JSON params object.
	 * @param Params    JSON params to validate.
	 * @param OutError  Receives an error message on failure.
	 * @return True if a valid node selector is present.
	 */
	static bool ValidateNodeSelector(const TSharedPtr<FJsonObject>& Params, FString& OutError);

	/**
	 * Validate a side node selector with a given prefix (e.g. "source_" or "target_").
	 * @param Params    JSON params to validate.
	 * @param Prefix    The prefix for the selector fields.
	 * @param OutError  Receives an error message on failure.
	 * @return True if a valid side node selector is present.
	 */
	static bool ValidateSideNodeSelector(
		const TSharedPtr<FJsonObject>& Params,
		const TCHAR* Prefix,
		FString& OutError);

	/**
	 * Resolve a readable graph from JSON params, returning the associated script.
	 * @param Params         JSON params with system/emitter/usage info.
	 * @param OutScript      Receives the resolved script.
	 * @param OutSourceDesc  Receives a human-readable source description.
	 * @param OutError       Receives an error message on failure.
	 * @return Pointer to the resolved graph, or nullptr on failure.
	 */
	static UNiagaraGraph* ResolveReadableGraph(
		const TSharedPtr<FJsonObject>& Params,
		UNiagaraScript*& OutScript,
		FString& OutSourceDesc,
		FString& OutError);

	/**
	 * Resolve a specific graph node from JSON params by index or GUID.
	 * @param Graph     The graph to search.
	 * @param Params    JSON params with node_index or node_guid.
	 * @param OutError  Receives an error message on failure.
	 * @return Pointer to the resolved node, or nullptr on failure.
	 */
	static UNiagaraNode* ResolveGraphNode(
		UNiagaraGraph* Graph,
		const TSharedPtr<FJsonObject>& Params,
		FString& OutError);

	/**
	 * Get a short display class name for a graph node (e.g. "NiagaraNodeInput").
	 * @param Node  The graph node.
	 * @return Short class name string.
	 */
	static FString ShortNodeClassName(const UEdGraphNode* Node);

	/**
	 * Get a short type name for a UClass (strips the "UNiagara" prefix if present).
	 * @param Cls  The class.
	 * @return Short type name string.
	 */
	static FString ShortNodeTypeName(const UClass* Cls);

	/**
	 * Get a human-readable display title for a graph node.
	 * @param Node  The graph node.
	 * @return Display title string.
	 */
	static FString NodeDisplayTitle(UEdGraphNode* Node);

	/**
	 * Build a map from graph nodes to sequential indices.
	 * @param Graph   The Niagara graph.
	 * @param OutMap  Receives the node-to-index mapping.
	 */
	static void BuildNodeIndexMap(UNiagaraGraph* Graph, TMap<UEdGraphNode*, int32>& OutMap);

	/**
	 * Gather class metadata (display name, tooltip, category, keywords) from a UClass.
	 * @param Cls             The class to inspect.
	 * @param OutDisplayName  Receives the display name.
	 * @param OutTooltip      Receives the tooltip text.
	 * @param OutCategory     Receives the category string.
	 * @param OutKeywords     Receives the keywords string.
	 */
	static void GatherClassMeta(
		UClass* Cls,
		FString& OutDisplayName,
		FString& OutTooltip,
		FString& OutCategory,
		FString& OutKeywords);

	/**
	 * Find a Niagara node class by short or full class name.
	 * @param ShortOrFullName  The class name to look up.
	 * @return Pointer to the UClass, or nullptr if not found.
	 */
	static UClass* FindNiagaraNodeClass(const FString& ShortOrFullName);

	/**
	 * Build a JSON schema describing custom HLSL node inputs and outputs.
	 * @return JSON object with the schema.
	 */
	static TSharedPtr<FJsonObject> BuildCustomHlslSchema();

	/**
	 * Serialize a single graph pin to a JSON object (without links or defaults).
	 * @param Pin  The pin to serialize.
	 * @return JSON object with pin name, type, and direction.
	 */
	static TSharedPtr<FJsonObject> SerializeNodePin(const UEdGraphPin* Pin);

	/**
	 * Serialize a graph node to a JSON object with position, class, and optionally pins.
	 * @param Node          The graph node.
	 * @param NodeIndex     The node's index in the graph.
	 * @param NodeIndexMap  Map from nodes to indices (for link resolution).
	 * @param Verbosity     Detail level: "minimal", "connections", or "full".
	 * @return JSON object representing the node.
	 */
	static TSharedPtr<FJsonObject> SerializeGraphNode(
		UEdGraphNode* Node,
		int32 NodeIndex,
		const TMap<UEdGraphNode*, int32>& NodeIndexMap,
		const FString& Verbosity);

	/**
	 * Create a temporary Niagara graph configured for a specific script usage.
	 * @param Usage  The script usage.
	 * @return Pointer to the newly created temporary graph.
	 */
	static UNiagaraGraph* CreateTemporaryGraphForUsage(ENiagaraScriptUsage Usage);

	/**
	 * Create an op introspection node in a graph for a given operation name.
	 * @param Graph   The target graph.
	 * @param OpName  The operation name.
	 * @return Pointer to the created op node.
	 */
	static UNiagaraNodeOp* CreateOpIntrospectionNode(UNiagaraGraph* Graph, const FName& OpName);

	/**
	 * Serialize a function script's input and output pins to JSON arrays.
	 * @param Script      The function script.
	 * @param OutInputs   Receives the serialized input pins.
	 * @param OutOutputs  Receives the serialized output pins.
	 */
	static void SerializeFunctionScriptPins(
		UNiagaraScript* Script,
		TArray<TSharedPtr<FJsonValue>>& OutInputs,
		TArray<TSharedPtr<FJsonValue>>& OutOutputs);

	/**
	 * Validate that all required string parameters are present in a JSON object.
	 * @param Params          The JSON params object.
	 * @param RequiredParams  List of required parameter names.
	 * @param OutError        Receives an error message listing the first missing param.
	 * @return True if all required params are present and non-empty.
	 */
	static bool ValidateRequiredStringParams(
		const TSharedPtr<FJsonObject>& Params,
		const TArray<FString>& RequiredParams,
		FString& OutError);

	/**
	 * Remove all connections from an override pin and clean up orphaned nodes.
	 * @param OverridePin  The pin to disconnect.
	 * @param Graph        The graph containing the pin.
	 */
	static void RemoveOverridePinConnections(UEdGraphPin& OverridePin, UNiagaraGraph* Graph);

	/**
	 * Populate a float curve data interface with keyframe data from JSON.
	 * @param CurveDI  The float curve data interface.
	 * @param Keys     JSON array of keyframe objects with "time" and "value".
	 * @return The number of keys added.
	 */
	static int32 PopulateFloatCurveDataInterface(
		UNiagaraDataInterfaceCurve* CurveDI,
		const TArray<TSharedPtr<FJsonValue>>& Keys);

	/**
	 * Populate a vector curve data interface with keyframe data from JSON.
	 * @param CurveDI  The vector curve data interface.
	 * @param Keys     JSON array of keyframe objects with "time" and "value" (vector).
	 * @return The number of keys added.
	 */
	static int32 PopulateVectorCurveDataInterface(
		UNiagaraDataInterfaceVectorCurve* CurveDI,
		const TArray<TSharedPtr<FJsonValue>>& Keys);

	/**
	 * Populate a color curve data interface with keyframe data from JSON.
	 * @param CurveDI  The color curve data interface.
	 * @param Keys     JSON array of keyframe objects with "time" and "value" (color).
	 * @return The number of keys added.
	 */
	static int32 PopulateColorCurveDataInterface(
		UNiagaraDataInterfaceColorCurve* CurveDI,
		const TArray<TSharedPtr<FJsonValue>>& Keys);

	/**
	 * Check whether an emitter handle refers to a stateless emitter.
	 * @param Handle  The emitter handle to check.
	 * @return True if the emitter is stateless.
	 */
	static bool IsStatelessEmitterHandle(const FNiagaraEmitterHandle* Handle);

	/**
	 * Collect the chain of module function-call nodes connected to an output node.
	 * @param OutputNode  The output node to start from.
	 * @param OutChain    Receives the ordered chain of function-call nodes.
	 */
	static void CollectModuleChain(
		UNiagaraNodeOutput* OutputNode,
		TArray<UNiagaraNodeFunctionCall*>& OutChain);

	/**
	 * Find the first pin of a given direction on a graph node.
	 * @param Node       The graph node.
	 * @param Direction  The desired pin direction.
	 * @return Pointer to the first matching pin, or nullptr if none exists.
	 */
	static UEdGraphPin* FindFirstPin(UEdGraphNode* Node, EEdGraphPinDirection Direction);

	/**
	 * Find the parameter-map-set node that provides input overrides for a module.
	 * @param StackFunctionCall  The module's function-call node in the emitter stack.
	 * @return Pointer to the override node, or nullptr if not found.
	 */
	static UNiagaraNodeParameterMapSet* FindModuleInputOverrideNode(UNiagaraNodeFunctionCall& StackFunctionCall);

	/**
	 * Find the specific override pin for a module input by its aliased parameter handle.
	 * @param StackFunctionCall  The module's function-call node.
	 * @param AliasedHandle      The parameter handle to look up.
	 * @return Pointer to the override pin, or nullptr if not found.
	 */
	static UEdGraphPin* FindModuleInputOverridePin(
		UNiagaraNodeFunctionCall& StackFunctionCall,
		const FNiagaraParameterHandle& AliasedHandle);

	/**
	 * Find a module function-call node by name in a graph.
	 * @param Graph       The Niagara graph.
	 * @param ModuleName  The module name to search for.
	 * @return Pointer to the function-call node, or nullptr if not found.
	 */
	static UNiagaraNodeFunctionCall* FindModuleFunctionCall(
		UNiagaraGraph* Graph,
		const FString& ModuleName);

	/**
	 * Resolve the emitter stack graph for a specific emitter and usage.
	 * @param System      The Niagara system.
	 * @param EmitterName  The emitter name.
	 * @param Usage        The script usage.
	 * @param OutScript    Receives the resolved script.
	 * @param OutError     Receives an error message on failure.
	 * @return Pointer to the resolved graph, or nullptr on failure.
	 */
	static UNiagaraGraph* ResolveEmitterStackGraph(
		UNiagaraSystem* System,
		const FString& EmitterName,
		ENiagaraScriptUsage Usage,
		UNiagaraScript*& OutScript,
		FString& OutError);

	/**
	 * Get the script associated with a specific usage from emitter data.
	 * @param EmitterData  The versioned emitter data.
	 * @param Usage        The script usage.
	 * @return Pointer to the script, or nullptr if not found.
	 */
	static UNiagaraScript* GetScriptForUsage(
		FVersionedNiagaraEmitterData* EmitterData,
		ENiagaraScriptUsage Usage);

	/**
	 * Serialize a rapid iteration parameter value from a parameter store into a JSON object.
	 * @param Store   The parameter store containing the value.
	 * @param Var     The variable with its offset in the store.
	 * @param OutObj  The JSON object to populate with the value.
	 */
	static void SerializeRapidIterationValue(
		const FNiagaraParameterStore& Store,
		const FNiagaraVariableWithOffset& Var,
		const TSharedRef<FJsonObject>& OutObj);

	/**
	 * Split an asset path into package path and asset name components.
	 * @param AssetPath       The full asset path (e.g. "/Game/Foo/Bar.Baz").
	 * @param OutPackagePath  Receives the package path portion.
	 * @param OutAssetName    Receives the asset name portion.
	 * @param OutError        Receives an error message on failure.
	 * @return True if the path was split successfully.
	 */
	static bool TrySplitAssetPath(
		const FString& AssetPath,
		FString& OutPackagePath,
		FString& OutAssetName,
		FString& OutError);

	/**
	 * Resolve a template name to its full asset path.
	 * @param TemplateName  The template name or path.
	 * @return The resolved full asset path.
	 */
	static FString ResolveTemplateReference(const FString& TemplateName);

	/**
	 * Copy a template Niagara system to a target system asset.
	 * @param TargetSystem  The destination system.
	 * @param TemplateName  The template to copy from.
	 * @return True if the copy succeeded.
	 */
	static bool TryCopyTemplateToSystem(UNiagaraSystem& TargetSystem, const FString& TemplateName);

	/**
	 * Collect external asset referencers for a given object path.
	 * @param ObjectPath  The asset path to find referencers for.
	 * @return Array of JSON string values with referencing asset paths.
	 */
	static TArray<TSharedPtr<FJsonValue>> CollectExternalReferencers(const FString& ObjectPath);

	/**
	 * Collect compile status information for all scripts in a Niagara system.
	 * @param System             The Niagara system.
	 * @param OutScriptStatuses  Receives JSON objects with script label and status.
	 * @param OutErrorCount      Receives the count of scripts with errors.
	 */
	static void CollectCompileStatuses(
		UNiagaraSystem* System,
		TArray<TSharedPtr<FJsonValue>>& OutScriptStatuses,
		int32& OutErrorCount);

	/**
	 * Resolve a requested system property name to its FProperty pointer.
	 * @param RequestedProperty  The property name from the request.
	 * @param ValueText          The value text (used for validation context).
	 * @return Pointer to the resolved property, or nullptr if not found.
	 */
	static FProperty* ResolveSystemProperty(const FString& RequestedProperty, const FString& ValueText);

	/**
	 * Build an error message for an unknown system property.
	 * @param RequestedProperty  The unrecognized property name.
	 * @param Filter             Optional filter context.
	 * @return Formatted error string.
	 */
	static FString BuildUnknownSystemPropertyError(
		const FString& RequestedProperty,
		const FString& Filter);

	/**
	 * Append compile-issue entries for an emitter to an output JSON array.
	 * @param EmitterData     The emitter data to check.
	 * @param EmitterName     The emitter name for labeling.
	 * @param SeverityFilter  Severity filter (e.g. "all", "error").
	 * @param OutIssues       The JSON array to append issues to.
	 */
	static void AppendCompileIssues(
		FVersionedNiagaraEmitterData& EmitterData,
		const FString& EmitterName,
		const FString& SeverityFilter,
		TArray<TSharedPtr<FJsonValue>>& OutIssues);

	/**
	 * Append renderer issue entries for an emitter to an output JSON array.
	 * @param Handle          The emitter handle.
	 * @param EmitterData     The emitter data.
	 * @param EmitterName     The emitter name for labeling.
	 * @param SeverityFilter  Severity filter.
	 * @param OutIssues       The JSON array to append issues to.
	 */
	static void AppendRendererIssues(
		const FNiagaraEmitterHandle& Handle,
		FVersionedNiagaraEmitterData& EmitterData,
		const FString& EmitterName,
		const FString& SeverityFilter,
		TArray<TSharedPtr<FJsonValue>>& OutIssues);

	/**
	 * Append system-level message issues to an output JSON array.
	 * @param System          The Niagara system.
	 * @param EmitterFilter   Emitter name filter; "all" includes all emitters.
	 * @param SeverityFilter  Severity filter.
	 * @param OutIssues       The JSON array to append issues to.
	 */
	static void AppendSystemMessageIssues(
		UNiagaraSystem* System,
		const FString& EmitterFilter,
		const FString& SeverityFilter,
		TArray<TSharedPtr<FJsonValue>>& OutIssues);

	/**
	 * Try to find a preview component and system instance for a Niagara system.
	 * @param System             The Niagara system.
	 * @param OutComponent       Receives the preview component.
	 * @param OutSystemInstance  Receives the system instance.
	 * @return True if both were found.
	 */
	static bool TryFindPreviewInstance(
		UNiagaraSystem* System,
		UNiagaraComponent*& OutComponent,
		FNiagaraSystemInstance*& OutSystemInstance);

	/**
	 * Get the editor data for a Niagara system.
	 * @param System    The Niagara system.
	 * @param OutError  Receives an error message on failure.
	 * @return Pointer to the editor data, or nullptr on failure.
	 */
	static UNiagaraSystemEditorData* GetSystemEditorData(UNiagaraSystem* System, FString& OutError);

	/**
	 * Apply a playback frame rate setting to the system editor data.
	 * @param EditorData      The system editor data.
	 * @param FrameRateValue  The frame rate value to set.
	 */
	static void ApplyPlaybackFrameRate(UNiagaraSystemEditorData* EditorData, int32 FrameRateValue);

	/**
	 * Format a Niagara asset version to a human-readable string (e.g. "1.2.3").
	 * @param Version  The asset version.
	 * @return Formatted version string.
	 */
	static FString FormatNiagaraAssetVersion(const FNiagaraAssetVersion& Version);

	/**
	 * Append module version info for a specific usage to an output JSON array.
	 * @param EmitterData  The emitter data.
	 * @param Usage        The script usage.
	 * @param Filter       Name filter for modules.
	 * @param OutModules   The JSON array to append to.
	 */
	static void AppendModuleVersionsForUsage(
		FVersionedNiagaraEmitterData& EmitterData,
		ENiagaraScriptUsage Usage,
		const FString& Filter,
		TArray<TSharedPtr<FJsonValue>>& OutModules);

	/**
	 * Pick the latest version from an array of asset versions, comparing major then minor.
	 * @param Versions  The array of candidate versions.
	 * @param Fallback  The fallback version if the array is empty.
	 * @return The latest version found, or Fallback if empty.
	 */
	static FNiagaraAssetVersion PickLatestVersion(
		const TArray<FNiagaraAssetVersion>& Versions,
		const FNiagaraAssetVersion& Fallback);
};

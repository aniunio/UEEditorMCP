#include "Actions/NiagaraActions/NiagaraGraphActions.h"

#include "MCPCommonUtils.h"
#include "Actions/NiagaraActions/NiagaraActions.h"
#include "Actions/NiagaraActions/NiagaraCommonUtils.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "EditorAssetLibrary.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraCommon.h"
#include "NiagaraDataInterface.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraGraph.h"
#include "NiagaraNode.h"
#include "NiagaraNodeCustomHlsl.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeOp.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeParameterMapGet.h"
#include "NiagaraNodeParameterMapSet.h"
#include "NiagaraNodeReroute.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraTypes.h"
#include "UObject/MetaData.h"
#include "UObject/UObjectIterator.h"

FString FGetNiagaraGraphNodesAction::GetActionName() const
{
	return TEXT("get_niagara_graph_nodes");
}

bool FGetNiagaraGraphNodesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	return NiagaraCommonUtils::ValidateReadableGraphParams(Params, OutError);
}

FString FGetNiagaraNodeInfoAction::GetActionName() const
{
	return TEXT("get_niagara_node_info");
}

bool FGetNiagaraNodeInfoAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	return NiagaraCommonUtils::ValidateReadableGraphParams(Params, OutError) &&
		NiagaraCommonUtils::ValidateNodeSelector(Params, OutError);
}

FString FTraceNiagaraConnectionAction::GetActionName() const
{
	return TEXT("trace_niagara_connection");
}

bool FTraceNiagaraConnectionAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	return NiagaraCommonUtils::ValidateReadableGraphParams(Params, OutError) &&
		NiagaraCommonUtils::ValidateNodeSelector(Params, OutError);
}

FString FValidateNiagaraGraphAction::GetActionName() const
{
	return TEXT("validate_niagara_graph");
}

bool FValidateNiagaraGraphAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	return NiagaraCommonUtils::ValidateReadableGraphParams(Params, OutError);
}

FString FGetNiagaraScriptPropertiesAction::GetActionName() const
{
	return TEXT("get_niagara_script_properties");
}

bool FGetNiagaraScriptPropertiesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Params;
	(void)Context;
	(void)OutError;
	return true;
}

FString FSetNiagaraScriptPropertiesAction::GetActionName() const
{
	return TEXT("set_niagara_script_properties");
}

bool FSetNiagaraScriptPropertiesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Params;
	(void)Context;
	(void)OutError;
	return true;
}

FString FAddNiagaraGraphNodeAction::GetActionName() const
{
	return TEXT("add_niagara_graph_node");
}

bool FAddNiagaraGraphNodeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString NodeType;
	if (!NiagaraCommonUtils::ValidateMutationGraphParams(Params, OutError))
	{
		return false;
	}
	if (!Params.IsValid() || !Params->TryGetStringField(TEXT("node_type"), NodeType) || NodeType.IsEmpty())
	{
		OutError = TEXT("Required parameter 'node_type' is missing or empty");
		return false;
	}
	NodeType.RemoveFromStart(TEXT("NiagaraNode"));
	if (NodeType.Equals(TEXT("Op"), ESearchCase::IgnoreCase))
	{
		FString Unused;
		return GetRequiredString(Params, TEXT("op_name"), Unused, OutError);
	}
	if (NodeType.Equals(TEXT("FunctionCall"), ESearchCase::IgnoreCase))
	{
		FString Unused;
		return GetRequiredString(Params, TEXT("function_script"), Unused, OutError);
	}
	if (NodeType.Equals(TEXT("Input"), ESearchCase::IgnoreCase))
	{
		FString Unused;
		return GetRequiredString(Params, TEXT("input_name"), Unused, OutError) &&
			GetRequiredString(Params, TEXT("input_type"), Unused, OutError);
	}
	if (NodeType.Equals(TEXT("DataInterfaceFunction"), ESearchCase::IgnoreCase))
	{
		FString Unused;
		return GetRequiredString(Params, TEXT("di_class"), Unused, OutError) &&
			GetRequiredString(Params, TEXT("function_name"), Unused, OutError);
	}
	return true;
}

FString FDeleteNiagaraGraphNodeAction::GetActionName() const
{
	return TEXT("delete_niagara_graph_node");
}

bool FDeleteNiagaraGraphNodeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	int32 NodeIndex = INDEX_NONE;
	FString NodeId;
	if (!NiagaraCommonUtils::ValidateMutationGraphParams(Params, OutError))
	{
		return false;
	}
	if ((Params.IsValid() && Params->TryGetNumberField(TEXT("node_index"), NodeIndex)) ||
		(Params.IsValid() && Params->TryGetStringField(TEXT("node_id"), NodeId) && !NodeId.IsEmpty()))
	{
		return true;
	}
	OutError = TEXT("Provide 'node_index' or 'node_id'");
	return false;
}

FString FListNiagaraDataInterfaceFunctionsAction::GetActionName() const
{
	return TEXT("list_niagara_data_interface_functions");
}

bool FListNiagaraDataInterfaceFunctionsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context,
                                                        FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("di_class"), Unused, OutError);
}

FString FListNiagaraOpsAction::GetActionName() const
{
	return TEXT("list_niagara_ops");
}

bool FListNiagaraOpsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Params;
	(void)Context;
	(void)OutError;
	return true;
}

FString FListNiagaraNodeTypesAction::GetActionName() const
{
	return TEXT("list_niagara_node_types");
}

bool FListNiagaraNodeTypesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Params;
	(void)Context;
	(void)OutError;
	return true;
}

FString FGetNiagaraNodeTypeInfoAction::GetActionName() const
{
	return TEXT("get_niagara_node_type_info");
}

bool FGetNiagaraNodeTypeInfoAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("type"), Unused, OutError);
}

FString FSearchNiagaraFunctionsAction::GetActionName() const
{
	return TEXT("search_niagara_functions");
}

bool FSearchNiagaraFunctionsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Params;
	(void)Context;
	(void)OutError;
	return true;
}

FString FDescribeNiagaraTypeAction::GetActionName() const
{
	return TEXT("describe_niagara_type");
}

bool FDescribeNiagaraTypeAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("type"), Unused, OutError);
}

FString FGetNiagaraSchemaActionsAction::GetActionName() const
{
	return TEXT("get_niagara_schema_actions");
}

bool FGetNiagaraSchemaActionsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	return NiagaraCommonUtils::ValidateScratchPadGraphParams(Params, OutError);
}

FString FListNiagaraAvailableParametersAction::GetActionName() const
{
	return TEXT("list_niagara_available_parameters");
}

bool FListNiagaraAvailableParametersAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Params;
	(void)Context;
	(void)OutError;
	return true;
}

FString FAddNiagaraMapGetPinAction::GetActionName() const
{
	return TEXT("add_niagara_map_get_pin");
}

bool FAddNiagaraMapGetPinAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return NiagaraCommonUtils::ValidateScratchPadGraphParams(Params, OutError) &&
		GetRequiredString(Params, TEXT("parameter_name"), Unused, OutError);
}

FString FAddNiagaraMapSetPinAction::GetActionName() const
{
	return TEXT("add_niagara_map_set_pin");
}

bool FAddNiagaraMapSetPinAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return NiagaraCommonUtils::ValidateScratchPadGraphParams(Params, OutError) &&
		GetRequiredString(Params, TEXT("parameter_name"), Unused, OutError);
}

FString FAddNiagaraNodePinAction::GetActionName() const
{
	return TEXT("add_niagara_node_pin");
}

bool FAddNiagaraNodePinAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return NiagaraCommonUtils::ValidateScratchPadGraphParams(Params, OutError) &&
		NiagaraCommonUtils::ValidateNodeSelector(Params, OutError) &&
		GetRequiredString(Params, TEXT("pin_name"), Unused, OutError);
}

FString FRenameNiagaraNodePinAction::GetActionName() const
{
	return TEXT("rename_niagara_node_pin");
}

bool FRenameNiagaraNodePinAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return NiagaraCommonUtils::ValidateScratchPadGraphParams(Params, OutError) &&
		NiagaraCommonUtils::ValidateNodeSelector(Params, OutError) &&
		GetRequiredString(Params, TEXT("old_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("new_name"), Unused, OutError);
}

FString FRemoveNiagaraNodePinAction::GetActionName() const
{
	return TEXT("remove_niagara_node_pin");
}

bool FRemoveNiagaraNodePinAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return NiagaraCommonUtils::ValidateScratchPadGraphParams(Params, OutError) &&
		NiagaraCommonUtils::ValidateNodeSelector(Params, OutError) &&
		GetRequiredString(Params, TEXT("pin_name"), Unused, OutError);
}

FString FConnectNiagaraPinsAction::GetActionName() const
{
	return TEXT("connect_niagara_pins");
}

bool FConnectNiagaraPinsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!NiagaraCommonUtils::ValidateScratchPadGraphParams(Params, OutError) ||
		!GetRequiredString(Params, TEXT("from_pin"), Unused, OutError) ||
		!GetRequiredString(Params, TEXT("to_pin"), Unused, OutError))
	{
		return false;
	}
	return NiagaraCommonUtils::ValidateSideNodeSelector(Params, TEXT("from"), OutError) &&
		NiagaraCommonUtils::ValidateSideNodeSelector(Params, TEXT("to"), OutError);
}

FString FDisconnectNiagaraPinsAction::GetActionName() const
{
	return TEXT("disconnect_niagara_pins");
}

bool FDisconnectNiagaraPinsAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return NiagaraCommonUtils::ValidateScratchPadGraphParams(Params, OutError) &&
		NiagaraCommonUtils::ValidateNodeSelector(Params, OutError) &&
		GetRequiredString(Params, TEXT("pin_name"), Unused, OutError);
}

FString FGetNiagaraDataInterfaceSchemaAction::GetActionName() const
{
	return TEXT("get_niagara_data_interface_schema");
}


TSharedPtr<FJsonObject> FGetNiagaraScriptPropertiesAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	(void)Params;
	return CreateSuccessResponse();
}

TSharedPtr<FJsonObject> FSetNiagaraScriptPropertiesAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	(void)Params;
	return CreateSuccessResponse();
}

bool FGetNiagaraDataInterfaceSchemaAction::Validate(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context,
	FString& OutError)
{
	(void)Context;
	FString ClassName = GetOptionalString(Params, TEXT("class_"));
	if (ClassName.IsEmpty())
	{
		ClassName = GetOptionalString(Params, TEXT("class"));
	}
	if (ClassName.IsEmpty())
	{
		OutError = TEXT("Required parameter 'class_' or 'class' is missing");
		return false;
	}
	return true;
}


// ---------------------------------------------------------------------------
// Graph introspection helpers.
// These actions mirror the material-graph read tools for Niagara scratch-pad,
// dynamic-input, and module-script graphs. They support filtered listing,
// deep node inspection, connection tracing, and graph validation.
// Read operations accept either system_path + module_name for scratch-pad
// resolution or script_path for a standalone UNiagaraScript asset.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// FGetNiagaraGraphNodesAction::ExecuteInternal
// Parameters:
// - system_path + module_name, or script_path
// - verbosity: summary, connections (default), or full
// - type_filter: optional case-insensitive node-class filter
// - name_filter: optional case-insensitive node-title filter
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FGetNiagaraGraphNodesAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Error;
	UNiagaraScript* Script = nullptr;
	FString SourceDesc;
	UNiagaraGraph* Graph = NiagaraCommonUtils::ResolveReadableGraph(Params, Script, SourceDesc, Error);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString Verbosity = TEXT("connections");
	Params->TryGetStringField(TEXT("verbosity"), Verbosity);

	FString TypeFilter;
	Params->TryGetStringField(TEXT("type_filter"), TypeFilter);

	FString NameFilter;
	Params->TryGetStringField(TEXT("name_filter"), NameFilter);

	TMap<UEdGraphNode*, int32> NodeIndexMap;
	NiagaraCommonUtils::BuildNodeIndexMap(Graph, NodeIndexMap);

	TArray<TSharedPtr<FJsonValue>> NodesArr;
	int32 SkippedByFilter = 0;
	for (int32 i = 0; i < Graph->Nodes.Num(); ++i)
	{
		UEdGraphNode* Node = Graph->Nodes[i];
		if (!Node)
		{
			continue;
		}

		if (!TypeFilter.IsEmpty())
		{
			const FString Short = NiagaraCommonUtils::ShortNodeClassName(Node);
			if (!Short.Contains(TypeFilter, ESearchCase::IgnoreCase))
			{
				++SkippedByFilter;
				continue;
			}
		}
		if (!NameFilter.IsEmpty())
		{
			const FString Title = NiagaraCommonUtils::NodeDisplayTitle(Node);
			if (!Title.Contains(NameFilter, ESearchCase::IgnoreCase))
			{
				++SkippedByFilter;
				continue;
			}
		}

		NodesArr.Add(MakeShared<FJsonValueObject>(
			NiagaraCommonUtils::SerializeGraphNode(Node, i, NodeIndexMap, Verbosity)));
	}

	auto R = MakeShared<FJsonObject>();
	R->SetBoolField(TEXT("success"), true);
	R->SetStringField(TEXT("source"), SourceDesc);
	R->SetNumberField(TEXT("total_nodes"), Graph->Nodes.Num());
	R->SetNumberField(TEXT("returned"), NodesArr.Num());
	R->SetNumberField(TEXT("filtered_out"), SkippedByFilter);
	R->SetStringField(TEXT("verbosity"), Verbosity);
	R->SetArrayField(TEXT("nodes"), NodesArr);
	return R;
}


// ---------------------------------------------------------------------------
// FGetNiagaraNodeInfoAction::ExecuteInternal
// Parameters:
// - graph resolver parameters
// - node_index, node_class, or node_id
// Returns full node serialization, including inputs, outputs, and both link
// directions.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FGetNiagaraNodeInfoAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Error;
	UNiagaraScript* Script = nullptr;
	FString SourceDesc;
	UNiagaraGraph* Graph = NiagaraCommonUtils::ResolveReadableGraph(Params, Script, SourceDesc, Error);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	TMap<UEdGraphNode*, int32> NodeIndexMap;
	NiagaraCommonUtils::BuildNodeIndexMap(Graph, NodeIndexMap);

	// Resolve by one of: node_index, node_class (first match), node_id (GUID).
	UEdGraphNode* Target = nullptr;
	int32 TargetIndex = INDEX_NONE;

	int32 NodeIndex = INDEX_NONE;
	if (Params->TryGetNumberField(TEXT("node_index"), NodeIndex))
	{
		if (!Graph->Nodes.IsValidIndex(NodeIndex) || !Graph->Nodes[NodeIndex])
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("node_index %d out of range (have %d nodes)"),
				                NodeIndex, Graph->Nodes.Num()));
		}
		Target = Graph->Nodes[NodeIndex];
		TargetIndex = NodeIndex;
	}
	else
	{
		FString NodeClassStr;
		FString NodeIdStr;
		const bool bHaveClass = Params->TryGetStringField(TEXT("node_class"), NodeClassStr);
		const bool bHaveGuid = Params->TryGetStringField(TEXT("node_id"), NodeIdStr);

		FGuid TargetGuid;
		const bool bGuidValid = bHaveGuid && FGuid::Parse(NodeIdStr, TargetGuid);

		for (int32 i = 0; i < Graph->Nodes.Num(); ++i)
		{
			UEdGraphNode* N = Graph->Nodes[i];
			if (!N)
			{
				continue;
			}

			if (bGuidValid && N->NodeGuid == TargetGuid)
			{
				Target = N;
				TargetIndex = i;
				break;
			}
			if (bHaveClass)
			{
				const FString Short = NiagaraCommonUtils::ShortNodeClassName(N);
				const FString Full = N->GetClass()->GetName();
				if (Short.Equals(NodeClassStr, ESearchCase::IgnoreCase) ||
					Full.Equals(NodeClassStr, ESearchCase::IgnoreCase))
				{
					Target = N;
					TargetIndex = i;
					break;
				}
			}
		}

		if (!Target)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Node not found — provide node_index, node_class (short or full), or node_id"));
		}
	}

	TSharedPtr<FJsonObject> NodeJson =
		NiagaraCommonUtils::SerializeGraphNode(Target, TargetIndex, NodeIndexMap, TEXT("full"));

	auto R = MakeShared<FJsonObject>();
	R->SetBoolField(TEXT("success"), true);
	R->SetStringField(TEXT("source"), SourceDesc);
	R->SetObjectField(TEXT("node"), NodeJson);
	return R;
}


// ---------------------------------------------------------------------------
// FTraceNiagaraConnectionAction::ExecuteInternal
// Parameters:
// - graph resolver parameters plus node_index, node_class, or node_id
// - direction: upstream, downstream, or both (default)
// - max_depth: traversal depth limit, default 8
// - pin_name: optional starting-pin filter
// Returns an ordered list of visited nodes with traversal depth.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FTraceNiagaraConnectionAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Error;
	UNiagaraScript* Script = nullptr;
	FString SourceDesc;
	UNiagaraGraph* Graph = NiagaraCommonUtils::ResolveReadableGraph(Params, Script, SourceDesc, Error);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	TMap<UEdGraphNode*, int32> NodeIndexMap;
	NiagaraCommonUtils::BuildNodeIndexMap(Graph, NodeIndexMap);

	// Resolve starting node (reuse lookup by index first, else by class/id).
	UEdGraphNode* Start = nullptr;
	int32 StartIndex = INDEX_NONE;
	int32 NodeIndex = INDEX_NONE;
	if (Params->TryGetNumberField(TEXT("node_index"), NodeIndex) &&
		Graph->Nodes.IsValidIndex(NodeIndex))
	{
		Start = Graph->Nodes[NodeIndex];
		StartIndex = NodeIndex;
	}
	if (!Start)
	{
		FString ClassStr, IdStr;
		const bool bC = Params->TryGetStringField(TEXT("node_class"), ClassStr);
		const bool bI = Params->TryGetStringField(TEXT("node_id"), IdStr);
		FGuid TargetGuid;
		const bool bGuidValid = bI && FGuid::Parse(IdStr, TargetGuid);
		for (int32 i = 0; i < Graph->Nodes.Num(); ++i)
		{
			UEdGraphNode* N = Graph->Nodes[i];
			if (!N)
			{
				continue;
			}
			if (bGuidValid && N->NodeGuid == TargetGuid)
			{
				Start = N;
				StartIndex = i;
				break;
			}
			if (bC && (NiagaraCommonUtils::ShortNodeClassName(N).Equals(ClassStr, ESearchCase::IgnoreCase) ||
				N->GetClass()->GetName().Equals(ClassStr, ESearchCase::IgnoreCase)))
			{
				Start = N;
				StartIndex = i;
				break;
			}
		}
	}
	if (!Start)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Starting node not found — provide node_index, node_class, or node_id"));
	}

	FString Direction = TEXT("both");
	Params->TryGetStringField(TEXT("direction"), Direction);
	const bool bUp = Direction.Equals(TEXT("upstream"), ESearchCase::IgnoreCase) ||
		Direction.Equals(TEXT("both"), ESearchCase::IgnoreCase);
	const bool bDown = Direction.Equals(TEXT("downstream"), ESearchCase::IgnoreCase) ||
		Direction.Equals(TEXT("both"), ESearchCase::IgnoreCase);

	int32 MaxDepth = 8;
	Params->TryGetNumberField(TEXT("max_depth"), MaxDepth);

	FString PinFilter;
	Params->TryGetStringField(TEXT("pin_name"), PinFilter);

	// BFS: (node, depth).
	struct FVisit
	{
		UEdGraphNode* Node;
		int32 Depth;
		EEdGraphPinDirection Flow;
	};
	TArray<FVisit> Frontier;
	TSet<UEdGraphNode*> Visited;
	Visited.Add(Start);

	auto EnqueueLinked = [&](UEdGraphNode* Node, EEdGraphPinDirection From, int32 Depth)
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (NiagaraCommonUtils::IsAddPin(Pin))
			{
				continue;
			}
			if (Pin->Direction != From)
			{
				continue;
			}
			if (!PinFilter.IsEmpty() && Node == Start &&
				!Pin->PinName.ToString().Contains(PinFilter, ESearchCase::IgnoreCase))
			{
				continue;
			}
			for (UEdGraphPin* Other : Pin->LinkedTo)
			{
				if (!Other || !Other->GetOwningNodeUnchecked())
				{
					continue;
				}
				UEdGraphNode* Next = Other->GetOwningNode();
				if (Visited.Contains(Next))
				{
					continue;
				}
				Visited.Add(Next);
				Frontier.Add({Next, Depth + 1, From});
			}
		}
	};

	if (bUp)
	{
		EnqueueLinked(Start, EGPD_Input, 0);
	}
	if (bDown)
	{
		EnqueueLinked(Start, EGPD_Output, 0);
	}

	TArray<TSharedPtr<FJsonValue>> Visits;
	for (int32 i = 0; i < Frontier.Num(); ++i)
	{
		const FVisit V = Frontier[i];
		auto Entry = MakeShared<FJsonObject>();
		const int32* IdxPtr = NodeIndexMap.Find(V.Node);
		Entry->SetNumberField(TEXT("index"), IdxPtr ? *IdxPtr : -1);
		Entry->SetNumberField(TEXT("depth"), V.Depth);
		Entry->SetStringField(TEXT("direction"),
		                      V.Flow == EGPD_Input ? TEXT("upstream") : TEXT("downstream"));
			Entry->SetStringField(TEXT("type"), NiagaraCommonUtils::ShortNodeClassName(V.Node));
			Entry->SetStringField(TEXT("title"), NiagaraCommonUtils::NodeDisplayTitle(V.Node));
		Visits.Add(MakeShared<FJsonValueObject>(Entry));

		if (V.Depth < MaxDepth)
		{
			EnqueueLinked(V.Node, V.Flow, V.Depth);
		}
	}

	auto R = MakeShared<FJsonObject>();
	R->SetBoolField(TEXT("success"), true);
	R->SetStringField(TEXT("source"), SourceDesc);
	R->SetNumberField(TEXT("start_index"), StartIndex);
	R->SetStringField(TEXT("start_title"), NiagaraCommonUtils::NodeDisplayTitle(Start));
	R->SetStringField(TEXT("direction"), Direction);
	R->SetNumberField(TEXT("max_depth"), MaxDepth);
	R->SetNumberField(TEXT("visited"), Visits.Num());
	R->SetArrayField(TEXT("nodes"), Visits);
	return R;
}


// ---------------------------------------------------------------------------
// FValidateNiagaraGraphAction::ExecuteInternal
// Parameters: graph resolver parameters.
// Returns a classification of orphaned, dead-end, and disconnected nodes.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FValidateNiagaraGraphAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Error;
	UNiagaraScript* Script = nullptr;
	FString SourceDesc;
	UNiagaraGraph* Graph = NiagaraCommonUtils::ResolveReadableGraph(Params, Script, SourceDesc, Error);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	TArray<TSharedPtr<FJsonValue>> Orphans; // No in + no out
	TArray<TSharedPtr<FJsonValue>> DeadEnds; // Has inputs connected, no outputs linked
	TArray<TSharedPtr<FJsonValue>> Disconnected; // Missing required input pins

	for (int32 i = 0; i < Graph->Nodes.Num(); ++i)
	{
		UEdGraphNode* N = Graph->Nodes[i];
		if (!N)
		{
			continue;
		}

		// Skip anchor nodes that always terminate. They are supposed to have
		// no downstream consumers in the same graph.
		const bool bIsAnchor =
			N->IsA(UNiagaraNodeOutput::StaticClass()) ||
			N->IsA(UNiagaraNodeInput::StaticClass());

		int32 ConnectedIn = 0, ConnectedOut = 0, MissingRequiredIn = 0;
		for (UEdGraphPin* Pin : N->Pins)
		{
			if (NiagaraCommonUtils::IsAddPin(Pin))
			{
				continue;
			}
			// Skip the dynamic "+Add" placeholder pin on nodes supporting
			// added inputs (UNiagaraNodeWithDynamicPins). Its name is
			// literally "Add" and its PinCategory is "Misc" - it is a UI
			// affordance, not a functional input, and counting it as a
			// missing input is a false positive.
			if (Pin->PinName == TEXT("Add") &&
				Pin->PinType.PinCategory == TEXT("Misc"))
			{
				continue;
			}
			const bool bHas = Pin->LinkedTo.Num() > 0;
			if (Pin->Direction == EGPD_Input)
			{
				if (bHas)
				{
					++ConnectedIn;
				}
				else if (Pin->DefaultValue.IsEmpty() && Pin->AutogeneratedDefaultValue.IsEmpty())
				{
					++MissingRequiredIn;
				}
			}
			else if (bHas)
			{
				++ConnectedOut;
			}
		}

		auto Entry = [&]()
		{
			auto E = MakeShared<FJsonObject>();
			E->SetNumberField(TEXT("index"), i);
			E->SetStringField(TEXT("type"), NiagaraCommonUtils::ShortNodeClassName(N));
			E->SetStringField(TEXT("title"), NiagaraCommonUtils::NodeDisplayTitle(N));
			return E;
		};

		if (!bIsAnchor && ConnectedIn == 0 && ConnectedOut == 0)
		{
			Orphans.Add(MakeShared<FJsonValueObject>(Entry()));
			continue;
		}
		if (!bIsAnchor && ConnectedIn > 0 && ConnectedOut == 0)
		{
			// Output node acts as sink so only non-Output classes count here.
			DeadEnds.Add(MakeShared<FJsonValueObject>(Entry()));
		}
		if (MissingRequiredIn > 0)
		{
			TSharedPtr<FJsonObject> E = Entry();
			E->SetNumberField(TEXT("missing_input_count"), MissingRequiredIn);
			Disconnected.Add(MakeShared<FJsonValueObject>(E));
		}
	}

	auto R = MakeShared<FJsonObject>();
	R->SetBoolField(TEXT("success"), true);
	R->SetStringField(TEXT("source"), SourceDesc);
	R->SetNumberField(TEXT("total_nodes"), Graph->Nodes.Num());
	R->SetArrayField(TEXT("orphaned"), Orphans);
	R->SetArrayField(TEXT("dead_ends"), DeadEnds);
	R->SetArrayField(TEXT("missing_inputs"), Disconnected);
	R->SetBoolField(TEXT("graph_clean"),
	                Orphans.Num() == 0 && DeadEnds.Num() == 0 && Disconnected.Num() == 0);
	return R;
}


// ---------------------------------------------------------------------------
// Local helpers.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// FListNiagaraOpsAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FListNiagaraOpsAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	FString CategoryFilter;
	Params->TryGetStringField(TEXT("category"), CategoryFilter);

	FString ExactName;
	Params->TryGetStringField(TEXT("exact_name"), ExactName);

	bool bIncludePins = true;
	Params->TryGetBoolField(TEXT("include_pins"), bIncludePins);

	int32 MaxResults = 500;
	Params->TryGetNumberField(TEXT("max_results"), MaxResults);

	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
	if (!Schema)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Niagara schema unavailable"));
	}

	UNiagaraGraph* TempGraph = NiagaraCommonUtils::CreateTemporaryGraphForUsage(ENiagaraScriptUsage::Module);
	if (!TempGraph)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create temporary Niagara graph"));
	}

	const TArray<TSharedPtr<FNiagaraAction_NewNode>> Actions =
		Schema->GetGraphActions(TempGraph, nullptr, TempGraph);
	if (Actions.Num() == 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Niagara schema returned no graph actions"));
	}

	TArray<TSharedPtr<FJsonValue>> OpsJson;
	TSet<FString> Categories;
	int32 TotalOps = 0;
	for (const TSharedPtr<FNiagaraAction_NewNode>& Action : Actions)
	{
		if (!Action.IsValid())
		{
			continue;
		}

		const UNiagaraNodeOp* OpTemplate = Cast<UNiagaraNodeOp>(Action->WeakNodeTemplate.Get());
		if (!OpTemplate)
		{
			continue;
		}

		++TotalOps;

		const FString Name = OpTemplate->OpName.ToString();
		const FString FriendlyName = Action->DisplayName.ToString();
		const FString Category =
			Action->Categories.Num() > 0 ? Action->Categories[0] : FString();
		const FString Description = Action->ToolTip.ToString();
		const FString Keywords = Action->Keywords.ToString();
		const FString AlternateName = Action->AlternateSearchName.IsSet()
			                              ? Action->AlternateSearchName.GetValue().ToString()
			                              : FString();
		const FString CompactName = OpTemplate->GetCompactTitle().ToString();

		if (!Category.IsEmpty())
		{
			Categories.Add(Category);
		}

		if (!ExactName.IsEmpty() && !Name.Equals(ExactName, ESearchCase::IgnoreCase))
		{
			continue;
		}

		if (!CategoryFilter.IsEmpty() && !Category.Equals(CategoryFilter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		if (!Filter.IsEmpty())
		{
			const bool bMatches = NiagaraCommonUtils::MatchesAnyFilter(
				Filter,
				{ Name, FriendlyName, Category, Description, Keywords, AlternateName, CompactName });
			if (!bMatches)
			{
				continue;
			}
		}

		auto Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("name"), Name);
		if (!FriendlyName.IsEmpty())
		{
			Entry->SetStringField(TEXT("friendly_name"), FriendlyName);
		}
		if (!Category.IsEmpty())
		{
			Entry->SetStringField(TEXT("category"), Category);
		}
		if (!Description.IsEmpty())
		{
			Entry->SetStringField(TEXT("description"), Description);
		}
		if (!Keywords.IsEmpty())
		{
			Entry->SetStringField(TEXT("keywords"), Keywords);
		}
		if (!AlternateName.IsEmpty())
		{
			Entry->SetStringField(TEXT("alternate_name"), AlternateName);
		}
		if (!CompactName.IsEmpty())
		{
			Entry->SetStringField(TEXT("compact_name"), CompactName);
		}
		Entry->SetBoolField(
			TEXT("show_pin_names_in_compact_mode"),
			const_cast<UNiagaraNodeOp*>(OpTemplate)->ShouldShowPinNamesInCompactMode());

		if (bIncludePins)
		{
			UNiagaraNodeOp* IntrospectionNode =
			NiagaraCommonUtils::CreateOpIntrospectionNode(TempGraph, OpTemplate->OpName);
			TArray<TSharedPtr<FJsonValue>> InputsJson;
			TArray<TSharedPtr<FJsonValue>> OutputsJson;
			if (IntrospectionNode)
			{
				for (UEdGraphPin* Pin : IntrospectionNode->Pins)
				{
					if (!Pin || Pin->PinName == NAME_None)
					{
						continue;
					}

					TSharedPtr<FJsonObject> PinObj = NiagaraCommonUtils::SerializeNodePin(Pin);
					if (Pin->Direction == EGPD_Input)
					{
						InputsJson.Add(MakeShared<FJsonValueObject>(PinObj));
					}
					else
					{
						OutputsJson.Add(MakeShared<FJsonValueObject>(PinObj));
					}
				}
			}
			Entry->SetArrayField(TEXT("inputs"), InputsJson);
			Entry->SetArrayField(TEXT("outputs"), OutputsJson);
		}

		OpsJson.Add(MakeShared<FJsonValueObject>(Entry));
		if (OpsJson.Num() >= MaxResults)
		{
			break;
		}
	}

	TArray<FString> SortedCategories = Categories.Array();
	SortedCategories.Sort();
	TArray<TSharedPtr<FJsonValue>> CategoriesJson;
	for (const FString& Category : SortedCategories)
	{
		CategoriesJson.Add(MakeShared<FJsonValueString>(Category));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("filter"), Filter);
	Result->SetStringField(TEXT("category_filter"), CategoryFilter);
	Result->SetStringField(TEXT("exact_name"), ExactName);
	Result->SetNumberField(TEXT("returned"), OpsJson.Num());
	Result->SetNumberField(TEXT("total_ops"), TotalOps);
	Result->SetBoolField(TEXT("include_pins"), bIncludePins);
	Result->SetArrayField(TEXT("all_categories"), CategoriesJson);
	Result->SetArrayField(TEXT("ops"), OpsJson);
	return Result;
}

// ---------------------------------------------------------------------------
// FListNiagaraNodeTypesAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FListNiagaraNodeTypesAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	FString KindFilter = TEXT("all");
	Params->TryGetStringField(TEXT("kind"), KindFilter);
	const FString KindLower = KindFilter.ToLower();

	bool bIncludeEngine = false;
	Params->TryGetBoolField(TEXT("include_engine"), bIncludeEngine);

	int32 MaxResults = 500;
	Params->TryGetNumberField(TEXT("max_results"), MaxResults);

	const bool bWantNodeClass = (KindLower == TEXT("all") || KindLower == TEXT("node_class"));
	const bool bWantModule = (KindLower == TEXT("all") || KindLower == TEXT("module_script"));
	const bool bWantDynIn = (KindLower == TEXT("all") || KindLower == TEXT("dynamic_input_script"));
	const bool bWantFunc = (KindLower == TEXT("all") || KindLower == TEXT("function_script"));
	const bool bWantDI = (KindLower == TEXT("all") || KindLower == TEXT("data_interface"));

	TArray<TSharedPtr<FJsonValue>> Results;

	// Collect UNiagaraNode subclasses.
	if (bWantNodeClass)
	{
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (Results.Num() >= MaxResults)
			{
				break;
			}
			UClass* Cls = *It;
			if (!Cls->IsChildOf<UNiagaraNode>())
			{
				continue;
			}
			if (Cls->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_HideDropDown))
			{
				continue;
			}

			FString DisplayName, Tooltip, Category, Keywords;
			NiagaraCommonUtils::GatherClassMeta(Cls, DisplayName, Tooltip, Category, Keywords);
			const FString ShortName = NiagaraCommonUtils::ShortNodeTypeName(Cls);

			if (!NiagaraCommonUtils::MatchesAnyFilter(Filter, { ShortName, DisplayName, Category, Keywords, Tooltip }))
			{
				continue;
			}

			auto Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("kind"), TEXT("node_class"));
			Entry->SetStringField(TEXT("type"), ShortName);
			Entry->SetStringField(TEXT("class_path"), Cls->GetPathName());
			Entry->SetStringField(TEXT("display_name"), DisplayName);
			if (!Category.IsEmpty())
			{
				Entry->SetStringField(TEXT("category"), Category);
			}
			if (!Tooltip.IsEmpty())
			{
				Entry->SetStringField(TEXT("tooltip"), Tooltip);
			}
			if (!Keywords.IsEmpty())
			{
				Entry->SetStringField(TEXT("keywords"), Keywords);
			}
			Results.Add(MakeShared<FJsonValueObject>(Entry));
		}
	}

	// Collect Niagara script assets by usage.
	if (bWantModule || bWantDynIn || bWantFunc)
	{
		IAssetRegistry& AssetRegistry =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

		FARFilter ARFilter;
		ARFilter.ClassPaths.Add(UNiagaraScript::StaticClass()->GetClassPathName());
		ARFilter.bRecursivePaths = true;
		if (!bIncludeEngine)
		{
			ARFilter.PackagePaths.Add(FName(TEXT("/Game")));
			ARFilter.PackagePaths.Add(FName(TEXT("/Niagara")));
		}

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssets(ARFilter, Assets);

		for (const FAssetData& Asset : Assets)
		{
			if (Results.Num() >= MaxResults)
			{
				break;
			}

			// Usage is exposed through the asset-registry "Usage" tag.
			FString UsageStr;
			Asset.GetTagValue(TEXT("Usage"), UsageStr);

			bool bWanted = false;
			FString KindLabel;
			if (bWantModule && UsageStr.Contains(TEXT("Module")))
			{
				bWanted = true;
				KindLabel = TEXT("module_script");
			}
			else if (bWantDynIn && UsageStr.Contains(TEXT("DynamicInput")))
			{
				bWanted = true;
				KindLabel = TEXT("dynamic_input_script");
			}
			else if (bWantFunc && UsageStr.Contains(TEXT("Function")))
			{
				bWanted = true;
				KindLabel = TEXT("function_script");
			}

			// Fall back to loading the asset when the usage tag is missing.
			if (!bWanted && UsageStr.IsEmpty())
			{
				UNiagaraScript* Script = Cast<UNiagaraScript>(Asset.GetAsset());
				if (!Script)
				{
					continue;
				}
				const ENiagaraScriptUsage Usage = Script->GetUsage();
				if (bWantModule && Usage == ENiagaraScriptUsage::Module)
				{
					bWanted = true;
					KindLabel = TEXT("module_script");
				}
				else if (bWantDynIn && Usage == ENiagaraScriptUsage::DynamicInput)
				{
					bWanted = true;
					KindLabel = TEXT("dynamic_input_script");
				}
				else if (bWantFunc && Usage == ENiagaraScriptUsage::Function)
				{
					bWanted = true;
					KindLabel = TEXT("function_script");
				}
			}
			if (!bWanted)
			{
				continue;
			}

			const FString AssetName = Asset.AssetName.ToString();
			const FString AssetPath = Asset.GetObjectPathString();
			const FString Category = FPackageName::GetLongPackagePath(Asset.PackageName.ToString());
			const FString Empty;

		if (!NiagaraCommonUtils::MatchesAnyFilter(Filter, { AssetName, AssetPath, Category, Empty, Empty }))
			{
				continue;
			}

			auto Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("kind"), KindLabel);
			Entry->SetStringField(TEXT("name"), AssetName);
			Entry->SetStringField(TEXT("script_path"), AssetPath);
			Entry->SetStringField(TEXT("category"), Category);
			Results.Add(MakeShared<FJsonValueObject>(Entry));
		}
	}

	// Collect UNiagaraDataInterface subclasses.
	if (bWantDI)
	{
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (Results.Num() >= MaxResults)
			{
				break;
			}
			UClass* Cls = *It;
			if (!Cls->IsChildOf<UNiagaraDataInterface>())
			{
				continue;
			}
			if (Cls->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
			{
				continue;
			}

			FString DisplayName, Tooltip, Category, Keywords;
			NiagaraCommonUtils::GatherClassMeta(Cls, DisplayName, Tooltip, Category, Keywords);
			const FString ShortName = Cls->GetName();

			if (!NiagaraCommonUtils::MatchesAnyFilter(Filter, { ShortName, DisplayName, Category, Keywords, Tooltip }))
			{
				continue;
			}

			auto Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("kind"), TEXT("data_interface"));
			Entry->SetStringField(TEXT("type"), ShortName);
			Entry->SetStringField(TEXT("class_path"), Cls->GetPathName());
			Entry->SetStringField(TEXT("display_name"), DisplayName);
			if (!Category.IsEmpty())
			{
				Entry->SetStringField(TEXT("category"), Category);
			}
			if (!Tooltip.IsEmpty())
			{
				Entry->SetStringField(TEXT("tooltip"), Tooltip);
			}
			Results.Add(MakeShared<FJsonValueObject>(Entry));
		}
	}

	auto Out = MakeShared<FJsonObject>();
	Out->SetBoolField(TEXT("success"), true);
	Out->SetStringField(TEXT("filter"), Filter);
	Out->SetStringField(TEXT("kind"), KindFilter);
	Out->SetNumberField(TEXT("count"), Results.Num());
	Out->SetArrayField(TEXT("nodes"), Results);
	return Out;
}

// ---------------------------------------------------------------------------
// FGetNiagaraNodeTypeInfoAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FGetNiagaraNodeTypeInfoAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString TypeName;
	if (!Params->TryGetStringField(TEXT("type"), TypeName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
	}
	FString ScriptAssetPath;
	Params->TryGetStringField(TEXT("script_path"), ScriptAssetPath);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("type"), TypeName);

	// Case A: script-asset information.
	if (!ScriptAssetPath.IsEmpty())
	{
		UNiagaraScript* Script = LoadObject<UNiagaraScript>(nullptr, *ScriptAssetPath);
		if (!Script)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Failed to load script at '%s'"), *ScriptAssetPath));
		}
		Result->SetStringField(TEXT("kind"), TEXT("script"));
		Result->SetStringField(TEXT("script_path"), Script->GetPathName());
		Result->SetStringField(TEXT("usage"), NiagaraCommonUtils::ScriptUsageToString(Script->GetUsage()));

		TArray<TSharedPtr<FJsonValue>> InputsJson;
		TArray<TSharedPtr<FJsonValue>> OutputsJson;
		NiagaraCommonUtils::SerializeFunctionScriptPins(Script, InputsJson, OutputsJson);
		Result->SetArrayField(TEXT("inputs"), InputsJson);
		Result->SetArrayField(TEXT("outputs"), OutputsJson);
		return Result;
	}

	// Case B: node-class information.
	UClass* NodeClass = NiagaraCommonUtils::FindNiagaraNodeClass(TypeName);
	if (!NodeClass)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unknown Niagara node type '%s'"), *TypeName));
	}
	Result->SetStringField(TEXT("kind"), TEXT("node_class"));
	Result->SetStringField(TEXT("class_path"), NodeClass->GetPathName());

	FString DisplayName, Tooltip, Category, Keywords;
	NiagaraCommonUtils::GatherClassMeta(NodeClass, DisplayName, Tooltip, Category, Keywords);
	Result->SetStringField(TEXT("display_name"), DisplayName);
	if (!Category.IsEmpty())
	{
		Result->SetStringField(TEXT("category"), Category);
	}
	if (!Tooltip.IsEmpty())
	{
		Result->SetStringField(TEXT("tooltip"), Tooltip);
	}
	if (!Keywords.IsEmpty())
	{
		Result->SetStringField(TEXT("keywords"), Keywords);
	}

	// Special case: Custom HLSL schema.
	if (NodeClass == UNiagaraNodeCustomHlsl::StaticClass())
	{
		Result->SetObjectField(TEXT("custom_hlsl_schema"), NiagaraCommonUtils::BuildCustomHlslSchema());
		Result->SetStringField(TEXT("note"),
		                       TEXT(
			                       "Use set_niagara_scratch_pad_hlsl with inputs/outputs to (re)build pins, or add/rename/remove per-pin via add_niagara_custom_hlsl_input / rename_niagara_custom_hlsl_pin / remove_niagara_custom_hlsl_pin."));
		return Result;
	}

	// Generic reflection: list exposed UPROPERTY fields that can be set on an instance.
	TArray<TSharedPtr<FJsonValue>> PropsJson;
	for (TFieldIterator<FProperty> PropIt(NodeClass); PropIt; ++PropIt)
	{
		FProperty* Prop = *PropIt;
		if (!Prop->HasAnyPropertyFlags(CPF_Edit))
		{
			continue;
		}
		auto P = MakeShared<FJsonObject>();
		P->SetStringField(TEXT("name"), Prop->GetName());
		P->SetStringField(TEXT("type"), Prop->GetCPPType());
		if (Prop->HasMetaData(TEXT("ToolTip")))
		{
			P->SetStringField(TEXT("tooltip"), Prop->GetMetaData(TEXT("ToolTip")));
		}
		PropsJson.Add(MakeShared<FJsonValueObject>(P));
	}
	Result->SetArrayField(TEXT("properties"), PropsJson);
	return Result;
}

// ---------------------------------------------------------------------------
// FSearchNiagaraFunctionsAction::ExecuteInternal
// Searches Niagara script assets by usage and name filter.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FSearchNiagaraFunctionsAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	FString UsageFilter = TEXT("function");
	Params->TryGetStringField(TEXT("usage"), UsageFilter);

	int32 MaxResults = 100;
	Params->TryGetNumberField(TEXT("max_results"), MaxResults);
	bool bIncludeEngine = true;
	Params->TryGetBoolField(TEXT("include_engine"), bIncludeEngine);

	bool bUsageOk = false;
	ENiagaraScriptUsage TargetUsage = NiagaraCommonUtils::ParseScriptUsage(UsageFilter, bUsageOk);
	if (!bUsageOk)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Invalid usage '%s'"), *UsageFilter));
	}

	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	FARFilter ARFilter;
	ARFilter.ClassPaths.Add(UNiagaraScript::StaticClass()->GetClassPathName());
	ARFilter.bRecursivePaths = true;
	if (!bIncludeEngine)
	{
		ARFilter.PackagePaths.Add(FName(TEXT("/Game")));
	}

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(ARFilter, Assets);

	TArray<TSharedPtr<FJsonValue>> Results;
	for (const FAssetData& Asset : Assets)
	{
		if (Results.Num() >= MaxResults)
		{
			break;
		}

		FString UsageStr;
		Asset.GetTagValue(TEXT("Usage"), UsageStr);

		const FString NeedUsage = NiagaraCommonUtils::ScriptUsageToString(TargetUsage);
		if (!UsageStr.Contains(NeedUsage, ESearchCase::IgnoreCase))
		{
			// Fall back to loading for untagged assets
			UNiagaraScript* Script = Cast<UNiagaraScript>(Asset.GetAsset());
			if (!Script || Script->GetUsage() != TargetUsage)
			{
				continue;
			}
		}

		const FString AssetName = Asset.AssetName.ToString();
		const FString AssetPath = Asset.GetObjectPathString();
		if (!Filter.IsEmpty() &&
			!AssetName.Contains(Filter, ESearchCase::IgnoreCase) &&
			!AssetPath.Contains(Filter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		auto J = MakeShared<FJsonObject>();
		J->SetStringField(TEXT("name"), AssetName);
		J->SetStringField(TEXT("script_path"), AssetPath);
		J->SetStringField(TEXT("usage"), NeedUsage);
		Results.Add(MakeShared<FJsonValueObject>(J));
	}

	auto Out = MakeShared<FJsonObject>();
	Out->SetBoolField(TEXT("success"), true);
	Out->SetStringField(TEXT("filter"), Filter);
	Out->SetStringField(TEXT("usage"), UsageFilter);
	Out->SetNumberField(TEXT("count"), Results.Num());
	Out->SetArrayField(TEXT("functions"), Results);
	return Out;
}

// ---------------------------------------------------------------------------
// FGetNiagaraSchemaActionsAction::ExecuteInternal
// Calls UEdGraphSchema_Niagara::GetGraphActions for a scratch-pad graph.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FGetNiagaraSchemaActionsAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}
	FString ModuleName;
	if (!Params->TryGetStringField(TEXT("module_name"), ModuleName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'module_name' parameter — must be a scratch pad module"));
	}
	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);
	int32 MaxResults = 300;
	Params->TryGetNumberField(TEXT("max_results"), MaxResults);

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	UNiagaraScript* Script = nullptr;
	for (UNiagaraScript* S : System->ScratchPadScripts)
	{
		if (S && S->GetName().Equals(ModuleName, ESearchCase::IgnoreCase))
		{
			Script = S;
			break;
		}
	}
	if (!Script)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Scratch pad module '%s' not found"), *ModuleName));
	}

	UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->GetLatestSource());
	if (!Source || !Source->NodeGraph)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Script has no graph"));
	}

	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
	if (!Schema)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Niagara schema unavailable"));
	}

	TArray<TSharedPtr<FNiagaraAction_NewNode>> Actions = Schema->GetGraphActions(
		Source->NodeGraph, nullptr, Source->NodeGraph);

	TArray<TSharedPtr<FJsonValue>> Results;
	for (const TSharedPtr<FNiagaraAction_NewNode>& Action : Actions)
	{
		if (!Action.IsValid())
		{
			continue;
		}
		if (Results.Num() >= MaxResults)
		{
			break;
		}

		const FString DisplayName = Action->DisplayName.ToString();
		const FString Tooltip = Action->ToolTip.ToString();
		const FString Keywords = Action->Keywords.ToString();

		FString Category;
		if (Action->Categories.Num() > 0)
		{
			Category = FString::Join(Action->Categories, TEXT("|"));
		}

		if (!NiagaraCommonUtils::MatchesAnyFilter(Filter, { DisplayName, Category, Keywords, Tooltip }))
		{
			continue;
		}

		auto J = MakeShared<FJsonObject>();
		J->SetStringField(TEXT("display_name"), DisplayName);
		if (!Category.IsEmpty())
		{
			J->SetStringField(TEXT("category"), Category);
		}
		if (!Tooltip.IsEmpty())
		{
			J->SetStringField(TEXT("tooltip"), Tooltip);
		}
		if (!Keywords.IsEmpty())
		{
			J->SetStringField(TEXT("keywords"), Keywords);
		}

		// Expose template-specific fields so callers can reuse the same values
		// the editor would provide. For each supported node type, publish the
		// parameters needed by add_niagara_graph_node.
		if (UEdGraphNode* Template = Action->WeakNodeTemplate.Get())
		{
			J->SetStringField(TEXT("template_class"), Template->GetClass()->GetName());

			FString ShortClass = Template->GetClass()->GetName();
			ShortClass.RemoveFromStart(TEXT("NiagaraNode"));
			J->SetStringField(TEXT("node_type"), ShortClass);

			if (UNiagaraNodeOp* OpTmpl = Cast<UNiagaraNodeOp>(Template))
			{
				J->SetStringField(TEXT("op_name"), OpTmpl->OpName.ToString());
			}
			else if (UNiagaraNodeFunctionCall* FnTmpl = Cast<UNiagaraNodeFunctionCall>(Template))
			{
				if (FnTmpl->FunctionScript)
				{
					J->SetStringField(TEXT("function_script"),
					                  FnTmpl->FunctionScript->GetPathName());
				}
				J->SetStringField(TEXT("function_display_name"), FnTmpl->GetFunctionName());
			}
			else if (UNiagaraNodeInput* InTmpl = Cast<UNiagaraNodeInput>(Template))
			{
				J->SetStringField(TEXT("input_name"), InTmpl->Input.GetName().ToString());
				J->SetStringField(TEXT("input_type"), InTmpl->Input.GetType().GetName());
			}

			// Publish the template pin schema so callers know which pins the
			// spawned node exposes.
			TArray<TSharedPtr<FJsonValue>> Inputs, Outputs;
			for (UEdGraphPin* Pin : Template->Pins)
			{
				if (!Pin || Pin->PinName == NAME_None)
				{
					continue;
				}
				auto PinObj = MakeShared<FJsonObject>();
				PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
				const FNiagaraTypeDefinition TypeDef =
					UEdGraphSchema_Niagara::PinTypeToTypeDefinition(Pin->PinType);
				PinObj->SetStringField(TEXT("type"),
				                       TypeDef.IsValid() ? TypeDef.GetName() : Pin->PinType.PinCategory.ToString());
				if (!Pin->DefaultValue.IsEmpty())
				{
					PinObj->SetStringField(TEXT("default"), Pin->DefaultValue);
				}
				if (Pin->Direction == EGPD_Input)
				{
					Inputs.Add(MakeShared<FJsonValueObject>(PinObj));
				}
				else
				{
					Outputs.Add(MakeShared<FJsonValueObject>(PinObj));
				}
			}
			if (Inputs.Num() > 0)
			{
				J->SetArrayField(TEXT("inputs"), Inputs);
			}
			if (Outputs.Num() > 0)
			{
				J->SetArrayField(TEXT("outputs"), Outputs);
			}
		}
		Results.Add(MakeShared<FJsonValueObject>(J));
	}

	auto Out = MakeShared<FJsonObject>();
	Out->SetBoolField(TEXT("success"), true);
	Out->SetStringField(TEXT("module_name"), ModuleName);
	Out->SetStringField(TEXT("filter"), Filter);
	Out->SetNumberField(TEXT("count"), Results.Num());
	Out->SetArrayField(TEXT("actions"), Results);
	return Out;
}

// ---------------------------------------------------------------------------
// FDescribeNiagaraTypeAction::ExecuteInternal
// Describes Niagara enums, structs, data interfaces, and primitive types.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FDescribeNiagaraTypeAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString TypeName;
	if (!Params->TryGetStringField(TEXT("type"), TypeName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
	}

	FNiagaraTypeDefinition TypeDef;
	if (NiagaraCommonUtils::ResolveTypeName(TypeName, TypeDef))
	{
		auto Result = MakeShared<FJsonObject>();
		Result->SetBoolField(TEXT("success"), true);
		Result->SetStringField(TEXT("type"), TypeName);
		Result->SetObjectField(TEXT("schema"), NiagaraCommonUtils::SerializeNiagaraType(TypeDef));
		return Result;
	}

	// Fall back to direct reflection. Some callers need plain UEnum or
	// UScriptStruct schema even when the type is not a registered
	// FNiagaraTypeDefinition.
	if (UEnum* Enum = FindFirstObjectSafe<UEnum>(*TypeName, EFindFirstObjectOptions::ExactClass))
	{
		auto Result = MakeShared<FJsonObject>();
		Result->SetBoolField(TEXT("success"), true);
		Result->SetStringField(TEXT("type"), TypeName);
		Result->SetStringField(TEXT("resolved_as"), TEXT("uenum_reflection"));
		Result->SetObjectField(TEXT("schema"), NiagaraCommonUtils::SerializeEnum(Enum));
		return Result;
	}
	if (UScriptStruct* Struct = FindFirstObjectSafe<UScriptStruct>(*TypeName, EFindFirstObjectOptions::ExactClass))
	{
		auto Result = MakeShared<FJsonObject>();
		Result->SetBoolField(TEXT("success"), true);
		Result->SetStringField(TEXT("type"), TypeName);
		Result->SetStringField(TEXT("resolved_as"), TEXT("uscriptstruct_reflection"));
		Result->SetObjectField(TEXT("schema"),
		                       NiagaraCommonUtils::SerializeStructFields(Struct, nullptr, FString(), true, 0));
		return Result;
	}

	return FMCPCommonUtils::CreateErrorResponse(
		FString::Printf(
			TEXT(
				"Unknown type '%s' — not in FNiagaraTypeRegistry, not a findable UEnum/UScriptStruct. For custom enums, pass the exact UEnum name (e.g. ENiagaraScriptLibraryVisibility)."),
			*TypeName));
}

// ---------------------------------------------------------------------------
// FGetNiagaraDataInterfaceSchemaAction::ExecuteInternal
// Returns the editable FProperty schema for a UNiagaraDataInterface subclass.
// Use this to discover supported configuration before instantiating a DI.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FGetNiagaraDataInterfaceSchemaAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString ClassName;
	if (!Params->TryGetStringField(TEXT("class_"), ClassName) &&
		!Params->TryGetStringField(TEXT("class"), ClassName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'class_' parameter"));
	}

	// Normalize the user-supplied class name. UClass::GetName() returns names
	// WITHOUT the leading "U" prefix, so the canonical short name for the
	// float-array DI is "NiagaraDataInterfaceArrayFloat" (no U). Accept any of:
	//   ArrayFloat
	//   NiagaraDataInterfaceArrayFloat
	//   UNiagaraDataInterfaceArrayFloat  (with U - strip it)
	//   /Script/Niagara.NiagaraDataInterfaceArrayFloat  (full path)
	FString Normalized = ClassName;
	if (Normalized.StartsWith(TEXT("U"), ESearchCase::CaseSensitive) &&
		Normalized.Len() > 1 &&
		FChar::IsUpper(Normalized[1]))
	{
		Normalized.RightChopInline(1);
	}

	UClass* DIClass = FindObject<UClass>(nullptr, *ClassName);
	if (!DIClass)
	{
		DIClass = FindObject<UClass>(nullptr, *Normalized);
	}
	if (!DIClass || !DIClass->IsChildOf(UNiagaraDataInterface::StaticClass()))
	{
		const FString WithPrefix = FString::Printf(TEXT("NiagaraDataInterface%s"), *Normalized);
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (!It->IsChildOf(UNiagaraDataInterface::StaticClass()))
			{
				continue;
			}
			if (It->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
			{
				continue;
			}
			const FString CandidateName = It->GetName();
			if (CandidateName.Equals(Normalized, ESearchCase::IgnoreCase) ||
				CandidateName.Equals(WithPrefix, ESearchCase::IgnoreCase))
			{
				DIClass = *It;
				break;
			}
		}
	}

	if (!DIClass || !DIClass->IsChildOf(UNiagaraDataInterface::StaticClass()))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("UNiagaraDataInterface subclass '%s' not found"), *ClassName));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("class"), DIClass->GetName());
	Result->SetStringField(TEXT("class_path"), DIClass->GetPathName());
	Result->SetObjectField(TEXT("schema"),
	                       NiagaraCommonUtils::SerializeDataInterfaceClass(DIClass));
	return Result;
}


// ---------------------------------------------------------------------------
// Local helpers.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// FListNiagaraAvailableParametersAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FListNiagaraAvailableParametersAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	FString Namespace = TEXT("all");
	Params->TryGetStringField(TEXT("namespace"), Namespace);
	const FString NamespaceLower = Namespace.ToLower();

	int32 MaxResults = 500;
	Params->TryGetNumberField(TEXT("max_results"), MaxResults);

	// Optionally scope the results to a specific scratch-pad script.
	FString SystemPath;
	FString ModuleName;
	Params->TryGetStringField(TEXT("system_path"), SystemPath);
	Params->TryGetStringField(TEXT("module_name"), ModuleName);

	TArray<TSharedPtr<FJsonValue>> ParamsJson;

	auto TryAdd = [&](const FString& Name, const FString& Type, const FString& Scope, const FString& Source)
	{
		if (ParamsJson.Num() >= MaxResults)
		{
			return;
		}
		if (!Filter.IsEmpty() && !Name.Contains(Filter, ESearchCase::IgnoreCase))
		{
			return;
		}

		if (NamespaceLower != TEXT("all"))
		{
			const FString Prefix = NamespaceLower + TEXT(".");
			if (!Name.StartsWith(Prefix, ESearchCase::IgnoreCase) &&
				!Scope.Equals(NamespaceLower, ESearchCase::IgnoreCase))
			{
				return;
			}
		}

		auto J = MakeShared<FJsonObject>();
		J->SetStringField(TEXT("name"), Name);
		J->SetStringField(TEXT("type"), Type);
		J->SetStringField(TEXT("scope"), Scope);
		J->SetStringField(TEXT("source"), Source);
		ParamsJson.Add(MakeShared<FJsonValueObject>(J));
	};

	// Add well-known engine parameters.
	static const TPair<const TCHAR*, const TCHAR*> EngineParams[] = {
		{TEXT("Engine.DeltaTime"), TEXT("float")},
		{TEXT("Engine.InverseDeltaTime"), TEXT("float")},
		{TEXT("Engine.Time"), TEXT("float")},
		{TEXT("Engine.RealTime"), TEXT("float")},
		{TEXT("Engine.Owner.Position"), TEXT("Position")},
		{TEXT("Engine.Owner.Velocity"), TEXT("Vector")},
		{TEXT("Engine.Owner.Rotation"), TEXT("Quat")},
		{TEXT("Engine.Owner.Scale"), TEXT("Vector")},
		{TEXT("Engine.Owner.SystemLocalToWorld"), TEXT("Matrix")},
		{TEXT("Engine.Owner.SystemWorldToLocal"), TEXT("Matrix")},
		{TEXT("Engine.Owner.LODDistance"), TEXT("float")},
		{TEXT("Engine.Emitter.NumParticles"), TEXT("int")},
		{TEXT("Engine.Emitter.SpawnCountScale"), TEXT("float")},
		{TEXT("Engine.System.Age"), TEXT("float")},
		{TEXT("Engine.System.TickCount"), TEXT("int")},
	};
	for (const auto& E : EngineParams) { TryAdd(E.Key, E.Value, TEXT("engine"), TEXT("well_known")); }

	// Add well-known particle attributes.
	static const TPair<const TCHAR*, const TCHAR*> ParticleAttrs[] = {
		{TEXT("Particles.Position"), TEXT("Position")},
		{TEXT("Particles.Velocity"), TEXT("Vector")},
		{TEXT("Particles.Color"), TEXT("LinearColor")},
		{TEXT("Particles.SpriteSize"), TEXT("Vector2D")},
		{TEXT("Particles.SpriteRotation"), TEXT("float")},
		{TEXT("Particles.SpriteFacing"), TEXT("Vector")},
		{TEXT("Particles.SpriteAlignment"), TEXT("Vector")},
		{TEXT("Particles.Scale"), TEXT("Vector")},
		{TEXT("Particles.Lifetime"), TEXT("float")},
		{TEXT("Particles.Age"), TEXT("float")},
		{TEXT("Particles.NormalizedAge"), TEXT("float")},
		{TEXT("Particles.Mass"), TEXT("float")},
		{TEXT("Particles.MeshOrientation"), TEXT("Quat")},
		{TEXT("Particles.UniqueID"), TEXT("int")},
		{TEXT("Particles.ID"), TEXT("NiagaraID")},
		{TEXT("Particles.RibbonID"), TEXT("NiagaraID")},
		{TEXT("Particles.RibbonWidth"), TEXT("float")},
		{TEXT("Particles.RibbonFacing"), TEXT("Vector")},
		{TEXT("Particles.RibbonTwist"), TEXT("float")},
		{TEXT("Particles.MaterialRandom"), TEXT("float")},
		{TEXT("Particles.DynamicMaterialParameter"), TEXT("Vector4")},
		{TEXT("Particles.SubImageIndex"), TEXT("float")},
		{TEXT("Particles.CameraOffset"), TEXT("float")},
		{TEXT("Particles.LightRadius"), TEXT("float")},
	};
	for (const auto& P : ParticleAttrs) { TryAdd(P.Key, P.Value, TEXT("particles"), TEXT("well_known")); }

	// If the caller scoped to a scratch-pad module, pull variables from that graph.
	if (!SystemPath.IsEmpty() && !ModuleName.IsEmpty())
	{
		FString LoadError;
		UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, LoadError);
		if (System)
		{
			// Collect user parameters from the exposed-parameters collection.
			TArray<FNiagaraVariable> UserVars;
			System->GetExposedParameters().GetParameters(UserVars);
			for (const FNiagaraVariable& V : UserVars)
			{
				TryAdd(V.GetName().ToString(), V.GetType().GetName(), TEXT("user"), TEXT("exposed"));
			}

			// Walk emitter scripts for rapid-iteration and variable-map parameters.
			for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
			{
				FVersionedNiagaraEmitterData* Data = Handle.GetEmitterData();
				if (!Data)
				{
					continue;
				}
				UNiagaraScript* ScriptsToWalk[] = {
					Data->SpawnScriptProps.Script,
					Data->UpdateScriptProps.Script,
					Data->EmitterSpawnScriptProps.Script,
					Data->EmitterUpdateScriptProps.Script,
				};
				for (UNiagaraScript* S : ScriptsToWalk)
				{
					if (!S)
					{
						continue;
					}
					TArrayView<const FNiagaraVariableWithOffset> Rapid = S->RapidIterationParameters.ReadParameterVariables();
					for (const FNiagaraVariableWithOffset& V : Rapid)
					{
						TryAdd(V.GetName().ToString(), V.GetType().GetName(), TEXT("rapid_iteration"), TEXT("emitter"));
					}
				}
			}

			// Collect the scratch-pad module's own graph parameters.
			for (UNiagaraScript* S : System->ScratchPadScripts)
			{
				if (!S || !S->GetName().Equals(ModuleName, ESearchCase::IgnoreCase))
				{
					continue;
				}
				UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(S->GetLatestSource());
				if (!Source || !Source->NodeGraph)
				{
					continue;
				}

				const UNiagaraGraph::FScriptVariableMap& MetaMap = Source->NodeGraph->GetAllMetaData();
				for (const TPair<FNiagaraVariable, TObjectPtr<UNiagaraScriptVariable>>& Pair : MetaMap)
				{
					const FNiagaraVariable& V = Pair.Key;
					const FString Name = V.GetName().ToString();
					FString Scope = TEXT("module");
					if (Name.StartsWith(TEXT("Module.")))
					{
						Scope = TEXT("module");
					}
					else if (Name.StartsWith(TEXT("Input.")))
					{
						Scope = TEXT("input");
					}
					else if (Name.StartsWith(TEXT("Output.")))
					{
						Scope = TEXT("output");
					}
					else if (Name.StartsWith(TEXT("Transient.")))
					{
						Scope = TEXT("transient");
					}
					else if (Name.StartsWith(TEXT("User.")))
					{
						Scope = TEXT("user");
					}
					TryAdd(Name, V.GetType().GetName(), Scope, TEXT("script_graph"));
				}
				break;
			}
		}
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("filter"), Filter);
	Result->SetStringField(TEXT("namespace"), Namespace);
	Result->SetNumberField(TEXT("count"), ParamsJson.Num());
	Result->SetArrayField(TEXT("parameters"), ParamsJson);
	return Result;
}

// ---------------------------------------------------------------------------
// Map-pin mutation helpers.
// ---------------------------------------------------------------------------

static TSharedPtr<FJsonObject> AddMapPinImpl(
	const TSharedPtr<FJsonObject>& Params,
	EEdGraphPinDirection Direction,
	UClass* MapNodeClass,
	const FString& OperationLabel)
{
	FString ParamName;
	if (!Params->TryGetStringField(TEXT("parameter_name"), ParamName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parameter_name' parameter"));
	}
	FString TypeStr = TEXT("float");
	Params->TryGetStringField(TEXT("parameter_type"), TypeStr);

	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	TArray<UNiagaraGraph*> Graphs;
	FString Error;
	if (!NiagaraCommonUtils::ResolveScratchPadGraphs(Params, System, ModuleName, Graphs, Error))
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FNiagaraTypeDefinition TypeDef;
	if (!NiagaraCommonUtils::ParseTypeDef(TypeStr, TypeDef))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unknown parameter type '%s'"), *TypeStr));
	}

	int32 Applied = 0;
	for (UNiagaraGraph* Graph : Graphs)
	{
		if (!Graph)
		{
			continue;
		}

		// Locate the target map node on this graph instance.
		UNiagaraNode* TargetNode = nullptr;
		for (UEdGraphNode* N : Graph->Nodes)
		{
			if (N && N->IsA(MapNodeClass))
			{
				TargetNode = Cast<UNiagaraNode>(N);
				break;
			}
		}
		if (!TargetNode)
		{
			continue;
		}

		if (NiagaraCommonUtils::AddTypedPin(TargetNode, Direction, TypeDef, FName(*ParamName)))
		{
			// Register the parameter in the graph's script-variable metadata map.
			// Without this, the stack-input system will not expose Module.* pins
			// as configurable inputs.
			FNiagaraVariable Var(TypeDef, FName(*ParamName));
		NiagaraCommonUtils::EnsureScriptVariableMetadata(
			Graph,
			Var,
			NiagaraCommonUtils::DefaultModeForNamespace(ParamName));

			++Applied;
			Graph->NotifyGraphChanged();
		}
	}
	if (Applied == 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("No %s node in scratch pad graph"), *OperationLabel));
	}

	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, ModuleName);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("operation"), OperationLabel);
	Result->SetStringField(TEXT("parameter_name"), ParamName);
	Result->SetStringField(TEXT("parameter_type"), NiagaraCommonUtils::TypeDefToString(TypeDef));
	Result->SetNumberField(TEXT("updated_graph_count"), Applied);
	return Result;
}

TSharedPtr<FJsonObject> FAddNiagaraMapGetPinAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	return AddMapPinImpl(Params, EGPD_Output, UNiagaraNodeParameterMapGet::StaticClass(), TEXT("map_get"));
}

TSharedPtr<FJsonObject> FAddNiagaraMapSetPinAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	return AddMapPinImpl(Params, EGPD_Input, UNiagaraNodeParameterMapSet::StaticClass(), TEXT("map_set"));
}

// ---------------------------------------------------------------------------
// FAddNiagaraNodePinAction::ExecuteInternal
// Adds a dynamic pin to any Niagara node that supports dynamic pins.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FAddNiagaraNodePinAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString PinName;
	if (!Params->TryGetStringField(TEXT("pin_name"), PinName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pin_name' parameter"));
	}
	FString PinType = TEXT("float");
	Params->TryGetStringField(TEXT("pin_type"), PinType);
	FString DirectionStr = TEXT("input");
	Params->TryGetStringField(TEXT("direction"), DirectionStr);
	const EEdGraphPinDirection Direction = DirectionStr.Equals(TEXT("output"), ESearchCase::IgnoreCase)
		                                       ? EGPD_Output
		                                       : EGPD_Input;

	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	TArray<UNiagaraGraph*> Graphs;
	FString GraphError;
	if (!NiagaraCommonUtils::ResolveScratchPadGraphs(Params, System, ModuleName, Graphs, GraphError))
	{
		return FMCPCommonUtils::CreateErrorResponse(GraphError);
	}

	FNiagaraTypeDefinition TypeDef;
	if (!NiagaraCommonUtils::ParseTypeDef(PinType, TypeDef))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unknown pin type '%s'"), *PinType));
	}

	int32 Applied = 0;
	FString LastNodeClass;
	for (UNiagaraGraph* Graph : Graphs)
	{
		FString NodeError;
		UNiagaraNode* Node = NiagaraCommonUtils::ResolveGraphNode(Graph, Params, NodeError);
		if (!Node)
		{
			continue;
		}
		LastNodeClass = Node->GetClass()->GetName();
		if (NiagaraCommonUtils::AddTypedPin(Node, Direction, TypeDef, FName(*PinName)))
		{
			++Applied;
			Graph->NotifyGraphChanged();
		}
	}
	if (Applied == 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to add pin on any graph"));
	}
	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, ModuleName);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("pin_name"), PinName);
	Result->SetStringField(TEXT("pin_type"), NiagaraCommonUtils::TypeDefToString(TypeDef));
	Result->SetStringField(TEXT("direction"), DirectionStr);
	Result->SetStringField(TEXT("node_class"), LastNodeClass);
	Result->SetNumberField(TEXT("updated_graph_count"), Applied);
	return Result;
}

// ---------------------------------------------------------------------------
// FRenameNiagaraNodePinAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FRenameNiagaraNodePinAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString OldName, NewName;
	if (!Params->TryGetStringField(TEXT("old_name"), OldName) ||
		!Params->TryGetStringField(TEXT("new_name"), NewName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'old_name' or 'new_name' parameter"));
	}

	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	TArray<UNiagaraGraph*> Graphs;
	FString GraphError;
	if (!NiagaraCommonUtils::ResolveScratchPadGraphs(Params, System, ModuleName, Graphs, GraphError))
	{
		return FMCPCommonUtils::CreateErrorResponse(GraphError);
	}

	int32 Applied = 0;
	FString LastError;
	for (UNiagaraGraph* Graph : Graphs)
	{
		FString NodeError;
		UNiagaraNode* Node = NiagaraCommonUtils::ResolveGraphNode(Graph, Params, NodeError);
		if (!Node)
		{
			continue;
		}
		FString RenameError;
		if (NiagaraCommonUtils::RenameDynamicPin(Node, FName(*OldName), FName(*NewName), RenameError))
		{
			++Applied;
			Graph->NotifyGraphChanged();
		}
		else
		{
			LastError = RenameError;
		}
	}
	if (Applied == 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(LastError.IsEmpty() ? TEXT("Rename failed") : LastError);
	}
	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, ModuleName);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("old_name"), OldName);
	Result->SetStringField(TEXT("new_name"), NewName);
	Result->SetNumberField(TEXT("updated_graph_count"), Applied);
	return Result;
}

// ---------------------------------------------------------------------------
// FRemoveNiagaraNodePinAction::ExecuteInternal
// Removes a dynamic pin from the target Niagara node across mirrored graphs.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FRemoveNiagaraNodePinAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString PinName;
	if (!Params->TryGetStringField(TEXT("pin_name"), PinName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pin_name' parameter"));
	}

	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	TArray<UNiagaraGraph*> Graphs;
	FString GraphError;
	if (!NiagaraCommonUtils::ResolveScratchPadGraphs(Params, System, ModuleName, Graphs, GraphError))
	{
		return FMCPCommonUtils::CreateErrorResponse(GraphError);
	}

	int32 Applied = 0;
	FString LastError;
	for (UNiagaraGraph* Graph : Graphs)
	{
		FString NodeError;
		UNiagaraNode* Node = NiagaraCommonUtils::ResolveGraphNode(Graph, Params, NodeError);
		if (!Node)
		{
			continue;
		}
		FString RemoveError;
		if (NiagaraCommonUtils::RemoveDynamicPinByName(Node, FName(*PinName), RemoveError))
		{
			++Applied;
			Graph->NotifyGraphChanged();
		}
		else
		{
			LastError = RemoveError;
		}
	}
	if (Applied == 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(LastError.IsEmpty() ? TEXT("Remove failed") : LastError);
	}
	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, ModuleName);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("pin_name"), PinName);
	Result->SetNumberField(TEXT("updated_graph_count"), Applied);
	return Result;
}

// ---------------------------------------------------------------------------
// FConnectNiagaraPinsAction::ExecuteInternal
// Connects pins between nodes inside a scratch-pad graph.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FConnectNiagaraPinsAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString FromPin, ToPin;
	if (!Params->TryGetStringField(TEXT("from_pin"), FromPin) ||
		!Params->TryGetStringField(TEXT("to_pin"), ToPin))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'from_pin' or 'to_pin'"));
	}

	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	TArray<UNiagaraGraph*> Graphs;
	FString GraphError;
	if (!NiagaraCommonUtils::ResolveScratchPadGraphs(Params, System, ModuleName, Graphs, GraphError))
	{
		return FMCPCommonUtils::CreateErrorResponse(GraphError);
	}

	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
	int32 Applied = 0;
	FString LastError;
	for (UNiagaraGraph* Graph : Graphs)
	{
		auto ResolveSide = [&](const TCHAR* Prefix, FString& OutErr) -> UNiagaraNode*
		{
			TSharedPtr<FJsonObject> Local = MakeShared<FJsonObject>();
			FString S;
			double D;
			if (Params->TryGetStringField(FString::Printf(TEXT("%s_node_class"), Prefix), S))
			{
				Local->SetStringField(TEXT("node_class"), S);
			}
			if (Params->TryGetStringField(FString::Printf(TEXT("%s_node_id"), Prefix), S))
			{
				Local->SetStringField(TEXT("node_id"), S);
			}
			if (Params->TryGetNumberField(FString::Printf(TEXT("%s_node_index"), Prefix), D))
			{
				Local->SetNumberField(TEXT("node_index"), D);
			}
			return NiagaraCommonUtils::ResolveGraphNode(Graph, Local, OutErr);
		};

		FString FromErr, ToErr;
		UNiagaraNode* FromNode = ResolveSide(TEXT("from"), FromErr);
		UNiagaraNode* ToNode = ResolveSide(TEXT("to"), ToErr);
		if (!FromNode || !ToNode)
		{
			LastError = FString::Printf(TEXT("%s%s"),
			                            !FromNode ? *FString::Printf(TEXT("from: %s "), *FromErr) : TEXT(""),
			                            !ToNode ? *FString::Printf(TEXT("to: %s"), *ToErr) : TEXT(""));
			continue;
		}

		UEdGraphPin* FromPinObj = FMCPCommonUtils::FindPin(FromNode, FromPin, EGPD_Output);
		UEdGraphPin* ToPinObj = FMCPCommonUtils::FindPin(ToNode, ToPin, EGPD_Input);
		if (!FromPinObj || !ToPinObj)
		{
			LastError = TEXT("Could not resolve both pins by name/direction");
			continue;
		}

		if (!Schema->TryCreateConnection(FromPinObj, ToPinObj))
		{
			LastError = TEXT("Schema refused the connection (type mismatch?)");
			continue;
		}
		Graph->NotifyGraphChanged();
		++Applied;
	}
	if (Applied == 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(LastError.IsEmpty() ? TEXT("Connect failed") : LastError);
	}
	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, ModuleName);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("from"), FromPin);
	Result->SetStringField(TEXT("to"), ToPin);
	Result->SetNumberField(TEXT("updated_graph_count"), Applied);
	return Result;
}

// ---------------------------------------------------------------------------
// FDisconnectNiagaraPinsAction::ExecuteInternal
// Breaks all links on the named pin across mirrored scratch-pad graphs.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FDisconnectNiagaraPinsAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString PinName;
	if (!Params->TryGetStringField(TEXT("pin_name"), PinName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pin_name' parameter"));
	}

	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	TArray<UNiagaraGraph*> Graphs;
	FString GraphError;
	if (!NiagaraCommonUtils::ResolveScratchPadGraphs(Params, System, ModuleName, Graphs, GraphError))
	{
		return FMCPCommonUtils::CreateErrorResponse(GraphError);
	}

	int32 Broken = 0;
	for (UNiagaraGraph* Graph : Graphs)
	{
		FString NodeError;
		UNiagaraNode* Node = NiagaraCommonUtils::ResolveGraphNode(Graph, Params, NodeError);
		if (!Node)
		{
			continue;
		}
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->PinName == FName(*PinName))
			{
				Broken += Pin->LinkedTo.Num();
				Pin->BreakAllPinLinks();
			}
		}
		Graph->NotifyGraphChanged();
	}
	if (Broken == 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Pin '%s' had no connections to break"), *PinName));
	}
	NiagaraCommonUtils::CompileAndSync(System);
	NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, ModuleName);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("pin_name"), PinName);
	Result->SetNumberField(TEXT("broken_links"), Broken);
	return Result;
}

// ---------------------------------------------------------------------------
// Node creation and deletion helpers.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// FAddNiagaraGraphNodeAction::ExecuteInternal
// Adds a node to the asset graph and any mirrored live edit-copy graphs so
// both representations stay in sync.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FAddNiagaraGraphNodeAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Error;
	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	UNiagaraScript* Script = nullptr;
	UNiagaraGraph* PrimaryGraph = nullptr;
	if (!NiagaraCommonUtils::ResolveScratchPadScriptAndGraph(
		Params, System, ModuleName, Script, PrimaryGraph, Error))
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString NodeType;
	if (!Params->TryGetStringField(TEXT("node_type"), NodeType))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing 'node_type'"));
	}
	NodeType.RemoveFromStart(TEXT("NiagaraNode"));

	int32 PosX = 0, PosY = 0;
	Params->TryGetNumberField(TEXT("pos_x"), PosX);
	Params->TryGetNumberField(TEXT("pos_y"), PosY);

	TArray<UNiagaraGraph*> MutGraphs = NiagaraCommonUtils::CollectMutationGraphs(System, ModuleName, Script);
	if (MutGraphs.Num() == 0)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("No graph to mutate"));
	}

	// Spawn on every mirrored graph so the asset graph and live edit copy stay
	// in lockstep.
	UEdGraphNode* FirstSpawned = nullptr;
	int32 FirstSpawnedIndex = INDEX_NONE;
	for (UNiagaraGraph* G : MutGraphs)
	{
		G->Modify();
		UEdGraphNode* Spawned = nullptr;

		if (NodeType.Equals(TEXT("Op"), ESearchCase::IgnoreCase))
		{
			FString OpName;
			if (!Params->TryGetStringField(TEXT("op_name"), OpName))
			{
				return FMCPCommonUtils::CreateErrorResponse(
					TEXT("node_type=Op requires 'op_name' (e.g. 'Numeric::Add')"));
			}
			FGraphNodeCreator<UNiagaraNodeOp> Creator(*G);
			UNiagaraNodeOp* N = Creator.CreateNode(false);
			N->NodePosX = PosX;
			N->NodePosY = PosY;
			N->OpName = FName(*OpName);
			Creator.Finalize();
			G->NotifyGraphChanged();
			Spawned = N;
		}
		else if (NodeType.Equals(TEXT("FunctionCall"), ESearchCase::IgnoreCase))
		{
			FString FuncPath;
			if (!Params->TryGetStringField(TEXT("function_script"), FuncPath))
			{
				return FMCPCommonUtils::CreateErrorResponse(
					TEXT("node_type=FunctionCall requires 'function_script' asset path"));
			}
			UNiagaraScript* FuncScript = Cast<UNiagaraScript>(UEditorAssetLibrary::LoadAsset(FuncPath));
			if (!FuncScript)
			{
				return FMCPCommonUtils::CreateErrorResponse(
					FString::Printf(TEXT("function_script not found: %s"), *FuncPath));
			}
			FGraphNodeCreator<UNiagaraNodeFunctionCall> Creator(*G);
			UNiagaraNodeFunctionCall* N = Creator.CreateNode(false);
			N->NodePosX = PosX;
			N->NodePosY = PosY;
			N->FunctionScript = FuncScript;
			Creator.Finalize();
			G->NotifyGraphChanged();
			Spawned = N;
		}
		else if (NodeType.Equals(TEXT("ParameterMapGet"), ESearchCase::IgnoreCase))
		{
			FGraphNodeCreator<UNiagaraNodeParameterMapGet> Creator(*G);
			UNiagaraNodeParameterMapGet* N = Creator.CreateNode(false);
			N->NodePosX = PosX;
			N->NodePosY = PosY;
			Creator.Finalize();
			G->NotifyGraphChanged();
			Spawned = N;
		}
		else if (NodeType.Equals(TEXT("ParameterMapSet"), ESearchCase::IgnoreCase))
		{
			FGraphNodeCreator<UNiagaraNodeParameterMapSet> Creator(*G);
			UNiagaraNodeParameterMapSet* N = Creator.CreateNode(false);
			N->NodePosX = PosX;
			N->NodePosY = PosY;
			Creator.Finalize();
			G->NotifyGraphChanged();
			Spawned = N;
		}
		else if (NodeType.Equals(TEXT("Reroute"), ESearchCase::IgnoreCase))
		{
			FGraphNodeCreator<UNiagaraNodeReroute> Creator(*G);
			UNiagaraNodeReroute* N = Creator.CreateNode(false);
			N->NodePosX = PosX;
			N->NodePosY = PosY;
			Creator.Finalize();
			G->NotifyGraphChanged();
			Spawned = N;
		}
		else if (NodeType.Equals(TEXT("Input"), ESearchCase::IgnoreCase))
		{
			FString InName, InTypeName;
			if (!Params->TryGetStringField(TEXT("input_name"), InName) ||
				!Params->TryGetStringField(TEXT("input_type"), InTypeName))
			{
				return FMCPCommonUtils::CreateErrorResponse(
					TEXT("node_type=Input requires 'input_name' and 'input_type'"));
			}
			FNiagaraTypeDefinition TypeDef;
			if (!NiagaraCommonUtils::ResolveTypeName(InTypeName, TypeDef) || !TypeDef.IsValid())
			{
				return FMCPCommonUtils::CreateErrorResponse(
					FString::Printf(TEXT("Unknown Niagara type '%s'"), *InTypeName));
			}
			FGraphNodeCreator<UNiagaraNodeInput> Creator(*G);
			UNiagaraNodeInput* N = Creator.CreateNode(false);
			N->NodePosX = PosX;
			N->NodePosY = PosY;
			N->Input = FNiagaraVariable(TypeDef, FName(*InName));
			Creator.Finalize();
			G->NotifyGraphChanged();
			Spawned = N;
		}
		else if (NodeType.Equals(TEXT("CustomHlsl"), ESearchCase::IgnoreCase))
		{
			FGraphNodeCreator<UNiagaraNodeCustomHlsl> Creator(*G);
			UNiagaraNodeCustomHlsl* N = Creator.CreateNode(false);
			N->NodePosX = PosX;
			N->NodePosY = PosY;
			Creator.Finalize();

			auto AddPinsFromField = [&](const TCHAR* FieldName, EEdGraphPinDirection Direction)
			{
				const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
				if (!Params->TryGetArrayField(FieldName, Entries))
				{
					return;
				}

				for (const TSharedPtr<FJsonValue>& Entry : *Entries)
				{
					const TSharedPtr<FJsonObject>* EntryObject = nullptr;
					if (!Entry.IsValid() || !Entry->TryGetObject(EntryObject) || !EntryObject || !EntryObject->IsValid())
					{
						continue;
					}

					FString PinName;
					if (!(*EntryObject)->TryGetStringField(TEXT("name"), PinName) || PinName.IsEmpty())
					{
						continue;
					}

					FString PinType = TEXT("float");
					(*EntryObject)->TryGetStringField(TEXT("type"), PinType);

					FNiagaraTypeDefinition TypeDef;
					if (!NiagaraCommonUtils::ParseTypeDef(PinType, TypeDef))
					{
						continue;
					}

					NiagaraCommonUtils::AddTypedPin(N, Direction, TypeDef, FName(*PinName));
				}
			};

			AddPinsFromField(TEXT("inputs"), EGPD_Input);
			AddPinsFromField(TEXT("outputs"), EGPD_Output);

			FString HlslCode;
			if (Params->TryGetStringField(TEXT("hlsl_code"), HlslCode) && !HlslCode.IsEmpty())
			{
				NiagaraCommonUtils::SetCustomHlslViaReflection(N, HlslCode);
			}

			G->NotifyGraphChanged();
			Spawned = N;
		}
		else if (NodeType.Equals(TEXT("DataInterfaceFunction"), ESearchCase::IgnoreCase))
		{
			// Spawn a function-call node from a data-interface member-function
			// signature. These nodes are signature-backed rather than
			// script-backed, so FunctionScript remains null.
			FString DIClassName, FunctionName;
			if (!Params->TryGetStringField(TEXT("di_class"), DIClassName) ||
				!Params->TryGetStringField(TEXT("function_name"), FunctionName))
			{
				return FMCPCommonUtils::CreateErrorResponse(
					TEXT("node_type=DataInterfaceFunction requires 'di_class' and 'function_name'"));
			}

			// Resolve the data-interface class from a short name or full path.
			UClass* DIClass = FindFirstObjectSafe<UClass>(*DIClassName, EFindFirstObjectOptions::ExactClass);
			if (!DIClass)
			{
				FString FullName = DIClassName.StartsWith(TEXT("/Script/Niagara."))
					                   ? DIClassName
					                   : (FString(TEXT("/Script/Niagara.")) + DIClassName);
				DIClass = LoadClass<UObject>(nullptr, *FullName);
			}
			if (!DIClass || !DIClass->IsChildOf(UNiagaraDataInterface::StaticClass()))
			{
				return FMCPCommonUtils::CreateErrorResponse(
					FString::Printf(TEXT("DI class '%s' not found or not a UNiagaraDataInterface"),
					                *DIClassName));
			}

			UNiagaraDataInterface* DICDO =
				Cast<UNiagaraDataInterface>(DIClass->GetDefaultObject());
			if (!DICDO)
			{
				return FMCPCommonUtils::CreateErrorResponse(
					TEXT("DI class has no default object"));
			}

			TArray<FNiagaraFunctionSignature> Signatures;
			DICDO->GetFunctionSignatures(Signatures);
			const FNiagaraFunctionSignature* MatchedSig = nullptr;
			for (const FNiagaraFunctionSignature& S : Signatures)
			{
				if (S.Name.ToString().Equals(FunctionName, ESearchCase::IgnoreCase))
				{
					MatchedSig = &S;
					break;
				}
			}
			if (!MatchedSig)
			{
				return FMCPCommonUtils::CreateErrorResponse(
					FString::Printf(
						TEXT("Function '%s' not found on DI '%s'. Use list_niagara_data_interface_functions to discover."),
						*FunctionName, *DIClassName));
			}

			FGraphNodeCreator<UNiagaraNodeFunctionCall> Creator(*G);
			UNiagaraNodeFunctionCall* N = Creator.CreateNode(false);
			N->NodePosX = PosX;
			N->NodePosY = PosY;
			N->Signature = *MatchedSig;
			// Leave FunctionScript null. The editor identifies data-interface
			// member functions from the signature plus a null script reference.
			Creator.Finalize();
			G->NotifyGraphChanged();
			Spawned = N;
		}
		else
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(
					TEXT(
						"Unsupported node_type '%s'. Supported: Op, FunctionCall, DataInterfaceFunction, ParameterMapGet, ParameterMapSet, Reroute, Input, CustomHlsl"),
					*NodeType));
		}

		if (!FirstSpawned)
		{
			FirstSpawned = Spawned;
			FirstSpawnedIndex = G->Nodes.IndexOfByKey(Spawned);
		}
	}

	if (System && !ModuleName.IsEmpty())
	{
		NiagaraCommonUtils::CompileAndSync(System);
		NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, ModuleName);
	}
	Script->MarkPackageDirty();

	auto R = MakeShared<FJsonObject>();
	R->SetBoolField(TEXT("success"), FirstSpawned != nullptr);
	R->SetStringField(TEXT("node_type"), NodeType);
	R->SetNumberField(TEXT("node_index"), FirstSpawnedIndex);
	if (FirstSpawned)
	{
		R->SetStringField(TEXT("node_id"),
		                  FirstSpawned->NodeGuid.ToString(EGuidFormats::DigitsWithHyphens));
	}
	return R;
}


// ---------------------------------------------------------------------------
// FDeleteNiagaraGraphNodeAction::ExecuteInternal
// Parameters: graph resolver parameters plus node_index or node_id.
// Deletes the matching node from the asset graph and any mirrored edit-copy
// graphs.
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FDeleteNiagaraGraphNodeAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString Error;
	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	UNiagaraScript* Script = nullptr;
	UNiagaraGraph* PrimaryGraph = nullptr;
	if (!NiagaraCommonUtils::ResolveScratchPadScriptAndGraph(
		Params, System, ModuleName, Script, PrimaryGraph, Error))
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 NodeIndex = INDEX_NONE;
	FString NodeIdStr;
	FGuid TargetGuid;
	const bool bHaveIndex = Params->TryGetNumberField(TEXT("node_index"), NodeIndex);
	const bool bHaveGuid = Params->TryGetStringField(TEXT("node_id"), NodeIdStr) &&
		FGuid::Parse(NodeIdStr, TargetGuid);
	if (!bHaveIndex && !bHaveGuid)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Provide 'node_index' or 'node_id'"));
	}

	TArray<UNiagaraGraph*> MutGraphs = NiagaraCommonUtils::CollectMutationGraphs(System, ModuleName, Script);
	int32 DeletedGraphs = 0;
	FString DeletedClass;

	// Resolve by index on the primary graph to obtain the node GUID first.
	// Index values are only stable within a single graph, so mirrored graphs
	// use the resolved GUID.
	if (bHaveIndex && PrimaryGraph && PrimaryGraph->Nodes.IsValidIndex(NodeIndex))
	{
		UEdGraphNode* N = PrimaryGraph->Nodes[NodeIndex];
		if (N)
		{
			TargetGuid = N->NodeGuid;
			DeletedClass = N->GetClass()->GetName();
		}
	}

	if (!TargetGuid.IsValid())
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Could not resolve target node"));
	}

	for (UNiagaraGraph* G : MutGraphs)
	{
		for (int32 i = G->Nodes.Num() - 1; i >= 0; --i)
		{
			UEdGraphNode* N = G->Nodes[i];
			if (N && N->NodeGuid == TargetGuid)
			{
				G->Modify();
				if (DeletedClass.IsEmpty())
				{
					DeletedClass = N->GetClass()->GetName();
				}
				N->BreakAllNodeLinks();
				G->RemoveNode(N);
				++DeletedGraphs;
				break;
			}
		}
		G->NotifyGraphChanged();
	}

	if (System && !ModuleName.IsEmpty())
	{
		NiagaraCommonUtils::NotifyScratchPadScriptChanged(System, ModuleName);
	}
	Script->MarkPackageDirty();

	auto R = MakeShared<FJsonObject>();
	R->SetBoolField(TEXT("success"), DeletedGraphs > 0);
	R->SetNumberField(TEXT("deleted_graph_count"), DeletedGraphs);
	R->SetStringField(TEXT("node_class"), DeletedClass);
	return R;
}


// ---------------------------------------------------------------------------
// FListNiagaraDataInterfaceFunctionsAction::ExecuteInternal
// Enumerates member functions on a Niagara data-interface class so callers
// can choose the exact function_name for node_type="DataInterfaceFunction".
// Parameters:
// - di_class: required short or full class name
// - filter: optional case-insensitive substring filter on function name
// - include_pins: include input and output pin schemas when true
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FListNiagaraDataInterfaceFunctionsAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& /*Context*/)
{
	FString DIClassName;
	if (!Params->TryGetStringField(TEXT("di_class"), DIClassName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'di_class'"));
	}
	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);
	bool bIncludePins = true;
	Params->TryGetBoolField(TEXT("include_pins"), bIncludePins);

	UClass* DIClass = FindFirstObjectSafe<UClass>(*DIClassName, EFindFirstObjectOptions::ExactClass);
	if (!DIClass)
	{
		FString FullName = DIClassName.StartsWith(TEXT("/Script/Niagara."))
			                   ? DIClassName
			                   : (FString(TEXT("/Script/Niagara.")) + DIClassName);
		DIClass = LoadClass<UObject>(nullptr, *FullName);
	}
	if (!DIClass || !DIClass->IsChildOf(UNiagaraDataInterface::StaticClass()))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("DI class '%s' not found"), *DIClassName));
	}

	UNiagaraDataInterface* CDO = Cast<UNiagaraDataInterface>(DIClass->GetDefaultObject());
	if (!CDO)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("DI has no CDO"));
	}

	TArray<FNiagaraFunctionSignature> Sigs;
	CDO->GetFunctionSignatures(Sigs);

	auto SerializeVar = [](const FNiagaraVariableBase& V)
	{
		auto J = MakeShared<FJsonObject>();
		J->SetStringField(TEXT("name"), V.GetName().ToString());
		J->SetStringField(TEXT("type"), V.GetType().GetName());
		return MakeShared<FJsonValueObject>(J);
	};

	TArray<TSharedPtr<FJsonValue>> Out;
	for (const FNiagaraFunctionSignature& S : Sigs)
	{
		const FString FnName = S.Name.ToString();
		if (!Filter.IsEmpty() && !FnName.Contains(Filter, ESearchCase::IgnoreCase))
		{
			continue;
		}
		auto Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("name"), FnName);
		Entry->SetBoolField(TEXT("supports_cpu"), S.bSupportsCPU);
		Entry->SetBoolField(TEXT("supports_gpu"), S.bSupportsGPU);
		Entry->SetBoolField(TEXT("read_function"), S.bReadFunction);
		Entry->SetBoolField(TEXT("write_function"), S.bWriteFunction);
		Entry->SetBoolField(TEXT("member_function"), S.bMemberFunction);
		if (!S.Description.IsEmpty())
		{
			Entry->SetStringField(TEXT("description"), S.Description.ToString());
		}
		if (bIncludePins)
		{
			TArray<TSharedPtr<FJsonValue>> Inputs, Outputs;
			for (const FNiagaraVariable& V : S.Inputs)
			{
				Inputs.Add(SerializeVar(V));
			}
			for (const FNiagaraVariableBase& V : S.Outputs)
			{
				Outputs.Add(SerializeVar(V));
			}
			Entry->SetArrayField(TEXT("inputs"), Inputs);
			Entry->SetArrayField(TEXT("outputs"), Outputs);
		}
		Out.Add(MakeShared<FJsonValueObject>(Entry));
	}

	auto R = MakeShared<FJsonObject>();
	R->SetBoolField(TEXT("success"), true);
	R->SetStringField(TEXT("di_class"), DIClass->GetName());
	R->SetStringField(TEXT("di_class_path"), DIClass->GetPathName());
	R->SetNumberField(TEXT("total_functions"), Sigs.Num());
	R->SetNumberField(TEXT("returned"), Out.Num());
	R->SetArrayField(TEXT("functions"), Out);
	return R;
}

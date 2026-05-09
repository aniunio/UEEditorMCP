#include "Actions/NiagaraActions/NiagaraActions.h"

#include "Actions/NiagaraActions/NiagaraEmitterActions.h"
#include "Actions/NiagaraActions/NiagaraGraphActions.h"
#include "Actions/NiagaraActions/NiagaraModuleActions.h"
#include "Actions/NiagaraActions/NiagaraParameterActions.h"
#include "Actions/NiagaraActions/NiagaraRendererActions.h"
#include "Actions/NiagaraActions/NiagaraRuntimeActions.h"
#include "Actions/NiagaraActions/NiagaraScratchPadActions.h"
#include "Actions/NiagaraActions/NiagaraSystemActions.h"

namespace NiagaraActionRegistration
{
	void RegisterAll(TMap<FString, TSharedRef<FEditorAction>>& ActionHandlers)
	{
		// Register system actions.
		ActionHandlers.Add(TEXT("create_niagara_system"), MakeShared<FCreateNiagaraSystemAction>());
		ActionHandlers.Add(TEXT("get_niagara_system_info"), MakeShared<FGetNiagaraSystemInfoAction>());
		ActionHandlers.Add(TEXT("list_niagara_systems"), MakeShared<FListNiagaraSystemsAction>());
		ActionHandlers.Add(TEXT("delete_niagara_system"), MakeShared<FDeleteNiagaraSystemAction>());
		ActionHandlers.Add(TEXT("compile_niagara_system"), MakeShared<FCompileNiagaraSystemAction>());
		ActionHandlers.Add(TEXT("set_niagara_system_property"), MakeShared<FSetNiagaraSystemPropertyAction>());
		ActionHandlers.Add(TEXT("get_niagara_system_errors"), MakeShared<FGetNiagaraSystemErrorsAction>());
		ActionHandlers.Add(TEXT("get_niagara_particle_stats"), MakeShared<FGetNiagaraParticleStatsAction>());
		ActionHandlers.Add(TEXT("set_niagara_playback_range"), MakeShared<FSetNiagaraPlaybackRangeAction>());
		ActionHandlers.Add(TEXT("get_niagara_playback_range"), MakeShared<FGetNiagaraPlaybackRangeAction>());
		ActionHandlers.Add(TEXT("get_niagara_module_versions"), MakeShared<FGetNiagaraModuleVersionsAction>());
		ActionHandlers.Add(TEXT("upgrade_niagara_module_version"), MakeShared<FUpgradeNiagaraModuleVersionAction>());

		// Register emitter actions.
		ActionHandlers.Add(TEXT("get_niagara_emitters"), MakeShared<FGetNiagaraEmittersAction>());
		ActionHandlers.Add(TEXT("add_niagara_emitter"), MakeShared<FAddNiagaraEmitterAction>());
		ActionHandlers.Add(TEXT("remove_niagara_emitter"), MakeShared<FRemoveNiagaraEmitterAction>());
		ActionHandlers.Add(TEXT("set_niagara_emitter_property"), MakeShared<FSetNiagaraEmitterPropertyAction>());
		ActionHandlers.Add(TEXT("duplicate_niagara_emitter"), MakeShared<FDuplicateNiagaraEmitterAction>());
		ActionHandlers.Add(TEXT("reorder_niagara_emitter"), MakeShared<FReorderNiagaraEmitterAction>());
		ActionHandlers.Add(TEXT("rename_niagara_emitter"), MakeShared<FRenameNiagaraEmittersAction>());
		ActionHandlers.Add(TEXT("add_niagara_event_handler"), MakeShared<FAddNiagaraEventHandlerAction>());
		ActionHandlers.Add(TEXT("add_niagara_simulation_stage"), MakeShared<FAddNiagaraSimulationStageAction>());
		ActionHandlers.Add(TEXT("get_niagara_event_handlers"), MakeShared<FGetNiagaraEventHandlersAction>());
		ActionHandlers.Add(TEXT("list_niagara_emitter_templates"), MakeShared<FListNiagaraEmitterTemplatesAction>());
		ActionHandlers.Add(TEXT("get_niagara_emitter_attributes"), MakeShared<FGetNiagaraEmitterAttributesAction>());

		// Register module actions.
		ActionHandlers.Add(TEXT("get_niagara_modules"), MakeShared<FGetNiagaraModulesAction>());
		ActionHandlers.Add(TEXT("add_niagara_module"), MakeShared<FAddNiagaraModuleAction>());
		ActionHandlers.Add(TEXT("remove_niagara_module"), MakeShared<FRemoveNiagaraModuleAction>());
		ActionHandlers.Add(TEXT("set_niagara_module_enabled"), MakeShared<FSetNiagaraModuleEnabledAction>());
		ActionHandlers.Add(TEXT("reorder_niagara_module"), MakeShared<FReorderNiagaraModuleAction>());
		ActionHandlers.Add(TEXT("get_niagara_module_inputs"), MakeShared<FGetNiagaraModuleInputsAction>());
		ActionHandlers.Add(TEXT("set_niagara_module_input"), MakeShared<FSetNiagaraModuleInputAction>());
		ActionHandlers.Add(TEXT("set_niagara_dynamic_input"), MakeShared<FSetNiagaraDynamicInputAction>());
		ActionHandlers.Add(TEXT("set_niagara_curve"), MakeShared<FSetNiagaraCurveAction>());
		ActionHandlers.Add(TEXT("get_niagara_rapid_iteration_parameters"), MakeShared<FGetNiagaraRapidIterationParametersAction>());
		ActionHandlers.Add(TEXT("set_niagara_rapid_iteration_parameter"), MakeShared<FSetNiagaraRapidIterationParameterAction>());
		ActionHandlers.Add(TEXT("list_niagara_modules"), MakeShared<FListNiagaraModulesAction>());
		ActionHandlers.Add(TEXT("get_niagara_module_input_binding"), MakeShared<FGetNiagaraModuleInputBindingAction>());
		ActionHandlers.Add(TEXT("clear_niagara_module_input"), MakeShared<FClearNiagaraModuleInputAction>());
		ActionHandlers.Add(TEXT("list_niagara_input_source_menu"), MakeShared<FListNiagaraInputSourceMenuAction>());
		ActionHandlers.Add(TEXT("resolve_niagara_built_in_dynamic_input"), MakeShared<FResolveNiagaraBuiltInDynamicInputAction>());

		// Register parameter actions.
		ActionHandlers.Add(TEXT("get_niagara_user_parameters"), MakeShared<FGetNiagaraUserParametersAction>());
		ActionHandlers.Add(TEXT("add_niagara_user_parameter"), MakeShared<FAddNiagaraUserParameterAction>());
		ActionHandlers.Add(TEXT("set_niagara_user_parameter"), MakeShared<FSetNiagaraUserParameterAction>());
		ActionHandlers.Add(TEXT("remove_niagara_user_parameter"), MakeShared<FRemoveNiagaraUserParameterAction>());
		ActionHandlers.Add(TEXT("link_niagara_parameter"), MakeShared<FLinkNiagaraParameterAction>());
		ActionHandlers.Add(TEXT("list_niagara_parameter_types"), MakeShared<FListNiagaraParameterTypesAction>());
		ActionHandlers.Add(TEXT("list_niagara_data_interfaces"), MakeShared<FListNiagaraDataInterfacesAction>());
		ActionHandlers.Add(TEXT("list_niagara_script_parameters"), MakeShared<FListNiagaraScriptParametersAction>());
		ActionHandlers.Add(TEXT("add_niagara_script_parameter"), MakeShared<FAddNiagaraScriptParameterAction>());
		ActionHandlers.Add(TEXT("remove_niagara_script_parameter"), MakeShared<FRemoveNiagaraScriptParameterAction>());
		ActionHandlers.Add(TEXT("rename_niagara_script_parameter"), MakeShared<FRenameNiagaraScriptParameterAction>());

		// Register renderer actions.
		ActionHandlers.Add(TEXT("add_niagara_renderer"), MakeShared<FAddNiagaraRendererAction>());
		ActionHandlers.Add(TEXT("remove_niagara_renderer"), MakeShared<FRemoveNiagaraRendererAction>());
		ActionHandlers.Add(TEXT("get_niagara_renderer_info"), MakeShared<FGetNiagaraRendererInfoAction>());
		ActionHandlers.Add(TEXT("set_niagara_renderer_property"), MakeShared<FSetNiagaraRendererPropertyAction>());
		ActionHandlers.Add(TEXT("set_niagara_renderer_binding"), MakeShared<FSetNiagaraRendererBindingAction>());
		ActionHandlers.Add(TEXT("get_niagara_renderer_properties"), MakeShared<FGetNiagaraRendererPropertiesAction>());

		// Register scratch-pad actions.
		ActionHandlers.Add(TEXT("create_niagara_scratch_pad_module"), MakeShared<FCreateNiagaraScratchPadModuleAction>());
		ActionHandlers.Add(TEXT("duplicate_niagara_scratch_pad_module"), MakeShared<FDuplicateNiagaraScratchPadModuleAction>());
		ActionHandlers.Add(TEXT("delete_niagara_scratch_pad_module"), MakeShared<FDeleteNiagaraScratchPadModuleAction>());
		ActionHandlers.Add(TEXT("rename_niagara_scratch_pad_module"), MakeShared<FRenameNiagaraScratchPadModuleAction>());
		ActionHandlers.Add(TEXT("list_niagara_scratch_pad_modules"), MakeShared<FListNiagaraScratchPadModulesAction>());
		ActionHandlers.Add(TEXT("apply_niagara_scratch_pad"), MakeShared<FApplyNiagaraScratchPadAction>());
		ActionHandlers.Add(TEXT("apply_and_save_niagara_scratch_pad"), MakeShared<FApplyAndSaveNiagaraScratchPadAction>());
		ActionHandlers.Add(TEXT("find_niagara_scratch_pad_usage"), MakeShared<FFindNiagaraScratchPadUsageAction>());
		ActionHandlers.Add(TEXT("create_niagara_module_asset"), MakeShared<FCreateNiagaraModuleAssetAction>());
		ActionHandlers.Add(TEXT("set_niagara_scratch_pad_hlsl"), MakeShared<FSetNiagaraScratchPadHlslAction>());
		ActionHandlers.Add(TEXT("add_niagara_custom_hlsl_input"), MakeShared<FAddNiagaraCustomHlslInputAction>());
		ActionHandlers.Add(TEXT("add_niagara_custom_hlsl_output"), MakeShared<FAddNiagaraCustomHlslOutputAction>());
		ActionHandlers.Add(TEXT("rename_niagara_custom_hlsl_pin"), MakeShared<FRenameNiagaraCustomHlslPinAction>());
		ActionHandlers.Add(TEXT("remove_niagara_custom_hlsl_pin"), MakeShared<FRemoveNiagaraCustomHlslPinAction>());

		// Register graph actions.
		ActionHandlers.Add(TEXT("get_niagara_graph_nodes"), MakeShared<FGetNiagaraGraphNodesAction>());
		ActionHandlers.Add(TEXT("get_niagara_node_info"), MakeShared<FGetNiagaraNodeInfoAction>());
		ActionHandlers.Add(TEXT("trace_niagara_connection"), MakeShared<FTraceNiagaraConnectionAction>());
		ActionHandlers.Add(TEXT("validate_niagara_graph"), MakeShared<FValidateNiagaraGraphAction>());
		ActionHandlers.Add(TEXT("get_niagara_script_properties"), MakeShared<FGetNiagaraScriptPropertiesAction>());
		ActionHandlers.Add(TEXT("set_niagara_script_properties"), MakeShared<FSetNiagaraScriptPropertiesAction>());
		ActionHandlers.Add(TEXT("add_niagara_graph_node"), MakeShared<FAddNiagaraGraphNodeAction>());
		ActionHandlers.Add(TEXT("delete_niagara_graph_node"), MakeShared<FDeleteNiagaraGraphNodeAction>());
		ActionHandlers.Add(TEXT("list_niagara_data_interface_functions"), MakeShared<FListNiagaraDataInterfaceFunctionsAction>());
		ActionHandlers.Add(TEXT("list_niagara_ops"), MakeShared<FListNiagaraOpsAction>());
		ActionHandlers.Add(TEXT("list_niagara_node_types"), MakeShared<FListNiagaraNodeTypesAction>());
		ActionHandlers.Add(TEXT("get_niagara_node_type_info"), MakeShared<FGetNiagaraNodeTypeInfoAction>());
		ActionHandlers.Add(TEXT("search_niagara_functions"), MakeShared<FSearchNiagaraFunctionsAction>());
		ActionHandlers.Add(TEXT("describe_niagara_type"), MakeShared<FDescribeNiagaraTypeAction>());
		ActionHandlers.Add(TEXT("get_niagara_schema_actions"), MakeShared<FGetNiagaraSchemaActionsAction>());
		ActionHandlers.Add(TEXT("list_niagara_available_parameters"), MakeShared<FListNiagaraAvailableParametersAction>());
		ActionHandlers.Add(TEXT("add_niagara_map_get_pin"), MakeShared<FAddNiagaraMapGetPinAction>());
		ActionHandlers.Add(TEXT("add_niagara_map_set_pin"), MakeShared<FAddNiagaraMapSetPinAction>());
		ActionHandlers.Add(TEXT("add_niagara_node_pin"), MakeShared<FAddNiagaraNodePinAction>());
		ActionHandlers.Add(TEXT("rename_niagara_node_pin"), MakeShared<FRenameNiagaraNodePinAction>());
		ActionHandlers.Add(TEXT("remove_niagara_node_pin"), MakeShared<FRemoveNiagaraNodePinAction>());
		ActionHandlers.Add(TEXT("connect_niagara_pins"), MakeShared<FConnectNiagaraPinsAction>());
		ActionHandlers.Add(TEXT("disconnect_niagara_pins"), MakeShared<FDisconnectNiagaraPinsAction>());
		ActionHandlers.Add(TEXT("get_niagara_data_interface_schema"), MakeShared<FGetNiagaraDataInterfaceSchemaAction>());

		// Register runtime actions.
		ActionHandlers.Add(TEXT("spawn_niagara_effect"), MakeShared<FSpawnNiagaraEffectAction>());
		ActionHandlers.Add(TEXT("control_niagara_effect"), MakeShared<FControlNiagaraEffectAction>());
		ActionHandlers.Add(TEXT("add_niagara_component"), MakeShared<FAddNiagaraComponentAction>());
		ActionHandlers.Add(TEXT("get_niagara_actors"), MakeShared<FGetNiagaraActorsAction>());
	}
}

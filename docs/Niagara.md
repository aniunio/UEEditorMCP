## Niagara System 层级结构

```
Niagara System
  └─ System 级 Graph（System Spawn/Update）
  └─ Emitter A
       ├─ Emitter Spawn Graph
       ├─ Emitter Update Graph
       ├─ Particle Spawn Graph
       ├─ Particle Update Graph
       └─ ...
  └─ Emitter B
       └─ ...（同上）
```

## Niagara Action 工具说明

本文档对应 `Plugins/UEEditorMCP/Source/UEEditorMCP/Private/Actions/NiagaraActions/NiagaraActions.cpp` 中注册的 Niagara Actions。标题中的第一个名字是 C++ command，括号里是 Python registry 暴露给 `ue_actions_run` 的工具 ID。

通用返回约定：

- 成功时一般返回 `success: true`，并附带本 action 的数据字段。
- 失败时一般返回 `success: false`、`error`、`error_type`。部分业务失败会额外返回可恢复信息，例如 `delete_niagara_system` 在有引用且未 `force` 时返回 `referencers`。

通用参数约定：

- `system_path`：Niagara System 资源路径，例如 `/Game/FX/NS_Test.NS_Test`。
- `emitter_name`：System 内 Emitter 名称，支持源码中的名称匹配逻辑。
- `script_usage` 支持：`emitter_spawn`、`emitter_update`、`particle_spawn`、`particle_update`、`system_spawn`、`system_update`、`module`、`dynamic_input`、`function`。多数 Emitter Stack action 主要使用前四类 Emitter/Particle usage。
- 图读取定位参数：传 `system_path + module_name` 读取 Scratch Pad 图，或 `system_path + emitter_name + script_usage` 读取 Emitter Stack 图，或 `script_path` 读取独立 Niagara Script 图。
- 图修改定位参数：传 `system_path + module_name` 修改 Scratch Pad 图，或传 `script_path` 修改独立 Niagara Script 图。
- Scratch Pad 图定位参数：必须传 `system_path + module_name`。
- 节点选择参数：可传 `node_id`、`node_index` 或 `node_class`。连接类 action 使用 `from_node_id/from_node_index/from_node_class` 和 `to_node_id/to_node_index/to_node_class`。
- Niagara 类型字符串常用值：`float`、`int`、`bool`、`vec2`、`vec3`、`vec4`、`color`、`quat`、`matrix`、`position`、`parametermap`、`id`，也可使用已注册 Niagara 类型名。可通过 `list_niagara_parameter_types`、`describe_niagara_type` 辅助查询。

## System Actions

### `create_niagara_system` (`niagara.create_system`)

- 作用：创建 Niagara System 资源，可从模板复制，也可创建空 System。
- 传参：必填 `asset_path`。可选 `template`，默认 `empty`；支持 `empty`、`simple_sprite`/`sprite`、`fountain`/`default`、`mesh`、`ribbon`、`minimal`，也可传模板资源路径。
- 返回：`success`、`asset_name`、`asset_path`、`package_path`、`template`、`template_applied`、`saved`、`emitter_count`；复制模板资源时还返回 `template_copy_mode`。

### `get_niagara_system_info` (`niagara.get_system_info`)

- 作用：读取 Niagara System 概览、Emitter、User 参数和编译状态。
- 传参：必填 `system_path`。可选 `include`，默认 `all`，可包含 `emitters`、`parameters`、`compilation`；可选 `filter` 过滤 Emitter 或参数名。
- 返回：`success`、`system_name`、`asset_path`、`emitter_count`；按 `include` 返回 `emitters`、`parameters`、`compilation`，其中 `compilation` 含 `is_valid`、`is_ready_to_run`。

### `list_niagara_systems` (`niagara.list_systems`)

- 作用：从 Asset Registry 列出项目里的 Niagara System。
- 传参：可选 `path`，默认 `/Game`；可选 `name_filter`；可选 `max_results`，默认 `100`。
- 返回：`success`、`systems`、`count`；`systems` 元素含 `name`、`path`。

### `delete_niagara_system` (`niagara.delete_system`)

- 作用：删除 Niagara System 资源，默认会先检查外部引用。
- 传参：必填 `system_path`。可选 `force`，为 `true` 时跳过引用保护。
- 返回：删除成功返回 `success`、`system_path`；删除失败返回 `success: false`、`error`；未 `force` 且有引用时返回 `referencers`。

### `compile_niagara_system` (`niagara.compile_system`)

- 作用：请求编译 Niagara System，并返回脚本编译状态。
- 传参：必填 `system_path`。可选 `wait_for_completion`，默认 `true`。
- 返回：`success`、`is_valid`、`is_ready_to_run`、`has_outstanding_compilations`、`error_count`、`script_statuses`。

### `set_niagara_system_property` (`niagara.set_system_property`)

- 作用：通过 UPROPERTY 名称设置 Niagara System 上的属性，并重新编译保存。
- 传参：必填 `system_path`、`property`、`value`。可选 `filter`，用于属性找不到时缩小错误提示里的候选属性。
- 返回：成功返回 `success`、`property`、`value`；失败时错误信息会包含可用属性提示。

### `get_niagara_system_errors` (`niagara.get_system_errors`)

- 作用：收集 Niagara System 的编译问题、系统消息和 Renderer 反馈。
- 传参：必填 `system_path`。可选 `emitter_name` 过滤 Emitter；可选 `severity`，默认 `all`。
- 返回：`success`、`system_path`、`issue_count`、`issues`；`issues` 元素描述问题级别、来源、消息和相关脚本/Emitter。

### `get_niagara_particle_stats` (`niagara.get_particle_stats`)

- 作用：读取运行时或预览实例中的粒子统计信息。
- 传参：必填 `system_path`。可选 `emitter_name` 过滤 Emitter；可选 `actor_name` 指定场景中的 Niagara Actor。
- 返回：`success`、`system_path`、可选 `actor_name`、`emitter_count`、`emitters`；`emitters` 元素含 `name`、`num_particles`、`total_spawned`、`execution_state`、`is_enabled`、`is_active`、可选 `bounds`。未找到活动实例时返回 `message`。

### `set_niagara_playback_range` (`niagara.set_playback_range`)

- 作用：设置 Niagara System 编辑器预览播放范围和帧率。
- 传参：必填 `system_path`、`range_end`。可选 `range_start`，默认 `0`；可选 `frame_rate`，默认 `60`。`range_end` 必须大于 `range_start`。
- 返回：`success`、`system_path`、`range_start`、`range_end`、`frame_rate`。

### `get_niagara_playback_range` (`niagara.get_playback_range`)

- 作用：读取 Niagara System 编辑器预览播放范围、帧率和锁定状态。
- 传参：必填 `system_path`。
- 返回：`success`、`system_path`、`range_start`、`range_end`、`frame_rate`、`frame_rate_denominator`、`is_locked`。

### `get_niagara_module_versions` (`niagara.get_module_versions`)

- 作用：检查指定 Emitter Stack 中模块的版本信息，用于发现过期模块。
- 传参：必填 `system_path`、`emitter_name`。可选 `filter` 过滤模块名。
- 返回：`success`、`system_path`、`emitter_name`、`module_count`、`modules`；`modules` 元素包含模块名、脚本 usage、当前版本、可用版本和是否过期等信息。

### `upgrade_niagara_module_version` (`niagara.upgrade_module_version`)

- 作用：把指定模块节点升级到最新版本或指定版本。
- 传参：必填 `system_path`、`emitter_name`、`module_name`、`script_usage`。可选 `target_version`，默认 `latest`。
- 返回：`success`、`system_path`、`emitter_name`、`module_name`、`script_usage`、`previous_version`、`current_version`、`changed`。

## Emitter Actions

### `get_niagara_emitters` (`niagara.get_emitters`)

- 作用：列出 Niagara System 内的 Emitters。
- 传参：必填 `system_path`。可选 `filter` 过滤 Emitter 名称。
- 返回：`success`、`emitters`、`count`；`emitters` 元素包含名称、ID、索引、启用状态和基础 Emitter 信息。

### `add_niagara_emitter` (`niagara.add_emitter`)

- 作用：向 Niagara System 添加 Emitter，可来自资源路径、模板或默认 Simple Sprite Burst。
- 传参：必填 `system_path`。可选 `emitter_path`、`template`、`emitter_name`。
- 返回：`success`、`emitter_name`、`emitter_id`、`emitter_index`、`total_emitters`、`previous_emitter_count`。

### `remove_niagara_emitter` (`niagara.remove_emitter`)

- 作用：按名称从 Niagara System 删除 Emitter。
- 传参：必填 `system_path`、`emitter_name`。
- 返回：`success`、`removed_emitter`、`remaining_emitters`。

### `set_niagara_emitter_property` (`niagara.set_emitter_property`)

- 作用：设置 Emitter 常用属性并重新编译保存。
- 传参：必填 `system_path`、`emitter_name`、`property`、`value`。`property` 支持 `enabled`、`sim_target`、`local_space`、`determinism`、`bounds_mode`、`max_particles`。`sim_target` 值为 `cpu` 或 `gpu`；`bounds_mode` 值为 `dynamic`、`fixed` 或 `programmable`。
- 返回：`success`、`emitter_name`、`property`、`value`。

### `duplicate_niagara_emitter` (`niagara.duplicate_emitter`)

- 作用：复制指定 Emitter。
- 传参：必填 `system_path`、`emitter_name`。可选 `new_name`。
- 返回：`success`、`new_emitter_name`、`new_emitter_id`、`total_emitters`。

### `reorder_niagara_emitter` (`niagara.reorder_emitter`)

- 作用：调整 Emitter 在 System Emitter 列表中的顺序。
- 传参：必填 `system_path`、`emitter_name`、`new_index`。
- 返回：`success`、`emitter_name`、`old_index`、`new_index`。

### `rename_niagara_emitter` (`niagara.rename_emitter`)

- 作用：重命名 Niagara System 内的 Emitter。
- 传参：必填 `system_path`、`emitter_name`、`new_name`。
- 返回：`success`、`old_name`、`new_name`、`emitter_id`、`emitter_index`。

### `add_niagara_event_handler` (`niagara.add_event_handler`)

- 作用：给 Emitter 添加事件处理脚本属性。
- 传参：必填 `system_path`、`emitter_name`。可选 `source_emitter`、`event_name`，默认 `CollisionEvent`；可选 `execution_mode`，支持 `spawned_particles`/`spawn`、`every_particle`/`all`、`single_particle`/`single`；可选 `max_events_per_frame`、`random_spawn_number`、`spawn_number`。
- 返回：`success`、`emitter_name`、`event_name`、`execution_mode`、`event_handler_index`、`total_handlers`。

### `add_niagara_simulation_stage` (`niagara.add_simulation_stage`)

- 作用：给 Emitter 添加通用 Simulation Stage。
- 传参：必填 `system_path`、`emitter_name`。可选 `stage_name`，默认 `SimulationStage`；可选 `iteration_source`，支持 `particles`/`particle`、`data_interface`/`datainterface`；可选 `num_iterations`，默认 `1`；可选 `enabled`，默认 `true`。
- 返回：`success`、`emitter_name`、`stage_name`、`iteration_source`、`num_iterations`、`stage_index`、`total_stages`。

### `get_niagara_event_handlers` (`niagara.get_event_handlers`)

- 作用：读取指定 Emitter 的 Event Handlers 和 Simulation Stages。
- 传参：必填 `system_path`、`emitter_name`。
- 返回：`success`、`emitter_name`、`event_handlers`、`event_handler_count`、`simulation_stages`、`simulation_stage_count`；事件元素含 `index`、`event_name`、`execution_mode`、`max_events_per_frame`、`spawn_number`、`random_spawn_number`、`source_emitter`；Stage 元素含 `index`、`name`、`enabled`、`simulation_stage_name`、`num_iterations`、`iteration_source`。

### `list_niagara_emitter_templates` (`niagara.list_emitter_templates`)

- 作用：列出内置 Emitter 模板路径，供 `add_niagara_emitter` 使用。
- 传参：可选 `category`，默认 `all`；支持源码中登记的 `sprites`、`meshes`、`ribbons`、`beams`、`behaviors`。
- 返回：`success`、`category_filter`、`templates`、`count`；模板元素含 `path`、`name`、`category`、`description`。

### `get_niagara_emitter_attributes` (`niagara.get_emitter_attributes`)

- 作用：列出 Emitter 中可用的快速迭代参数和常见粒子属性。
- 传参：必填 `system_path`、`emitter_name`。可选 `filter`。
- 返回：`success`、`emitter_name`、`attributes`、`count`；属性元素含 `name`、`type`、`scope`、`source`，`source` 可能是 `rapid_iteration` 或 `well_known`。

## Module Actions

### `get_niagara_modules` (`niagara.get_modules`)

- 作用：读取指定 Emitter Stack 的模块链。
- 传参：必填 `system_path`、`emitter_name`。可选 `script_usage`，默认 `all`；可选 `include_inputs`，默认 `true`；可选 `filter`。
- 返回：`success`、`system_path`、`emitter_name`、`modules`、`count`；模块元素含模块名、索引、脚本 usage、启用状态、节点 ID 和可选输入信息。

### `add_niagara_module` (`niagara.add_module`)

- 作用：把 Niagara Module Script 插入到指定 Emitter Stack。
- 传参：必填 `system_path`、`emitter_name`、`module_path`、`script_usage`。可选 `index`，未传则插入默认位置；可选 `suggested_name`。
- 返回：`success`、`system_path`、`emitter_name`、`module_name`、`module_path`、`script_usage`、`input_pins`。

### `remove_niagara_module` (`niagara.remove_module`)

- 作用：从指定 Emitter Stack 删除模块，并尝试重连上下游执行链。
- 传参：必填 `system_path`、`emitter_name`、`module_name`、`script_usage`。
- 返回：`success`、`removed_module`、`emitter_name`、`script_usage`。

### `set_niagara_module_enabled` (`niagara.set_module_enabled`)

- 作用：启用或禁用指定模块节点。
- 传参：必填 `system_path`、`emitter_name`、`module_name`、`script_usage`。可选 `enabled`，默认 `true`。
- 返回：`success`、`module_name`、`enabled`。

### `reorder_niagara_module` (`niagara.reorder_module`)

- 作用：调整指定 Stack 中模块节点的顺序。
- 传参：必填 `system_path`、`emitter_name`、`module_name`、`script_usage`、`new_index`。
- 返回：`success`、`module_name`、`old_index`、`new_index`；如果位置未变，返回 `index`、`note`。

### `get_niagara_module_inputs` (`niagara.get_module_inputs`)

- 作用：列出模块输入参数，兼容直接 FunctionCall Pin 和 ParameterMap 风格的 `Module.*` 输入。
- 传参：必填 `system_path`、`emitter_name`、`module_name`、`script_usage`。可选 `input_filter`；可选 `include_schema`。
- 返回：`success`、`module_name`、`script_usage`、`inputs`、`count`；输入元素含 `name`、`type`、`default_value`、`is_connected`、`source`，ParameterMap 输入还含 `aliased_name`，启用 schema 时含 `type_schema`。

### `set_niagara_module_input` (`niagara.set_module_input`)

- 作用：给模块输入设置静态默认值；支持嵌套动态输入路径，例如 `Outer.Inner`。
- 传参：必填 `system_path`、`emitter_name`、`module_name`、`input_name`、`script_usage`、`value`。`value` 可为字符串、数字、布尔、对象或数组，按目标 Niagara 类型序列化。
- 返回：`success`、`module_name`、`input_name`、`input_source`、`input_type`、`value`；嵌套路径返回 `leaf_name`、`type`、`path`。

### `set_niagara_dynamic_input` (`niagara.set_dynamic_input`)

- 作用：给模块输入绑定动态输入，支持随机范围、参数链接、自定义 HLSL 表达式或任意 DynamicInput 脚本。
- 传参：必填 `system_path`、`emitter_name`、`module_name`、`input_name`、`script_usage`、`dynamic_input_type`。`dynamic_input_type` 支持 `random_range`、`uniform_random`、`parameter_link`、`custom_expression`、`script`/`asset`。随机范围可传 `min_value`、`max_value`；参数链接必填 `parameter_name`；自定义表达式必填 `expression`；脚本方式必填 `dynamic_input_script_path`，可选 `suggested_name`、`pin_defaults`。
- 返回：`success`、`module_name`、`input_name`、`dynamic_input_type`，并按类型返回 `script_path`、`dynamic_input_created`、`source_type`、`target_type`、`types_differ`、`actual_target_type`、`linked_parameter`、`type_conversion_used`、`conversion_script`、`expression`、`input_type` 等调试字段。

### `set_niagara_curve` (`niagara.set_curve`)

- 作用：给模块输入设置曲线数据，必要时创建或使用曲线 Data Interface。
- 传参：必填 `system_path`、`emitter_name`、`module_name`、`input_name`、`script_usage`、`curve_type`、`keys`。`keys` 元素至少包含 `time`、`value`，可选 `arrive_tangent`、`leave_tangent`、`interp_mode`；颜色值可用 `{r,g,b,a}`，向量值可用 `{x,y,z}` 或数组。
- 返回：`success`、`module_name`、`input_name`、`curve_input_name`、`curve_type`、`key_count`，以及 `dynamic_input_used`、`direct_data_interface`、`stateless_emitter`、`property_name` 等实现路径字段。

### `get_niagara_rapid_iteration_parameters` (`niagara.get_rapid_iteration_parameters`)

- 作用：读取 Emitter 脚本上的 Rapid Iteration 参数。
- 传参：必填 `system_path`、`emitter_name`。可选 `script_usage`、`filter`。
- 返回：`success`、`emitter_name`、`parameters`、`count`；参数元素含 `name`、`module_name`、`input_name`、`type`、`script_usage`。

### `set_niagara_rapid_iteration_parameter` (`niagara.set_rapid_iteration_parameter`)

- 作用：设置指定模块输入对应的 Rapid Iteration 参数值。
- 传参：必填 `system_path`、`emitter_name`、`module_name`、`input_name`、`script_usage`、`value`。向量、颜色等类型可按 `{x,y,z,w}` 或 `{r,g,b,a}` 传对象。
- 返回：`success`、`module_name`、`input_name`、`parameter_name`、`script_usage`、`type`、`value`。

### `list_niagara_modules` (`niagara.list_modules`)

- 作用：列出可添加的 Niagara Module Script。
- 传参：可选 `category`，默认 `all`；可选 `search`。
- 返回：`success`、`category_filter`、`search_filter`、`modules`、`count`；模块元素含 `name`、`path`、`category`、`description`、`script_path`、`function_name`，部分元素带 `children` 或默认输入值信息。

### `get_niagara_module_input_binding` (`niagara.get_module_input_binding`)

- 作用：读取模块输入当前的绑定模式，支持递归展开动态输入子输入。
- 传参：必填 `system_path`、`emitter_name`、`module_name`、`script_usage`。可选 `input_filter`；可选 `max_depth`，默认 `3`。
- 返回：`success`、`system_path`、`emitter_name`、`module_name`、`script_usage`、`input_count`、`inputs`；输入元素包含 `name`、`type`、`mode`、`value`、`linked_parameter`、`script_path`、`children` 等字段。

### `clear_niagara_module_input` (`niagara.clear_module_input`)

- 作用：把模块输入重置为默认值，相当于 Stack UI 的 Reset to Default，并清理孤立上游节点。
- 传参：必填 `system_path`、`emitter_name`、`module_name`、`script_usage`、`input_name`。`input_name` 支持点号路径进入嵌套动态输入。
- 返回：`success`、`was_overridden`；实际清理时返回 `input_name`、`removed_node_count`；原本就是默认值时返回 `message`。

### `list_niagara_input_source_menu` (`niagara.list_input_source_menu`)

- 作用：复现指定输入在 Niagara Stack UI 中的 Source 下拉菜单，列出可用动态输入和可链接参数。
- 传参：必填 `system_path`、`emitter_name`、`module_name`、`script_usage`、`input_name`。可选 `name_filter`。
- 返回：`success`、`input_name`、可选 `input_type`、`dynamic_input_count`、`dynamic_inputs`、`link_parameter_count`、`link_parameters`；动态输入元素含 `name`、`script_path`、`source`、`category`，链接参数元素含 `name`、`type`、`namespace`。

### `resolve_niagara_built_in_dynamic_input` (`niagara.resolve_built_in_dynamic_input`)

- 作用：通过 Asset Registry 查找内置 DynamicInput 脚本资源路径。
- 传参：可选 `name_filter`、`exact_name`、`max_results`，默认 `20`。
- 返回：`success`、`total_scanned`、`match_count`、`matches`；匹配元素含 `name`、`path`、`package`。

## Parameter Actions

### `get_niagara_user_parameters` (`niagara.get_user_parameters`)

- 作用：读取 Niagara System 资源或场景 Niagara Actor 使用的 System 上的 User 参数。
- 传参：必填二选一：`system_path` 或 `actor_name`。可选 `filter`。
- 返回：`success`、`source`、`system_name`、`parameters`、`count`；参数元素含 `name`、`type`、`value_type`。

### `add_niagara_user_parameter` (`niagara.add_user_parameter`)

- 作用：向 Niagara System 添加 User 参数。
- 传参：必填 `system_path`、`parameter_name`。可选 `parameter_type`，默认 `float`；可选 `default_value`。未以 `User.` 开头的名称会自动补前缀。
- 返回：`success`、`parameter_name`、`parameter_type`、`kind`、`size_bytes`，Data Interface 类型还返回 `class_path`。

### `set_niagara_user_parameter` (`niagara.set_user_parameter`)

- 作用：设置 User 参数值，可设置 System 资源默认值，也可设置场景 Actor 实例运行时值。
- 传参：必填 `parameter_name`、`value`。必填二选一：`system_path` 或 `actor_name`。可选 `parameter_type`，运行时模式支持 `float`、`int`、`bool`、`vector`/`vec3`、`color`/`linear_color`；资源默认值按实际参数类型写入。
- 返回：`success`、`parameter_name`、`mode`；运行时模式还返回 `actor_name`，资源默认值模式还返回 `parameter_type`。

### `remove_niagara_user_parameter` (`niagara.remove_user_parameter`)

- 作用：从 Niagara System 删除 User 参数。
- 传参：必填 `system_path`、`parameter_name`。
- 返回：`success`、`removed_parameter`。

### `link_niagara_parameter` (`niagara.link_parameter`)

- 作用：把模块输入链接到指定 Niagara 参数。
- 传参：必填 `system_path`、`emitter_name`、`module_name`、`input_name`、`linked_parameter`。可选 `script_usage`，源码读取时会按参数解析对应 Stack。
- 返回：`success`、`module_name`、`input_name`、`input_type`、`input_source`、`linked_parameter`、`script_usage`。

### `list_niagara_parameter_types` (`niagara.list_parameter_types`)

- 作用：列出可用于 Niagara 参数、Pin、User 参数的类型。
- 传参：可选 `filter`、`kind`、`scope`、`max_results`。
- 返回：`success`、`scope`、`kind_filter`、可选 `name_filter`、`types`、`count`、`total_matched`、`max_results`；类型元素含 `name`、`short_name`、`kind`、`category`、`description`、`size_bytes`、`class_path`、`struct_path`、`enum_path`、`num_entries` 等。

### `list_niagara_data_interfaces` (`niagara.list_data_interfaces`)

- 作用：列出可用的 `UNiagaraDataInterface` 子类。
- 传参：可选 `filter`。
- 返回：`success`、`filter`、`data_interfaces`、`count`；元素含 `class_name`、`class_path`、`display_name`。

### `list_niagara_script_parameters` (`niagara.list_script_parameters`)

- 作用：列出 Scratch Pad 或独立 Niagara Script 图中的输入/输出脚本参数。
- 传参：图修改定位参数。
- 返回：`success`、`script_path`、`outputs`、`inputs`；参数元素含名称、类型、索引，输入还可能含 `default_mode`。

### `add_niagara_script_parameter` (`niagara.add_script_parameter`)

- 作用：向 Scratch Pad 或独立 Niagara Script 图添加输入或输出参数。
- 传参：图修改定位参数，必填 `name`、`type`。可选 `direction`，默认 `output`；非 `output` 时作为输入元数据处理，未带命名空间会补 `Module.`。
- 返回：`success`、`direction`、`name`、`type`。

### `remove_niagara_script_parameter` (`niagara.remove_script_parameter`)

- 作用：从 Scratch Pad 或独立 Niagara Script 图删除输入或输出参数，并清理相关 Map Get/Set Pin。
- 传参：图修改定位参数，必填 `name`。可选 `direction`，默认 `output`。
- 返回：`success`、`removed_graph_count`、`name`。

### `rename_niagara_script_parameter` (`niagara.rename_script_parameter`)

- 作用：重命名 Scratch Pad 或独立 Niagara Script 图中的输入或输出参数。
- 传参：图修改定位参数，必填 `old_name`、`new_name`。可选 `direction`，默认 `output`。
- 返回：`success`、`old_name`、`new_name`、`renamed_graph_count`。

## Renderer Actions

### `add_niagara_renderer` (`niagara.add_renderer`)

- 作用：给 Emitter 添加 Renderer。
- 传参：必填 `system_path`、`emitter_name`、`renderer_type`。`renderer_type` 支持 `sprite`、`mesh`、`ribbon`、`light`、`component`。可选 `material_path`；Mesh Renderer 可选 `mesh_path`。
- 返回：`success`、`renderer_type`、`renderer_index`、`emitter_name`、`renderer_count`。

### `remove_niagara_renderer` (`niagara.remove_renderer`)

- 作用：按索引删除 Emitter Renderer。
- 传参：必填 `system_path`、`emitter_name`。可选 `renderer_index`，默认 `0`。
- 返回：`success`、`removed_renderer`、`removed_index`、`remaining_count`。

### `get_niagara_renderer_info` (`niagara.get_renderer_info`)

- 作用：读取 Emitter Renderer 列表或指定 Renderer 信息。
- 传参：必填 `system_path`、`emitter_name`。可选 `renderer_index`，未传则返回全部。
- 返回：`success`、`emitter_name`、`renderers`、`count`；Renderer 元素含类型、索引、启用状态、排序、材质/网格等可序列化信息。

### `set_niagara_renderer_property` (`niagara.set_renderer_property`)

- 作用：设置 Renderer 属性，内置支持材质、网格、排序、启用、FacingMode，也可通过 UPROPERTY 名称设置其他可编辑属性。
- 传参：必填 `system_path`、`emitter_name`、`value`，并必填 `property` 或 `property_name`。可选 `renderer_index`，默认 `0`。常用属性包括 `material`/`material_path`、`mesh`/`mesh_path`、`sort_order`/`sort_order_hint`、`enabled`/`is_enabled`、`facing_mode`。
- 返回：`success`、`property`、`value`；反射属性还返回 `renderer_type`。

### `set_niagara_renderer_binding` (`niagara.set_renderer_binding`)

- 作用：设置 Renderer 的 `FNiagaraVariableAttributeBinding` 类型绑定属性。
- 传参：必填 `system_path`、`emitter_name`、`binding_name`，并必填 `attribute_name` 或 `attribute`。可选 `renderer_index`，默认 `0`。未带命名空间的属性会按 `Particles.<name>` 处理。
- 返回：`success`、`binding_name`、`attribute_name`、`renderer_index`。

### `get_niagara_renderer_properties` (`niagara.get_renderer_properties`)

- 作用：列出指定 Renderer 可编辑 UPROPERTY 及当前值。
- 传参：必填 `system_path`、`emitter_name`。可选 `renderer_index`，默认 `0`；可选 `filter`。
- 返回：`success`、`renderer_type`、`renderer_index`、`properties`、`count`、可选 `filter`；属性元素含 `name`、`type`、`value`，枚举属性还含 `valid_values`。

## Scratch Pad Actions

### `create_niagara_scratch_pad_module` (`niagara.create_scratch_pad_module`)

- 作用：在 Niagara System 中创建 Scratch Pad 脚本。
- 传参：必填 `system_path`。可选 `module_name`，默认 `ScratchPadModule`；可选 `module_type`，默认 `module`，支持 `module`、`dynamic_input`、`function`。
- 返回：`success`、`module_name`、`system_path`、`module_type`、`used_template`、`scratch_pad_count`。

### `duplicate_niagara_scratch_pad_module` (`niagara.duplicate_scratch_pad_module`)

- 作用：复制 Niagara System 中的 Scratch Pad 脚本。
- 传参：必填 `system_path`、`module_name`。可选 `new_name`，默认 `<module_name>_Copy`。
- 返回：`success`、`source_name`、`new_name`、`scratch_pad_count`。

### `delete_niagara_scratch_pad_module` (`niagara.delete_scratch_pad_module`)

- 作用：从 Niagara System 删除 Scratch Pad 脚本。
- 传参：必填 `system_path`、`module_name`。
- 返回：`success`、`module_name`、`removed_index`、`scratch_pad_count`。

### `rename_niagara_scratch_pad_module` (`niagara.rename_scratch_pad_module`)

- 作用：重命名 Niagara System 中的 Scratch Pad 脚本。
- 传参：必填 `system_path`、`module_name`、`new_name`。
- 返回：`success`、`old_name`、`new_name`。

### `list_niagara_scratch_pad_modules` (`niagara.list_scratch_pad_modules`)

- 作用：列出 Niagara System 中的 Scratch Pad 脚本。
- 传参：必填 `system_path`。
- 返回：`success`、`system_path`、`modules`、`count`；模块元素含名称、索引、usage、路径等信息。

### `apply_niagara_scratch_pad` (`niagara.apply_scratch_pad`)

- 作用：应用 Scratch Pad 脚本改动；若有 Niagara 编辑器 ViewModel，则走编辑器应用流程，否则直接编译资产。
- 传参：必填 `system_path`、`module_name`。
- 返回：`success`、`system_path`、`module_name`、`action: "apply"`、`used_view_model`、`apply_mode`。

### `apply_and_save_niagara_scratch_pad` (`niagara.apply_and_save_scratch_pad`)

- 作用：应用并保存 Scratch Pad 脚本改动。
- 传参：必填 `system_path`、`module_name`。
- 返回：`success`、`system_path`、`module_name`、`action: "apply_and_save"`、`used_view_model`、`apply_mode`。

### `find_niagara_scratch_pad_usage` (`niagara.find_scratch_pad_usage`)

- 作用：反查 Scratch Pad 脚本在 System、Emitter Stack 或动态输入链中的使用位置。
- 传参：必填 `system_path`、`module_name`。
- 返回：`success`、`target_module`、`target_is_dynamic_input`、`usage_count`、`sites`；使用点元素含 `emitter`、`script_usage`、`function_name`、`node_id`、`is_dynamic_input`。

### `create_niagara_module_asset` (`niagara.create_module_asset`)

- 作用：创建独立 Niagara Script 资源，可用于 Module、Dynamic Input 或 Function。
- 传参：必填 `asset_path`。可选 `module_type`，默认 `module`，支持 `module`、`dynamic_input`、`function`；可选 `description`。
- 返回：`success`、`asset_path`、`asset_name`、`module_type`、`package_path`、`used_template`、`saved`。

### `set_niagara_scratch_pad_hlsl` (`niagara.set_scratch_pad_hlsl`)

- 作用：设置 Scratch Pad 模块中 Custom HLSL 节点的 HLSL 代码，并按需重建输入/输出 Pin。
- 传参：必填 `system_path`、`module_name`、`hlsl_code`。可选 `inputs`、`outputs`，元素为 `{ "name": "...", "type": "float" }`；可选 `clear_existing_pins`。
- 返回：`success`、`module_name`、`hlsl_length`、`inputs_added`、`outputs_added`、`cleared_existing_pins`。

### `add_niagara_custom_hlsl_input` (`niagara.add_custom_hlsl_input`)

- 作用：给 Scratch Pad 模块中的 Custom HLSL 节点添加输入 Pin。
- 传参：必填 `system_path`、`module_name`、`pin_name`。可选 `pin_type`，默认 `float`。
- 返回：`success`、`pin_name`、`pin_type`、`updated_node_count`。

### `add_niagara_custom_hlsl_output` (`niagara.add_custom_hlsl_output`)

- 作用：给 Scratch Pad 模块中的 Custom HLSL 节点添加输出 Pin。
- 传参：必填 `system_path`、`module_name`、`pin_name`。可选 `pin_type`，默认 `float`。
- 返回：`success`、`pin_name`、`pin_type`、`updated_node_count`。

### `rename_niagara_custom_hlsl_pin` (`niagara.rename_custom_hlsl_pin`)

- 作用：重命名 Scratch Pad 模块中 Custom HLSL 节点的动态 Pin，并更新相关节点签名。
- 传参：必填 `system_path`、`module_name`、`old_name`、`new_name`。
- 返回：`success`、`old_name`、`new_name`、`updated_node_count`。

### `remove_niagara_custom_hlsl_pin` (`niagara.remove_custom_hlsl_pin`)

- 作用：删除 Scratch Pad 模块中 Custom HLSL 节点的动态 Pin。
- 传参：必填 `system_path`、`module_name`、`pin_name`。
- 返回：`success`、`pin_name`、`updated_node_count`。

## Graph Actions

### `get_niagara_graph_nodes` (`niagara.get_graph_nodes`)

- 作用：读取 Niagara 图节点列表，支持摘要、连接、完整三种详细程度。
- 传参：图读取定位参数。可选 `verbosity`，支持 `summary`、`connections`、`full`，默认 `connections`；可选 `name_filter`、`type_filter`。
- 返回：`success`、`source`、`verbosity`、`nodes`、`total_nodes`、`returned`、`filtered_out`；节点元素含 `index`、`node_id`、`title`、`type`、位置、Pin 和连接信息。

### `get_niagara_node_info` (`niagara.get_node_info`)

- 作用：读取单个 Niagara 图节点的详细信息。
- 传参：图读取定位参数 + 节点选择参数。
- 返回：`success`、`source`、`node`；`node` 包含节点标题、类型、GUID、位置、Pin、连接和可序列化属性。

### `trace_niagara_connection` (`niagara.trace_connection`)

- 作用：从指定节点和 Pin 开始沿连接追踪上游或下游节点。
- 传参：图读取定位参数 + 节点选择参数。可选 `pin_name`、`direction`，默认由实现解析；可选 `max_depth`。
- 返回：`success`、`source`、`start_index`、`start_title`、`direction`、`max_depth`、`visited`、`nodes`；追踪节点元素含 `index`、`title`、`type`、`depth`。

### `validate_niagara_graph` (`niagara.validate_graph`)

- 作用：检查 Niagara 图的孤立节点、断链和缺失输入。
- 传参：图读取定位参数。
- 返回：`success`、`source`、`graph_clean`、`total_nodes`、`orphaned`、`dead_ends`、`missing_inputs`、`missing_input_count`；问题元素含 `index`、`title`、`type`。

### `get_niagara_script_properties` (`niagara.get_script_properties`)

- 作用：占位型读取 Script 属性 action；当前实现只返回成功，不返回额外属性。
- 传参：无强制参数。
- 返回：`success`。

### `set_niagara_script_properties` (`niagara.set_script_properties`)

- 作用：占位型设置 Script 属性 action；当前实现不修改属性，只返回成功。
- 传参：无强制参数。
- 返回：`success`。

### `add_niagara_graph_node` (`niagara.add_graph_node`)

- 作用：向 Scratch Pad 或独立 Niagara Script 图添加节点。
- 传参：图修改定位参数，必填 `node_type`。可选 `pos_x`、`pos_y`。当 `node_type` 为 `Op` 时必填 `op_name`；为 `FunctionCall` 时必填 `function_script`；为 `Input` 时必填 `input_name`、`input_type`；为 `DataInterfaceFunction` 时必填 `di_class`、`function_name`。Custom HLSL 可传 `hlsl_code`，部分节点可传 `name`、`type` 等属性。
- 返回：`success`、`node_type`、`node_index`、`node_id`。

### `delete_niagara_graph_node` (`niagara.delete_graph_node`)

- 作用：从 Scratch Pad 或独立 Niagara Script 图删除节点。
- 传参：图修改定位参数，必填 `node_index` 或 `node_id`。
- 返回：`success`、`node_class`、`deleted_graph_count`。

### `list_niagara_data_interface_functions` (`niagara.list_data_interface_functions`)

- 作用：列出指定 Data Interface 类可暴露给 Niagara 图的函数。
- 传参：必填 `di_class`。可选 `filter`、`include_pins`。
- 返回：`success`、`di_class`、`di_class_path`、`functions`、`returned`、`total_functions`；函数元素含 `name`、`description`、`member_function`、`read_function`、`write_function`、`supports_cpu`、`supports_gpu`，启用 Pin 时含 `inputs`、`outputs`。

### `list_niagara_ops` (`niagara.list_ops`)

- 作用：列出 Niagara Op 节点类型及上下文菜单信息。
- 传参：可选 `filter`、`category`、`exact_name`、`max_results`、`include_pins`。
- 返回：`success`、`filter`、`category_filter`、`exact_name`、`returned`、`total_ops`、`include_pins`、`all_categories`、`ops`；Op 元素含 `name`、`friendly_name`、`category`、`description`、`keywords`、`alternate_name`、`compact_name`，启用 Pin 时含 `inputs`、`outputs`。

### `list_niagara_node_types` (`niagara.list_node_types`)

- 作用：枚举可添加到 Niagara 图中的节点类、脚本资产和 Data Interface 类型。
- 传参：可选 `filter`、`kind`、`include_engine`、`max_results`。`kind` 支持 `all`、`node_class`、`module_script`、`dynamic_input_script`、`function_script`、`data_interface`。
- 返回：`success`、`filter`、`kind`、`nodes`、`count`；元素按 kind 返回 `type`、`name`、`class_path`、`script_path`、`display_name`、`category`、`tooltip`、`keywords`。

### `get_niagara_node_type_info` (`niagara.get_node_type_info`)

- 作用：读取 Niagara 节点类型或 Niagara Script 资产的 Pin/属性 schema。
- 传参：必填 `type`。可选 `script_path`，传入时按脚本资产解析。
- 返回：`success`、`type`、`kind`，节点类返回 `class_path`、`display_name`、`category`、`tooltip`、`keywords`、`properties`；脚本资产返回 `script_path`、`usage`、`inputs`、`outputs`；Custom HLSL 返回 `custom_hlsl_schema`、`note`。

### `search_niagara_functions` (`niagara.search_functions`)

- 作用：按 usage 和名称搜索 Niagara Script 资源。
- 传参：可选 `filter`、`usage`，默认 `function`；可选 `include_engine`，默认 `true`；可选 `max_results`，默认 `100`。
- 返回：`success`、`filter`、`usage`、`functions`、`count`；函数元素含 `name`、`script_path`、`usage`。

### `describe_niagara_type` (`niagara.describe_type`)

- 作用：解析并描述 Niagara 类型，包括基础类型、枚举、结构体和 Data Interface。
- 传参：必填 `type`。
- 返回：`success`、`type`、`resolved_as`、`schema`；`schema` 内容随类型不同返回字段、枚举项或类属性。

### `get_niagara_schema_actions` (`niagara.get_schema_actions`)

- 作用：读取 Scratch Pad 图上下文菜单可创建的 Niagara schema actions。
- 传参：Scratch Pad 图定位参数。可选 `filter`、`max_results`。
- 返回：`success`、`module_name`、`filter`、`actions`、`count`；Action 元素含 `type`、`display_name`、`category`、`tooltip`、`keywords`、`node_type`、`template_class`、`function_script`、`function_display_name`、`op_name`、`inputs`、`outputs`、`default` 等。

### `list_niagara_available_parameters` (`niagara.list_available_parameters`)

- 作用：列出可绑定或添加到 Map Get/Map Set Pin 的参数。
- 传参：可选 `filter`、`namespace`、`max_results`、`system_path`、`module_name`。
- 返回：`success`、`filter`、`namespace`、`parameters`、`count`；参数元素含 `name`、`type`、`scope`、`source`。

### `add_niagara_map_get_pin` (`niagara.add_map_get_pin`)

- 作用：给 Scratch Pad 图中第一个 ParameterMapGet 节点添加输出 Pin，并注册脚本变量元数据。
- 传参：Scratch Pad 图定位参数，必填 `parameter_name`。可选 `parameter_type`，默认 `float`。
- 返回：`success`、`operation: "map_get"`、`parameter_name`、`parameter_type`、`updated_graph_count`。

### `add_niagara_map_set_pin` (`niagara.add_map_set_pin`)

- 作用：给 Scratch Pad 图中第一个 ParameterMapSet 节点添加输入 Pin，并注册脚本变量元数据。
- 传参：Scratch Pad 图定位参数，必填 `parameter_name`。可选 `parameter_type`，默认 `float`。
- 返回：`success`、`operation: "map_set"`、`parameter_name`、`parameter_type`、`updated_graph_count`。

### `add_niagara_node_pin` (`niagara.add_node_pin`)

- 作用：给 Scratch Pad 图中的指定动态 Pin 节点添加 Pin。
- 传参：Scratch Pad 图定位参数 + 节点选择参数，必填 `pin_name`。可选 `pin_type`，默认 `float`；可选 `direction`，默认 `input`，可传 `output`。
- 返回：`success`、`pin_name`、`pin_type`、`direction`、`node_class`、`updated_graph_count`。

### `rename_niagara_node_pin` (`niagara.rename_node_pin`)

- 作用：重命名 Scratch Pad 图中指定节点的动态 Pin。
- 传参：Scratch Pad 图定位参数 + 节点选择参数，必填 `old_name`、`new_name`。
- 返回：`success`、`old_name`、`new_name`、`updated_graph_count`。

### `remove_niagara_node_pin` (`niagara.remove_node_pin`)

- 作用：删除 Scratch Pad 图中指定节点的动态 Pin。
- 传参：Scratch Pad 图定位参数 + 节点选择参数，必填 `pin_name`。
- 返回：`success`、`pin_name`、`updated_graph_count`。

### `connect_niagara_pins` (`niagara.connect_pins`)

- 作用：在 Scratch Pad 图中连接两个节点的 Pin。
- 传参：Scratch Pad 图定位参数，必填 `from_pin`、`to_pin`，并通过 `from_node_id/from_node_index/from_node_class` 和 `to_node_id/to_node_index/to_node_class` 选择两端节点。
- 返回：`success`、`from`、`to`、`updated_graph_count`；`from`/`to` 对象含 `node_class`、`node_index`、`node_id` 和 Pin 名。

### `disconnect_niagara_pins` (`niagara.disconnect_pins`)

- 作用：断开 Scratch Pad 图中指定节点 Pin 的所有连接。
- 传参：Scratch Pad 图定位参数 + 节点选择参数，必填 `pin_name`。
- 返回：`success`、`pin_name`、`broken_links`。

### `get_niagara_data_interface_schema` (`niagara.get_data_interface_schema`)

- 作用：返回指定 Data Interface 类的可编辑属性 schema。
- 传参：必填 `class_` 或 `class`。
- 返回：`success`、`class`、`class_path`、`schema`；`schema` 描述属性名、类型、默认值和可用枚举值等。

## Runtime Actions

### `spawn_niagara_effect` (`niagara.spawn_effect`)

- 作用：在当前 Editor World 中生成 Niagara Actor 并绑定 Niagara System。
- 传参：必填 `system_path`。可选 `name`、`folder`、`location`、`rotation`、`scale`、`auto_activate`，默认 `true`；可选 `force_solo`，默认 `false`；可选 `warmup_ticks`、`warmup_delta_time`。
- 返回：`success`、`actor_name`、`internal_name`、`system_path`、`location`、`rotation`、`scale`、`auto_activate`、`force_solo`、`warmup_ticks`、`warmup_delta_time`。

### `control_niagara_effect` (`niagara.control_effect`)

- 作用：控制场景中的 Niagara Actor 组件播放状态。
- 传参：必填 `actor_name`、`action`。`action` 支持 `activate`、`deactivate`、`reset`、`reinitialize`/`reset_system`。
- 返回：`success`、`actor_name`、`action`、`is_active`。

### `add_niagara_component` (`niagara.add_component`)

- 作用：给当前关卡中的 Actor 添加 NiagaraComponent 并绑定 Niagara System。
- 传参：必填 `actor_name`、`system_path`。可选 `component_name`，默认 `NiagaraComponent`；可选 `relative_location`、`relative_rotation`、`auto_activate`，默认 `true`。
- 返回：`success`、`actor_name`、`component_name`、`system_path`、`auto_activate`。

### `get_niagara_actors` (`niagara.get_actors`)

- 作用：列出当前 Editor World 中的 Niagara Actor。
- 传参：可选 `name_filter`、`system_filter`。
- 返回：`success`、`actors`、`count`；Actor 元素含 `name`、`internal_name`、`system_name`、`system_path`、`location`、`rotation`、`scale`、`is_active`、`auto_activate`、可选 `folder`。

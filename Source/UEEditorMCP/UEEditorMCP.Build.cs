// 

using UnrealBuildTool;

public class UEEditorMCP : ModuleRules
{
	public UEEditorMCP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"InputCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"EditorSubsystem",
			"BlueprintGraph",
			"Kismet",
			"KismetCompiler",
			"GraphEditor",
			"Json",
			"JsonUtilities",
			"Networking",
			"Sockets",
			"UMG",
			"UMGEditor",
			"EnhancedInput",
			"InputBlueprintNodes",
			"EditorScriptingUtilities",
			"AssetTools",
			"SourceControl",      // For Diff Against Depot (ISourceControlModule, ISourceControlProvider, etc.)
			"MaterialEditor",     // For UMaterialEditingLibrary and material expression manipulation
			"RenderCore",         // For material shader compilation
			"RHI",                // For GMaxRHIShaderPlatform (compile diagnostics)
			"Niagara",
			"NiagaraEditor",
			"NiagaraShader",
			"ModelViewViewModel",           // MVVM runtime types (EMVVMBindingMode, EMVVMExecutionMode)
			"ModelViewViewModelBlueprint",  // MVVM editor-time binding API (UMVVMBlueprintView, etc.)
			"FieldNotification",            // INotifyFieldValueChanged interface
			"PropertyBindingUtils",
			"PythonScriptPlugin",
		});

		PrivateIncludePaths.AddRange(new string[]
		{
			System.IO.Path.Combine(ModuleDirectory, "Private"),
			System.IO.Path.Combine(ModuleDirectory, "Private/Actions/NiagaraActions"),
			System.IO.Path.Combine(EngineDirectory, "Plugins/FX/Niagara/Source/NiagaraEditor/Private"),
		});

		// Ensure proper RTTI/exceptions for crash handling
		bUseRTTI = true;
		bEnableExceptions = true;
	}
}

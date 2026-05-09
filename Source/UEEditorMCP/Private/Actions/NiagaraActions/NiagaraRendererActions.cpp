#include "Actions/NiagaraActions/NiagaraRendererActions.h"

#include "MCPCommonUtils.h"
#include "Actions/NiagaraActions/NiagaraCommonUtils.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraLightRendererProperties.h"
#include "NiagaraComponentRendererProperties.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "UObject/UnrealType.h"

bool FSetNiagaraRendererPropertyAction::Validate(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context,
	FString& OutError)
{
	(void)Context;
	const TArray<FString> RequiredParams = {TEXT("system_path"), TEXT("emitter_name")};
	if (!NiagaraCommonUtils::ValidateRequiredStringParams(Params, RequiredParams, OutError))
	{
		return false;
	}
	if (GetOptionalString(Params, TEXT("property")).IsEmpty() &&
		GetOptionalString(Params, TEXT("property_name")).IsEmpty())
	{
		OutError = TEXT("Required parameter 'property' or 'property_name' is missing");
		return false;
	}
	if (!Params.IsValid() || !Params->HasField(TEXT("value")))
	{
		OutError = TEXT("Required parameter 'value' is missing");
		return false;
	}
	return true;
}

bool FSetNiagaraRendererBindingAction::Validate(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context,
	FString& OutError)
{
	(void)Context;
	const TArray<FString> RequiredParams = {TEXT("system_path"), TEXT("emitter_name")};
	if (!NiagaraCommonUtils::ValidateRequiredStringParams(Params, RequiredParams, OutError))
	{
		return false;
	}
	{
		FString Unused;
		if (!GetRequiredString(Params, TEXT("binding_name"), Unused, OutError))
		{
			return false;
		}
	}
	if (GetOptionalString(Params, TEXT("attribute_name")).IsEmpty() &&
		GetOptionalString(Params, TEXT("attribute")).IsEmpty())
	{
		OutError = TEXT("Required parameter 'attribute_name' or 'attribute' is missing");
		return false;
	}
	return true;
}

TSharedPtr<FJsonObject> FAddNiagaraRendererAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	(void)Context;
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

	FString RendererType;
	if (!Params->TryGetStringField(TEXT("renderer_type"), RendererType))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'renderer_type' parameter"));
	}

	FString MaterialPath;
	Params->TryGetStringField(TEXT("material_path"), MaterialPath);

	FString MeshPath;
	Params->TryGetStringField(TEXT("mesh_path"), MeshPath);

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 EmitterIndex = INDEX_NONE;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, EmitterIndex, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	UNiagaraEmitter* Emitter = Handle->GetInstance().Emitter;
	if (!Emitter)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	UMaterialInterface* Material = nullptr;
	if (!MaterialPath.IsEmpty())
	{
		Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
		if (!Material)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
		}
	}

	UNiagaraRendererProperties* NewRenderer = nullptr;
	FString NormalizedType = RendererType.ToLower();

	if (NormalizedType == TEXT("sprite"))
	{
		UNiagaraSpriteRendererProperties* SpriteRenderer = NewObject<UNiagaraSpriteRendererProperties>(Emitter);
		if (Material)
		{
			SpriteRenderer->Material = Material;
		}
		NewRenderer = SpriteRenderer;
	}
	else if (NormalizedType == TEXT("mesh"))
	{
		UNiagaraMeshRendererProperties* MeshRenderer = NewObject<UNiagaraMeshRendererProperties>(Emitter);
		if (!MeshPath.IsEmpty())
		{
			UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
			if (Mesh)
			{
				FNiagaraMeshRendererMeshProperties MeshProps;
				MeshProps.Mesh = Mesh;
				MeshRenderer->Meshes.Add(MeshProps);
			}
		}
		if (Material)
		{
			FNiagaraMeshMaterialOverride Override;
			Override.ExplicitMat = Material;
			MeshRenderer->OverrideMaterials.Add(Override);
		}
		NewRenderer = MeshRenderer;
	}
	else if (NormalizedType == TEXT("ribbon"))
	{
		UNiagaraRibbonRendererProperties* RibbonRenderer = NewObject<UNiagaraRibbonRendererProperties>(Emitter);
		if (Material)
		{
			RibbonRenderer->Material = Material;
		}
		NewRenderer = RibbonRenderer;
	}
	else if (NormalizedType == TEXT("light"))
	{
		NewRenderer = NewObject<UNiagaraLightRendererProperties>(Emitter);
	}
	else if (NormalizedType == TEXT("component"))
	{
		NewRenderer = NewObject<UNiagaraComponentRendererProperties>(Emitter);
	}
	else
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unknown renderer type '%s'. Use: sprite, mesh, ribbon, light, component"), *RendererType));
	}

	Emitter->AddRenderer(NewRenderer, EmitterData->Version.VersionGuid);
	NiagaraCommonUtils::CompileAndSync(System);

	const int32 NewIndex = EmitterData->GetRenderers().Num() - 1;

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("renderer_type"), NormalizedType);
	Result->SetNumberField(TEXT("renderer_index"), NewIndex);
	Result->SetStringField(TEXT("emitter_name"), EmitterName);
	Result->SetNumberField(TEXT("renderer_count"), EmitterData->GetRenderers().Num());
	return Result;
}

TSharedPtr<FJsonObject> FRemoveNiagaraRendererAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	(void)Context;
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

	int32 RendererIndex = 0;
	Params->TryGetNumberField(TEXT("renderer_index"), RendererIndex);

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 EmitterIdx = INDEX_NONE;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, EmitterIdx, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	UNiagaraEmitter* Emitter = Handle->GetInstance().Emitter;
	if (!Emitter)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
	if (RendererIndex < 0 || RendererIndex >= Renderers.Num())
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Renderer index %d out of range (emitter has %d renderers)"),
			                RendererIndex, Renderers.Num()));
	}

	UNiagaraRendererProperties* ToRemove = Renderers[RendererIndex];
	const FString RemovedName = ToRemove ? ToRemove->GetName() : TEXT("unknown");

	Emitter->RemoveRenderer(ToRemove, EmitterData->Version.VersionGuid);
	NiagaraCommonUtils::CompileAndSync(System);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("removed_renderer"), RemovedName);
	Result->SetNumberField(TEXT("removed_index"), RendererIndex);
	Result->SetNumberField(TEXT("remaining_count"), EmitterData->GetRenderers().Num());
	return Result;
}

TSharedPtr<FJsonObject> FGetNiagaraRendererInfoAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	(void)Context;
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

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 EmitterIdx = INDEX_NONE;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, EmitterIdx, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	int32 RendererIndex = -1;
	Params->TryGetNumberField(TEXT("renderer_index"), RendererIndex);

	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();

	TArray<TSharedPtr<FJsonValue>> RenderersArr;
	for (int32 i = 0; i < Renderers.Num(); ++i)
	{
		if (RendererIndex >= 0 && i != RendererIndex)
		{
			continue;
		}
		RenderersArr.Add(MakeShared<FJsonValueObject>(NiagaraCommonUtils::RendererToJson(Renderers[i], i)));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("emitter_name"), EmitterName);
	Result->SetArrayField(TEXT("renderers"), RenderersArr);
	Result->SetNumberField(TEXT("count"), RenderersArr.Num());
	return Result;
}

TSharedPtr<FJsonObject> FSetNiagaraRendererPropertyAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params,
                                                                           FMCPEditorContext& Context)
{
	(void)Context;
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

	FString PropertyName;
	if (!Params->TryGetStringField(TEXT("property"), PropertyName) &&
		!Params->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property' parameter"));
	}

	int32 RendererIndex = 0;
	Params->TryGetNumberField(TEXT("renderer_index"), RendererIndex);

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 EmitterIdx = INDEX_NONE;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, EmitterIdx, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
	if (RendererIndex < 0 || RendererIndex >= Renderers.Num())
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Renderer index %d out of range (%d renderers)"),
			                RendererIndex, Renderers.Num()));
	}

	UNiagaraRendererProperties* Renderer = Renderers[RendererIndex];
	const FString LowerProp = PropertyName.ToLower();

	if (LowerProp == TEXT("material") || LowerProp == TEXT("material_path"))
	{
		FString MatPath;
		if (!Params->TryGetStringField(TEXT("value"), MatPath))
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' (material path) for material property"));
		}

		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MatPath);
		if (!Material)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Failed to load material: %s"), *MatPath));
		}

		if (UNiagaraSpriteRendererProperties* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer))
		{
			Sprite->Material = Material;
		}
		else if (UNiagaraRibbonRendererProperties* Ribbon = Cast<UNiagaraRibbonRendererProperties>(Renderer))
		{
			Ribbon->Material = Material;
		}
		else if (UNiagaraMeshRendererProperties* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer))
		{
			FNiagaraMeshMaterialOverride Override;
			Override.ExplicitMat = Material;
			if (Mesh->OverrideMaterials.Num() > 0)
			{
				Mesh->OverrideMaterials[0] = Override;
			}
			else
			{
				Mesh->OverrideMaterials.Add(Override);
			}
		}
		else
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("This renderer type does not support materials"));
		}

		NiagaraCommonUtils::CompileAndSync(System);
		auto Result = MakeShared<FJsonObject>();
		Result->SetBoolField(TEXT("success"), true);
		Result->SetStringField(TEXT("property"), TEXT("material"));
		Result->SetStringField(TEXT("value"), MatPath);
		return Result;
	}

	if (LowerProp == TEXT("mesh") || LowerProp == TEXT("mesh_path"))
	{
		UNiagaraMeshRendererProperties* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer);
		if (!Mesh)
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("'mesh' property only applies to mesh renderers"));
		}

		FString MeshAssetPath;
		if (!Params->TryGetStringField(TEXT("value"), MeshAssetPath))
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' (mesh asset path)"));
		}

		UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *MeshAssetPath);
		if (!StaticMesh)
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Failed to load mesh: %s"), *MeshAssetPath));
		}

		if (Mesh->Meshes.Num() > 0)
		{
			Mesh->Meshes[0].Mesh = StaticMesh;
		}
		else
		{
			FNiagaraMeshRendererMeshProperties MeshProps;
			MeshProps.Mesh = StaticMesh;
			Mesh->Meshes.Add(MeshProps);
		}

		NiagaraCommonUtils::CompileAndSync(System);
		auto Result = MakeShared<FJsonObject>();
		Result->SetBoolField(TEXT("success"), true);
		Result->SetStringField(TEXT("property"), TEXT("mesh"));
		Result->SetStringField(TEXT("value"), MeshAssetPath);
		return Result;
	}

	if (LowerProp == TEXT("sort_order") || LowerProp == TEXT("sort_order_hint"))
	{
		int32 SortOrder = 0;
		if (!Params->TryGetNumberField(TEXT("value"), SortOrder))
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' (integer) for sort_order"));
		}

		Renderer->SortOrderHint = SortOrder;
		NiagaraCommonUtils::CompileAndSync(System);

		auto Result = MakeShared<FJsonObject>();
		Result->SetBoolField(TEXT("success"), true);
		Result->SetStringField(TEXT("property"), TEXT("sort_order"));
		Result->SetNumberField(TEXT("value"), SortOrder);
		return Result;
	}

	if (LowerProp == TEXT("enabled") || LowerProp == TEXT("is_enabled"))
	{
		bool bEnabled = true;
		Params->TryGetBoolField(TEXT("value"), bEnabled);
		Renderer->SetIsEnabled(bEnabled);
		NiagaraCommonUtils::CompileAndSync(System);

		auto Result = MakeShared<FJsonObject>();
		Result->SetBoolField(TEXT("success"), true);
		Result->SetStringField(TEXT("property"), TEXT("enabled"));
		Result->SetBoolField(TEXT("value"), bEnabled);
		return Result;
	}

	if (LowerProp == TEXT("facing_mode") || LowerProp == TEXT("facingmode"))
	{
		FString ValueStr;
		if (!Params->TryGetStringField(TEXT("value"), ValueStr))
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' for facing_mode"));
		}

		FProperty* Prop = Renderer->GetClass()->FindPropertyByName(TEXT("FacingMode"));
		if (Prop)
		{
			Prop->ImportText_Direct(*ValueStr, Prop->ContainerPtrToValuePtr<void>(Renderer), Renderer, PPF_None);
			NiagaraCommonUtils::CompileAndSync(System);

			auto Result = MakeShared<FJsonObject>();
			Result->SetBoolField(TEXT("success"), true);
			Result->SetStringField(TEXT("property"), TEXT("FacingMode"));
			Result->SetStringField(TEXT("value"), ValueStr);
			Result->SetStringField(TEXT("renderer_type"), Renderer->GetClass()->GetName());
			return Result;
		}

		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("This renderer type does not have a FacingMode property"));
	}

	FString ValueStr;
	if (Params->TryGetStringField(TEXT("value"), ValueStr))
	{
		FProperty* Prop = Renderer->GetClass()->FindPropertyByName(FName(*PropertyName));

		if (!Prop)
		{
			FString PascalName = PropertyName;
			PascalName.ReplaceInline(TEXT("_"), TEXT(""));
			Prop = Renderer->GetClass()->FindPropertyByName(FName(*PascalName));
		}

		if (!Prop && (ValueStr == TEXT("true") || ValueStr == TEXT("false")))
		{
			FString BoolName = TEXT("b") + PropertyName;
			BoolName.ReplaceInline(TEXT("_"), TEXT(""));
			Prop = Renderer->GetClass()->FindPropertyByName(FName(*BoolName));
		}

		if (Prop)
		{
			Renderer->Modify();
			Prop->ImportText_Direct(*ValueStr, Prop->ContainerPtrToValuePtr<void>(Renderer), Renderer, PPF_None);
			NiagaraCommonUtils::CompileAndSync(System);

			auto Result = MakeShared<FJsonObject>();
			Result->SetBoolField(TEXT("success"), true);
			Result->SetStringField(TEXT("property"), Prop->GetName());
			Result->SetStringField(TEXT("value"), ValueStr);
			Result->SetStringField(TEXT("renderer_type"), Renderer->GetClass()->GetName());
			return Result;
		}

		TArray<FString> AvailableProps;
		for (TFieldIterator<FProperty> It(Renderer->GetClass()); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Edit))
			{
				AvailableProps.Add(It->GetName());
			}
		}

		if (AvailableProps.Num() > 30)
		{
			AvailableProps.SetNum(30);
			AvailableProps.Add(TEXT("... (truncated)"));
		}

		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Property '%s' not found on %s. Available editable properties: %s"),
			                *PropertyName, *Renderer->GetClass()->GetName(),
			                *FString::Join(AvailableProps, TEXT(", "))));
	}

	return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
}

TSharedPtr<FJsonObject> FSetNiagaraRendererBindingAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context)
{
	(void)Context;
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

	FString BindingName;
	if (!Params->TryGetStringField(TEXT("binding_name"), BindingName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'binding_name' parameter (e.g., 'PositionBinding')"));
	}

	FString AttributeName;
	if (!Params->TryGetStringField(TEXT("attribute_name"), AttributeName) &&
		!Params->TryGetStringField(TEXT("attribute"), AttributeName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'attribute_name' parameter (e.g., 'Particles.Position')"));
	}

	int32 RendererIndex = 0;
	Params->TryGetNumberField(TEXT("renderer_index"), RendererIndex);

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 EmitterIdx = INDEX_NONE;
	FNiagaraEmitterHandle* Handle = NiagaraCommonUtils::FindEmitterHandle(System, EmitterName, EmitterIdx, Error);
	if (!Handle)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
	if (RendererIndex < 0 || RendererIndex >= Renderers.Num())
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Renderer index %d out of range (%d renderers)"),
			                RendererIndex, Renderers.Num()));
	}

	UNiagaraRendererProperties* Renderer = Renderers[RendererIndex];

	FStructProperty* BindingProp = CastField<FStructProperty>(
		Renderer->GetClass()->FindPropertyByName(FName(*BindingName)));

	if (!BindingProp || BindingProp->Struct->GetName() != TEXT("NiagaraVariableAttributeBinding"))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("'%s' is not a valid binding property on this renderer"), *BindingName));
	}

	FNiagaraVariableAttributeBinding* Binding = BindingProp->ContainerPtrToValuePtr<FNiagaraVariableAttributeBinding>(Renderer);
	if (!Binding)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Failed to access binding property"));
	}

	FNiagaraVariableBase BoundVar;
	FString Namespace;
	FString VarName;
	if (AttributeName.Split(TEXT("."), &Namespace, &VarName))
	{
		BoundVar.SetName(FName(*AttributeName));
	}
	else
	{
		BoundVar.SetName(FName(*FString::Printf(TEXT("Particles.%s"), *AttributeName)));
	}

	FVersionedNiagaraEmitterBase VersionedEmitterBase;
	VersionedEmitterBase.Emitter = Handle->GetInstance().Emitter;
	VersionedEmitterBase.Version = EmitterData->Version.VersionGuid;
	Binding->SetValue(
		BoundVar.GetName(),
		VersionedEmitterBase,
		ENiagaraRendererSourceDataMode::Particles);

	NiagaraCommonUtils::CompileAndSync(System);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("binding_name"), BindingName);
	Result->SetStringField(TEXT("attribute_name"), BoundVar.GetName().ToString());
	Result->SetNumberField(TEXT("renderer_index"), RendererIndex);
	return Result;
}

TSharedPtr<FJsonObject> FGetNiagaraRendererPropertiesAction::ExecuteInternal(const TSharedPtr<FJsonObject>& Params,
                                                                             FMCPEditorContext& Context)
{
	(void)Context;
	FString SystemPath;
	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath) ||
		!Params->TryGetStringField(TEXT("emitter_name"), EmitterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing required: system_path, emitter_name"));
	}

	double IdxD = 0;
	Params->TryGetNumberField(TEXT("renderer_index"), IdxD);
	const int32 RendererIndex = static_cast<int32>(IdxD);

	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	int32 EmitterIdx = INDEX_NONE;
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

	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
	if (RendererIndex < 0 || RendererIndex >= Renderers.Num())
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Renderer index %d out of range"), RendererIndex));
	}

	UNiagaraRendererProperties* Renderer = Renderers[RendererIndex];
	TArray<TSharedPtr<FJsonValue>> PropsArr;

	for (TFieldIterator<FProperty> It(Renderer->GetClass()); It; ++It)
	{
		FProperty* Prop = *It;

		if (!Prop->HasAnyPropertyFlags(CPF_Edit))
		{
			continue;
		}

		const FString PropName = Prop->GetName();
		if (!Filter.IsEmpty() &&
			!PropName.Contains(Filter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		auto PropObj = MakeShared<FJsonObject>();
		PropObj->SetStringField(TEXT("name"), PropName);
		PropObj->SetStringField(TEXT("type"), Prop->GetCPPType());

		FString ValueStr;
		const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Renderer);
		Prop->ExportTextItem_Direct(ValueStr, ValuePtr, nullptr, Renderer, PPF_None);

		if (ValueStr.Len() > 200)
		{
			ValueStr = ValueStr.Left(200) + TEXT("...(truncated)");
		}
		PropObj->SetStringField(TEXT("value"), ValueStr);

		if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
		{
			if (UEnum* Enum = EnumProp->GetEnum())
			{
				TArray<TSharedPtr<FJsonValue>> EnumVals;
				for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
				{
					EnumVals.Add(MakeShared<FJsonValueString>(
						Enum->GetNameStringByIndex(i)));
				}
				PropObj->SetArrayField(TEXT("valid_values"), EnumVals);
			}
		}
		else if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
		{
			if (UEnum* Enum = ByteProp->Enum)
			{
				TArray<TSharedPtr<FJsonValue>> EnumVals;
				for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
				{
					EnumVals.Add(MakeShared<FJsonValueString>(
						Enum->GetNameStringByIndex(i)));
				}
				PropObj->SetArrayField(TEXT("valid_values"), EnumVals);
			}
		}

		PropsArr.Add(MakeShared<FJsonValueObject>(PropObj));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("renderer_type"), Renderer->GetClass()->GetName());
	Result->SetNumberField(TEXT("renderer_index"), RendererIndex);
	Result->SetArrayField(TEXT("properties"), PropsArr);
	Result->SetNumberField(TEXT("count"), PropsArr.Num());
	if (!Filter.IsEmpty())
	{
		Result->SetStringField(TEXT("filter"), Filter);
	}
	return Result;
}

// ---------------------------------------------------------------------------
// Action metadata
// ---------------------------------------------------------------------------

FString FAddNiagaraRendererAction::GetActionName() const
{
	return TEXT("add_niagara_renderer");
}

bool FAddNiagaraRendererAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	const TArray<FString> RequiredParams = {TEXT("system_path"), TEXT("emitter_name"), TEXT("renderer_type")};
	return NiagaraCommonUtils::ValidateRequiredStringParams(Params, RequiredParams, OutError);
}

FString FRemoveNiagaraRendererAction::GetActionName() const
{
	return TEXT("remove_niagara_renderer");
}

bool FRemoveNiagaraRendererAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	const TArray<FString> RequiredParams = {TEXT("system_path"), TEXT("emitter_name")};
	return NiagaraCommonUtils::ValidateRequiredStringParams(Params, RequiredParams, OutError);
}

FString FGetNiagaraRendererInfoAction::GetActionName() const
{
	return TEXT("get_niagara_renderer_info");
}

bool FGetNiagaraRendererInfoAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	const TArray<FString> RequiredParams = {TEXT("system_path"), TEXT("emitter_name")};
	return NiagaraCommonUtils::ValidateRequiredStringParams(Params, RequiredParams, OutError);
}

FString FGetNiagaraRendererPropertiesAction::GetActionName() const
{
	return TEXT("get_niagara_renderer_properties");
}

bool FGetNiagaraRendererPropertiesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	const TArray<FString> RequiredParams = {TEXT("system_path"), TEXT("emitter_name")};
	return NiagaraCommonUtils::ValidateRequiredStringParams(Params, RequiredParams, OutError);
}

FString FSetNiagaraRendererPropertyAction::GetActionName() const
{
	return TEXT("set_niagara_renderer_property");
}

FString FSetNiagaraRendererBindingAction::GetActionName() const
{
	return TEXT("set_niagara_renderer_binding");
}

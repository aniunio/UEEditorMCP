#include "Actions/NiagaraActions/NiagaraParameterActions.h"

#include "MCPCommonUtils.h"
#include "Actions/NiagaraActions/NiagaraCommonUtils.h"

#include "Editor.h"
#include "Engine/World.h"

#include "EdGraph/EdGraphPin.h"
#include "NiagaraActor.h"
#include "NiagaraCommon.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterface.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraParameterStore.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraSystem.h"
#include "NiagaraTypeRegistry.h"
#include "NiagaraTypes.h"
#include "UObject/UObjectIterator.h"

#include "EdGraphSchema_Niagara.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeParameterMapGet.h"
#include "NiagaraNodeParameterMapSet.h"
#include "NiagaraScriptVariable.h"
#include "ScopedTransaction.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"

// ---------------------------------------------------------------------------
// Local registration helper.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// FGetNiagaraUserParametersAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FGetNiagaraUserParametersAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context)
{
	(void)Context;

	UNiagaraSystem* System = nullptr;
	FString SourceName;

	FString ActorName;
	if (Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		FString Error;
		ANiagaraActor* Actor = NiagaraCommonUtils::FindNiagaraActorByName(ActorName, Error);
		if (!Actor)
		{
			return FMCPCommonUtils::CreateErrorResponse(Error);
		}

		UNiagaraComponent* Comp = Actor->GetNiagaraComponent();
		if (!Comp || !Comp->GetAsset())
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Actor '%s' has no Niagara system assigned"), *ActorName));
		}

		System = Comp->GetAsset();
		SourceName = ActorName;
	}
	else
	{
		FString SystemPath;
		if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Provide either 'actor_name' or 'system_path'"));
		}

		FString Error;
		System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
		if (!System)
		{
			return FMCPCommonUtils::CreateErrorResponse(Error);
		}
		SourceName = SystemPath;
	}

	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	const FNiagaraUserRedirectionParameterStore& ExposedParams = System->GetExposedParameters();
	TArrayView<const FNiagaraVariableWithOffset> UserParameters = ExposedParams.ReadParameterVariables();

	TArray<TSharedPtr<FJsonValue>> ParamsArr;
	for (const FNiagaraVariableWithOffset& VarWithOffset : UserParameters)
	{
		const FNiagaraVariableBase& Var = VarWithOffset;
		FString ParamName = Var.GetName().ToString();
		if (!Filter.IsEmpty() && !ParamName.Contains(Filter, ESearchCase::IgnoreCase))
		{
			continue;
		}
		auto ParamObj = MakeShared<FJsonObject>();
		ParamObj->SetStringField(TEXT("name"), ParamName);
		ParamObj->SetStringField(TEXT("type"), Var.GetType().GetName());

		FString TypeName = Var.GetType().GetName();
		if (TypeName.Contains(TEXT("Float")))
		{
			ParamObj->SetStringField(TEXT("value_type"), TEXT("float"));
		}
		else if (TypeName.Contains(TEXT("Int")))
		{
			ParamObj->SetStringField(TEXT("value_type"), TEXT("int"));
		}
		else if (TypeName.Contains(TEXT("Bool")))
		{
			ParamObj->SetStringField(TEXT("value_type"), TEXT("bool"));
		}
		else if (TypeName.Contains(TEXT("Vector")) || TypeName.Contains(TEXT("Position")))
		{
			ParamObj->SetStringField(TEXT("value_type"), TEXT("vector"));
		}
		else if (TypeName.Contains(TEXT("Color")))
		{
			ParamObj->SetStringField(TEXT("value_type"), TEXT("color"));
		}
		else
		{
			ParamObj->SetStringField(TEXT("value_type"), TypeName);
		}

		ParamsArr.Add(MakeShared<FJsonValueObject>(ParamObj));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("source"), SourceName);
	Result->SetStringField(TEXT("system_name"), System->GetName());
	Result->SetArrayField(TEXT("parameters"), ParamsArr);
	Result->SetNumberField(TEXT("count"), ParamsArr.Num());
	return Result;
}

// ---------------------------------------------------------------------------
// FAddNiagaraUserParameterAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FAddNiagaraUserParameterAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context)
{
	(void)Context;

	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString ParameterName;
	if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parameter_name' parameter"));
	}

	FString ParameterType = TEXT("float");
	Params->TryGetStringField(TEXT("parameter_type"), ParameterType);

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FNiagaraTypeDefinition TypeDef = NiagaraCommonUtils::ResolveNiagaraTypeOrDefault(ParameterType);
	if (!TypeDef.IsValid())
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(
				TEXT("Unknown parameter_type '%s'. Use list_niagara_parameter_types to discover valid names "
					"(primitives, structs, enums, and data interfaces)."),
				*ParameterType));
	}

	FString FullName = ParameterName;
	if (!FullName.StartsWith(TEXT("User.")))
	{
		FullName = FString::Printf(TEXT("User.%s"), *ParameterName);
	}

	FNiagaraVariable NewVar(TypeDef, FName(*FullName));

	if (!TypeDef.IsDataInterface())
	{
		NewVar.AllocateData();
	}

	if (TypeDef == FNiagaraTypeDefinition::GetFloatDef())
	{
		double Default = 0.0;
		Params->TryGetNumberField(TEXT("default_value"), Default);
		NewVar.SetValue(static_cast<float>(Default));
	}
	else if (TypeDef == FNiagaraTypeDefinition::GetIntDef())
	{
		int32 Default = 0;
		Params->TryGetNumberField(TEXT("default_value"), Default);
		NewVar.SetValue(Default);
	}
	else if (TypeDef == FNiagaraTypeDefinition::GetBoolDef())
	{
		FNiagaraBool BoolVal;
		bool bDefault = false;
		Params->TryGetBoolField(TEXT("default_value"), bDefault);
		BoolVal.SetValue(bDefault);
		NewVar.SetValue(BoolVal);
	}
	else if (TypeDef == FNiagaraTypeDefinition::GetVec2Def()
		|| TypeDef == FNiagaraTypeDefinition::GetVec3Def()
		|| TypeDef == FNiagaraTypeDefinition::GetVec4Def()
		|| TypeDef == FNiagaraTypeDefinition::GetPositionDef()
		|| TypeDef == FNiagaraTypeDefinition::GetColorDef()
		|| TypeDef == FNiagaraTypeDefinition::GetQuatDef())
	{
		FString DefaultStr;
		if (Params->TryGetStringField(TEXT("default_value"), DefaultStr) && !DefaultStr.IsEmpty())
		{
			if (UScriptStruct* Struct = TypeDef.GetScriptStruct())
			{
				Struct->ImportText(*DefaultStr, NewVar.GetData(), nullptr, PPF_None, nullptr,
				                   Struct->GetName());
			}
		}
	}

	System->GetExposedParameters().AddParameter(NewVar, /*bInitInterfaces=*/true, /*bTriggerRebind=*/true);
	System->MarkPackageDirty();
	System->PostEditChange();

	FString Kind = TEXT("primitive");
	FString ClassPath;
	if (TypeDef.IsDataInterface())
	{
		Kind = TEXT("data_interface");
		if (UClass* Cls = TypeDef.GetClass())
		{
			ClassPath = Cls->GetPathName();
		}
	}
	else if (TypeDef.IsEnum())
	{
		Kind = TEXT("enum");
	}
	else if (TypeDef.IsUObject())
	{
		Kind = TEXT("object");
	}
	else if (TypeDef.GetScriptStruct())
	{
		Kind = TEXT("struct");
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("parameter_name"), FullName);
	Result->SetStringField(TEXT("parameter_type"), TypeDef.GetName());
	Result->SetStringField(TEXT("kind"), Kind);
	if (!ClassPath.IsEmpty())
	{
		Result->SetStringField(TEXT("class_path"), ClassPath);
	}
	Result->SetNumberField(TEXT("size_bytes"), TypeDef.GetSize());
	return Result;
}

// ---------------------------------------------------------------------------
// FSetNiagaraUserParameterAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FSetNiagaraUserParameterAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context)
{
	(void)Context;

	FString ParameterName;
	if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parameter_name' parameter"));
	}

	FString ParameterType = TEXT("float");
	Params->TryGetStringField(TEXT("parameter_type"), ParameterType);

	FString ActorName;
	FString SystemPath;
	bool bIsRuntime = Params->TryGetStringField(TEXT("actor_name"), ActorName);
	if (!bIsRuntime)
	{
		if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Provide either 'actor_name' (runtime) or 'system_path' (asset default)"));
		}
	}

	if (bIsRuntime)
	{
		FString Error;
		ANiagaraActor* Actor = NiagaraCommonUtils::FindNiagaraActorByName(ActorName, Error);
		if (!Actor)
		{
			return FMCPCommonUtils::CreateErrorResponse(Error);
		}

		UNiagaraComponent* Comp = Actor->GetNiagaraComponent();
		if (!Comp)
		{
			return FMCPCommonUtils::CreateErrorResponse(TEXT("Actor has no Niagara component"));
		}

		FString LowerType = ParameterType.ToLower();

		if (LowerType == TEXT("float"))
		{
			double Value = 0.0;
			if (!Params->TryGetNumberField(TEXT("value"), Value))
			{
				return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' for float parameter"));
			}
			Comp->SetVariableFloat(FName(*ParameterName), static_cast<float>(Value));
		}
		else if (LowerType == TEXT("int"))
		{
			int32 Value = 0;
			if (!Params->TryGetNumberField(TEXT("value"), Value))
			{
				return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' for int parameter"));
			}
			Comp->SetVariableInt(FName(*ParameterName), Value);
		}
		else if (LowerType == TEXT("bool"))
		{
			bool Value = false;
			if (!Params->TryGetBoolField(TEXT("value"), Value))
			{
				return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' for bool parameter"));
			}
			Comp->SetVariableBool(FName(*ParameterName), Value);
		}
		else if (LowerType == TEXT("vector") || LowerType == TEXT("vec3"))
		{
			FVector Value = FMCPCommonUtils::GetVectorFromJson(Params, TEXT("value"));
			Comp->SetVariableVec3(FName(*ParameterName), Value);
		}
		else if (LowerType == TEXT("color") || LowerType == TEXT("linear_color"))
		{
			const TSharedPtr<FJsonObject>* ValueObj = nullptr;
			if (!Params->TryGetObjectField(TEXT("value"), ValueObj))
			{
				return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' object for color parameter"));
			}

			FLinearColor Color;
			Color.R = static_cast<float>((*ValueObj)->GetNumberField(TEXT("r")));
			Color.G = static_cast<float>((*ValueObj)->GetNumberField(TEXT("g")));
			Color.B = static_cast<float>((*ValueObj)->GetNumberField(TEXT("b")));
			Color.A = (*ValueObj)->HasField(TEXT("a")) ? static_cast<float>((*ValueObj)->GetNumberField(TEXT("a"))) : 1.0f;
			Comp->SetVariableLinearColor(FName(*ParameterName), Color);
		}
		else
		{
			return FMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Unsupported parameter type '%s'. Use: float, int, bool, vector, color"), *ParameterType));
		}

		auto Result = MakeShared<FJsonObject>();
		Result->SetBoolField(TEXT("success"), true);
		Result->SetStringField(TEXT("parameter_name"), ParameterName);
		Result->SetStringField(TEXT("mode"), TEXT("runtime"));
		Result->SetStringField(TEXT("actor_name"), ActorName);
		return Result;
	}
	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString FullName = ParameterName;
	if (!FullName.StartsWith(TEXT("User.")))
	{
		FullName = FString::Printf(TEXT("User.%s"), *ParameterName);
	}

	FNiagaraUserRedirectionParameterStore& Store = System->GetExposedParameters();
	const FNiagaraVariableWithOffset* Found = nullptr;
	for (const FNiagaraVariableWithOffset& Existing : Store.ReadParameterVariables())
	{
		if (Existing.GetName().ToString().Equals(FullName, ESearchCase::IgnoreCase))
		{
			Found = &Existing;
			break;
		}
	}

	if (!Found)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Parameter '%s' not found. Add it first with add_niagara_user_parameter"), *FullName));
	}

	const FNiagaraTypeDefinition ActualType = Found->GetType();
	if (!ActualType.IsValid())
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Parameter '%s' has an invalid Niagara type"), *FullName));
	}
	if (ActualType.IsDataInterface())
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Setting data-interface user parameter '%s' is not supported yet"), *FullName));
	}

	const TSharedPtr<FJsonValue>* ValueField = Params->Values.Find(TEXT("value"));
	if (!ValueField || !ValueField->IsValid())
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
	}

	FNiagaraVariable Var(ActualType, Found->GetName());
	Var.AllocateData();
	if (ActualType == FNiagaraTypeDefinition::GetFloatDef())
	{
		Var.SetValue(static_cast<float>((*ValueField)->AsNumber()));
	}
	else if (ActualType == FNiagaraTypeDefinition::GetIntDef())
	{
		Var.SetValue(static_cast<int32>((*ValueField)->AsNumber()));
	}
	else if (ActualType == FNiagaraTypeDefinition::GetBoolDef())
	{
		FNiagaraBool BoolVal;
		BoolVal.SetValue((*ValueField)->AsBool());
		Var.SetValue(BoolVal);
	}
	else if (ActualType == FNiagaraTypeDefinition::GetVec2Def())
	{
		FVector2f Value(0.0f, 0.0f);
		if ((*ValueField)->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject> Obj = (*ValueField)->AsObject();
			Value.X = static_cast<float>(Obj->GetNumberField(TEXT("x")));
			Value.Y = static_cast<float>(Obj->GetNumberField(TEXT("y")));
		}
		else if ((*ValueField)->Type == EJson::Array)
		{
			const TArray<TSharedPtr<FJsonValue>>& Arr = (*ValueField)->AsArray();
			if (Arr.Num() >= 2)
			{
				Value.X = static_cast<float>(Arr[0]->AsNumber());
				Value.Y = static_cast<float>(Arr[1]->AsNumber());
			}
		}
		else
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Vec2 user parameters require a JSON object {x,y} or array [x,y]"));
		}
		*reinterpret_cast<FVector2f*>(Var.GetData()) = Value;
	}
	else if (ActualType == FNiagaraTypeDefinition::GetVec3Def() ||
		ActualType == FNiagaraTypeDefinition::GetPositionDef())
	{
		FVector3f Value(0.0f, 0.0f, 0.0f);
		if ((*ValueField)->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject> Obj = (*ValueField)->AsObject();
			Value.X = static_cast<float>(Obj->GetNumberField(TEXT("x")));
			Value.Y = static_cast<float>(Obj->GetNumberField(TEXT("y")));
			Value.Z = static_cast<float>(Obj->GetNumberField(TEXT("z")));
		}
		else if ((*ValueField)->Type == EJson::Array)
		{
			const TArray<TSharedPtr<FJsonValue>>& Arr = (*ValueField)->AsArray();
			if (Arr.Num() >= 3)
			{
				Value.X = static_cast<float>(Arr[0]->AsNumber());
				Value.Y = static_cast<float>(Arr[1]->AsNumber());
				Value.Z = static_cast<float>(Arr[2]->AsNumber());
			}
		}
		else
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Vec3/Position user parameters require a JSON object {x,y,z} or array [x,y,z]"));
		}
		*reinterpret_cast<FVector3f*>(Var.GetData()) = Value;
	}
	else if (ActualType == FNiagaraTypeDefinition::GetVec4Def())
	{
		FVector4f Value(0.0f, 0.0f, 0.0f, 0.0f);
		if ((*ValueField)->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject> Obj = (*ValueField)->AsObject();
			Value.X = static_cast<float>(Obj->GetNumberField(TEXT("x")));
			Value.Y = static_cast<float>(Obj->GetNumberField(TEXT("y")));
			Value.Z = static_cast<float>(Obj->GetNumberField(TEXT("z")));
			Value.W = static_cast<float>(Obj->GetNumberField(TEXT("w")));
		}
		else if ((*ValueField)->Type == EJson::Array)
		{
			const TArray<TSharedPtr<FJsonValue>>& Arr = (*ValueField)->AsArray();
			if (Arr.Num() >= 4)
			{
				Value.X = static_cast<float>(Arr[0]->AsNumber());
				Value.Y = static_cast<float>(Arr[1]->AsNumber());
				Value.Z = static_cast<float>(Arr[2]->AsNumber());
				Value.W = static_cast<float>(Arr[3]->AsNumber());
			}
		}
		else
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Vec4 user parameters require a JSON object {x,y,z,w} or array [x,y,z,w]"));
		}
		*reinterpret_cast<FVector4f*>(Var.GetData()) = Value;
	}
	else if (ActualType == FNiagaraTypeDefinition::GetColorDef())
	{
		FLinearColor Value(0.0f, 0.0f, 0.0f, 1.0f);
		if ((*ValueField)->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject> Obj = (*ValueField)->AsObject();
			Value.R = static_cast<float>(Obj->GetNumberField(TEXT("r")));
			Value.G = static_cast<float>(Obj->GetNumberField(TEXT("g")));
			Value.B = static_cast<float>(Obj->GetNumberField(TEXT("b")));
			double Alpha = 1.0;
			Obj->TryGetNumberField(TEXT("a"), Alpha);
			Value.A = static_cast<float>(Alpha);
		}
		else if ((*ValueField)->Type == EJson::Array)
		{
			const TArray<TSharedPtr<FJsonValue>>& Arr = (*ValueField)->AsArray();
			if (Arr.Num() >= 3)
			{
				Value.R = static_cast<float>(Arr[0]->AsNumber());
				Value.G = static_cast<float>(Arr[1]->AsNumber());
				Value.B = static_cast<float>(Arr[2]->AsNumber());
				if (Arr.Num() >= 4)
				{
					Value.A = static_cast<float>(Arr[3]->AsNumber());
				}
			}
		}
		else
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Color user parameters require a JSON object {r,g,b,a} or array [r,g,b,a]"));
		}
		*reinterpret_cast<FLinearColor*>(Var.GetData()) = Value;
	}
	else if (ActualType == FNiagaraTypeDefinition::GetQuatDef())
	{
		FQuat4f Value(0.0f, 0.0f, 0.0f, 1.0f);
		if ((*ValueField)->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject> Obj = (*ValueField)->AsObject();
			Value.X = static_cast<float>(Obj->GetNumberField(TEXT("x")));
			Value.Y = static_cast<float>(Obj->GetNumberField(TEXT("y")));
			Value.Z = static_cast<float>(Obj->GetNumberField(TEXT("z")));
			Value.W = static_cast<float>(Obj->GetNumberField(TEXT("w")));
		}
		else if ((*ValueField)->Type == EJson::Array)
		{
			const TArray<TSharedPtr<FJsonValue>>& Arr = (*ValueField)->AsArray();
			if (Arr.Num() >= 4)
			{
				Value.X = static_cast<float>(Arr[0]->AsNumber());
				Value.Y = static_cast<float>(Arr[1]->AsNumber());
				Value.Z = static_cast<float>(Arr[2]->AsNumber());
				Value.W = static_cast<float>(Arr[3]->AsNumber());
			}
		}
		else
		{
			return FMCPCommonUtils::CreateErrorResponse(
				TEXT("Quat user parameters require a JSON object {x,y,z,w} or array [x,y,z,w]"));
		}
		*reinterpret_cast<FQuat4f*>(Var.GetData()) = Value;
	}
	else
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(
				TEXT("Unsupported asset-default user parameter type '%s' for '%s'"),
				*ActualType.GetName(),
				*FullName));
	}

	Store.SetParameterData(Var.GetData(), Var, true);
	System->MarkPackageDirty();
	System->PostEditChange();

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("parameter_name"), FullName);
	Result->SetStringField(TEXT("parameter_type"), ActualType.GetName());
	Result->SetStringField(TEXT("mode"), TEXT("asset_default"));
	return Result;
}

// ---------------------------------------------------------------------------
// FRemoveNiagaraUserParameterAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FRemoveNiagaraUserParameterAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context)
{
	(void)Context;

	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	FString ParameterName;
	if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parameter_name' parameter"));
	}

	FString Error;
	UNiagaraSystem* System = NiagaraCommonUtils::LoadNiagaraSystem(SystemPath, Error);
	if (!System)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString FullName = ParameterName;
	if (!FullName.StartsWith(TEXT("User.")))
	{
		FullName = FString::Printf(TEXT("User.%s"), *ParameterName);
	}

	FNiagaraUserRedirectionParameterStore& Store = System->GetExposedParameters();

	FNiagaraVariable VarToRemove;
	bool bFound = false;
	for (const FNiagaraVariableWithOffset& Existing : Store.ReadParameterVariables())
	{
		if (Existing.GetName().ToString().Equals(FullName, ESearchCase::IgnoreCase))
		{
			VarToRemove = FNiagaraVariable(Existing.GetType(), Existing.GetName());
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("User parameter '%s' not found"), *FullName));
	}

	Store.RemoveParameter(VarToRemove);
	System->MarkPackageDirty();
	System->PostEditChange();

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("removed_parameter"), FullName);
	return Result;
}

// ---------------------------------------------------------------------------
// FLinkNiagaraParameterAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FLinkNiagaraParameterAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
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

	FString LinkedParameterName;
	if (!Params->TryGetStringField(TEXT("linked_parameter"), LinkedParameterName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'linked_parameter' parameter (e.g., 'User.MyColor')"));
	}

	FString ScriptUsageStr = TEXT("particle_update");
	Params->TryGetStringField(TEXT("script_usage"), ScriptUsageStr);

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

	bool bUsageOk = false;
	ENiagaraScriptUsage Usage = NiagaraCommonUtils::ParseScriptUsage(ScriptUsageStr, bUsageOk);
	if (!bUsageOk)
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Invalid script_usage '%s'"), *ScriptUsageStr));
	}

	FVersionedNiagaraEmitterData* EmitterData = NiagaraCommonUtils::GetEmitterData(Handle);
	if (!EmitterData)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Emitter has no data"));
	}

	UNiagaraGraph* Graph = NiagaraCommonUtils::GetGraphForUsage(EmitterData, Usage);
	if (!Graph)
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Could not get graph for script usage"));
	}

	UNiagaraNodeFunctionCall* ModuleNode = NiagaraCommonUtils::FindModuleNode(Graph, Usage, ModuleName, Error);
	if (!ModuleNode)
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	// Resolve nested input paths first.
	{
		FString LeafName, DescentError;
		UNiagaraNodeFunctionCall* TargetCall =
			NiagaraCommonUtils::DescendNestedPath(*ModuleNode, InputName, LeafName, DescentError);
		if (!TargetCall)
		{
			return FMCPCommonUtils::CreateErrorResponse(DescentError);
		}
		ModuleNode = TargetCall;
		InputName = LeafName;
	}

	// Resolve the module input variable and type.
	const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
	FNiagaraTypeDefinition InputType;
	FString ResolvedInputName;
	FString InputSource;

	for (UEdGraphPin* Pin : ModuleNode->Pins)
	{
		if (Pin->Direction != EGPD_Input)
		{
			continue;
		}
		const FString PinNameStr = Pin->PinName.ToString();
		if (PinNameStr.Equals(InputName, ESearchCase::IgnoreCase))
		{
			InputType = NiagaraSchema->PinToTypeDefinition(Pin);
			ResolvedInputName = PinNameStr;
			InputSource = TEXT("function_call_pin");
			break;
		}
	}

	if (!InputType.IsValid())
	{
		TArray<FNiagaraVariable> StackInputs;
		FCompileConstantResolver ConstantResolver(System, Usage);
		FNiagaraStackGraphUtilities::GetStackFunctionInputs(
			*ModuleNode, StackInputs, ConstantResolver,
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

	FNiagaraParameterHandle InputHandle =
		FNiagaraParameterHandle::CreateModuleParameterHandle(FName(*ResolvedInputName));
	FNiagaraParameterHandle AliasedHandle =
		FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(InputHandle, ModuleNode);

	FScopedTransaction Transaction(
		NSLOCTEXT("UnrealMCPBridge", "LinkNiagaraParameter", "Link Niagara Parameter"));
	ModuleNode->Modify();
	Graph->Modify();

	UEdGraphPin& OverridePin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
		*ModuleNode, AliasedHandle, InputType, FGuid(), FGuid());

	// Clear any existing override.
	if (OverridePin.LinkedTo.Num() > 0)
	{
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
			if (!NodeToRemove)
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

	TSet<FNiagaraVariableBase> KnownParameters;
	{
		TArray<FNiagaraVariable> UserVars;
		System->GetExposedParameters().GetParameters(UserVars);
		for (const FNiagaraVariable& Var : UserVars)
		{
			KnownParameters.Add(Var);
		}

		if (Graph)
		{
			for (const auto& It : Graph->GetAllMetaData())
			{
				KnownParameters.Add(It.Key);
			}
		}
	}

	FNiagaraVariableBase LinkedParameter(InputType, FName(*LinkedParameterName));

	FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(
		OverridePin,
		LinkedParameter,
		KnownParameters,
		ENiagaraDefaultMode::FailIfPreviouslyNotSet,
		FGuid());

	Graph->NotifyGraphChanged();
	NiagaraCommonUtils::CompileAndSync(System);

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("module_name"), ModuleName);
	Result->SetStringField(TEXT("input_name"), ResolvedInputName);
	Result->SetStringField(TEXT("input_source"), InputSource);
	Result->SetStringField(TEXT("input_type"), InputType.GetName());
	Result->SetStringField(TEXT("linked_parameter"), LinkedParameterName);
	Result->SetStringField(TEXT("script_usage"), ScriptUsageStr);
	return Result;
}

// ---------------------------------------------------------------------------
// FListNiagaraParameterTypesAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FListNiagaraParameterTypesAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context)
{
	(void)Context;

	FString Scope = TEXT("user");
	Params->TryGetStringField(TEXT("scope"), Scope);

	FString Kind = TEXT("all");
	Params->TryGetStringField(TEXT("kind"), Kind);

	FString NameFilter;
	Params->TryGetStringField(TEXT("filter"), NameFilter);

	int32 MaxResults = 500;
	{
		double MaxResultsD = 500.0;
		if (Params->TryGetNumberField(TEXT("max_results"), MaxResultsD))
		{
			MaxResults = static_cast<int32>(MaxResultsD);
		}
	}

	TArray<FNiagaraTypeDefinition> Types;
	const FString LowerScope = Scope.ToLower();
	if (LowerScope == TEXT("user"))
	{
		Types = FNiagaraTypeRegistry::GetRegisteredUserVariableTypes();
	}
	else if (LowerScope == TEXT("system"))
	{
		Types = FNiagaraTypeRegistry::GetRegisteredSystemVariableTypes();
	}
	else if (LowerScope == TEXT("emitter"))
	{
		Types = FNiagaraTypeRegistry::GetRegisteredEmitterVariableTypes();
	}
	else if (LowerScope == TEXT("particle"))
	{
		Types = FNiagaraTypeRegistry::GetRegisteredParticleVariableTypes();
	}
	else if (LowerScope == TEXT("parameter"))
	{
		Types = FNiagaraTypeRegistry::GetRegisteredParameterTypes();
	}
	else if (LowerScope == TEXT("payload"))
	{
		Types = FNiagaraTypeRegistry::GetRegisteredPayloadTypes();
	}
	else if (LowerScope == TEXT("numeric"))
	{
		Types = FNiagaraTypeRegistry::GetNumericTypes();
	}
	else if (LowerScope == TEXT("index"))
	{
		Types = FNiagaraTypeRegistry::GetIndexTypes();
	}
	else
	{
		Types = FNiagaraTypeRegistry::GetRegisteredTypes();
	}

	const FString LowerKind = Kind.ToLower();

	auto ClassifyKind = [](const FNiagaraTypeDefinition& T) -> FString
	{
		if (T.IsDataInterface())
		{
			return TEXT("data_interface");
		}
		if (T.IsEnum())
		{
			return TEXT("enum");
		}
		if (T.IsUObject())
		{
			return TEXT("object");
		}
		if (T.GetScriptStruct())
		{
			return TEXT("struct");
		}
		return TEXT("primitive");
	};

	TArray<TSharedPtr<FJsonValue>> TypesArr;
	int32 TotalMatched = 0;
	for (const FNiagaraTypeDefinition& TypeDef : Types)
	{
		if (!TypeDef.IsValid())
		{
			continue;
		}

		const FString EntryKind = ClassifyKind(TypeDef);
		if (LowerKind != TEXT("all") && !EntryKind.Equals(LowerKind, ESearchCase::IgnoreCase))
		{
			continue;
		}

		const FString EntryName = TypeDef.GetName();
		if (!NameFilter.IsEmpty() && !EntryName.Contains(NameFilter, ESearchCase::IgnoreCase))
		{
			if (!(TypeDef.IsDataInterface() && EntryName.Replace(TEXT("NiagaraDataInterface"), TEXT("")).Contains(
				NameFilter, ESearchCase::IgnoreCase)))
			{
				continue;
			}
		}

		TotalMatched++;
		if (TypesArr.Num() >= MaxResults)
		{
			continue;
		}

		auto Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("name"), EntryName);
		Obj->SetStringField(TEXT("kind"), EntryKind);
		Obj->SetNumberField(TEXT("size_bytes"), TypeDef.GetSize());

		if (TypeDef.IsDataInterface())
		{
			if (UClass* Cls = TypeDef.GetClass())
			{
				Obj->SetStringField(TEXT("class_path"), Cls->GetPathName());
				FString Short = Cls->GetName();
				if (Short.StartsWith(TEXT("NiagaraDataInterface")))
				{
					Short.RightChopInline(FString(TEXT("NiagaraDataInterface")).Len());
					Obj->SetStringField(TEXT("short_name"), Short);
				}
#if WITH_EDITOR
				const FString Category = Cls->GetMetaData(TEXT("Category"));
				if (!Category.IsEmpty())
				{
					Obj->SetStringField(TEXT("category"), Category);
				}
				const FString Tooltip = Cls->GetMetaData(TEXT("ToolTip"));
				if (!Tooltip.IsEmpty())
				{
					Obj->SetStringField(TEXT("description"), Tooltip);
				}
#endif
			}
		}
		else if (TypeDef.IsEnum())
		{
			if (UEnum* Enum = TypeDef.GetEnum())
			{
				Obj->SetStringField(TEXT("enum_path"), Enum->GetPathName());
				Obj->SetNumberField(TEXT("num_entries"), Enum->NumEnums());
			}
		}
		else if (UScriptStruct* Struct = TypeDef.GetScriptStruct())
		{
			Obj->SetStringField(TEXT("struct_path"), Struct->GetPathName());
#if WITH_EDITOR
			const FString Tooltip = Struct->GetMetaData(TEXT("ToolTip"));
			if (!Tooltip.IsEmpty())
			{
				Obj->SetStringField(TEXT("description"), Tooltip);
			}
#endif
		}

		TypesArr.Add(MakeShared<FJsonValueObject>(Obj));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("scope"), Scope);
	Result->SetStringField(TEXT("kind_filter"), Kind);
	if (!NameFilter.IsEmpty())
	{
		Result->SetStringField(TEXT("name_filter"), NameFilter);
	}
	Result->SetArrayField(TEXT("types"), TypesArr);
	Result->SetNumberField(TEXT("count"), TypesArr.Num());
	Result->SetNumberField(TEXT("total_matched"), TotalMatched);
	Result->SetNumberField(TEXT("max_results"), MaxResults);
	return Result;
}

// ---------------------------------------------------------------------------
// FListNiagaraDataInterfacesAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FListNiagaraDataInterfacesAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context)
{
	(void)Context;

	FString Filter;
	Params->TryGetStringField(TEXT("filter"), Filter);

	TArray<UClass*> DerivedClasses;
	GetDerivedClasses(UNiagaraDataInterface::StaticClass(), DerivedClasses, true);

	TArray<TSharedPtr<FJsonValue>> InterfacesArr;

	for (UClass* Class : DerivedClasses)
	{
		if (!Class || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
		{
			continue;
		}

		FString ClassName = Class->GetName();

		if (!Filter.IsEmpty() && !ClassName.Contains(Filter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		auto Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("class_name"), ClassName);
		Obj->SetStringField(TEXT("class_path"), Class->GetPathName());

		FString DisplayName = ClassName;
		DisplayName.RemoveFromStart(TEXT("UNiagaraDataInterface"));
		DisplayName.RemoveFromStart(TEXT("NiagaraDataInterface"));
		if (DisplayName.IsEmpty())
		{
			DisplayName = ClassName;
		}
		Obj->SetStringField(TEXT("display_name"), DisplayName);

		InterfacesArr.Add(MakeShared<FJsonValueObject>(Obj));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("filter"), Filter);
	Result->SetArrayField(TEXT("data_interfaces"), InterfacesArr);
	Result->SetNumberField(TEXT("count"), InterfacesArr.Num());
	return Result;
}

// ---------------------------------------------------------------------------
// FListNiagaraScriptParametersAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FListNiagaraScriptParametersAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context)
{
	(void)Context;

	FString Error;
	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	UNiagaraScript* Script = nullptr;
	UNiagaraGraph* Graph = nullptr;
	if (!NiagaraCommonUtils::ResolveScratchPadScriptAndGraph(Params, System, ModuleName, Script, Graph, Error))
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	// Collect output parameters.
	TArray<TSharedPtr<FJsonValue>> OutputParams;
	UNiagaraNodeOutput* OutputNode = NiagaraCommonUtils::FindFirstOutputNode(Graph);
	if (OutputNode)
	{
		for (int32 i = 0; i < OutputNode->Outputs.Num(); ++i)
		{
			OutputParams.Add(MakeShared<FJsonValueObject>(
				NiagaraCommonUtils::ScriptParameterToJson(OutputNode->Outputs[i], i)));
		}
	}

	// Collect input parameters.
	TArray<TSharedPtr<FJsonValue>> InputParams;
	int32 InputIndex = 0;
	const UNiagaraGraph::FScriptVariableMap& MetaMap = Graph->GetAllMetaData();
	for (const TPair<FNiagaraVariable, UNiagaraScriptVariable*>& Pair : MetaMap)
	{
		const FString VarName = Pair.Key.GetName().ToString();
		const bool bIsInputLike =
			VarName.StartsWith(TEXT("Module."), ESearchCase::IgnoreCase) ||
			VarName.StartsWith(TEXT("Input."), ESearchCase::IgnoreCase) ||
			VarName.StartsWith(TEXT("User."), ESearchCase::IgnoreCase);
		if (!bIsInputLike)
		{
			continue;
		}

		auto J = NiagaraCommonUtils::ScriptParameterToJson(Pair.Key, InputIndex++);
		if (Pair.Value)
		{
			J->SetStringField(TEXT("default_mode"),
			                  StaticEnum<ENiagaraDefaultMode>()
			                  ->GetNameStringByValue(static_cast<int64>(Pair.Value->DefaultMode)));
		}
		InputParams.Add(MakeShared<FJsonValueObject>(J));
	}

	auto R = MakeShared<FJsonObject>();
	R->SetBoolField(TEXT("success"), true);
	R->SetStringField(TEXT("script_path"), Script->GetPathName());
	R->SetArrayField(TEXT("outputs"), OutputParams);
	R->SetArrayField(TEXT("inputs"), InputParams);
	return R;
}

// ---------------------------------------------------------------------------
// FAddNiagaraScriptParameterAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FAddNiagaraScriptParameterAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context)
{
	(void)Context;

	FString Error;
	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	UNiagaraScript* Script = nullptr;
	UNiagaraGraph* PrimaryGraph = nullptr;
	if (!NiagaraCommonUtils::ResolveScratchPadScriptAndGraph(Params, System, ModuleName, Script, PrimaryGraph, Error))
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString Direction = TEXT("output");
	Params->TryGetStringField(TEXT("direction"), Direction);

	FString ParamName, TypeName;
	if (!Params->TryGetStringField(TEXT("name"), ParamName) ||
		!Params->TryGetStringField(TEXT("type"), TypeName))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing 'name' or 'type'"));
	}

	FNiagaraTypeDefinition TypeDef;
	if (!NiagaraCommonUtils::ResolveTypeName(TypeName, TypeDef) || !TypeDef.IsValid())
	{
		return FMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unknown Niagara type '%s'"), *TypeName));
	}

	TArray<UNiagaraGraph*> MutGraphs = NiagaraCommonUtils::CollectMutationGraphs(System, ModuleName, Script);

	for (UNiagaraGraph* G : MutGraphs)
	{
		G->Modify();
		if (Direction.Equals(TEXT("output"), ESearchCase::IgnoreCase))
		{
			if (UNiagaraNodeOutput* OutNode = NiagaraCommonUtils::FindFirstOutputNode(G))
			{
				OutNode->Modify();
				FNiagaraVariable NewVar(TypeDef, FName(*ParamName));
				bool bExists = false;
				for (const FNiagaraVariable& V : OutNode->Outputs)
				{
					if (V.GetName() == NewVar.GetName())
					{
						bExists = true;
						break;
					}
				}
				if (!bExists)
				{
					OutNode->Outputs.Add(NewVar);
					if (FProperty* OutputsProp = UNiagaraNodeOutput::StaticClass()
						->FindPropertyByName(TEXT("Outputs")))
					{
						FPropertyChangedEvent Evt(OutputsProp, EPropertyChangeType::ArrayAdd);
						OutNode->PostEditChangeProperty(Evt);
					}
				}
			}
		}
		else
		{
			FString FullName = ParamName;
			if (!FullName.Contains(TEXT(".")))
			{
				FullName = FString::Printf(TEXT("Module.%s"), *ParamName);
			}
			FNiagaraVariable NewVar(TypeDef, FName(*FullName));
			UNiagaraGraph::FScriptVariableMap& MetaMap = G->GetAllMetaData();
			if (!MetaMap.Contains(NewVar))
			{
				UNiagaraScriptVariable* SV =
					NewObject<UNiagaraScriptVariable>(G, NAME_None, RF_Transactional);
				FNiagaraVariableMetaData Meta;
				SV->Init(NewVar, Meta);
				SV->DefaultMode = FullName.StartsWith(TEXT("Module."))
					                  ? ENiagaraDefaultMode::Value
					                  : ENiagaraDefaultMode::Binding;
				MetaMap.Add(NewVar, SV);
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
	R->SetBoolField(TEXT("success"), true);
	R->SetStringField(TEXT("direction"), Direction);
	R->SetStringField(TEXT("name"), ParamName);
	R->SetStringField(TEXT("type"), TypeDef.GetName());
	return R;
}

// ---------------------------------------------------------------------------
// FRemoveNiagaraScriptParameterAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FRemoveNiagaraScriptParameterAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context)
{
	(void)Context;

	FString Error;
	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	UNiagaraScript* Script = nullptr;
	UNiagaraGraph* PrimaryGraph = nullptr;
	if (!NiagaraCommonUtils::ResolveScratchPadScriptAndGraph(Params, System, ModuleName, Script, PrimaryGraph, Error))
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString Direction = TEXT("output");
	Params->TryGetStringField(TEXT("direction"), Direction);
	FString ParamName;
	if (!Params->TryGetStringField(TEXT("name"), ParamName))
	{
		return FMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name'"));
	}

	TArray<UNiagaraGraph*> MutGraphs = NiagaraCommonUtils::CollectMutationGraphs(System, ModuleName, Script);
	int32 Removed = 0;
	for (UNiagaraGraph* G : MutGraphs)
	{
		G->Modify();
		if (Direction.Equals(TEXT("output"), ESearchCase::IgnoreCase))
		{
			if (UNiagaraNodeOutput* OutNode = NiagaraCommonUtils::FindFirstOutputNode(G))
			{
				OutNode->Modify();
				const int32 Before = OutNode->Outputs.Num();
				OutNode->Outputs.RemoveAll([&](const FNiagaraVariable& V)
				{
					return V.GetName() == FName(*ParamName);
				});
				if (OutNode->Outputs.Num() != Before)
				{
					if (FProperty* OutputsProp = UNiagaraNodeOutput::StaticClass()
						->FindPropertyByName(TEXT("Outputs")))
					{
						FPropertyChangedEvent Evt(OutputsProp, EPropertyChangeType::ArrayRemove);
						OutNode->PostEditChangeProperty(Evt);
					}
					++Removed;
				}
			}
		}
		else
		{
			UNiagaraGraph::FScriptVariableMap& MetaMap = G->GetAllMetaData();
			FNiagaraVariable Key;
			for (const TPair<FNiagaraVariable, UNiagaraScriptVariable*>& Pair : MetaMap)
			{
				if (Pair.Key.GetName() == FName(*ParamName))
				{
					Key = Pair.Key;
					break;
				}
			}
			if (Key.GetType().IsValid())
			{
				MetaMap.Remove(Key);
				++Removed;

				const FName ParamFName(*ParamName);
				for (UEdGraphNode* N : G->Nodes)
				{
					if (!N)
					{
						continue;
					}
					const bool bIsMapGet = N->IsA(UNiagaraNodeParameterMapGet::StaticClass());
					const bool bIsMapSet = N->IsA(UNiagaraNodeParameterMapSet::StaticClass());
					if (!bIsMapGet && !bIsMapSet)
					{
						continue;
					}

					TArray<UEdGraphPin*> PinsCopy = N->Pins;
					for (UEdGraphPin* Pin : PinsCopy)
					{
						if (!Pin || Pin->PinName != ParamFName)
						{
							continue;
						}
						N->Modify();
						Pin->BreakAllPinLinks();
						N->RemovePin(Pin);
					}
				}
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
	R->SetBoolField(TEXT("success"), Removed > 0);
	R->SetNumberField(TEXT("removed_graph_count"), Removed);
	R->SetStringField(TEXT("name"), ParamName);
	return R;
}

// ---------------------------------------------------------------------------
// FRenameNiagaraScriptParameterAction::ExecuteInternal
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> FRenameNiagaraScriptParameterAction::ExecuteInternal(
	const TSharedPtr<FJsonObject>& Params,
	FMCPEditorContext& Context)
{
	(void)Context;

	FString Error;
	UNiagaraSystem* System = nullptr;
	FString ModuleName;
	UNiagaraScript* Script = nullptr;
	UNiagaraGraph* PrimaryGraph = nullptr;
	if (!NiagaraCommonUtils::ResolveScratchPadScriptAndGraph(Params, System, ModuleName, Script, PrimaryGraph, Error))
	{
		return FMCPCommonUtils::CreateErrorResponse(Error);
	}

	FString Direction = TEXT("output");
	Params->TryGetStringField(TEXT("direction"), Direction);
	FString OldName, NewName;
	if (!Params->TryGetStringField(TEXT("old_name"), OldName) ||
		!Params->TryGetStringField(TEXT("new_name"), NewName))
	{
		return FMCPCommonUtils::CreateErrorResponse(
			TEXT("Missing 'old_name' or 'new_name'"));
	}

	TArray<UNiagaraGraph*> MutGraphs = NiagaraCommonUtils::CollectMutationGraphs(System, ModuleName, Script);
	int32 Renamed = 0;
	for (UNiagaraGraph* G : MutGraphs)
	{
		G->Modify();
		if (Direction.Equals(TEXT("output"), ESearchCase::IgnoreCase))
		{
			if (UNiagaraNodeOutput* OutNode = NiagaraCommonUtils::FindFirstOutputNode(G))
			{
				OutNode->Modify();
				for (FNiagaraVariable& V : OutNode->Outputs)
				{
					if (V.GetName() == FName(*OldName))
					{
						V.SetName(FName(*NewName));
						++Renamed;
					}
				}
				if (FProperty* OutputsProp = UNiagaraNodeOutput::StaticClass()
					->FindPropertyByName(TEXT("Outputs")))
				{
					FPropertyChangedEvent Evt(OutputsProp, EPropertyChangeType::ValueSet);
					OutNode->PostEditChangeProperty(Evt);
				}
			}
		}
		else
		{
			UNiagaraGraph::FScriptVariableMap& MetaMap = G->GetAllMetaData();
			FNiagaraVariable OldKey;
			UNiagaraScriptVariable* SV = nullptr;
			for (const TPair<FNiagaraVariable, UNiagaraScriptVariable*>& Pair : MetaMap)
			{
				if (Pair.Key.GetName() == FName(*OldName))
				{
					OldKey = Pair.Key;
					SV = Pair.Value;
					break;
				}
			}
			if (SV && OldKey.GetType().IsValid())
			{
				FNiagaraVariable NewKey(OldKey.GetType(), FName(*NewName));
				MetaMap.Remove(OldKey);
				SV->Variable = NewKey;
				MetaMap.Add(NewKey, SV);
				++Renamed;
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
	R->SetBoolField(TEXT("success"), Renamed > 0);
	R->SetStringField(TEXT("old_name"), OldName);
	R->SetStringField(TEXT("new_name"), NewName);
	R->SetNumberField(TEXT("renamed_graph_count"), Renamed);
	return R;
}

// ---------------------------------------------------------------------------
// Registration

// ---------------------------------------------------------------------------
// Action metadata
// ---------------------------------------------------------------------------

FString FGetNiagaraUserParametersAction::GetActionName() const
{
	return TEXT("get_niagara_user_parameters");
}

bool FGetNiagaraUserParametersAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	if (!GetOptionalString(Params, TEXT("actor_name")).IsEmpty() ||
		!GetOptionalString(Params, TEXT("system_path")).IsEmpty())
	{
		return true;
	}
	OutError = TEXT("Provide either 'actor_name' or 'system_path'");
	return false;
}

FString FAddNiagaraUserParameterAction::GetActionName() const
{
	return TEXT("add_niagara_user_parameter");
}

bool FAddNiagaraUserParameterAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("parameter_name"), Unused, OutError);
}

FString FSetNiagaraUserParameterAction::GetActionName() const
{
	return TEXT("set_niagara_user_parameter");
}

bool FSetNiagaraUserParameterAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("parameter_name"), Unused, OutError))
	{
		return false;
	}
	if (GetOptionalString(Params, TEXT("actor_name")).IsEmpty() &&
		GetOptionalString(Params, TEXT("system_path")).IsEmpty())
	{
		OutError = TEXT("Provide either 'actor_name' (runtime) or 'system_path' (asset default)");
		return false;
	}
	if (!Params.IsValid() || !Params->HasField(TEXT("value")))
	{
		OutError = TEXT("Required parameter 'value' is missing");
		return false;
	}
	return true;
}

FString FRemoveNiagaraUserParameterAction::GetActionName() const
{
	return TEXT("remove_niagara_user_parameter");
}

bool FRemoveNiagaraUserParameterAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!GetRequiredString(Params, TEXT("system_path"), Unused, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("parameter_name"), Unused, OutError);
}

FString FLinkNiagaraParameterAction::GetActionName() const
{
	return TEXT("link_niagara_parameter");
}

bool FLinkNiagaraParameterAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	return GetRequiredString(Params, TEXT("system_path"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("emitter_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("module_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("input_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("linked_parameter"), Unused, OutError);
}

FString FListNiagaraParameterTypesAction::GetActionName() const
{
	return TEXT("list_niagara_parameter_types");
}

bool FListNiagaraParameterTypesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Params;
	(void)Context;
	(void)OutError;
	return true;
}

FString FListNiagaraDataInterfacesAction::GetActionName() const
{
	return TEXT("list_niagara_data_interfaces");
}

bool FListNiagaraDataInterfacesAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Params;
	(void)Context;
	(void)OutError;
	return true;
}

FString FListNiagaraScriptParametersAction::GetActionName() const
{
	return TEXT("list_niagara_script_parameters");
}

bool FListNiagaraScriptParametersAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	return NiagaraCommonUtils::ValidateMutationGraphParams(Params, OutError);
}

FString FAddNiagaraScriptParameterAction::GetActionName() const
{
	return TEXT("add_niagara_script_parameter");
}

bool FAddNiagaraScriptParameterAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!NiagaraCommonUtils::ValidateMutationGraphParams(Params, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("type"), Unused, OutError);
}

FString FRemoveNiagaraScriptParameterAction::GetActionName() const
{
	return TEXT("remove_niagara_script_parameter");
}

bool FRemoveNiagaraScriptParameterAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!NiagaraCommonUtils::ValidateMutationGraphParams(Params, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("name"), Unused, OutError);
}

FString FRenameNiagaraScriptParameterAction::GetActionName() const
{
	return TEXT("rename_niagara_script_parameter");
}

bool FRenameNiagaraScriptParameterAction::Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError)
{
	(void)Context;
	FString Unused;
	if (!NiagaraCommonUtils::ValidateMutationGraphParams(Params, OutError))
	{
		return false;
	}
	return GetRequiredString(Params, TEXT("old_name"), Unused, OutError) &&
		GetRequiredString(Params, TEXT("new_name"), Unused, OutError);
}

#pragma once

#include "Actions/NiagaraActions/NiagaraActions.h"

class UEEDITORMCP_API FGetNiagaraModulesAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FAddNiagaraModuleAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FRemoveNiagaraModuleAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FSetNiagaraModuleEnabledAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FReorderNiagaraModuleAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FGetNiagaraModuleInputsAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FSetNiagaraModuleInputAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FSetNiagaraDynamicInputAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FSetNiagaraCurveAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FGetNiagaraRapidIterationParametersAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FSetNiagaraRapidIterationParameterAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FListNiagaraModulesAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FGetNiagaraModuleInputBindingAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FClearNiagaraModuleInputAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FListNiagaraInputSourceMenuAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FResolveNiagaraBuiltInDynamicInputAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

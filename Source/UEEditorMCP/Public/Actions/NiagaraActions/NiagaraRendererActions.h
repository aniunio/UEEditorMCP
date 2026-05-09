#pragma once

#include "Actions/NiagaraActions/NiagaraActions.h"

class UEEDITORMCP_API FAddNiagaraRendererAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FRemoveNiagaraRendererAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FGetNiagaraRendererInfoAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FGetNiagaraRendererPropertiesAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FSetNiagaraRendererPropertyAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FSetNiagaraRendererBindingAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

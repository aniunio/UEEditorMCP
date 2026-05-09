#pragma once

#include "Actions/NiagaraActions/NiagaraActions.h"

class UEEDITORMCP_API FGetNiagaraUserParametersAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FAddNiagaraUserParameterAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FSetNiagaraUserParameterAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FRemoveNiagaraUserParameterAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FLinkNiagaraParameterAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FListNiagaraParameterTypesAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FListNiagaraDataInterfacesAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FListNiagaraScriptParametersAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FAddNiagaraScriptParameterAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FRemoveNiagaraScriptParameterAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FRenameNiagaraScriptParameterAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

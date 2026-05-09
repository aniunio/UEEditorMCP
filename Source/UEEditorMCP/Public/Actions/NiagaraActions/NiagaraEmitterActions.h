#pragma once

#include "Actions/NiagaraActions/NiagaraActions.h"

class UNiagaraSystem;
struct FNiagaraEmitterHandle;
struct FVersionedNiagaraEmitterData;

class UEEDITORMCP_API FGetNiagaraEmittersAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FAddNiagaraEmitterAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FRemoveNiagaraEmitterAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FSetNiagaraEmitterPropertyAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FDuplicateNiagaraEmitterAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FReorderNiagaraEmitterAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FRenameNiagaraEmittersAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FAddNiagaraEventHandlerAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FAddNiagaraSimulationStageAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FGetNiagaraEventHandlersAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FListNiagaraEmitterTemplatesAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FGetNiagaraEmitterAttributesAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

#pragma once

#include "Actions/NiagaraActions/NiagaraActions.h"

class UEEDITORMCP_API FGetNiagaraGraphNodesAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FGetNiagaraNodeInfoAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FTraceNiagaraConnectionAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FValidateNiagaraGraphAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FGetNiagaraScriptPropertiesAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FSetNiagaraScriptPropertiesAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FAddNiagaraGraphNodeAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FDeleteNiagaraGraphNodeAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FListNiagaraDataInterfaceFunctionsAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FListNiagaraOpsAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FListNiagaraNodeTypesAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FGetNiagaraNodeTypeInfoAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FSearchNiagaraFunctionsAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FDescribeNiagaraTypeAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FGetNiagaraSchemaActionsAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FListNiagaraAvailableParametersAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FAddNiagaraMapGetPinAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FAddNiagaraMapSetPinAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FAddNiagaraNodePinAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FRenameNiagaraNodePinAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FRemoveNiagaraNodePinAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FConnectNiagaraPinsAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FDisconnectNiagaraPinsAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
};

class UEEDITORMCP_API FGetNiagaraDataInterfaceSchemaAction : public FEditorAction
{
public:
	virtual FString GetActionName() const override;

protected:
	virtual TSharedPtr<FJsonObject> ExecuteInternal(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context) override;
	virtual bool Validate(const TSharedPtr<FJsonObject>& Params, FMCPEditorContext& Context, FString& OutError) override;
};

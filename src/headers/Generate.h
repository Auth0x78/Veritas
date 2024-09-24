#pragma once

#include <memory>
#include <vector>
#include <string>
#include "llvm_includes.h"

#include "Node.h"
#include "Parser.h"
#include "token.h"

struct varInfo
{
	llvm::Value* vAddr = nullptr;
	llvm::Type* vType = nullptr;
};

struct fnInfo
{
	llvm::FunctionType* fnType = nullptr;
	bool isDefined = false;
};

class Generator
{
public:
	Generator(std::unique_ptr<Program> program, const std::string& moduleName, const std::string& outPath);
	void Generate();

	llvm::Value* CreateGlobalDecl(const std::unique_ptr<DeclStmt>& declStmt);

	llvm::Function* CreateFunction(const std::unique_ptr<FnStmt>& fnStmt);
	
	llvm::CallInst* CreateFunctionCall(const std::unique_ptr<FnCall>& FunctionCall);

	void GenerateCompoundStatement(const std::unique_ptr<CompoundStmt>& cmpndStmt);

	void GenerateStatment(const std::unique_ptr<Stmt>& stmt);

	llvm::Value* GenerateExpr(const std::unique_ptr<Expr>& expr);

	void saveModuleToFile();

	void moduleInit();

	int getTypePriority(llvm::Type* type);

	llvm::Value* autoTypeCast(llvm::Value* val, llvm::Type* targetType);

	llvm::Type* findTypeFromPrimitive(PrimitiveDataType pdt);

public:
	std::string m_outPath;
	std::string m_moduleName;
	std::unique_ptr<Program> m_program;
	llvm::FunctionType* m_FunctionType;

	std::unique_ptr<llvm::LLVMContext> ctx;
	std::unique_ptr<llvm::Module> cModule;
	std::unique_ptr<llvm::IRBuilder<>> builder;
	
	/* TODO: Replace current finding of symbol with name with ident and scope
	struct VarID
	{
		std::string scope;
		std::string ident;
	};

	and we keep a stack which keeps track of current scope
	and we try to find var with all scopes in that stack
	global scope: "GLOBAL"
	or rather the key of map should be string(ident) + ':' + string(scopeName)
	*/

	std::unordered_map<PrimitiveDataType, llvm::Type*> m_TypeMap;
	std::unordered_map<std::string, varInfo> m_symbolMap;
	std::unordered_map<std::string, fnInfo> m_FunctionMap;
};
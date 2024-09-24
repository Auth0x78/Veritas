#pragma once
#include <optional>
#include <vector>
#include <memory>
#include <variant>

#include "token.h"


struct Ident
{
	std::string name;
};

//struct Type
//{
//	Ident ident;
//	PrimitiveDataType type;
//	bool isBuiltin = false;
//};
// will PLUS when custom data types are introduced

struct Literal
{
	PrimitiveDataType type;
	std::string value;
};

struct Expr;
struct ArgsList
{
	std::vector<std::unique_ptr<Expr>> list;
};

struct FnCall
{
	std::string name;
	std::unique_ptr<ArgsList> args;
};

struct ReturnStmt
{
	std::unique_ptr<Expr> value;
};

struct DeclStmt
{
	PrimitiveDataType type;
	std::string IDENT;
	std::unique_ptr<Expr> expr;
};

struct BinaryOp
{
	TokenType type;
	std::unique_ptr<Expr> LHS;
	std::unique_ptr<Expr> RHS;
};

struct Expr
{
	std::variant<std::unique_ptr<BinaryOp>, std::unique_ptr<Ident>, std::unique_ptr<Literal>> value;
};

struct CompoundStmt;
struct Stmt
{
	std::variant<std::unique_ptr<DeclStmt>, std::unique_ptr<CompoundStmt>, std::unique_ptr<ReturnStmt>, std::unique_ptr<FnCall>> stmt;
};

struct CompoundStmt
{
	std::vector<std::unique_ptr<Stmt>> statementList;
};

struct ParamDecl
{
	std::string ident;
	PrimitiveDataType type;
	bool VarArg = false;
};

struct FnStmt
{
	std::string name;
	PrimitiveDataType returnType;
	
	std::vector<std::unique_ptr<ParamDecl>> params; //-> For now no params
	std::unique_ptr<CompoundStmt> compoundStmt;

	bool isExtern = false;
};


struct Program
{
	std::vector<std::unique_ptr<DeclStmt>> DeclStmts;
	std::vector<std::unique_ptr<FnStmt>> FnStmts;
};

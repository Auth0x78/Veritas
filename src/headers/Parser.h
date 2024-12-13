#pragma once
#include <optional>
#include <variant>
#include <vector>
#include <unordered_map>
#include <stack>
#include <memory>

#include "token.h"
#include "Node.h"
#include "Logger.h"

class Parser
{
public:
	Parser(std::vector<Token>& tokens);

	bool Parse();
	bool ParseGlobalDecl();
	std::unique_ptr<FnStmt> ParseFunction();
	std::unique_ptr<ParamDecl> ParseParamDecl();
	std::unique_ptr<CompoundStmt> ParseCompoundStmt();
	std::unique_ptr<Stmt> ParseStmt();
	std::unique_ptr<DeclStmt> ParseDeclStmt();
	std::unique_ptr<ReturnStmt> ParseReturnStmt();
	std::unique_ptr<Expr> ParseFunctionCallExpr();
	std::unique_ptr<FnCall> ParseFunctionCallStmt();
	std::unique_ptr<ArgsList> ParseArgsList();
	bool ParseArg(std::unique_ptr<ArgsList>& argsList);
	std::unique_ptr<Expr> ParseExpr();
	std::unique_ptr<Expr> CreateVarExpr(const std::string& name);
	std::unique_ptr<Expr> CreateLiteralExpr(const std::string& value, PrimitiveDataType dataType);
	std::unique_ptr<Program> getProgram();
private:
	std::optional<Token> peek(int ahead = 0);
	Token consume();
	std::optional<Token> PeekAndCheck(TokenType type, int ahead = 0);
	bool ApplyOperator();
	PrimitiveDataType ptrTypeof(PrimitiveDataType type);

	std::vector<Token> m_tokens;
	size_t m_index = 0;
	std::unique_ptr<Program> m_programAST;
	
	std::unordered_map<TokenType, std::pair<int, char>> OperandTuple;
	
	std::stack<std::unique_ptr<Expr>> m_nodeStack;
	std::stack<Token> m_operatorStack;
};
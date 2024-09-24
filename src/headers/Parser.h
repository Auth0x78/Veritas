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
	std::unique_ptr<FnCall> ParseFunctionCall();
	std::unique_ptr<ArgsList> ParseArgsList();
	std::unique_ptr<Expr> ParseExpr();

	std::unique_ptr<Program> getProgram();
private:
	std::optional<Token> peek(int ahead = 0);
	Token consume();
	std::optional<Token> PeekAndCheck(TokenType type, int ahead = 0);
	std::unique_ptr<Expr> GenerateBinaryOpNode(Token& op, std::unique_ptr<Expr>& LHS, std::unique_ptr<Expr>& RHS);
	PrimitiveDataType ptrTypeof(PrimitiveDataType type);

	std::vector<Token> m_tokens;
	size_t m_index = 0;
	std::unique_ptr<Program> m_programAST;
	
	std::unordered_map<TokenType, std::pair<int, char>> OperandTuple;
};
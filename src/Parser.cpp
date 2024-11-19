#include "headers/Parser.h"

#define RUN_AND_RETURN(cmd, exitValue) { cmd; return exitValue;}

Parser::Parser(std::vector<Token>& tokens) 
	: m_tokens(std::move(tokens)), m_programAST(std::make_unique<Program>())
{
	OperandTuple[TokenType::PLUS]	  = { 12, 'L' };
	OperandTuple[TokenType::MINUS] = { 12, 'L' };
	OperandTuple[TokenType::STAR] = { 13, 'L' };
	OperandTuple[TokenType::FORWARD_SLASH] = { 13, 'L' };
	OperandTuple[TokenType::MODULUS]  = { 13, 'L' };
}

bool Parser::Parse()
{
	while(peek().has_value() && peek().value().type != TokenType::_EOF)
	{
		switch (peek().value().type)
		{
		case TokenType::FN:
		{
			auto fn = std::move(ParseFunction());
			if (fn.get() != NULL)
				m_programAST->FnStmts.push_back(std::move(fn));
			else 
				return false;
			break;
		}
		case TokenType::LET:
			if (!ParseGlobalDecl())
				return false;
			break;
		default:
			Logger::fmtLog(Error, "Expected a declaration on line: %ld", peek().value().lineNum);
			return false;
		}
	}
	return true;
}

bool Parser::ParseGlobalDecl()
{
	std::unique_ptr<DeclStmt> stmt = std::make_unique<DeclStmt>();
	consume(/* Consume the LET Token */);

	if (PeekAndCheck(TokenType::IDENT))
		stmt->IDENT = consume().value;
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected identifier on line: %ld", peek(-1).value().lineNum), false)

	if (!PeekAndCheck(TokenType::COLON))
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected ':' on line: %ld", peek(-1).value().lineNum), false)
	else consume();

	if (PeekAndCheck(TokenType::BuiltinType))
		stmt->type = consume().dataType;
	else 
		RUN_AND_RETURN(Logger::fmtLog("Expected variable type on line: %ld", peek(-1).value().lineNum), false)

	if (PeekAndCheck(TokenType::EQUALS))
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Warning, "Expression declaration not implemented!"), false)
	else consume();

	if (!PeekAndCheck(TokenType::SEMICOLON))
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected ';' at the end of declaration on line: %ld", peek(-1).value().lineNum), false)
	else consume();

	m_programAST->DeclStmts.push_back(std::move(stmt));
	return true;
}

std::unique_ptr<FnStmt> Parser::ParseFunction()
{
	std::unique_ptr<FnStmt> fnStmt = std::make_unique<FnStmt>();
	consume(/*Consume the fn keyword*/);
	
	if (PeekAndCheck(TokenType::EXTERN))
	{
		consume();
		fnStmt->isExtern = true;
	}

	if (PeekAndCheck(TokenType::IDENT))
		fnStmt->name = consume().value;
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected function name on line: %ld", peek(-1).value().lineNum), NULL);

	if (PeekAndCheck(TokenType::LParan))
		consume();
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error,"Expected '(' on line: %ld", peek(-1).value().lineNum), NULL);
	
	while (peek().has_value() && peek().value().type != TokenType::RParan)
	{
		auto res = ParseParamDecl();
		if (res.get() != NULL)
			fnStmt->params.emplace_back(std::move(res));
		else
			return NULL;
	}
	
	if (PeekAndCheck(TokenType::RParan))
		consume();
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error,"Expected ')' on line: %ld", peek(-1).value().lineNum), NULL);

	if (PeekAndCheck(TokenType::ARROW))
		consume();
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error,"Expected '->' on line: %ld", peek(-1).value().lineNum), NULL);

	if (PeekAndCheck(TokenType::BuiltinType))
		fnStmt->returnType = consume().dataType;
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error,"Expected function return type on line: %ld", peek(-1).value().lineNum), NULL);

	if (PeekAndCheck(TokenType::SEMICOLON))
	{
		consume();
		return fnStmt;
	}

	auto compoundStmt = ParseCompoundStmt();
	if (compoundStmt.get() != NULL)
		fnStmt->compoundStmt = std::move(compoundStmt);
	else 
		return NULL;

	return fnStmt;
}

std::unique_ptr<ParamDecl> Parser::ParseParamDecl()
{
	/*
	*	Grammar:
	*		Param: ident ':' Type <','>? || '...' <','>?
	*/

	auto param = std::make_unique<ParamDecl>();
	
	if (PeekAndCheck(TokenType::ELLIPSIS))
	{
		param->VarArg = true;
		consume();
	}

	else if (PeekAndCheck(TokenType::IDENT))
	{
		param->ident = consume().value;

		if (PeekAndCheck(TokenType::COLON))
			consume();
		else
		{
			Logger::fmtLog(LogLevel::Error, "Missing a ':' after identifier on line: %ld", peek(-1).value().lineNum);
			return nullptr;
		}

		if (PeekAndCheck(TokenType::BuiltinType)) 
		{
			param->type = consume().dataType;
			if (PeekAndCheck(TokenType::STAR))
			{
				consume();

				PrimitiveDataType PtrType = ptrTypeof(param->type);
				if (PtrType == PrimitiveDataType::EMPTY)
				{
					Logger::fmtLog(LogLevel::Error, "Pointer to invalid type on line: %ld", peek(-1).value().lineNum);
					return nullptr;
				}
				param->type = PtrType;
			}
		}
		else
		{
			Logger::fmtLog(LogLevel::Error, "Expected type of parameter on line: %ld", peek(-1).value().lineNum);
			return nullptr;
		}

		if (PeekAndCheck(TokenType::COMMA))
			consume();
	}
	else {
		Logger::fmtLog(LogLevel::Error, "Unknown token found on line: %ld", peek(-1).value().lineNum);
		return nullptr;
	}

	return param;
}

std::unique_ptr<CompoundStmt> Parser::ParseCompoundStmt()
{
	std::unique_ptr<CompoundStmt> CStmt = std::make_unique<CompoundStmt>();

	if (PeekAndCheck(TokenType::LCURLY))
		consume();
	else RUN_AND_RETURN(Logger::fmtLog("Expected '{' on line: %ld", peek(-1).value().lineNum), NULL);

	while (peek().has_value() && peek().value().type != TokenType::RCURLY)
	{
		auto stmt = std::move(ParseStmt());
		if (stmt.get() != NULL)
			CStmt->statementList.push_back(std::move(stmt));
		else 
			return NULL;
	}

	if (PeekAndCheck(TokenType::RCURLY))
		consume();
	else RUN_AND_RETURN(Logger::fmtLog("Expected '}' on line: %ld", peek(-1).value().lineNum), NULL);

	return CStmt;
}

std::unique_ptr<Stmt> Parser::ParseStmt()
{
	std::unique_ptr<Stmt> Statement = std::make_unique<Stmt>();
	/*TODO: PLUS other stmt parsing*/

	if (peek().has_value())
	{
		switch (peek().value().type)
		{
			case TokenType::RETURN:
			{
				auto retStmt = ParseReturnStmt();
				if (retStmt.get() != NULL)
					Statement->stmt = std::move(retStmt);
				else
					return nullptr;
			}
			break;
			case TokenType::LET:
			{
				auto declStmt = ParseDeclStmt();
				if (declStmt.get() != NULL)
					Statement->stmt = std::move(declStmt);
				else
					return nullptr;
			}
			break;
			case TokenType::IDENT:
			{
				if(peek(1).has_value())
				{
					if (peek(1).value().type == TokenType::LParan)
					{
						// Function call
						auto fnCall = ParseFunctionCallStmt(/*IDENT & '(' is not consumed*/);
						if (fnCall.get() != nullptr)
							Statement->stmt = std::move(fnCall);
						else
							return nullptr;
					}
					else if (peek(1).value().type == TokenType::EQUALS)
					{
						Logger::fmtLog("Not implemented (\nFile:%s, line: %ld)!", __FILE__, __LINE__);
						return nullptr;
					}
				}
			}
			break;
			default:
				Logger::fmtLog(LogLevel::Error, "Unexpected statement on line: %ld", peek(-1).value().lineNum);
				return nullptr;
		}
	}

	return Statement;
}

std::unique_ptr<DeclStmt> Parser::ParseDeclStmt()
{
	auto declStmt = std::make_unique<DeclStmt>();
	consume(/*LET Token*/);
	
	if (PeekAndCheck(TokenType::IDENT))
		declStmt->IDENT = consume().value;
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected an identifier after let on line: %ld", peek(-1).value().lineNum), NULL);
	
	if (PeekAndCheck(TokenType::COLON))
		consume();
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected an ':' after identifier on line: %ld", peek(-1).value().lineNum), NULL);
	
	if (PeekAndCheck(TokenType::BuiltinType))
		declStmt->type = consume().dataType;
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected an type on line: %ld", peek(-1).value().lineNum), NULL);

	if (PeekAndCheck(TokenType::EQUALS))
		consume();
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected an '=' on line: %ld", peek(-1).value().lineNum), NULL);
	
	/*EXPRESSION PARSING*/
	auto ExprTree = ParseExpr();
	if (ExprTree.get() == NULL)
		return NULL;

	if (PeekAndCheck(TokenType::SEMICOLON))
		consume();
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Missing ';' at the end of line: %ld", peek(-1).value().lineNum), NULL);
	
	declStmt->expr = std::move(ExprTree);
	return declStmt;
}

std::unique_ptr<ReturnStmt> Parser::ParseReturnStmt()
{
	auto retStmt = std::make_unique<ReturnStmt>();
	consume(/*Return Token*/);
	
	auto ExprTree = ParseExpr();
	if (ExprTree.get() == NULL)
		return NULL;

	if (PeekAndCheck(TokenType::SEMICOLON))
		consume();
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Missing ';' at the end of line: %ld", peek(-1).value().lineNum), NULL);

	retStmt->value = std::move(ExprTree);
	return retStmt;
}

std::unique_ptr<Expr> Parser::ParseFunctionCallExpr()
{
	auto expr = std::make_unique<Expr>();
	auto fnCall = std::make_unique<FnCall>();
	
	fnCall->name = consume(/* TOKEN: IDENT */).value;
	consume(/* TOKEN: LParan */);

	if (peek().has_value() && peek().value().type == TokenType::RParan)
	{
		expr->value = std::move(fnCall);
		return expr;
	}
	else
	{
		fnCall->args = ParseArgsList();
		if (fnCall->args.get() == nullptr)
			return nullptr;
	}

	expr->value = std::move(fnCall);
	return expr;
}

std::unique_ptr<FnCall> Parser::ParseFunctionCallStmt(/* IDENT & LParan is not consumed */)
{
	auto fnCall = std::make_unique<FnCall>();
	fnCall->name = consume(/* TOKEN: IDENT */).value;
	consume(/* TOKEN: LParan */);
	
	if (peek().has_value() && peek().value().type == TokenType::RParan)
		return fnCall;
	else
	{
		fnCall->args = ParseArgsList();
		if (fnCall->args.get() == nullptr)
			return nullptr;
	}
	if (PeekAndCheck(TokenType::SEMICOLON))
		consume();
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected a ';' at line: %ld", peek(-1).value().lineNum), nullptr);
	
	return fnCall;
}

std::unique_ptr<ArgsList> Parser::ParseArgsList()
{
	auto argsList = std::make_unique<ArgsList>();

	// TODO: Break the argslist into exprs
	
	while (peek().has_value() && peek().value().type != TokenType::RParan)
	{
		auto arg = ParseExpr();
		if (arg.get() != nullptr)
			argsList->list.emplace_back(std::move(arg));
		else
			return nullptr;

		if (peek().value().type == TokenType::COMMA)
			consume();
	}

	consume(/* Consume the RParan ')' */);
	return argsList;
}

std::unique_ptr<Expr> Parser::ParseExpr()
{
	if (m_nodeStack.size() != 0)
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Node stack contains previously added nodes"), nullptr);
	if (m_operatorStack.size() != 0)
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Operator stack is not empty"), nullptr);

	auto mainExpr = std::make_unique<Expr>();

	while (peek().has_value() && 
		peek().value().type != TokenType::SEMICOLON && 
		peek().value().type != TokenType::COMMA)
	{
		switch (peek().value().type)
		{
			case TokenType::INT_LITERAL:
				m_nodeStack.push(std::move(CreateLiteralExpr(consume().value, PrimitiveDataType::i32)));
				break;
			case TokenType::FLOAT_LITERAL:
				m_nodeStack.push(std::move(CreateLiteralExpr(consume().value, PrimitiveDataType::f64)));
				break;
			case TokenType::STRING_LITERAL:
				m_nodeStack.push(std::move(CreateLiteralExpr(consume().value, PrimitiveDataType::str)));
				break;
			case TokenType::IDENT:
				if (peek(1).value().type == TokenType::LParan)
					m_nodeStack.push(std::move(ParseFunctionCallExpr()));
				else
				{
					// This is a case where the ident is a variable
					m_nodeStack.push(std::move(CreateVarExpr(consume().value)));
				}
				break;
			case TokenType::LParan:
				m_operatorStack.push(consume());
				break;
			
			case TokenType::RParan:
			{
				consume(/* Consume ')' */);
				// If the scanned character is an ‘)’, pop form a new binOP with the prev 2 nodes on nodeStack
				// until an ‘(‘ is encountered.
				while (!m_operatorStack.empty() && m_operatorStack.top().type != TokenType::LParan)
					if (!ApplyOperator(/*Directly accesses Node and Operator Stack*/))
						return nullptr;
				if (!m_operatorStack.empty() && m_operatorStack.top().type == TokenType::LParan)
					m_operatorStack.pop();
				else
					RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected a '(' for ')' found on line: %ld!", peek(-1).value().lineNum), nullptr);
			}
			break;
			
			case TokenType::PLUS:
			case TokenType::MINUS:
			case TokenType::FORWARD_SLASH:
			case TokenType::STAR:
			case TokenType::MODULUS:
			{
				Token curr = consume();
				while (!m_operatorStack.empty() && (OperandTuple[curr.type].first < OperandTuple[m_operatorStack.top().type].first || 
					OperandTuple[curr.type].first == OperandTuple[m_operatorStack.top().type].first && 
					OperandTuple[curr.type].second == 'L'))
				{
					if (!ApplyOperator())
						return nullptr;
				}

				// Push the current operator onto stack
				m_operatorStack.push(curr);
			}
			break;

			default:
				RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Unexpected token found on line: %lu", peek().value().lineNum), NULL);
		}
	}
	
	// Pop all the remaining elements from the stack
	while (!m_operatorStack.empty()) {
		if (!ApplyOperator())
			return nullptr;
	}

	if (m_nodeStack.size() != 1)
	{
		Logger::fmtLog(LogLevel::Error, "Mismatched operands and operators in expression!");
		return nullptr;
	}

	mainExpr = std::move(m_nodeStack.top());
	m_nodeStack.pop();

	return mainExpr;
}

std::unique_ptr<Expr> Parser::CreateVarExpr(const std::string& name)
{
	auto IdentExpr = std::make_unique<Expr>();

	auto ident = std::make_unique<Ident>();
	ident->name = name;

	IdentExpr->value = std::move(ident);

	return IdentExpr;
}

std::unique_ptr<Expr> Parser::CreateLiteralExpr(const std::string& value, PrimitiveDataType dataType)
{
	auto LiteralExpr = std::make_unique<Expr>();

	auto _Literal = std::make_unique<Literal>();
	_Literal->type = dataType;
	_Literal->value = value;

	LiteralExpr->value = std::move(_Literal);
	
	return LiteralExpr;
}

std::unique_ptr<Program> Parser::getProgram()
{
	return std::move(m_programAST);
}


std::optional<Token> Parser::peek(int ahead)
{
	size_t lookAt = m_index + ahead;
	if (lookAt < m_tokens.size())
		return m_tokens[lookAt];
	return {};
}

Token Parser::consume()
{
	return m_tokens[m_index++];
}

std::optional<Token> Parser::PeekAndCheck(TokenType type, int ahead)
{
	size_t lookAt = m_index + ahead;
	if (lookAt < m_tokens.size() && m_tokens[lookAt].type == type)
		return m_tokens[lookAt];
	return {};
}

bool Parser::ApplyOperator()
{
	if (m_nodeStack.size() < 2)
	{
		Logger::fmtLog(LogLevel::Error, "Insufficient operands for operator on line: %ld", m_operatorStack.top().lineNum);
		return false;
	}
	auto RHS = std::move(m_nodeStack.top());
	m_nodeStack.pop();
	auto LHS = std::move(m_nodeStack.top());
	m_nodeStack.pop();

	m_nodeStack.push(GenerateBinaryOpNode(m_operatorStack.top(), LHS, RHS));
	m_operatorStack.pop();
	return true;
}

std::unique_ptr<Expr> Parser::GenerateBinaryOpNode(Token& op, std::unique_ptr<Expr>& LHS, std::unique_ptr<Expr>& RHS)
{
	auto binOpNode = std::make_unique<BinaryOp>();

	binOpNode->type = op.type;
	binOpNode->RHS = std::move(RHS);
	binOpNode->LHS = std::move(LHS);
	
	auto opExpr = std::make_unique<Expr>();
	opExpr->value = std::move(binOpNode);
	
	return opExpr;
}

PrimitiveDataType Parser::ptrTypeof(PrimitiveDataType type)
{
	switch (type)
	{
	case PrimitiveDataType::i8:
	case PrimitiveDataType::i16:
	case PrimitiveDataType::i32:
	case PrimitiveDataType::i64:
	case PrimitiveDataType::u8:
	case PrimitiveDataType::u16:
	case PrimitiveDataType::u32:
	case PrimitiveDataType::u64:
		return (PrimitiveDataType)((int)type + 4);
		break;
	case PrimitiveDataType::f32:
		return PrimitiveDataType::f32ptr;
		break;
	case PrimitiveDataType::f64:
		return PrimitiveDataType::f64ptr;
		break;
	case PrimitiveDataType::str:
		return PrimitiveDataType::i8ptr;
		break;
	default:
		return PrimitiveDataType::EMPTY;
	}
}



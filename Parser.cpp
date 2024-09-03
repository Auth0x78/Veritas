#include "Parser.h"

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
				if(peek().has_value())
				{
					if (peek().value().type == TokenType::LParan)
					{
						// Function call
						auto fnCall = ParseFunctionCall(/*IDENT & '(' is not consumed*/);
						if (fnCall.get() != nullptr)
							Statement->stmt = std::move(fnCall);
						else
							nullptr;
					}
					else if (peek().value().type == TokenType::EQUALS)
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

	retStmt->value = std::move(ExprTree);
	return retStmt;
}

std::unique_ptr<FnCall> Parser::ParseFunctionCall(/* IDENT & LParan is not consumed */)
{
	auto fnCall = std::make_unique<FnCall>();
	fnCall->name = consume(/* TOKEN: IDENT */).value;
	consume(/* TOKEN: LParan */);

	while (peek().has_value() && peek().value().type != TokenType::RParan)
	{

	}
	return nullptr;
}

std::unique_ptr<Expr> Parser::ParseExpr()
{
	auto expr = std::make_unique<Expr>();

	std::stack<std::unique_ptr<Expr>> nodeStack;
	std::stack<Token> opStack;

	while (peek().has_value() && peek().value().type != TokenType::SEMICOLON)
	{
		switch (peek().value().type)
		{
			case TokenType::INT_LITERAL:
			{
				auto LExpr = std::make_unique<Expr>();

				auto intLiteral = std::make_unique<Literal>();
				intLiteral->type = PrimitiveDataType::i32;
				intLiteral->value = consume().value;
				
				LExpr->value = std::move(intLiteral);
				nodeStack.push(std::move(LExpr));
			}
			break;
			
			case TokenType::FLOAT_LITERAL:
			{
				auto LExpr = std::make_unique<Expr>();

				auto intLiteral = std::make_unique<Literal>();
				intLiteral->type = PrimitiveDataType::f64;
				intLiteral->value = consume().value;

				LExpr->value = std::move(intLiteral);
				nodeStack.push(std::move(LExpr));
			}
			break;
			
			case TokenType::IDENT:
			{
				auto IExpr = std::make_unique<Expr>();

				auto ident = std::make_unique<Ident>();
				ident->name = consume().value;

				IExpr->value = std::move(ident);
				nodeStack.push(std::move(IExpr));
			}
			break;

			case TokenType::LParan:
				opStack.push(consume());
			break;
			
			case TokenType::RParan:
			{
				Token token = consume();
				// If the scanned character is an ‘)’, pop form a new binOP with the prev 2 nodes on nodeStack
				// until an ‘(‘ is encountered.
				while (!opStack.empty() && opStack.top().type != TokenType::LParan)
				{
					std::unique_ptr<Expr> LHS;
					std::unique_ptr<Expr> RHS;

					if (nodeStack.empty())
						RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected an expression before operator on line: %ld", opStack.top().lineNum), NULL);
					RHS = std::move(nodeStack.top());
					nodeStack.pop();

					if (nodeStack.empty())
						RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected an expression after operator on line: %ld", opStack.top().lineNum), NULL);
					LHS = std::move(nodeStack.top());
					nodeStack.pop();

					nodeStack.push(std::move(GenerateBinaryOpNode(opStack.top(), LHS, RHS)));
					opStack.pop();
				}
				if (!opStack.empty() && opStack.top().type == TokenType::LParan)
					opStack.pop();
				else
					RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected a '(' for ')' found on line: %ld!", token.lineNum),NULL);
			}
			break;
			
			case TokenType::PLUS:
			case TokenType::MINUS:
			case TokenType::FORWARD_SLASH:
			case TokenType::STAR:
			case TokenType::MODULUS:
			{
				auto binOpNode = std::make_unique<BinaryOp>();
				Token curr = consume();
				while (!opStack.empty() && (OperandTuple[curr.type].first < OperandTuple[opStack.top().type].first || 
					OperandTuple[curr.type].first == OperandTuple[opStack.top().type].first && 
					OperandTuple[curr.type].second == 'L'))
				{
					Token op = opStack.top();
					opStack.pop();
					binOpNode->type = op.type;
					
					if (nodeStack.empty())
						RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected an expression before operator on line: %ld", op.lineNum), NULL);
					binOpNode->RHS = std::move(nodeStack.top());
					nodeStack.pop();

					if (nodeStack.empty())
						RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected an expression after operator on line: %ld", op.lineNum), NULL);
					binOpNode->LHS = std::move(nodeStack.top());
					nodeStack.pop();

					auto opExpr = std::make_unique<Expr>();
					opExpr->value = std::move(binOpNode);
					nodeStack.push(std::move(opExpr));
				}
				opStack.push(curr);
			}
			break;
			
			default:
				RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Unexpected token found on line: %lu", peek().value().lineNum), NULL);
		}
	}
	
	// Pop all the remaining elements from the stack
	while (!opStack.empty())
	{
		std::unique_ptr<Expr> LHS;
		std::unique_ptr<Expr> RHS;

		if (nodeStack.empty())
			RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected an expression before operator on line: %ld", opStack.top().lineNum), NULL);
		RHS = std::move(nodeStack.top());
		nodeStack.pop();
		
		if (nodeStack.empty())
			RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected an expression after operator on line: %ld", opStack.top().lineNum), NULL);
		LHS = std::move(nodeStack.top());
		nodeStack.pop();

		nodeStack.push(std::move(GenerateBinaryOpNode(opStack.top(), LHS, RHS)));
		opStack.pop();
	}

	if (PeekAndCheck(TokenType::SEMICOLON))
		consume();
	else
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Missing ';' at the end of line: %ld", peek(-1).value().lineNum), NULL);
	
	if (nodeStack.size() > 1)
		RUN_AND_RETURN(Logger::fmtLog(LogLevel::Error, "Expected operator before operand!"), NULL);

	expr = std::move(nodeStack.top());
	return expr;
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
	default:
		return PrimitiveDataType::EMPTY;
	}
}



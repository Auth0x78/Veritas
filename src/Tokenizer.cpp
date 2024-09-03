#include "headers/Tokenizer.h"

Tokenizer::Tokenizer(const std::string& program)
	: m_index(0), m_currentLine(1), m_program(program) 
{
	// Data Types
	m_builtinTypeMap["void"] = PrimitiveDataType::VOID;

	m_builtinTypeMap["i8"]	 = PrimitiveDataType::i8;
	m_builtinTypeMap["i16"]	 = PrimitiveDataType::i16;
	m_builtinTypeMap["i32"]	 = PrimitiveDataType::i32;
	m_builtinTypeMap["i64"]	 = PrimitiveDataType::i64;
	m_builtinTypeMap["i128"] = PrimitiveDataType::i128;
	
	m_builtinTypeMap["u8"]   = PrimitiveDataType::u8;
	m_builtinTypeMap["u16"]  = PrimitiveDataType::u16;
	m_builtinTypeMap["u32"]  = PrimitiveDataType::u32;
	m_builtinTypeMap["u64"]  = PrimitiveDataType::u64;
	m_builtinTypeMap["u128"] = PrimitiveDataType::u128;
	
	m_builtinTypeMap["f32"]	 = PrimitiveDataType::f32;
	m_builtinTypeMap["f64"]	 = PrimitiveDataType::f64;

	// Keywords
	m_keywordMap["return"]	= TokenType::RETURN;
	m_keywordMap["let"]		= TokenType::LET;
	m_keywordMap["fn"]		= TokenType::FN;
	m_keywordMap["extern"]  = TokenType::EXTERN;

	m_keywordMap["void"]= TokenType::BuiltinType;

	m_keywordMap["i8"]  = TokenType::BuiltinType;
	m_keywordMap["i16"] = TokenType::BuiltinType;
	m_keywordMap["i32"] = TokenType::BuiltinType;
	m_keywordMap["i64"] = TokenType::BuiltinType;
	m_keywordMap["i128"]= TokenType::BuiltinType;

	m_keywordMap["u8"]  = TokenType::BuiltinType;
	m_keywordMap["u16"] = TokenType::BuiltinType;
	m_keywordMap["u32"] = TokenType::BuiltinType;
	m_keywordMap["u64"] = TokenType::BuiltinType;
	m_keywordMap["u128"]= TokenType::BuiltinType;

	m_keywordMap["f32"] = TokenType::BuiltinType;
	m_keywordMap["f64"] = TokenType::BuiltinType;

	// Symbols
	m_symbolMap["("]	= TokenType::LParan;
	m_symbolMap[")"]	= TokenType::RParan;
	m_symbolMap["{"]	= TokenType::LCURLY;
	m_symbolMap["}"]	= TokenType::RCURLY;
	m_symbolMap[":"]	= TokenType::COLON;
	m_symbolMap[";"]	= TokenType::SEMICOLON;
	m_symbolMap["="]	= TokenType::EQUALS;
	m_symbolMap["=="]	= TokenType::EQUALITY;
	m_symbolMap["+"]	= TokenType::PLUS;
	m_symbolMap["-"]	= TokenType::MINUS;
	m_symbolMap["->"]	= TokenType::ARROW;
	m_symbolMap["*"]	= TokenType::STAR;
	m_symbolMap["/"]	= TokenType::FORWARD_SLASH;
	m_symbolMap["%"]	= TokenType::MODULUS;
	m_symbolMap["..."]	= TokenType::ELLIPSIS;
	m_symbolMap[","]	= TokenType::COMMA;
}

bool Tokenizer::Tokenize()
{
	while (peek().has_value())
	{
		if (std::isalpha(peek().value()))
		{
			// readWord will automatically consume the characters
			std::string word(std::move(readWord()));

			std::optional<TokenType> res = keywordExist(word);
			if (res.has_value()) {
				if(isDataType(word))
					m_tokens.emplace_back(res.value(), word, m_currentLine, m_builtinTypeMap[word]);
				else 
					m_tokens.emplace_back(res.value(), word, m_currentLine);
			}
			else /* Its an IDENTifier */
				m_tokens.emplace_back(TokenType::IDENT, word, m_currentLine);
		}
		else if (std::isdigit(peek().value()))
		{
			std::string buffer;
			buffer.reserve(32);

			while (peek().has_value() && std::isdigit(peek().value()))
				buffer.push_back(consume());

			if (peek().has_value() && peek().value() == '.')
				buffer.push_back(consume());
			else
			{
				m_tokens.emplace_back(TokenType::INT_LITERAL, buffer, m_currentLine);
				continue;
			}
	
			// Atleast 1 number should be there after .
			if (peek().has_value() && std::isdigit(peek().value()))
				buffer.push_back(consume());
			else
			{
				Logger::fmtLog(LogLevel::Error, "Atleast 1 digit should be present after '.'");
				return false;
			}
			while (peek().has_value() && std::isdigit(peek().value()))
				buffer.push_back(consume());
			m_tokens.emplace_back(TokenType::FLOAT_LITERAL, buffer, m_currentLine);
		}
		else if (peek().value() == '/' && (peek(1).has_value() && peek(1).value() == '/'))
		{
			consume();
			if (peek().has_value() && peek().value() == '/')
			{
				while (peek().has_value() && peek().value() != '\n')
					consume();
				//To consume the newline character;
				m_currentLine += 1;
				consume();
			}
		}
		else if (peek().value() == '/' && (peek(1).has_value() && peek(1).value() == '*'))
		{
			bool toBreak = false;
			while (!toBreak && peek().has_value())
			{
				if (peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/')
				{
					toBreak = true;
					//Cosume *
					consume();
					//Consume /
					consume();
					break;
				}
				if (consume() == '\n')
					m_currentLine += 1;
			}
		}
		else if (isSymbol(peek().value()))
		{
			std::string buf;
			buf.push_back(consume());
			
			if (peek().has_value())
			{
				if (buf == "-" && peek().value() == '>')
					buf.push_back(consume());
				else if (buf == "=" && peek().value() == '=')
					buf.push_back(consume());
				else if (buf == "." && peek().value() == '.' && (peek(1).has_value() && peek(1).value() == '.'))
				{
					buf.push_back(consume(/*Consume the '.'*/));
					buf.push_back(consume(/*Consume the '.'*/));
				}
			}


			auto it = m_symbolMap.find(buf);
			if (it != m_symbolMap.end())
				m_tokens.emplace_back(it->second, buf, m_currentLine);
			else
			{
				Logger::fmtLog(LogLevel::Error, "Invalid symbol found '%s' on line: %lu", buf, m_currentLine);
				return false;
			}
		}
		else if (std::isspace(peek().value()))
		{
			if (consume() == '\n')
				m_currentLine += 1;
		}
		else
		{
			Logger::fmtLog(LogLevel::Error, "Invalid character found '%c' on line: %lu", peek().value(), m_currentLine);
			return false;
		}
	}

	return true;
}

std::vector<Token>& Tokenizer::getTokens()
{
	m_tokens.push_back(Token(TokenType::_EOF, "$", INT_MAX));
	return m_tokens;
}

std::string Tokenizer::readWord()
{
	std::string buffer;
	buffer.reserve(16);

	while (peek().has_value() && std::isalnum(peek().value()))
		buffer.push_back(consume());

	return buffer;
}

std::optional<char> Tokenizer::peek(size_t ahead)
{
	size_t lookAt = m_index + ahead;
	if (lookAt < m_program.size())
		return m_program[lookAt];
	return {};
}

char Tokenizer::consume()
{
	return m_program[m_index++];
}


bool Tokenizer::isDataType(const std::string& key)
{
	return m_keywordMap[key] == TokenType::BuiltinType;
}

std::optional<TokenType> Tokenizer::keywordExist(const std::string& key)
{
	auto it = m_keywordMap.find(key);

	if (it != m_keywordMap.end())
		return it->second;
	
	return {};
}

bool Tokenizer::isSymbol(char c)
{
	switch (c)
	{
		case '(':
		case ')':
		case '{':
		case '}':
		case ';':
		case '-':
		case '+':
		case '/':
		case '*':
		case '%':
		case '>':
		case ':':
		case '=':
		case '.':
		case ',':
			return true;
		default:
			return false;
	}
}



#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "token.h"
#include "Logger.h"

class Tokenizer
{
public:
	Tokenizer(const std::string& program);
	bool Tokenize();
	std::vector<Token>& getTokens();

private:
	std::string readWord();
	std::optional<char> peek(size_t ahead = 0);
	char consume();

	bool isDataType(const std::string& key);
	std::optional<TokenType> keywordExist(const std::string& key);
	bool isSymbol(char c);

	size_t m_index;
	size_t m_currentLine;
	
	std::string m_program;
	std::vector<Token> m_tokens;
	std::unordered_map<std::string, TokenType> m_keywordMap;
	std::unordered_map<std::string, PrimitiveDataType> m_builtinTypeMap;
	std::unordered_map<std::string, TokenType> m_symbolMap;
};


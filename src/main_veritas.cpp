#include <iostream>
#include <fstream>
#include <ostream>
#include <sstream>
#include <filesystem>

#include "headers/llvm_includes.h"
#include "headers/Tokenizer.h"
#include "headers/Generate.h"
#include "headers/Logger.h"

// #define TEST_LLVM 0

int main(int argc, char* argv[])
{
#ifndef TEST_LLVM
	Logger::SetLogLevel(LogLevel::Info);

	std::string path;
#ifndef _DEBUG
	// IF *not* in debug mode
	if (argc < 2)
	{
		Logger::Log(LogLevel::Error, "No input file given");
		return -1;
	}
#else // IF in debug mode
	Logger::Log("Enter a file path: ");
	std::cin >> path;
#endif // !_DEBUG

	std::string program;
	{
		std::ifstream in(path);
		if (!in.is_open())
		{
			Logger::fmtLog(LogLevel::Error, "Failed to open file: %s", path);
			return -1;
		}

		std::ostringstream sstr;
		sstr << in.rdbuf();
		program = sstr.str();
	}

	Tokenizer tokenizer(program);
	if (!tokenizer.Tokenize())
		return -1;
	std::vector<Token> tokens(std::move(tokenizer.getTokens()));

	Parser parser(tokens);
	if (!parser.Parse())
		return -1;
	
	std::string outFile = "./tempVeritas/out.ll";
	Generator llvmGEN(parser.getProgram(), path, outFile);
	
	llvmGEN.Generate();
#else
	std::string str = "out.ll";
	Generator gen(str);

	gen.Generate();
#endif // !TEST_LLVM



	return 0;
}
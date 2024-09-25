#include "headers/Generate.h"

Generator::Generator(std::unique_ptr<Program> program, const std::string& moduleName, const std::string& outPath)
	: m_program(std::move(program)), m_outPath(outPath), m_moduleName(moduleName)
{
	// Always initialize context module and builder first
	moduleInit();

	m_TypeMap[PrimitiveDataType::VOID] = llvm::Type::getVoidTy(*ctx);

	m_TypeMap[PrimitiveDataType::i8] = llvm::Type::getInt8Ty(*ctx);
	m_TypeMap[PrimitiveDataType::i16] = llvm::Type::getInt16Ty(*ctx);
	m_TypeMap[PrimitiveDataType::i32] = llvm::Type::getInt32Ty(*ctx);
	m_TypeMap[PrimitiveDataType::i64] = llvm::Type::getInt64Ty(*ctx);
	m_TypeMap[PrimitiveDataType::i128] = llvm::Type::getInt128Ty(*ctx);

	m_TypeMap[PrimitiveDataType::u8] = llvm::Type::getInt8Ty(*ctx);
	m_TypeMap[PrimitiveDataType::u16] = llvm::Type::getInt16Ty(*ctx);
	m_TypeMap[PrimitiveDataType::u32] = llvm::Type::getInt32Ty(*ctx);
	m_TypeMap[PrimitiveDataType::u64] = llvm::Type::getInt64Ty(*ctx);
	m_TypeMap[PrimitiveDataType::u128] = llvm::Type::getInt128Ty(*ctx);

	m_TypeMap[PrimitiveDataType::f32] = llvm::Type::getFloatTy(*ctx);
	m_TypeMap[PrimitiveDataType::f64] = llvm::Type::getDoubleTy(*ctx);

	m_TypeMap[PrimitiveDataType::i8ptr] = llvm::Type::getInt8PtrTy(*ctx);
	m_TypeMap[PrimitiveDataType::i16ptr] = llvm::Type::getInt16PtrTy(*ctx);
	m_TypeMap[PrimitiveDataType::i32ptr] = llvm::Type::getInt32PtrTy(*ctx);
	m_TypeMap[PrimitiveDataType::i64ptr] = llvm::Type::getInt64PtrTy(*ctx);

	m_TypeMap[PrimitiveDataType::u8ptr] = llvm::Type::getInt8PtrTy(*ctx);
	m_TypeMap[PrimitiveDataType::u16ptr] = llvm::Type::getInt16PtrTy(*ctx);
	m_TypeMap[PrimitiveDataType::u32ptr] = llvm::Type::getInt32PtrTy(*ctx);
	m_TypeMap[PrimitiveDataType::u64ptr] = llvm::Type::getInt64PtrTy(*ctx);

	m_TypeMap[PrimitiveDataType::f32ptr] = llvm::Type::getFloatPtrTy(*ctx);
	m_TypeMap[PrimitiveDataType::f64ptr] = llvm::Type::getDoublePtrTy(*ctx);

}

void Generator::Generate()
{
	for (auto& decl : m_program->DeclStmts)
		if (CreateGlobalDecl(decl) == nullptr)
			return;

	for (auto& fn : m_program->FnStmts)
		if (CreateFunction(fn) == nullptr)
			return;

	cModule->print(llvm::outs(), nullptr);

	// 3. Save m_module IR to file
	saveModuleToFile();
}

llvm::Value* Generator::CreateGlobalDecl(const std::unique_ptr<DeclStmt>& declStmt)
{
	llvm::Value* vAddr = nullptr;
	llvm::Type* vType = nullptr;
	llvm::Constant* initializer = nullptr;
	llvm::Value* initialValue = GenerateExpr(declStmt->expr);

	vType = findTypeFromPrimitive(declStmt->type);
	if (vType == nullptr)
	{
		Logger::fmtLog(LogLevel::Error, "Unknown type found!");
		return nullptr;
	}

	initializer = llvm::ConstantInt::get(vType, llvm::dyn_cast<llvm::ConstantInt>(initialValue)->getSExtValue());

	vAddr = new llvm::GlobalVariable(*cModule,
		vType,
		false,
		llvm::GlobalValue::ExternalLinkage,
		initializer,
		declStmt->IDENT
	);

	return vAddr;
}

llvm::Function* Generator::CreateFunction(const std::unique_ptr<FnStmt>& fnStmt)
{
	auto fn = cModule->getFunction(fnStmt->name);

	llvm::FunctionType* fnType = nullptr;

	bool isVarArgs = false;

	std::vector<llvm::Type*> paramsList;
	paramsList.reserve(fnStmt->params.size());

	for (auto& param : fnStmt->params)
	{
		if (param->VarArg)
			isVarArgs = true;
		else
			paramsList.push_back(m_TypeMap[param->type]);
	}

	llvm::Type* retType = findTypeFromPrimitive(fnStmt->returnType);

	if (retType == nullptr) {
		Logger::fmtLog(LogLevel::Error, "Unkown function return type found!");
		return nullptr;
	}

	fnType = llvm::FunctionType::get(retType, paramsList, true);

	if (fn == nullptr)
	{
		// No function exists/declared
		if (fnStmt->isExtern || fnStmt->name == "main")
			fn = llvm::Function::Create(fnType, llvm::GlobalValue::ExternalLinkage, fnStmt->name, *cModule);
		else
			fn = llvm::Function::Create(fnType, llvm::GlobalValue::InternalLinkage, fnStmt->name, *cModule);
		llvm::verifyFunction(*fn);
	}

	// If there is no compound statement then just return the current
	if (fnStmt->compoundStmt.get() == NULL)
		return fn;
	
	// Create function block
	auto entry = llvm::BasicBlock::Create(*ctx, "entry", fn);
	builder->SetInsertPoint(entry);
	m_FunctionType = fn->getFunctionType();

	// Generate Compound Statment
	GenerateCompoundStatement(fnStmt->compoundStmt);

	m_FunctionType = nullptr;
	return fn;
}

llvm::CallInst* Generator::CreateFunctionCall(const std::unique_ptr<FnCall>& FunctionCall)
{
	std::vector<llvm::Value*> ArgsV;

	for (auto& argExpr : FunctionCall->args->list)
		ArgsV.push_back(GenerateExpr(argExpr));

	int count = 0;
	if (FunctionCall->name == "printf")
	{
		// Convert all Argv of type f32 to f64, reason: Only God knows why, but only that way "%f" works
		for (auto& argv : ArgsV)
			if (argv->getType()->isFloatTy())
				argv = builder->CreateFPExt(argv, llvm::Type::getDoubleTy(*ctx), "ftod" + std::to_string(count));
	}

	llvm::Function* calledFn = cModule->getFunction(FunctionCall->name);
	llvm::CallInst* Call = builder->CreateCall(calledFn, ArgsV, FunctionCall->name + "calltmp");
	return Call;
}

void Generator::GenerateCompoundStatement(const std::unique_ptr<CompoundStmt>& cmpndStmt)
{
	for (auto& s : cmpndStmt->statementList)
		GenerateStatment(s);
}

void Generator::GenerateStatment(const std::unique_ptr<Stmt>& stmt)
{
	struct stmtVisitor
	{
		void operator()(const std::unique_ptr<DeclStmt>& declStmt)
		{
			if (gen.m_symbolMap.count(declStmt->IDENT) == 1)
			{
				Logger::fmtLog(LogLevel::Error, "Identifier '%s' has been declared twice", declStmt->IDENT);
				return;
			}

			llvm::Value* initialValue = nullptr;
			if (declStmt->expr.get() != NULL)
				initialValue = gen.GenerateExpr(declStmt->expr);

			llvm::Type* _type = nullptr;
			_type = gen.findTypeFromPrimitive(declStmt->type);

			if (_type == nullptr)
			{
				Logger::fmtLog(LogLevel::Error, "Invalid type found of variable!");
				return;
			}

			// Create the variable on stack
			auto vAddr = gen.builder->CreateAlloca(_type, nullptr, declStmt->IDENT);
			gen.m_symbolMap[declStmt->IDENT] = { vAddr, _type };

			// Initialize the variable with the initial value
			initialValue = gen.autoTypeCast(initialValue, _type);
			gen.builder->CreateStore(initialValue, vAddr);
		}
		void operator()(const std::unique_ptr<ReturnStmt>& retStmt)
		{
			gen.builder->CreateRet(gen.autoTypeCast(gen.GenerateExpr(retStmt->value), gen.m_FunctionType->getReturnType()));
		}
		void operator()(const std::unique_ptr<CompoundStmt>& compoundStmt)
		{
			/*TODO*/
		}
		void operator()(const std::unique_ptr<FnCall>& fnCall)
		{
			gen.CreateFunctionCall(fnCall);
		}
		Generator& gen;
	};
	stmtVisitor visitor = { *this };
	std::visit(visitor, stmt->stmt);
}

llvm::Value* Generator::GenerateExpr(const std::unique_ptr<Expr>& expr)
{
	struct exprVisitor
	{
		llvm::Value* operator()(const std::unique_ptr<BinaryOp>& binop)
		{
			llvm::Value* LHS = std::visit(*this, binop->LHS->value);
			llvm::Value* RHS = std::visit(*this, binop->RHS->value);

			if (gen.getTypePriority(LHS->getType()) > gen.getTypePriority(RHS->getType()))
				RHS = gen.autoTypeCast(RHS, LHS->getType());
			else
				LHS = gen.autoTypeCast(LHS, RHS->getType());

			// For float types, the operation performed should be float op
			if (LHS->getType()->isFloatingPointTy() || RHS->getType()->isFloatingPointTy())
			{
				switch (binop->type) {
				case TokenType::PLUS:
					return gen.builder->CreateFAdd(LHS, RHS, "PLUStmp");
				case TokenType::MINUS:
					return gen.builder->CreateFSub(LHS, RHS, "subtmp");
				case TokenType::STAR:
					return gen.builder->CreateFMul(LHS, RHS, "multmp");
				case TokenType::FORWARD_SLASH:
					return gen.builder->CreateFDiv(LHS, RHS, "divtmp");
				default:
					Logger::fmtLog("Invalid binary operator found!");
					return nullptr;
				}
			}

			// Else perform Integer operations
			switch (binop->type) {
			case TokenType::PLUS:
				return gen.builder->CreateAdd(LHS, RHS, "PLUStmp");
			case TokenType::MINUS:
				return gen.builder->CreateSub(LHS, RHS, "subtmp");
			case TokenType::STAR:
				return gen.builder->CreateMul(LHS, RHS, "multmp");
			case TokenType::FORWARD_SLASH:
				return gen.builder->CreateSDiv(LHS, RHS, "divtmp");
			default:
				Logger::fmtLog("Invalid binary operator found!");
				return nullptr;
			}
		}
		llvm::Value* operator()(const std::unique_ptr<Literal>& lit)
		{
			switch (lit->type)
			{
			case PrimitiveDataType::i32:
				return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), llvm::APInt(32, lit->value, 10));
			case PrimitiveDataType::f64:
				return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*gen.ctx), std::stod(lit->value));
			case PrimitiveDataType::str:
			{
				// Create a string global variable
				llvm::Value* strVal = gen.builder->CreateGlobalStringPtr(lit->value);
				return strVal; // Return pointer to the string
			}
			default:
				Logger::fmtLog(LogLevel::Error, "Unkown type literal found!");
				return nullptr;
				break;
			}
		}
		llvm::Value* operator()(const std::unique_ptr<Ident>& ident)
		{
			if (gen.m_symbolMap.count(ident->name) == 1)
			{
				auto vInfo = gen.m_symbolMap[ident->name];
				return gen.builder->CreateLoad(vInfo.vType, vInfo.vAddr, ident->name + "load");
			}

			return nullptr;
		}
		llvm::Value* operator()(const std::unique_ptr<FnCall>& fnCall)
		{
			return gen.CreateFunctionCall(fnCall);
		}

		Generator& gen;
	};

	exprVisitor visitor = { *this };
	return std::visit(visitor, expr->value);
}

void Generator::saveModuleToFile()
{
	std::error_code errorCode;
	llvm::raw_fd_ostream outLLFile(m_outPath, errorCode);

	cModule->print(outLLFile, nullptr);

}
void Generator::moduleInit()
{
	ctx = std::make_unique<llvm::LLVMContext>();
	cModule = std::make_unique<llvm::Module>(m_moduleName, *ctx);

	// Create a new builder for the m_module
	builder = std::make_unique<llvm::IRBuilder<>>(*ctx);

}
int Generator::getTypePriority(llvm::Type* type)
{
	if (type->isIntegerTy()) {
		return type->getIntegerBitWidth(); // Higher bit width means higher priority
	}
	else if (type->isFloatingPointTy()) {
		if (type->isDoubleTy()) return 164;
		if (type->isFloatTy()) return 132;
	}
	else if (type->isPointerTy()) {
		return 2; // Assuming pointers are 64-bit
	}
	else if (type->isVoidTy()) {
		return 0; // Void types have the lowest priority
	}
	// PLUS more type categories as needed
	return -1; // Unknown types have lowest priority
}
llvm::Value* Generator::autoTypeCast(llvm::Value* val, llvm::Type* targetType) {
	llvm::Type* valType = val->getType();

	if (valType == targetType)
		return val; // No cast needed

	// Handle constant values
	if (llvm::Constant* constVal = llvm::dyn_cast<llvm::Constant>(val)) {
		if (valType->isIntegerTy() && targetType->isIntegerTy()) {
			if (valType->getIntegerBitWidth() < targetType->getIntegerBitWidth()) {
				return llvm::ConstantExpr::getZExtOrBitCast(constVal, targetType);
			}
			else if (valType->getIntegerBitWidth() > targetType->getIntegerBitWidth()) {
				return llvm::ConstantExpr::getTruncOrBitCast(constVal, targetType);
			}
		}
		else if (targetType->isPointerTy() && valType->isPointerTy()) {
			return llvm::ConstantExpr::getBitCast(constVal, targetType);
		}
		else if (targetType->isIntegerTy() && valType->isPointerTy()) {
			return llvm::ConstantExpr::getPtrToInt(constVal, targetType);
		}
		else if (targetType->isPointerTy() && valType->isIntegerTy()) {
			return llvm::ConstantExpr::getIntToPtr(constVal, targetType);
		}
		else if (targetType->isFloatingPointTy() && valType->isFloatingPointTy()) {
			return llvm::ConstantExpr::getFPCast(constVal, targetType);
		}
		else if (targetType->isFloatingPointTy() && valType->isIntegerTy()) {
			return llvm::ConstantExpr::getSIToFP(constVal, targetType);
		}
		else if (targetType->isIntegerTy() && valType->isFloatingPointTy()) {
			return llvm::ConstantExpr::getFPToSI(constVal, targetType);
		}
	}

	if (targetType->isIntegerTy() && valType->isIntegerTy()) {
		unsigned targetWidth = targetType->getIntegerBitWidth();
		unsigned valWidth = valType->getIntegerBitWidth();

		if (valWidth < targetWidth) {
			return builder->CreateZExtOrBitCast(val, targetType, "zext");
		}
		else if (valWidth > targetWidth) {
			return builder->CreateTruncOrBitCast(val, targetType, "trunc");
		}
	}
	else if (targetType->isPointerTy() && valType->isPointerTy()) {
		return builder->CreateBitCast(val, targetType, "ptrcast");
	}
	else if (targetType->isIntegerTy() && valType->isPointerTy()) {
		return builder->CreatePtrToInt(val, targetType, "ptrtoint");
	}
	else if (targetType->isPointerTy() && valType->isIntegerTy()) {
		return builder->CreateIntToPtr(val, targetType, "inttoptr");
	}
	else if (targetType->isFloatingPointTy() && valType->isFloatingPointTy()) {
		return builder->CreateFPCast(val, targetType, "fpcast");
	}
	else if (targetType->isFloatingPointTy() && valType->isIntegerTy()) {
		return builder->CreateSIToFP(val, targetType, "sitofp");
	}
	else if (targetType->isIntegerTy() && valType->isFloatingPointTy()) {
		return builder->CreateFPToSI(val, targetType, "fptosi");
	}

	llvm::errs() << "Unsupported cast from " << *valType << " to " << *targetType << "\n";
	return nullptr;
}
llvm::Type* Generator::findTypeFromPrimitive(PrimitiveDataType pdt)
{
	if (m_TypeMap.count(pdt) > 0)
		return m_TypeMap[pdt];
	return nullptr;
}
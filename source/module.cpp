#include <marklang.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cassert>

#ifndef __FUNCTION_NAME__
	#if defined(WIN32) || defined(_WIN32)
	#ifdef __PRETTY_FUNCTION__
		#define __FUNCTION_NAME__  __PRETTY_FUNCTION__  
	#else
		#define __FUNCTION_NAME__  __FUNCTION__
	#endif
#else
	#define __FUNCTION_NAME__  __func__ 
	#endif
#endif

namespace mlang {
	const std::unordered_map<Token::Type, size_t> typeSizes = {
		{Token::Type::INT8,		1},
		{Token::Type::INT16,	2},
		{Token::Type::INT32,	4},
		{Token::Type::INT64,	8},

		{Token::Type::UINT8,	1},
		{Token::Type::UINT16,	2},
		{Token::Type::UINT32,	4},
		{Token::Type::UINT64,	8},
	};
	RespCode Module::AddSectionFromFile(const std::string &file) {
		if (!std::filesystem::exists(file)) {
			std::cout << "File '" << std::filesystem::absolute(file) << "' doesn't exist\n";
			return RespCode::ERR;
		}

		std::ifstream fp(file);
		tokenizer.AddCode(std::string(std::istreambuf_iterator<char>(fp), {}));
		fp.close();

		return RespCode::SUCCESS;
	}
	RespCode Module::AddSectionFromMemory(const std::string &inCode) {
		if (!inCode.size()) {
			std::cout << "Code is empty\n";
			return RespCode::ERR;
		}
		tokenizer.AddCode(inCode);

		return RespCode::SUCCESS;
	}

	Module::Module(Engine *engine_, const std::string &name_)
		:engine(engine_), name(name_), moduleStmts(std::make_unique<BlockStmt>()) {}

	std::optional<std::variant<ScriptFunc *, ScriptObject *>> Module::NameResolution(const std::string &name) {
		auto scope = engine->GetScope();
		if (name.find(':') == std::string::npos && name.find('.') == std::string::npos) {
			if (scope->FindObjectByName(name).data.value()) {
				return scope->FindObjectByName(name).data.value();
			}
			else if (scope->FindFuncByName(name).data.value()) {
				return scope->FindFuncByName(name).data.value();
			}

			// Class name resolution
			if (scope->parentFunc && scope->IsOfType(Scope::Type::CLASS)) {
				if (scope->parentFunc->GetScriptObject() && scope->parentFunc->GetScriptObject()->GetMember(name)) {
					return scope->parentFunc->GetScriptObject()->GetMember(name);
				}
				else if (scope->parentFunc->GetScriptObject() && scope->parentFunc->GetScriptObject()->GetType()->GetMethod(name)) {
					return scope->parentFunc->GetUnderlyingFunc()->funcScope->FindObjectByName(name).data;
				}
				
			}
			return std::nullopt;
		}

		std::vector<std::string> ret;
		size_t pos, last = 0;

		std::optional<std::variant<ScriptFunc *, ScriptObject *>> tmp;

		while ((pos = name.find_first_of(".:", last)) != std::string::npos) {
			if (name[pos] == ':' && name[pos + 1] != ':') {
				return std::nullopt;
			}
			else if (name[pos] == ':') pos++;

			if (pos > last) {
				auto word = name.substr(last, pos - last);
				if (tmp && std::holds_alternative<ScriptObject*>(tmp.value())) {
					auto object = std::get<ScriptObject *>(tmp.value());
					if (object->GetMember(word)) {
						tmp = object->GetMember(word);
					}
					else if (object->GetType()->GetMethod(word)) {
						tmp = object->GetType()->GetMethod(word);
						if (tmp) {
							std::get<ScriptFunc *>(tmp.value())->SetClassObject(object);
							std::get<ScriptFunc *>(tmp.value())->func->funcScope->parentFunc = std::get<ScriptFunc *>(tmp.value());
						}
					}
					else if (scope->parentFunc && scope->IsOfType(Scope::Type::CLASS)) {
						auto obj = scope->parentFunc->GetScriptObject();
						if (!obj) {
							return std::nullopt;
						}

						tmp = obj->GetMember(word);
					}
					else {
						return std::nullopt;
					}
				}
				else {
					if (scope->FindFuncByName(word).data.value()) {
						tmp = (scope->FindFuncByName(word).data.value());
					}
					else if (scope->FindObjectByName(word).data.value()) {
						tmp = scope->FindObjectByName(word).data.value();
					}
					else {
						return std::nullopt;
					}
				}
			}

			last = pos + 2;
		}
		if (last < name.length() && std::holds_alternative<ScriptObject *>(tmp.value())) {
			if (last) last--;
			std::string word = name.substr(last, std::string::npos);

			auto object = std::get<ScriptObject *>(tmp.value());
			if (object->GetMember(word)) {
				tmp = object->GetMember(word);
			}
			else if (object->GetType()->GetMethod(word)) {
				tmp = object->GetType()->GetMethod(word);
				auto func = std::get<ScriptFunc *>(tmp.value());
				func->SetClassObject(object);
				func->func->funcScope->parentFunc = func;
			}
			else {
				return std::nullopt;
			}
		}

		return tmp;
	}

	RespCode Module::CopyObjInto(ScriptObject *&dest, ScriptObject *src) {
		// Checks if only one is class
		if (dest->GetType()->isClass ^ src->GetType()->isClass) {
			return RespCode::ERR;
		}
		// Checks if classes are not the same
		else if (dest->GetType()->isClass && (dest->GetType()->TypeID() != src->GetType()->TypeID())) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid type conversion from '" << dest->GetType()->GetName() << "' to '" << src->GetType()->GetName() << "'\n";
			return RespCode::ERR;
		}

		if (dest->IsModifier(ScriptObject::Modifier::REFERENCE)) {
			dest = src;	// Simple memory copy
			src->refCount++;
		}
		else if (!dest->GetType()->isClass) {
			auto destType = dest->GetType();
			auto srcType = src->GetType();
			bool isDestFloat = destType->GetName() == "float" || destType->GetName() == "double";
			bool isSrcFloat = srcType->GetName() == "float" || srcType->GetName() == "double";

			if (isDestFloat && !isSrcFloat) {
				uint64_t intSrc = 0;
				switch (srcType->Size()) {
					case 1:
						intSrc = *reinterpret_cast<uint8_t *>(src->ptr);
						break;
					case 2:
						intSrc = *reinterpret_cast<uint16_t *>(src->ptr);
						break;
					case 4:
						intSrc = *reinterpret_cast<uint32_t *>(src->ptr);
						break;
					case 8:
						intSrc = *reinterpret_cast<uint64_t *>(src->ptr);
						break;
				}

				if (destType->GetName() == "float")
					*static_cast<float *>(dest->ptr) = static_cast<float>(srcType->unsig ? intSrc : static_cast<int64_t>(intSrc));
				else
					*static_cast<double *>(dest->ptr) = static_cast<double>(srcType->unsig ? intSrc : static_cast<int64_t>(intSrc));
			}
			else if (!isDestFloat && isSrcFloat) {
				void *srcPtr = src->ptr;
				float *floatPtr = reinterpret_cast<float *>(srcPtr);
				double *doublePtr = reinterpret_cast<double *>(srcPtr);

				switch (destType->Size()) {
					case 1:
						*reinterpret_cast<uint8_t *>(dest->ptr) = static_cast<uint8_t>((srcType->GetName() == "float" ? *floatPtr : *doublePtr));
						break;
					case 2:
						*reinterpret_cast<uint16_t *>(dest->ptr) = static_cast<uint16_t>((srcType->GetName() == "float" ? *floatPtr : *doublePtr));
						break;
					case 4:
						*reinterpret_cast<uint32_t *>(dest->ptr) = static_cast<uint32_t>((srcType->GetName() == "float" ? *floatPtr : *doublePtr));
						break;
					case 8:
						*reinterpret_cast<uint64_t *>(dest->ptr) = static_cast<uint64_t>((srcType->GetName() == "float" ? *floatPtr : *doublePtr));
						break;
				}
			}
			else if (isDestFloat == isSrcFloat) {
				if (destType->Size() == srcType->Size()) {
					std::memcpy(dest->ptr, src->ptr, destType->Size());
					return RespCode::SUCCESS;
				}

				void *srcPtr = src->ptr;

				if (isDestFloat) {
					float *floatPtr = reinterpret_cast<float *>(srcPtr);
					double *doublePtr = reinterpret_cast<double *>(srcPtr);
				}
				else {
					uint64_t srcVal = 0;
					// Sets the source value based on type
					switch (srcType->Size()) {
						case 1:
							srcVal = static_cast<uint8_t>(*reinterpret_cast<uint8_t *>(srcPtr));
							break;
						case 2:
							srcVal = static_cast<uint16_t>(*reinterpret_cast<uint16_t *>(srcPtr));
							break;
						case 4:
							srcVal = static_cast<uint32_t>(*reinterpret_cast<uint32_t *>(srcPtr));
							break;
						case 8:
							srcVal = static_cast<uint64_t>(*reinterpret_cast<uint64_t *>(srcPtr));
							break;
					}

					// Sets destination value
					switch (destType->Size()) {
						case 1:
							*reinterpret_cast<uint8_t *>(dest->ptr) = static_cast<uint8_t>(srcVal);
							break;
						case 2:
							*reinterpret_cast<uint16_t *>(dest->ptr) = static_cast<uint16_t>(srcVal);
							break;
						case 4:
							*reinterpret_cast<uint32_t *>(dest->ptr) = static_cast<uint32_t>(srcVal);
							break;
						case 8:
							*reinterpret_cast<uint64_t *>(dest->ptr) = static_cast<uint64_t>(srcVal);
							break;
					}
				}
			}
		}
		else {
			for (auto &[name, obj] : dest->members) {
				CopyObjInto(obj, src->members.at(name));
			}
		}
		return RespCode::SUCCESS;
	}

	static void PrintTabs(int tabs) {
		while (tabs--) {
			std::cout << ' ';
		}
	}

	static void PrintExpr(Expression *expr, int tabs = 0) {
		if (!expr) return;

		switch (expr->type) {
			case Expression::Type::BINARY:
				PrintTabs(tabs);
				std::cout << "BINARY:\n";
				PrintTabs(tabs + 1);
				std::cout << "LHS:\n";
				PrintExpr(dynamic_cast<BinaryExpr *>(expr)->lhs, tabs + 3);
				PrintTabs(tabs + 1);
				std::cout << "OP: ";
				std::cout << dynamic_cast<BinaryExpr *>(expr)->op.val << "\n";
				PrintTabs(tabs + 1);
				std::cout << "RHS:\n";
				PrintExpr(dynamic_cast<BinaryExpr *>(expr)->rhs, tabs + 3);
				break;
			case Expression::Type::VALUE:
				PrintTabs(tabs);
				std::cout << "VALUE: " << dynamic_cast<ValueExpr *>(expr)->val.val << "\n";
				break;
			case Expression::Type::FUNCCALL:
				PrintTabs(tabs);
				std::cout << "FUNCCALL: " << dynamic_cast<FuncCallExpr *>(expr)->funcName.val << "\n";
				if (dynamic_cast<FuncCallExpr *>(expr)->params.size()) {
					PrintTabs(tabs + 1);
					std::cout << "PARAMS:\n";
					for (auto &param : dynamic_cast<FuncCallExpr *>(expr)->params) {
						PrintTabs(tabs + 2);
						std::cout << "PARAM:\n";
						PrintExpr(param, tabs + 3);
					}
				}
				break;
		}
	}
	static void PrintStmt(Statement *stmt, int tabs = 0) {
		if (!stmt) return;

		switch (stmt->type) {
			case Statement::Type::BLOCK: {
				PrintTabs(tabs);
				std::cout << "BLOCK\n";
				for (auto &child : dynamic_cast<BlockStmt *>(stmt)->stmts) {
					PrintStmt(child, tabs + 1);
				}
				break;
			}
			case Statement::Type::VARDECL: {
				PrintTabs(tabs);
				std::cout << "VarDecl: " << dynamic_cast<VarDeclStmt *>(stmt)->ident.val << "\n";
				PrintExpr(dynamic_cast<VarDeclStmt *>(stmt)->expr, tabs + 1);
				break;
			}
			case Statement::Type::ASSIGNEMENT: {
				PrintTabs(tabs);
				std::cout << "Assignement: " << dynamic_cast<VarAssignStmt *>(stmt)->ident.val << "\n";
				PrintExpr(dynamic_cast<VarAssignStmt *>(stmt)->expr, tabs + 1);
				break;
			}
			case Statement::Type::FUNCDEF: {
				PrintTabs(tabs);
				std::cout << "Function: " << dynamic_cast<FuncStmt *>(stmt)->ident.val << "\n";
				if (dynamic_cast<FuncStmt *>(stmt)->params.size()) {
					PrintTabs(tabs + 1);
					std::cout << "PARAMS:\n";
					for (auto &param : dynamic_cast<FuncStmt *>(stmt)->params) {
						PrintStmt(param, tabs + 2);
					}
				}
				PrintStmt(dynamic_cast<FuncStmt *>(stmt)->block, tabs + 1);
				break;
			}
			case Statement::Type::FUNCCALL: {
				PrintTabs(tabs);
				std::cout << "Function call:\n";
				PrintTabs(tabs + 1);
				std::cout << "NAME: " << dynamic_cast<FuncCallStmt *>(stmt)->funcName.val << "\n";
				break;
			}
			case Statement::Type::IF: {
				PrintTabs(tabs);
				std::cout << "If:\n";

				PrintTabs(tabs + 1);
				std::cout << "CONDITION:\n";
				PrintExpr(dynamic_cast<IfStmt *>(stmt)->condition, tabs + 2);

				PrintTabs(tabs + 1);
				std::cout << "THEN:\n";
				PrintStmt(dynamic_cast<IfStmt *>(stmt)->then, tabs + 2);

				if (dynamic_cast<IfStmt *>(stmt)->els) {
					PrintTabs(tabs + 1);
					std::cout << "ELSE:\n";
					PrintStmt(dynamic_cast<IfStmt *>(stmt)->els, tabs + 2);
				}
				break;
			}
			case Statement::Type::WHILE: {
				PrintTabs(tabs);
				std::cout << "While:\n";

				PrintTabs(tabs + 1);
				std::cout << "CONDITION:\n";
				PrintExpr(dynamic_cast<WhileStmt *>(stmt)->cond, tabs + 2);

				PrintTabs(tabs + 1);
				std::cout << "THEN:\n";
				PrintStmt(dynamic_cast<WhileStmt *>(stmt)->then, tabs + 2);

				break;
			}
			case Statement::Type::FOR: {
				auto casted = dynamic_cast<ForStmt *>(stmt);
				PrintTabs(tabs);
				std::cout << "For:\n";

				PrintTabs(tabs + 1);
				std::cout << "BEGIN:\n";
				PrintStmt(casted->start, tabs + 2);

				PrintTabs(tabs + 1);
				std::cout << "CONDITION:\n";
				PrintExpr(casted->cond, tabs + 2);

				PrintTabs(tabs + 1);
				std::cout << "END:\n";
				PrintStmt(casted->end, tabs + 2);

				PrintTabs(tabs + 1);
				std::cout << "THEN:\n";
				PrintStmt(casted->then, tabs + 2);

				break;
			}
			case Statement::Type::BREAK: {
				PrintTabs(tabs);
				std::cout << "Break\n";
				break;
			}
			case Statement::Type::RETURN: {
				PrintTabs(tabs);
				std::cout << "Return:\n";

				PrintExpr(dynamic_cast<ReturnStmt *>(stmt)->val, tabs + 1);


				break;
			}
		}
	}

	double Module::EvaluateExpr(Scope *scope, Expression *expr) {
		if (expr->type == Expression::Type::VALUE) {
			auto casted = dynamic_cast<ValueExpr *>(expr);

			if(casted->val.type >= Token::Type::LITERALS_BEGIN && casted->val.type <= Token::Type::LITERALS_END)
				return std::stod(casted->val.val);
			if (casted->val.type != Token::Type::IDENTIFIER) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " Invalid token " << casted->val.val << "\n";
				errCode = RespCode::ERR;
				return 0;
			}

			auto resolution = NameResolution(casted->val.val);
			if (!resolution) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " Invalid object " << casted->val.val << "\n";
				errCode = RespCode::ERR;
				return 0;
			}
			ScriptObject *objFound = std::get<ScriptObject *>(resolution.value());

			if (objFound->GetType()->IsClass()) return 0;

			auto objType = objFound->GetType();
			void *ptr = objFound->GetAddressOfObj();

			if (objType->GetName() != "float" && objType->GetName() != "double") {
					switch (objType->Size()) {
						case 1:
							return static_cast<double>(objType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(ptr) : *reinterpret_cast<int8_t *>(ptr));
						case 2:
							return static_cast<double>(objType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(ptr) : *reinterpret_cast<uint16_t *>(ptr));
						case 4:
							return static_cast<double>(objType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(ptr) : *reinterpret_cast<uint32_t *>(ptr));
						case 8:
							return static_cast<double>(objType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(ptr) : *reinterpret_cast<uint64_t *>(ptr));
						default:
							return 0;
					}
				}
			else {
				if (objType->GetName() == "float") {
					return *reinterpret_cast<float *>(ptr);
				}
				else {
					return *reinterpret_cast<double *>(ptr);
				}
			}

			errCode = RespCode::ERR;
			return 0;
		}
		else if (expr->type == Expression::Type::BINARY) {
			auto casted = dynamic_cast<BinaryExpr *>(expr);
			double val = EvaluateExpr(scope, casted->lhs);

			switch (casted->op.type) {
				case Token::Type::PLUS:
					val += EvaluateExpr(scope, casted->rhs);
					break;
				case Token::Type::MINUS:
					val -= EvaluateExpr(scope, casted->rhs);
					break;
				case Token::Type::STAR:
					val *= EvaluateExpr(scope, casted->rhs);
					break;
				case Token::Type::SLASH:
					val /= EvaluateExpr(scope, casted->rhs);
					break;
				case Token::Type::LESS:
					val = val < EvaluateExpr(scope, casted->rhs);
					break;
				case Token::Type::LEQ:
					val = val <= EvaluateExpr(scope, casted->rhs);
					break;
				case Token::Type::GREATER:
					val = val > EvaluateExpr(scope, casted->rhs);
					break;
				case Token::Type::GEQ:
					val = val >= EvaluateExpr(scope, casted->rhs);
					break;
				case Token::Type::NEQ:
					val = val != EvaluateExpr(scope, casted->rhs);
					break;
			}

			return val;
		}
		else if (expr->type == Expression::Type::FUNCCALL) {
			auto casted = dynamic_cast<FuncCallExpr *>(expr);

			auto resolution = NameResolution(casted->funcName.val);
			if (!resolution || !std::holds_alternative<ScriptFunc *>(resolution.value())) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " Invalid function " << casted->funcName.val << "\n";
				errCode = RespCode::ERR;
				return 0;
			}
			ScriptFunc *funcFind = std::get<ScriptFunc*>(resolution.value());

			if (!funcFind) return 0;
			if (funcFind->GetUnderlyingFunc()->type.val == "void") return 0;

			auto r = RunFunc(funcFind->GetUnderlyingFunc(), casted->params); assert(r == RespCode::SUCCESS);
			auto retObj = funcFind->func->funcScope->returnObj;
			ScriptObject retExpr(engine, engine->GetScope()->FindTypeInfoByName("double").data.value());
			auto tmp = &retExpr;
			CopyObjInto(tmp, retObj);

			delete funcFind->func->funcScope->returnObj;
			funcFind->func->funcScope->returnObj = nullptr;

			double ret = *reinterpret_cast<double *>(retExpr.ptr);

			return ret;
		}

		errCode = RespCode::ERR;
		return 0;
	}

	RespCode Module::Build() {
		Token tok;

		while ((tok = tokenizer.Tokenize()).type != Token::Type::END) {
			toks.push_back(tok);
		}
		toks.push_back(Token());	// Makes sure the last token is END token
		currTok = &toks.front();

		while (true) {
			auto stmt = ParseStatement();
			if (!stmt || errCode != RespCode::SUCCESS) {
				break;
			}
			if (
				stmt->type == Statement::Type::ASSIGNEMENT ||
				stmt->type == Statement::Type::VARDECL ||
				stmt->type == Statement::Type::RETURN ||
				stmt->type == Statement::Type::FUNCCALL) {
				NextToken();	// Semicolon
			}

			moduleStmts->AddStatement(stmt);
		}

		// PrintStmt(moduleStmts.get());

		return errCode;
	}

	RespCode Module::RunWhile(WhileStmt *stmt) {
		while (EvaluateExpr(engine->GetScope(), stmt->cond) != 0) {
			auto scope = stmt->scope;
			engine->SetScope(scope);

			if (RunBlockStmt(dynamic_cast<BlockStmt*>(stmt->then)) != RespCode::SUCCESS) {
				engine->SetScope(scope->parent);
				return RespCode::ERR;
			}

			engine->SetScope(scope->parent);
		}
		return RespCode::SUCCESS;
	}
	RespCode Module::RunFunc(FuncStmt *stmt, std::vector<Expression *> params) {
		auto scope = engine->GetScope();
		engine->SetScope(stmt->funcScope);
		for (size_t i = 0; i < stmt->params.size(); ++i) {
			auto funcParam = dynamic_cast<VarDeclStmt*>(stmt->params[i]);
			auto providedValue = EvaluateExpr(stmt->funcScope, params[i]);

			auto foundParam = stmt->funcScope->FindObjectByName(funcParam->ident.val).data;
			if (!foundParam) continue;

			ScriptObject tmpObj(engine, engine->GetScope()->FindTypeInfoByName("double").data.value());
			*reinterpret_cast<double *>(tmpObj.ptr) = providedValue;
			CopyObjInto(foundParam.value(), &tmpObj);
		}
		auto retCode = RunBlockStmt(dynamic_cast<BlockStmt *>(stmt->block));
		engine->SetScope(scope);

		return retCode;
	}
	RespCode Module::RunFuncCall(FuncCallStmt *stmt) {
		auto scope = engine->GetScope();

		auto nameResolved = NameResolution(stmt->funcName.val);
		if (!nameResolved || !std::holds_alternative<ScriptFunc *>(nameResolved.value())) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid function '" << stmt->funcName.val << "'\n";
			return RespCode::ERR;
		}
		ScriptFunc *foundFunc = std::get<ScriptFunc *>(nameResolved.value());

		return RunFunc(foundFunc->func, stmt->params);
	}
	RespCode Module::RunReturn(ReturnStmt *stmt) {
		auto scope = engine->GetScope();
		if (!scope->IsOfType(Scope::Type::FUNCTION)) {
			// TODO: change to RespCode::ERR
			return RespCode::SUCCESS;
		}
		if (stmt->val && scope->parentFunc->returnType->GetName() == "void") {
			return RespCode::ERR;
		}

		scope->returnObj = new ScriptObject(engine, scope->parentFunc->returnType);
		ScriptObject tmp(engine, scope->FindTypeInfoByName("double").data.value());
		*reinterpret_cast<double *>(tmp.ptr) = EvaluateExpr(scope, stmt->val);

		return CopyObjInto(scope->returnObj, &tmp);
	}
	RespCode Module::RunIf(IfStmt *stmt) {
		double value = EvaluateExpr(engine->GetScope(), stmt->condition);

		if (value != 0) {
			engine->SetScope(stmt->thenScope);
			RunStmt(stmt->then);
			engine->SetScope(stmt->thenScope->parent);
		}
		else if (stmt->els) {
			engine->SetScope(stmt->elseScope);
			RunStmt(stmt->els);
			engine->SetScope(stmt->elseScope->parent);
		}

		return RespCode::SUCCESS;
	}
	RespCode Module::RunBlockStmt(BlockStmt *stmt) {
		for (auto &subStmt : stmt->stmts) {
			if (RunStmt(subStmt) != RespCode::SUCCESS) {
				return RespCode::ERR;
			}
			if (subStmt->type == Statement::Type::RETURN) {
				return RespCode::SUCCESS;
			}
			if (subStmt->type == Statement::Type::BREAK) {
				return RespCode::SUCCESS;
			}
		}

		return RespCode::SUCCESS;
	}
	RespCode Module::RunVarDeclStmt(VarDeclStmt *stmt) {
		auto scope = engine->GetScope();

		if (scope->FindObjectByName(stmt->ident.val).code == RespCode::SUCCESS) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Variable name '" << stmt->ident.val << "' already reserved at line " << stmt->ident.row << "[" << stmt->ident.col << "]\n";
			return RespCode::ERR;
		}

		auto typeFind = scope->FindTypeInfoByName(stmt->type.val).data;
		if (!typeFind) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Type '" << stmt->type.val << "' not found at line " << stmt->ident.row << "[" << stmt->ident.col << "]\n";
			return RespCode::ERR;
		}
		auto obj = new ScriptObject(engine, typeFind.value());
		obj->modifiers = static_cast<ScriptObject::Modifier>(stmt->modifiers);
		obj->identifier = stmt->ident.val;
		scope->RegisterObject(obj);

		auto foundObj = scope->FindObjectByName(stmt->ident.val).data;
		if (!foundObj) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Error instantiating '" << stmt->ident.val << "' at line " << stmt->ident.row << "[" << stmt->ident.col << "]\n";
			return RespCode::ERR;
		}

		if (stmt->expr) {
			ScriptObject tmpObj(engine, scope->FindTypeInfoByName("double").data.value());

			if (dynamic_cast<ValueExpr *>(stmt->expr) && dynamic_cast<ValueExpr *>(stmt->expr)->val.type == Token::Type::IDENTIFIER) {
				auto nameResolution = NameResolution(dynamic_cast<ValueExpr *>(stmt->expr)->val.val);

				ScriptObject *foundSrc = std::get<ScriptObject*>(nameResolution.value());

				std::cout << stmt->ident.val << " initialized to " << dynamic_cast<ValueExpr *>(stmt->expr)->val.val << "\n";
				return CopyObjInto(foundObj.value(), foundSrc);
			}

			double toSet = EvaluateExpr(scope, stmt->expr);
			std::cout << foundObj.value()->identifier << " initialized to " << toSet << "\n";
			*reinterpret_cast<double *>(tmpObj.ptr) = toSet;

			return CopyObjInto(foundObj.value(), &tmpObj);
		}

		return RespCode::SUCCESS;
	}
	RespCode Module::RunVarAssignStmt(VarAssignStmt *stmt) {
		auto scope = engine->GetScope();

		std::optional<std::variant<ScriptFunc *, ScriptObject *>> source = std::nullopt;

		auto nameResolution = NameResolution(stmt->ident.val);
		if (!nameResolution || !std::holds_alternative<ScriptObject *>(nameResolution.value())) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid variable '" << stmt->ident.val << "' at line " << stmt->ident.row << "[" << stmt->ident.col << "]\n";
			return RespCode::ERR;
		}

		ScriptObject *foundObj = std::get<ScriptObject *>(nameResolution.value());
		if (foundObj->IsModifier(ScriptObject::Modifier::CONST)) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " <<
				"Assigning a value to a const object '" << foundObj->GetName() << "' at line " << stmt->ident.row << "[" << stmt->ident.col << "]\n";
			return RespCode::ERR;
		}
		if (scope->parentFunc->isConstMethod) {
			if (foundObj->parentClass == scope->parentFunc->object) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " <<
					"Assigning a member in constant method '" << scope->parentFunc->GetName() << "' at line " << stmt->ident.row << "[" << stmt->ident.col << "]\n";
				return RespCode::ERR;
			}
		}

		/*if (source) {
			if (std::holds_alternative<ScriptObject *>(source.value())) {
				std::cout << stmt->ident.val << " set to " << dynamic_cast<ValueExpr *>(stmt->expr)->val.val << "\n";
				return CopyObjInto(foundObj, std::get<ScriptObject *>(source.value()));
			}
			else if (std::holds_alternative<ScriptFunc *>(source.value())) {
				std::cout << stmt->ident.val << " set to " << dynamic_cast<FuncCallExpr *>(stmt->expr)->funcName.val << "\n";

				return CopyObjInto(foundObj, std::get<ScriptFunc *>(source.value())->GetUnderlyingFunc()->funcScope->returnObj);
			}
			return RespCode::ERR;
		}*/

		ScriptObject tmpObj(engine, scope->FindTypeInfoByName("double").data.value());
		double toSet = EvaluateExpr(scope, stmt->expr);
		std::cout << stmt->ident.val << " set to " << toSet << "\n";
		*reinterpret_cast<double *>(tmpObj.ptr) = toSet;

		return CopyObjInto(foundObj, &tmpObj);
	}
	RespCode Module::RunStmt(Statement *stmt) {
		switch (stmt->type) {
			case Statement::Type::BLOCK:
				return RunBlockStmt(dynamic_cast<BlockStmt *>(stmt));
			case Statement::Type::VARDECL:
				return RunVarDeclStmt(dynamic_cast<VarDeclStmt *>(stmt));
			case Statement::Type::ASSIGNEMENT:
				return RunVarAssignStmt(dynamic_cast<VarAssignStmt *>(stmt));
			case Statement::Type::IF:
				return RunIf(dynamic_cast<IfStmt *>(stmt));
			case Statement::Type::WHILE:
				return RunWhile(dynamic_cast<WhileStmt *>(stmt));
			case Statement::Type::FUNCCALL:
				return RunFuncCall(dynamic_cast<FuncCallStmt *>(stmt));
			case Statement::Type::RETURN:
				return RunReturn(dynamic_cast<ReturnStmt *>(stmt));
			default:
				return RespCode::ERR;
		}
	}
	RespCode Module::Run() {
		if (!moduleStmts) {
			return RespCode::ERR;
		}

		for (Statement *stmt : moduleStmts->stmts) {
			if (stmt->type != Statement::Type::FUNCDEF) {
				if (RunStmt(stmt) == RespCode::ERR) {
					return RespCode::ERR;
				}
				continue;
			}

			FuncStmt *castedStmt = dynamic_cast<FuncStmt *>(stmt);
			if (castedStmt->ident.val != "main") continue;

			return RunFunc(castedStmt, std::vector<Expression*>());
		}

		return RespCode::ERR;
	}
}
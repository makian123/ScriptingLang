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

	static int Precedence(Token::Type tok) {
		using Type = Token::Type;

		switch (tok) {
			case Type::STAR:
			case Type::SLASH:
				return 3;
			case Type::PLUS:
			case Type::MINUS:
				return 2;
			case Type::LESS:
			case Type::LEQ:
			case Type::GREATER:
			case Type::GEQ:
			case Type::NEQ:
				return 1;
			default:
				return 0;
		}
	}

	// Tokenizer
	static const std::unordered_map<std::string, Token::Type> keywords = {
		{"void", Token::Type::VOID},

		{"char", Token::Type::INT8},
		{"short", Token::Type::INT16},
		{"int", Token::Type::INT32},
		{"long", Token::Type::INT64},

		{"float", Token::Type::FLOAT},
		{"double", Token::Type::DOUBLE},

		{"unsigned", Token::Type::UNSIGNED},
		{"enum", Token::Type::ENUM},
		{"class", Token::Type::CLASS},

		{"if", Token::Type::IF},
		{"else", Token::Type::ELSE},
		{"while", Token::Type::WHILE},
		{"for", Token::Type::FOR},
		{"break", Token::Type::BREAK},

		{"return", Token::Type::RETURN}
	};
	Token Module::Tokenizer::Tokenize() {
		if (currIdx >= code.size()) return Token(Token::Type::END, "", currRow, currCol);

		// Skip whitespace
		while (currIdx < code.size() && std::isspace(code[currIdx])) {
			if (code[currIdx] == '\n') {
				currRow++;
				currCol = 1;
			}
			else if (code[currIdx] == '\r') { currCol = 0; }
			else { currCol++; }

			currIdx++;
		}
		if (currIdx >= code.size()) return Token(Token::Type::END, "", currRow, currCol);


		// Check if identifier or keyword
		if (std::isalpha(code[currIdx]) || code[currIdx] == '_') {
			std::string tmp;
			while (currIdx < code.size() && (std::isalnum(code[currIdx]) || code[currIdx] == '_')) {
				tmp += code[currIdx++];
				currCol++;
			}

			if (keywords.contains(tmp)) {
				return Token(keywords.at(tmp), tmp, currRow, currCol);
			}

			return Token(Token::Type::IDENTIFIER, tmp, currRow, currCol - (int)tmp.size());
		}

		// Check if numeric
		if (std::isdigit(code[currIdx])) {
			std::string tmp;
			bool digitFound = false;
			while (currIdx < code.size() && (std::isdigit(code[currIdx]) || (code[currIdx] == '.' && !digitFound))) {
				if (code[currIdx] == '.') digitFound = true;
				tmp += code[currIdx++];
				currCol++;
			}

			return Token((digitFound ? Token::Type::DECIMAL : Token::Type::INTEGER), tmp, currRow, currCol - (int)tmp.size());
		}

		// Special characters (. * + >=) and more
		char lookahead = ((currIdx + 1) < code.size() ? code[currIdx + 1] : '\0');
		switch (code[currIdx]) {
			case '+': {
				return Token(Token::Type::PLUS, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '-': {
				return Token(Token::Type::MINUS, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '*': {
				return Token(Token::Type::STAR, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '/': {
				return Token(Token::Type::SLASH, std::string{ code[currIdx++] }, currRow, currCol++);
			}

			case '(': {
				return Token(Token::Type::OPEN_PARENTH, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case ')': {
				return Token(Token::Type::CLOSED_PARENTH, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '{': {
				return Token(Token::Type::OPEN_BRACE, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '}': {
				return Token(Token::Type::CLOSED_BRACE, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '[': {
				return Token(Token::Type::OPEN_BRACKET, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case ']': {
				return Token(Token::Type::CLOSED_BRACKET, std::string{ code[currIdx++] }, currRow, currCol++);
			}

			case ';': {
				return Token(Token::Type::SEMICOLON, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case ',': {
				return Token(Token::Type::COMMA, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '.': {
				return Token(Token::Type::DOT, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '=': {
				return  Token(Token::Type::ASSIGN, std::string{ code[currIdx++] }, currRow, currCol++);
			}

			case '!': {
				if (lookahead == '=') {
					return Token(Token::Type::NEQ, std::string{ code[currIdx++], code[currIdx++] }, currRow, currCol++);
				}
				return  Token(Token::Type::NOT, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '<': {
				if (lookahead == '=') {
					return Token(Token::Type::LEQ, std::string{ code[currIdx++], code[currIdx++] }, currRow, currCol++);
				}
				return Token(Token::Type::LESS, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '>': {
				if (lookahead == '=') {

					return  Token(Token::Type::GEQ, std::string{ code[currIdx++], code[currIdx++] }, currRow, currCol++);
				}
				return  Token(Token::Type::GREATER, std::string{ code[currIdx++] }, currRow, currCol++);
			}
		}

		return Token();
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

	Token *Module::NextToken() {
		if ((currTokIdx + 1) >= toks.size()) return &toks.back();	//End token is always of type END

		auto idx = currTokIdx;
		currTok = &toks[++currTokIdx];

		return &toks[idx];
	}
	Expression *Module::ParseExpression(int precedence) {
		auto left = ParsePrimaryExpr();

		while (true) {
			auto thisPrecedence = Precedence(currTok->type);
			if (thisPrecedence == 0 || thisPrecedence < precedence) { break; }

			auto op = NextToken();
			auto right = ParseExpression(thisPrecedence);
			left = new BinaryExpr(left, *op, right);
		}
		return left;
	}
	Expression *Module::ParsePrimaryExpr() {
		if (currTok->type == Token::Type::INTEGER || currTok->type == Token::Type::DECIMAL) {
			return new ValueExpr(*NextToken());
		}
		else if (currTok->type == Token::Type::IDENTIFIER) {
			auto nameTok = *NextToken();
			auto beginIdx = currTokIdx;

			if (GetToken()->type == Token::Type::OPEN_PARENTH) {
				NextToken();
				std::vector<Expression *> params;

				if(GetToken()->type == Token::Type::CLOSED_PARENTH){
					NextToken();
					return new FuncCallExpr(nameTok, std::vector<Expression *>());
				}
				while (true) {
					auto expr = ParseExpression();
					if (!expr) {
						std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid parameter n" << params.size() + 1 << "' at line " << GetToken()->row << "[" << GetToken()->col << "]\n";
						errCode = RespCode::ERR;
						for (auto param : params) delete param;

						return nullptr;
					}

					params.push_back(expr);
					if (GetToken()->type == Token::Type::COMMA) {
						NextToken();
						continue;
					}
					if (GetToken()->type != Token::Type::CLOSED_PARENTH) {
						std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid value '" << GetToken()->val << "' at line " << GetToken()->row << "[" << GetToken()->col << "]\n";
						errCode = RespCode::ERR;
						for (auto param : params) delete param;

						return nullptr;
					}

					NextToken();
					break;
				}
				return new FuncCallExpr(nameTok, params);
			}
			else if (GetToken()->type == Token::Type::DOT) {
				while (GetToken()->type == Token::Type::IDENTIFIER || GetToken()->type == Token::Type::DOT) {
					nameTok.val += GetToken()->val;
					NextToken();
				}

				if (GetToken()->type == Token::Type::OPEN_PARENTH) {
					NextToken();
					std::vector<Expression *> params;

					if (GetToken()->type == Token::Type::CLOSED_PARENTH) {
						NextToken();
						return new FuncCallExpr(nameTok, std::vector<Expression *>());
					}
					while (true) {
						auto expr = ParseExpression();
						if (!expr) {
							std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid parameter n" << params.size() + 1 << "' at line " << GetToken()->row << "[" << GetToken()->col << "]\n";
							errCode = RespCode::ERR;
							for (auto param : params) delete param;

							return nullptr;
						}

						params.push_back(expr);
						if (GetToken()->type == Token::Type::COMMA) {
							NextToken();
							continue;
						}
						if (GetToken()->type != Token::Type::CLOSED_PARENTH) {
							std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid value '" << GetToken()->val << "' at line " << GetToken()->row << "[" << GetToken()->col << "]\n";
							errCode = RespCode::ERR;
							for (auto param : params) delete param;

							return nullptr;
						}

						NextToken();
						break;
					}

					return new FuncCallExpr(nameTok, params);
				}

				return new ValueExpr(nameTok);
			}
			
			GoToIndex(beginIdx);
			return new ValueExpr(nameTok);
		}

		errCode = RespCode::ERR;
		return nullptr;
	}

	RespCode Module::ParseClass() {
		Token *tok;
		if ((tok = NextToken())->type != Token::Type::CLASS) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " Unexpected '" << tok->val << "' at line " << tok->row << "[" << tok->col << "]\n";
			return RespCode::ERR;
		}

		TypeInfo *type = new TypeInfo(nullptr, 0, "", 0, 0, 0, nullptr, true);

		auto idenTok = NextToken();
		if (idenTok->type != Token::Type::IDENTIFIER) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " Unexpected '" << tok->val << "' at line " << tok->row << "[" << tok->col << "]\n";
			delete type;
			return RespCode::ERR;
		}
		type->name = idenTok->val;

		size_t typeSz = 0;

		if ((tok = NextToken())->type != Token::Type::OPEN_BRACE) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " Unexpected '" << tok->val << "' at line " << tok->row << "[" << tok->col << "]\n";
			delete type;
			return RespCode::ERR;
		}
		
		auto scope = engine->GetScope();
		scope->RegisterType(type);
		while (true) {
			if (GetToken()->type == Token::Type::CLOSED_BRACE) break;

			auto beginIdx = currTokIdx;
			auto memberType = scope->FindTypeInfoByName((tok = NextToken())->val).data;
			if (!memberType || !memberType.has_value() || !memberType.value()) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " Invalid type '" << tok->val << "' at line " << tok->row << "[" << tok->col << "]\n";
				delete type;
				return RespCode::ERR;
			}

			auto name = NextToken();
			if (name->type != Token::Type::IDENTIFIER) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " Invalid value '" << tok->val << "' at line " << tok->row << "[" << tok->col << "]\n";
				delete type;
				return RespCode::ERR;
			}
			if ((tok = NextToken())->type == Token::Type::OPEN_PARENTH) {
				GoToIndex(beginIdx);
				auto funcStmt = ParseFuncDecl();
				if (!funcStmt) {
					std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " Method error\n";
					return RespCode::ERR;
				}
				auto newMethod = new ScriptFunc(
					name->val, 
					dynamic_cast<FuncStmt*>(funcStmt)->params.size(), dynamic_cast<FuncStmt *>(funcStmt), 
					memberType.value(), true
				);
				newMethod->func->funcScope->SetScopeType(Scope::Type::CLASS);
				type->AddMethod(name->val, newMethod);
				continue;
			}
			else if (tok->type != Token::Type::SEMICOLON) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " Invalid value '" << tok->val << "' at line " << tok->row << "[" << tok->col << "]\n";
				delete type;
				return RespCode::ERR;
			}
			typeSz += memberType.value()->Size();

			auto newType = new TypeInfo(
				engine, engine->GenerateTID(), 
				memberType.value()->GetName(), 
				memberType.value()->Size(), false, typeSz - memberType.value()->Size(), 
				type, memberType.value()->isClass
			);
			for (auto &[name, obj] : memberType.value()->members) {
				newType->members[name] = new TypeInfo(*obj);
			}
			type->AddMember(name->val, newType);
		}

		if ((tok = NextToken())->type != Token::Type::CLOSED_BRACE) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " Invalid value '" << tok->val << "' at line " << tok->row << "[" << tok->col << "]\n";
			delete type;
			return RespCode::ERR;
		}
		type->engine = engine;
		type->typeSz = typeSz;
		type->typeID = engine->GenerateTID();

		return RespCode::SUCCESS;
	}
	Statement *Module::ParseFuncCall() {
		if (GetToken()->type != Token::Type::IDENTIFIER) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Function name '" << GetToken()->val << "' invalid at line " << GetToken()->row << "[" << GetToken()->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		Token nameTok = *NextToken();
		

		while (GetToken()->type == Token::Type::IDENTIFIER || GetToken()->type == Token::Type::DOT) {
			nameTok.val += NextToken()->val;
		}

		Token *tok;
		if ((tok = NextToken())->type != Token::Type::OPEN_PARENTH) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Expected '(' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		std::vector<Expression *> params;
		if (GetToken()->type == Token::Type::CLOSED_PARENTH) {
			NextToken();
			return new FuncCallStmt(nameTok, params);
		}

		while (true) {
			auto expr = ParseExpression();
			if (!expr) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid parameter n" << params.size() + 1 << " at line " << GetToken()->row << "[" << GetToken()->col << "]\n";
				errCode = RespCode::ERR;
				for (auto param : params) delete param;

				return nullptr;
			}

			params.push_back(expr);
			if (GetToken()->type == Token::Type::SEMICOLON) {
				NextToken();
				continue;
			}
			if(GetToken()->type != Token::Type::CLOSED_PARENTH) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid value '" << GetToken()->val << " at line " << GetToken()->row << "[" << GetToken()->col << "]\n";
				errCode = RespCode::ERR;
				for (auto param : params) delete param;

				return nullptr;
			}

			NextToken();
			break;
		}

		return new FuncCallStmt(nameTok, params);
	}
	Statement *Module::ParseFuncDecl() {
		auto typeTok = NextToken();
		if (!(typeTok->type >= Token::Type::TYPES_BEGIN && typeTok->type <= Token::Type::TYPES_END) && typeTok->type != Token::Type::IDENTIFIER) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid type '" << typeTok->val << "' specified at line " << typeTok->row << "[" << typeTok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}
		auto idenTok = NextToken();
		if (idenTok->type != Token::Type::IDENTIFIER) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid name '" << idenTok->val << "' specified at line " << idenTok->row << "[" << idenTok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}
		Token *tok;
		if ((tok = NextToken())->type != Token::Type::OPEN_PARENTH) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Expected '(' specified at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}
		auto scope = engine->GetScope()->AddChild();
		scope->SetScopeType(Scope::Type::FUNCTION);

		engine->SetScope(scope);

		auto params = ParseParams();
		if (NextToken()->type != Token::Type::CLOSED_PARENTH) {
			engine->SetScope(scope->parent);
			scope->parent->DeleteChildScope(scope);

			errCode = RespCode::ERR;
			return nullptr;
		}

		auto retType = scope->FindTypeInfoByName(typeTok->val).data;
		if (!retType.has_value() || !retType.value()) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Type '" << typeTok->val << "' not found at line " << typeTok->row << "[" << typeTok->col << "]\n";

			errCode = RespCode::ERR;
			return nullptr;
		}
		auto block = ParseBlock();

		auto ret = new FuncStmt(params, block, *currTok, Token(Token::Type::IDENTIFIER, idenTok->val));
		ret->funcScope = scope;

		auto scriptFunc = new ScriptFunc(idenTok->val, params.size(), ret, retType.value());
		scope->parentFunc = scriptFunc;

		engine->SetScope(scope->parent);
		engine->GetScope()->RegisterFunc(scriptFunc);

		return ret;
	}
	Statement *Module::ParseWhile() {
		if (NextToken()->type != Token::Type::WHILE) {
			errCode = RespCode::ERR;
			return nullptr;
		}

		Token *tok;
		if ((tok = NextToken())->type != Token::Type::OPEN_PARENTH) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Expected '(' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		auto scope = engine->GetScope()->AddChild(engine->GetScope()->scopeType | static_cast<int>(Scope::Type::LOOP));
		engine->SetScope(scope);
		auto cond = ParseExpression();
		if ((tok = NextToken())->type != Token::Type::CLOSED_PARENTH) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Expected ')' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			delete cond;
			return nullptr;
		}

		auto then = ParseBlock();
		engine->SetScope(scope->parent);

		auto ret = new WhileStmt(cond, then);
		ret->scope = scope;

		return ret;
	}
	Statement *Module::ParseFor() {
		if (NextToken()->type != Token::Type::FOR) {
			errCode = RespCode::ERR;
			return nullptr;
		}
		Token *tok;
		if ((tok = NextToken())->type != Token::Type::OPEN_PARENTH) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Expected '(' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		auto parentScope = engine->GetScope();
		engine->SetScope(parentScope->AddChild(parentScope->scopeType | static_cast<int>(Scope::Type::LOOP)));

		auto first = ParseStatement();
		if ((tok = NextToken())->type != Token::Type::SEMICOLON) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Expected ';' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			delete first;
			return nullptr;
		}
		auto second = ParseExpression();
		if ((tok = NextToken())->type != Token::Type::SEMICOLON) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Expected ';' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			delete first;
			delete second;
			return nullptr;
		}
		auto third = ParseStatement();
		if ((tok = NextToken())->type != Token::Type::CLOSED_PARENTH) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Expected ')' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			delete first;
			delete second;
			delete third;
			return nullptr;
		}

		auto then = ParseBlock();
		engine->SetScope(parentScope);

		return new ForStmt(first, second, third, then);
	}
	Statement *Module::ParseIf() {
		if (NextToken()->type != Token::Type::IF) {
			errCode = RespCode::ERR;
			return nullptr;
		}
		Token *tok;
		if ((tok = NextToken())->type != Token::Type::OPEN_PARENTH) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Expected '(' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		auto condition = ParseExpression();
		if (!condition) {
			errCode = RespCode::ERR;
			return nullptr;
		}

		if ((tok = NextToken())->type != Token::Type::CLOSED_PARENTH) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Expected ')' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			delete condition;
			return nullptr;
		}

		auto thenScope = engine->GetScope()->AddChild(engine->GetScope()->scopeType);

		engine->SetScope(thenScope);
		auto then = ParseBlock();
		engine->SetScope(thenScope->parent);

		Statement *els = nullptr;
		Scope *elseScope = nullptr;
		if (GetToken()->type == Token::Type::ELSE) {
			NextToken();
			elseScope = engine->GetScope()->AddChild(engine->GetScope()->scopeType);

			engine->SetScope(elseScope);
			els = ParseBlock();
			engine->SetScope(elseScope->parent);
		}

		auto ret = new IfStmt(then, condition, els);
		ret->thenScope = thenScope;
		ret->elseScope = elseScope;

		return ret;
	}
	std::vector<Statement *>Module::ParseParams() {
		std::vector<Statement *> params;

		auto tok = GetToken();

		Statement *currParam = nullptr;
		while (true) {
			if (GetToken()->type == Token::Type::CLOSED_PARENTH) { break; }
			tok = NextToken();
			if (tok->type != Token::Type::IDENTIFIER && !(tok->type >= Token::Type::TYPES_BEGIN && tok->type <= Token::Type::TYPES_END)) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid value '" << tok->val << "' at line " << tok->row << "[" << tok->col << "]\n";
				return params;
			}
			if (tok->type == Token::Type::IDENTIFIER) {
				if (engine->GetScope()->FindTypeInfoByName(tok->val).code != RespCode::SUCCESS) {
					std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid type specified: '" << tok->val << "' at line " << tok->row << "[" << tok->col << "]\n";
					for (auto &param : params) delete param;
					errCode = RespCode::ERR;
					return {};
				}
			}

			auto typeTok = tok;
			tok = NextToken();
			if (tok->type != Token::Type::IDENTIFIER) { return params; }
			auto idenTok = tok;

			currParam = new VarDeclStmt(*typeTok, *idenTok, nullptr);

			if (!currParam) { return params; }

			params.push_back(currParam);
			auto typeFind = engine->GetScope()->FindTypeInfoByName(typeTok->val).data.value();
			auto obj = new ScriptObject(engine, typeFind);
			obj->identifier = idenTok->val;
			engine->GetScope()->RegisterObject(obj);

			currParam = nullptr;
			if (currTok->type == Token::Type::COMMA) { NextToken(); }
		}

		return params;
	}
	Statement *Module::ParseBlock() {
		if (errCode != RespCode::SUCCESS) return nullptr;
		auto currIdx = currTokIdx;
		auto tok = NextToken();

		if (tok->type != Token::Type::OPEN_BRACE) {
			GoToIndex(currIdx);

			return ParseStatement();
		}

		BlockStmt *stmt = new BlockStmt();
		Statement *subStmt = nullptr;
		while (true) {
			if (GetToken()->type == Token::Type::CLOSED_BRACE) {
				NextToken();
				break;
			}

			subStmt = ParseStatement();
			if (!subStmt) break;
			stmt->AddStatement(subStmt);
			if (
				subStmt->type == Statement::Type::ASSIGNEMENT ||
				subStmt->type == Statement::Type::VARDECL ||
				subStmt->type == Statement::Type::RETURN ||
				subStmt->type == Statement::Type::FUNCCALL) {
				NextToken();	// Semicolon
			}

			if (errCode != RespCode::SUCCESS) {
				break;
			}
		}

		return stmt;
	}
	Statement *Module::ParseVarDecl() {
		auto typeTok = NextToken();
		if (!(typeTok->type >= Token::Type::TYPES_BEGIN && typeTok->type <= Token::Type::TYPES_END) && 
			typeTok->type != Token::Type::UNSIGNED && typeTok->type != Token::Type::IDENTIFIER) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid type specified at line " << typeTok->row << "[" << typeTok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		// Deals with modifiers (UNSIGNED...)
		if (typeTok->type == Token::Type::UNSIGNED) {
			// Checks if its unsigned
			if (typeTok->type == Token::Type::UNSIGNED) {
				auto modifierToken = typeTok;
				typeTok = NextToken();
				if (!(typeTok->type >= Token::Type::TYPES_BEGIN && typeTok->type <= Token::Type::TYPES_END)) {
					std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid type specified at line " << typeTok->row << "[" << typeTok->col << "]\n";
					errCode = RespCode::ERR;
					return nullptr;
				}

				switch (typeTok->type) {
					case Token::Type::INT8:
						typeTok->type = Token::Type::UINT8;
						break;
					case Token::Type::INT16:
						typeTok->type = Token::Type::UINT16;
						break;
					case Token::Type::INT32:
						typeTok->type = Token::Type::UINT32;
						break;
					case Token::Type::INT64:
						typeTok->type = Token::Type::UINT64;
						break;
					default:
						std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid '" << modifierToken->val << "' before '" << typeTok->val << "'\nAt line " << modifierToken->row << "[" << modifierToken->col << "]\n";
						errCode = RespCode::ERR;
						return nullptr;
				}
			}
		}

		auto identTok = NextToken();
		if (identTok->type != Token::Type::IDENTIFIER) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid name '" << identTok->val << "' specified at line " << typeTok->row << "[" << typeTok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		Expression *expr = nullptr;

		if (GetToken()->type == Token::Type::ASSIGN) {
			NextToken();
			expr = ParseExpression();
		}

		return new VarDeclStmt(*typeTok, *identTok, expr);
	}
	Statement *Module::ParseVarAssign() {
		auto identTok = NextToken();
		if (identTok->type != Token::Type::IDENTIFIER) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid value '" << identTok->val << "' at line " << identTok->row << "[" << identTok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		auto assignType = NextToken();
		if (assignType->type != Token::Type::ASSIGN && assignType->type != Token::Type::DOT) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid value '" << assignType->val << "' at line " << assignType->row << "[" << assignType->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}
		else if (assignType->type == Token::Type::DOT) {
			auto newTok = *identTok;
			newTok.val += '.';
			Token *tok;
			while (true) {
				if ((tok = NextToken())->type != Token::Type::DOT && tok->type != Token::Type::IDENTIFIER) {
					break;
				}

				newTok.val += tok->val;
			}
			*identTok = newTok;
		}

		auto expr = ParseExpression();
		if (!expr) {
			errCode = RespCode::ERR;
			return nullptr;
		}

		return new VarAssignStmt(*identTok, expr);
	}
	Statement *Module::ParseStatement() {
		auto beginIdx = currTokIdx;	// Makes sure to go back to the beginning of the statement
		auto tok = GetToken();
		auto type = engine->GetScope()->FindTypeInfoByName(tok->val);
		if ((tok->type >= Token::Type::TYPES_BEGIN && tok->type <= Token::Type::TYPES_END) || 
			tok->type == Token::Type::UNSIGNED || type.code == RespCode::SUCCESS) {
			if (tok->type == Token::Type::CLASS) {
				if (ParseClass() != RespCode::SUCCESS) {
					std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Class declaration error at line " << tok->row << "[" << tok->col << "]\n";
					errCode = RespCode::ERR;
					return nullptr;
				}
				NextToken();	// Semicolon

				return ParseStatement();
			}
			NextToken();
			bool unsignedFlag = false;
			if (tok->type == Token::Type::UNSIGNED) {
				tok = NextToken();
				if (!(tok->type >= Token::Type::TYPES_BEGIN && tok->type <= Token::Type::TYPES_END)) {
					std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid type specified at line " << tok->row << "[" << tok->col << "]\n";
					errCode = RespCode::ERR;
					return nullptr;
				}
			}

			tok = NextToken();
			if (tok->type != Token::Type::IDENTIFIER) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid name '" << tok->val << "' specified at line " << tok->row << "[" << tok->col << "]\n";
				errCode = RespCode::ERR;
				return nullptr;
			}

			bool functionDecl = false;
			if (NextToken()->type == Token::Type::OPEN_PARENTH) { functionDecl = true; }

			GoToIndex(beginIdx);

			return (functionDecl ? ParseFuncDecl() : ParseVarDecl());
		}
		else if (tok->type == Token::Type::IDENTIFIER) {
			NextToken();
			while (GetToken()->type == Token::Type::IDENTIFIER || GetToken()->type == Token::Type::DOT) {
				NextToken();
			}
			if (NextToken()->type == Token::Type::OPEN_PARENTH) {
				GoToIndex(beginIdx);
				return ParseFuncCall();
			}

			GoToIndex(beginIdx);
			return ParseVarAssign();
		}
		else if (tok->type == Token::Type::IF) {
			return ParseIf();
		}
		else if (tok->type == Token::Type::WHILE) {
			return ParseWhile();
		}
		else if (tok->type == Token::Type::FOR) {
			return ParseFor();
		}
		else if (tok->type == Token::Type::RETURN) {
			NextToken();
			if (!engine->GetScope()->IsOfType(Scope::Type::FUNCTION)) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Return not in function at line " << tok->row << "[" << tok->col << "]\n";
				errCode = RespCode::ERR;
				return nullptr;
			}
			auto toRet = ParseExpression();
			return new ReturnStmt(toRet);
		}
		else if (tok->type == Token::Type::BREAK) {
			NextToken();
			if (!engine->GetScope()->IsOfType(Scope::Type::LOOP)) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Break not in loop at line " << tok->row << "[" << tok->col << "]\n";
				errCode = RespCode::ERR;
				return nullptr;
			}
			auto tok = NextToken();
			if (NextToken()->type != Token::Type::SEMICOLON) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Expected ';' specified at line " << tok->row << "[" << tok->col << "]\n";
				errCode = RespCode::ERR;
				return nullptr;
			}

			return new BreakStmt();
		}
		if (tok->type == Token::Type::END) return nullptr;

		std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " "  << "Invalid value '" << tok->val << "' at line " << tok->row << "[" << tok->col << "]\n";
		errCode = RespCode::ERR;
		return nullptr;
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

		if (dest->IsRef()) {
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
		engine->SetScope(stmt->funcScope->parent);

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
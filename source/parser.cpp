#include <marklang.h>
#include <iostream>
#include <unordered_set>

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
	static bool inMethod = false;
	static bool constMethod = false;

	// Tokenizer
	static const std::unordered_map<std::string, Token::Type> keywords = {
		{"void", Token::Type::VOID},

		{"bool", Token::Type::BOOL},
		{"char", Token::Type::INT8},
		{"short", Token::Type::INT16},
		{"int", Token::Type::INT32},
		{"long", Token::Type::INT64},

		{"float", Token::Type::FLOAT},
		{"double", Token::Type::DOUBLE},

		{"unsigned", Token::Type::UNSIGNED},
		{"const", Token::Type::CONST},

		{"enum", Token::Type::ENUM},
		{"class", Token::Type::CLASS},

		{"public", Token::Type::PUBLIC},
		{"protected", Token::Type::PROTECTED},
		{"private", Token::Type::PRIVATE},

		{"if", Token::Type::IF},
		{"else", Token::Type::ELSE},
		{"while", Token::Type::WHILE},
		{"for", Token::Type::FOR},
		{"break", Token::Type::BREAK},

		{"return", Token::Type::RETURN}
	};
	static const std::unordered_map<Token::Type, Token::Type> assignToOP = {{
		{ Token::Type::ASSIGN_PLUS, Token::Type::PLUS },
		{ Token::Type::ASSIGN_MINUS, Token::Type::MINUS },
		{ Token::Type::ASSIGN_MUL, Token::Type::STAR },
		{ Token::Type::ASSIGN_DIV, Token::Type::SLASH },
	}};

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
				if (lookahead == '=') {
					return Token(Token::Type::ASSIGN_PLUS, std::string{ code[currIdx++], code[currIdx++] }, currRow, currCol++);
				}
				return Token(Token::Type::PLUS, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '-': {
				if (lookahead == '=') {
					return Token(Token::Type::ASSIGN_MINUS, std::string{ code[currIdx++], code[currIdx++] }, currRow, currCol++);
				}
				return Token(Token::Type::MINUS, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '*': {
				if (lookahead == '=') {
					return Token(Token::Type::ASSIGN_MUL, std::string{ code[currIdx++], code[currIdx++] }, currRow, currCol++);
				}
				return Token(Token::Type::STAR, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '/': {
				if (lookahead == '=') {
					return Token(Token::Type::ASSIGN_DIV, std::string{ code[currIdx++], code[currIdx++] }, currRow, currCol++);
				}
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
			case ':': {
				return  Token(Token::Type::DOUBLECOLON, std::string{ code[currIdx++] }, currRow, currCol++);
			}
			case '=': {
				if (lookahead == '=') {
					return Token(Token::Type::EQ, std::string{ code[currIdx++], code[currIdx++] }, currRow, currCol++);
				}
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

		inMethod = true;
		TypeInfo::Visibility currentVisibility = TypeInfo::Visibility::PRIVATE;
		while (true) {
			if (GetToken()->type == Token::Type::CLOSED_BRACE) break;

			auto beginIdx = currTokIdx;

			tok = NextToken();

			// Deals with visibility
			switch (tok->type) {
				case Token::Type::PUBLIC:
					currentVisibility = TypeInfo::Visibility::PUBLIC;
					if ((tok = NextToken())->type != Token::Type::DOUBLECOLON) {
						std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected ':' at line " << tok->row << "[" << tok->col << "]\n";
					}
					beginIdx = currTokIdx;
					tok = NextToken();
					break;
				case Token::Type::PROTECTED:
					currentVisibility = TypeInfo::Visibility::PROTECTED;
					if ((tok = NextToken())->type != Token::Type::DOUBLECOLON) {
						std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected ':' at line " << tok->row << "[" << tok->col << "]\n";
					}
					beginIdx = currTokIdx;
					tok = NextToken();
					break;
				case Token::Type::PRIVATE:
					currentVisibility = TypeInfo::Visibility::PRIVATE;
					if ((tok = NextToken())->type != Token::Type::DOUBLECOLON) {
						std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected ':' at line " << tok->row << "[" << tok->col << "]\n";
					}
					beginIdx = currTokIdx;
					tok = NextToken();
					break;
			}

			if (tok->type == Token::Type::CONST) {
				tok = NextToken();
			}
			auto memberType = scope->FindTypeInfoByName(tok->val).data;
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
				dynamic_cast<FuncStmt *>(funcStmt)->funcScope->parentFunc->methodVisibility = currentVisibility;
				dynamic_cast<FuncStmt *>(funcStmt)->funcScope->parentFunc->isMethod = true;
				type->AddMethod(name->val, dynamic_cast<FuncStmt *>(funcStmt)->funcScope->parentFunc);

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
				newType->members[name] = obj;
			}
			for (auto &[name, method] : memberType.value()->methods) {
				newType->methods[name] = method;
			}
			newType->visibility = currentVisibility;

			type->AddMember(name->val, newType);
		}
		inMethod = false;

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
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected '(' at line " << tok->row << "[" << tok->col << "]\n";
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
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid parameter n" << params.size() + 1 << " at line " << GetToken()->row << "[" << GetToken()->col << "]\n";
				errCode = RespCode::ERR;
				for (auto param : params) delete param;

				return nullptr;
			}

			params.push_back(expr);
			if (GetToken()->type == Token::Type::SEMICOLON) {
				NextToken();
				continue;
			}
			if (GetToken()->type != Token::Type::CLOSED_PARENTH) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid value '" << GetToken()->val << " at line " << GetToken()->row << "[" << GetToken()->col << "]\n";
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
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid type '" << typeTok->val << "' specified at line " << typeTok->row << "[" << typeTok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}
		auto idenTok = NextToken();
		if (idenTok->type != Token::Type::IDENTIFIER) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid name '" << idenTok->val << "' specified at line " << idenTok->row << "[" << idenTok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}
		Token *tok;
		if ((tok = NextToken())->type != Token::Type::OPEN_PARENTH) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected '(' specified at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}
		auto scope = engine->GetScope()->AddChild(static_cast<int>(Scope::Type::FUNCTION));
		if (inMethod) { scope->SetScopeType(Scope::Type::CLASS); }

		engine->SetScope(scope);

		auto params = ParseParams();
		if (NextToken()->type != Token::Type::CLOSED_PARENTH) {
			engine->SetScope(scope->parent);
			scope->parent->DeleteChildScope(scope);

			errCode = RespCode::ERR;
			return nullptr;
		}

		bool isConst = false;
		if (GetToken()->type == Token::Type::CONST) {
			if (!inMethod) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Unexpected 'const' specified at line " << GetToken()->row << "[" << GetToken()->col << "]\n";
				engine->SetScope(scope->parent);
				scope->parent->DeleteChildScope(scope);

				errCode = RespCode::ERR;
				return nullptr;
			}

			NextToken();
			isConst = true;
		}

		auto retType = scope->FindTypeInfoByName(typeTok->val).data;
		if (!retType.has_value() || !retType.value()) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Type '" << typeTok->val << "' not found at line " << typeTok->row << "[" << typeTok->col << "]\n";

			errCode = RespCode::ERR;
			return nullptr;
		}

		bool lastConst = constMethod;
		constMethod = constMethod || isConst;
		auto block = ParseBlock();
		constMethod = lastConst;

		auto ret = new FuncStmt(params, block, *currTok, Token(Token::Type::IDENTIFIER, idenTok->val));
		ret->funcScope = scope;

		auto scriptFunc = new ScriptFunc(idenTok->val, params.size(), ret, retType.value(), inMethod, isConst);
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
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected '(' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		auto scope = engine->GetScope()->AddChild(engine->GetScope()->scopeType | static_cast<int>(Scope::Type::LOOP));
		engine->SetScope(scope);
		auto cond = ParseExpression();
		if ((tok = NextToken())->type != Token::Type::CLOSED_PARENTH) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected ')' at line " << tok->row << "[" << tok->col << "]\n";
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
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected '(' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		auto parentScope = engine->GetScope();
		engine->SetScope(parentScope->AddChild(parentScope->scopeType | static_cast<int>(Scope::Type::LOOP)));

		auto first = ParseStatement();
		if ((tok = NextToken())->type != Token::Type::SEMICOLON) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected ';' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			delete first;
			return nullptr;
		}
		auto second = ParseExpression();
		if ((tok = NextToken())->type != Token::Type::SEMICOLON) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected ';' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			delete first;
			delete second;
			return nullptr;
		}
		auto third = ParseStatement();
		if ((tok = NextToken())->type != Token::Type::CLOSED_PARENTH) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected ')' at line " << tok->row << "[" << tok->col << "]\n";
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
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected '(' at line " << tok->row << "[" << tok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		auto condition = ParseExpression();
		if (!condition) {
			errCode = RespCode::ERR;
			return nullptr;
		}

		if ((tok = NextToken())->type != Token::Type::CLOSED_PARENTH) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected ')' at line " << tok->row << "[" << tok->col << "]\n";
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
			if (tok->type != Token::Type::IDENTIFIER && 
				!(tok->type >= Token::Type::TYPES_BEGIN && tok->type <= Token::Type::TYPES_END) &&
				tok->type != Token::Type::CONST) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid value '" << tok->val << "' at line " << tok->row << "[" << tok->col << "]\n";
				return params;
			}
			if (tok->type == Token::Type::IDENTIFIER) {
				if (engine->GetScope()->FindTypeInfoByName(tok->val).code != RespCode::SUCCESS) {
					std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid type specified: '" << tok->val << "' at line " << tok->row << "[" << tok->col << "]\n";
					for (auto &param : params) delete param;
					errCode = RespCode::ERR;
					return {};
				}
			}

			bool constVar = false;
			if (tok->type == Token::Type::CONST) {
				constVar = true;
				tok = NextToken();
			}

			auto typeTok = tok;
			tok = NextToken();
			if (tok->type != Token::Type::IDENTIFIER) { return params; }
			auto idenTok = tok;

			currParam = new VarDeclStmt(*typeTok, *idenTok, nullptr);

			if (!currParam) { return params; }

			params.push_back(currParam);
			auto typeFind = engine->GetScope()->FindTypeInfoByName(typeTok->val).data.value();
			auto obj = new ScriptObject(engine, typeFind, (constVar ? ScriptObject::Modifier::CONST : static_cast<ScriptObject::Modifier>(0)));
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
		auto typeTok = GetToken();
		
		if (!(typeTok->type >= Token::Type::TYPES_BEGIN && typeTok->type <= Token::Type::TYPES_END) && typeTok->type != Token::Type::IDENTIFIER &&
			typeTok->type != Token::Type::UNSIGNED && typeTok->type != Token::Type::CONST) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid value '" << typeTok->val << "' specified at line " << typeTok->row << "[" << typeTok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		int mods = 0;
		bool isUnsigned = false;
		if (typeTok->type == Token::Type::UNSIGNED || typeTok->type == Token::Type::CONST) {
			while (typeTok->type == Token::Type::UNSIGNED || typeTok->type == Token::Type::CONST) {
				if (typeTok->type == Token::Type::UNSIGNED) {
					if (isUnsigned) {
						errCode = RespCode::ERR;
						return nullptr;
					}

					isUnsigned = true;

					continue;
				}

				switch (typeTok->type) {
					case Token::Type::CONST:
						mods |= static_cast<int>(ScriptObject::Modifier::CONST);
						break;

					default:
						errCode = RespCode::ERR;
						std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " <<
							"Invalid value '" << GetToken()->val << "' specified at line " << typeTok->row << "[" << typeTok->col << "]\n";
						return nullptr;
				}

				typeTok = NextToken();
			}
		}
		else {
			NextToken();
		}

		// Deals with modifiers (UNSIGNED...)
		if (isUnsigned) {
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
					std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " Invalid unsigned next to type" << "\n";//"Invalid '" << modifierToken->val << "' before '" << typeTok->val << "'\nAt line " << modifierToken->row << "[" << modifierToken->col << "]\n";
					errCode = RespCode::ERR;
					return nullptr;
			}
		}

		auto identTok = NextToken();
		if (identTok->type != Token::Type::IDENTIFIER) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid name '" << identTok->val << "' specified at line " << typeTok->row << "[" << typeTok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		Expression *expr = nullptr;

		if (GetToken()->type == Token::Type::ASSIGN) {
			NextToken();
			expr = ParseExpression();
		}
		else if (mods & static_cast<int>(ScriptObject::Modifier::CONST) && typeTok->type != Token::Type::IDENTIFIER) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Constant variable '" << identTok->val << "' uninitialized at line " << identTok->row << "[" << identTok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		auto ret = new VarDeclStmt(*typeTok, *identTok, expr);
		ret->modifiers = mods;
		return ret;
	}
	Statement *Module::ParseVarAssign() {
		auto identTok = NextToken();
		if (identTok->type != Token::Type::IDENTIFIER) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid value '" << identTok->val << "' at line " << identTok->row << "[" << identTok->col << "]\n";
			errCode = RespCode::ERR;
			return nullptr;
		}

		auto assignType = NextToken();
		if (
			assignType->type != Token::Type::ASSIGN && 
			assignType->type != Token::Type::ASSIGN_PLUS && 
			assignType->type != Token::Type::ASSIGN_MINUS &&
			assignType->type != Token::Type::ASSIGN_DIV &&
			assignType->type != Token::Type::ASSIGN_MUL &&
			assignType->type != Token::Type::DOT) {
			std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid value '" << assignType->val << "' at line " << assignType->row << "[" << assignType->col << "]\n";
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

		if (assignType->type != Token::Type::ASSIGN && assignType->type != Token::Type::DOT) {
			expr = new BinaryExpr(new ValueExpr(*identTok), Token(assignToOP.at(assignType->type)), expr);
		}

		return new VarAssignStmt(*identTok, expr);
	}
	Statement *Module::ParseStatement() {
		auto beginIdx = currTokIdx;	// Makes sure to go back to the beginning of the statement
		auto tok = GetToken();
		auto type = engine->GetScope()->FindTypeInfoByName(tok->val);
		if ((tok->type >= Token::Type::TYPES_BEGIN && tok->type <= Token::Type::TYPES_END) ||
			tok->type == Token::Type::UNSIGNED || tok->type == Token::Type::CONST || type.code == RespCode::SUCCESS) {
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
			if (tok->type == Token::Type::UNSIGNED || tok->type == Token::Type::CONST) {
				tok = NextToken();
				if (!(tok->type >= Token::Type::TYPES_BEGIN && tok->type <= Token::Type::TYPES_END) && tok->type != Token::Type::IDENTIFIER) {
					std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid type specified at line " << tok->row << "[" << tok->col << "]\n";
					errCode = RespCode::ERR;
					return nullptr;
				}
			}

			tok = NextToken();
			if (tok->type != Token::Type::IDENTIFIER) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid name '" << tok->val << "' specified at line " << tok->row << "[" << tok->col << "]\n";
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
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Return not in function at line " << tok->row << "[" << tok->col << "]\n";
				errCode = RespCode::ERR;
				return nullptr;
			}
			auto toRet = ParseExpression();
			return new ReturnStmt(toRet);
		}
		else if (tok->type == Token::Type::BREAK) {
			NextToken();
			if (!engine->GetScope()->IsOfType(Scope::Type::LOOP)) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Break not in loop at line " << tok->row << "[" << tok->col << "]\n";
				errCode = RespCode::ERR;
				return nullptr;
			}
			auto tok = NextToken();
			if (NextToken()->type != Token::Type::SEMICOLON) {
				std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Expected ';' specified at line " << tok->row << "[" << tok->col << "]\n";
				errCode = RespCode::ERR;
				return nullptr;
			}

			return new BreakStmt();
		}
		if (tok->type == Token::Type::END) return nullptr;

		std::cerr << __FUNCTION_NAME__ << " " << __LINE__ << " " << "Invalid value '" << tok->val << "' at line " << tok->row << "[" << tok->col << "]\n";
		errCode = RespCode::ERR;
		return nullptr;
	}
}
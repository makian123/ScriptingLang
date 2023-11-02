#pragma once

#include <string>
#include <vector>
#include <cstddef>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <optional>
#include <variant>
#include <string_view>

namespace mlang {
	class Module;
	class TypeInfo;
	class ScriptObject;
	class ScriptFunc;
	class ScriptRval;
	class Scope;

	enum class RespCode : int {
		ERR = -1,
		SUCCESS = 0,
	};
	template<typename T>
	struct Response {
		std::optional<T> data;
		RespCode code;

		Response(RespCode code_) : data{ std::nullopt },  code(code_) {}
		Response(T val, RespCode code_) : data{ val }, code(code_) {}
	};

	class Engine final {
		private:
		std::unordered_map<std::string, std::unique_ptr<Module>> modules;
		Scope *globalScope;
		Scope *currScope;
		size_t typeIndex = 0;

		public:
		Engine();
		~Engine();

		RespCode NewModule(const std::string &name);
		Response<Module*> GetModule(const std::string &name) const;
		RespCode DestroyModule(const std::string &name);

		Scope *GetScope() const;
		void SetScope(Scope *scope);

		RespCode RegisterGlobalType(const std::string &name, size_t size = 0, TypeInfo *classInfo = nullptr, size_t offset = 0);
		Response<size_t> GetTypeIdxByName(const std::string &name) const;
		Response<TypeInfo *> GetTypeInfoByIdx(size_t idx) const;

		size_t GenerateTID() { return typeIndex++; }
	};

	// AST nodes
	struct Token {
		enum class Type : int {
			IDENTIFIER,					// [a-zA-Z_]+([a-zA-Z0-9_]*)?

			LITERALS_BEGIN,
			INTEGER = LITERALS_BEGIN,	// [0-9]+
			DECIMAL,					// [0-9]+(.[0-9]+)?
			LITERALS_END = DECIMAL,

			TYPES_BEGIN,
			VOID = TYPES_BEGIN,			// void

			INT8,						// char
			INT16,						// short
			INT32,						// int
			INT64,						// long
			UINT8,						// unsigned char
			UINT16,						// unsigned short
			UINT32,						// unsigned int
			UINT64,						// unsigned long

			FLOAT,						// float
			DOUBLE,						// double

			ENUM,						// enum
			CLASS,						// class
			TYPES_END = CLASS,

			UNSIGNED,					// unsigned

			CONST,						// const

			SEMICOLON,					// ;
			COMMA,						// ,
			DOT,						// .

			IF,							// if
			ELSE,						// else
			WHILE,						// while
			FOR,						// for
			BREAK,						// break

			RETURN,						// return

			OPEN_PARENTH,				// (
			CLOSED_PARENTH,				// )
			OPEN_BRACE,					// {
			CLOSED_BRACE,				// }
			OPEN_BRACKET,				// [
			CLOSED_BRACKET,				// ]

			ASSIGN,						// =
			PLUS,						// +
			MINUS,						// -
			STAR,						// *
			SLASH,						// /
			NOT,						// !

			LESS,						// <
			GREATER,					// >
			LEQ,						// <=
			GEQ,						// >=
			NEQ,						// !=

			END
		};

		int row, col;
		Type type;
		std::string val;

		Token(Type type_ = Type::END, const std::string &val_ = "", int row_ = 0, int col_ = 0)
			:type(type_), val(val_), row(row_), col(col_) {}
	};
	struct Expression {
		enum class Type {
			VALUE,
			BINARY,
			UNARY,
			FUNCCALL
		};
		Type type;

		virtual ~Expression() = default;

		protected:
		Expression(Type type) : type(type) {}
	};
	struct BinaryExpr : public Expression {
		Expression *lhs;
		Expression *rhs;
		Token op;

		BinaryExpr(Expression *lhs_, const Token &op_, Expression *rhs_)
			:lhs(lhs_), rhs(rhs_), op(op_), Expression(Expression::Type::BINARY) {}
	};
	struct ValueExpr : public Expression {
		Token val;

		ValueExpr(const Token &val_)
			:val(val_), Expression(Expression::Type::VALUE) {}
	};
	struct FuncCallExpr : public Expression {
		Token funcName;
		std::vector<Expression *> params;

		FuncCallExpr(const Token &name, const std::vector<Expression*> &parameters)
			:funcName(name), params(parameters), Expression(Expression::Type::FUNCCALL) {}
	};
	
	struct Statement {
		enum class Type {
			VARDECL,
			FUNCDEF,
			FUNCCALL,
			ASSIGNEMENT,
			BLOCK,
			IF,
			WHILE,
			FOR,
			RETURN,
			BREAK,
		};
		Type type;

		virtual ~Statement() = default;

		protected:
		Statement(Type type) : type(type) {}
	};
	struct VarDeclStmt : public Statement {
		int modifiers = 0;
		Token type;
		Token ident;
		Expression *expr;

		VarDeclStmt(const Token &type_, const Token &ident_, Expression *expr_)
			:type(type_), ident(ident_), expr(expr_), Statement(Statement::Type::VARDECL) {}
		~VarDeclStmt() { delete expr; }
	};
	struct VarAssignStmt : public Statement {
		Token ident;
		Expression *expr;

		VarAssignStmt(const Token &ident_, Expression *expr_)
			:ident(ident_), expr(expr_), Statement(Statement::Type::ASSIGNEMENT) {}
		~VarAssignStmt() { delete expr; }
	};
	struct BlockStmt : public Statement {
		std::vector<Statement *> stmts;

		BlockStmt(): Statement(Statement::Type::BLOCK) {}
		~BlockStmt() { for(auto &stmt: stmts) delete stmt; }

		inline void AddStatement(Statement *stmt) { stmts.push_back(stmt); }
		inline void RemoveStatement(Statement *stmt) { 
			(void)std::remove_if(stmts.begin(), stmts.end(), [stmt](const Statement *stmt2) {return stmt2 == stmt; }); 
		}
	};
	struct FuncStmt: public Statement {
		std::vector<Statement *> params;
		Statement *block;
		Token ident;
		Token type;
		Scope *funcScope = nullptr;

		FuncStmt(const std::vector<Statement *> &params_, Statement *block_, const Token &retType, const Token &ident_)
			:params(params_), block(block_), type(retType), ident(ident_), Statement(Statement::Type::FUNCDEF) {}
		~FuncStmt() {
			for (auto &param : params) {
				delete param;
			}
			delete block;
		}
	};
	struct IfStmt : public Statement {
		Statement *then;
		Expression *condition;
		Statement *els;

		Scope *thenScope = nullptr, *elseScope = nullptr;

		IfStmt(Statement *then_, Expression *cond, Statement *els_)
			: then(then_), condition(cond), els(els_), Statement(Statement::Type::IF) {}
		~IfStmt() {
			delete then;
			delete condition;
			delete els;
		}
	};
	struct WhileStmt : public Statement {
		Expression *cond;
		Statement *then;
		Scope *scope = nullptr;

		WhileStmt(Expression *cond_, Statement *then_)
			:cond(cond_), then(then_), Statement(Statement::Type::WHILE) {}
		~WhileStmt() {
			delete cond;
			delete then;
		}
	};
	struct ForStmt : public Statement {
		Statement *start;
		Expression *cond;
		Statement *end;
		Statement *then;

		Scope *scope = nullptr;

		ForStmt(Statement *start_, Expression *cond_, Statement *end_, Statement *then_)
			:start(start_), cond(cond_), end(end_), then(then_), Statement(Statement::Type::FOR) {}
		~ForStmt() {
			delete start;
			delete cond;
			delete end;
			delete then;
		}
	};
	struct ReturnStmt : public Statement {
		Expression *val;

		ReturnStmt(Expression *val_) : val(val_), Statement(Statement::Type::RETURN) {}
		~ReturnStmt() { delete val; }
	};
	struct BreakStmt : public Statement {
		BreakStmt() : Statement(Statement::Type::BREAK) {}
	};
	struct FuncCallStmt : public Statement {
		Token funcName;
		std::vector<Expression*> params;

		FuncCallStmt(const Token &name, const std::vector<Expression*> &param)
			:funcName(name), params(param), Statement(Statement::Type::FUNCCALL) {}
		~FuncCallStmt() {
			for (auto param : params)
				delete param;
		}
	};

	class TypeInfo final {
		private:
		// General info
		size_t typeID;
		std::string name;
		size_t typeSz;
		bool unsig;
		Engine *engine;

		// Member info
		size_t offset;
		TypeInfo *parentClass;
		
		// Class info
		bool isClass;
		std::vector<TypeInfo *> baseClasses;
		std::unordered_map<std::string, TypeInfo *> members;
		std::unordered_map<std::string, ScriptFunc *> methods;

		public:
		friend class Engine;
		friend class Module;
		friend class Scope;
		friend class ScriptObject;

		TypeInfo(Engine *engine, size_t tID, const std::string &name, size_t sz, bool unsign = false, size_t off = 0, TypeInfo *parentClass_ = nullptr, bool isClass_ = false)
			: engine(engine), typeID(tID), name(name), typeSz(sz), unsig(unsign), offset(off), parentClass(parentClass_), isClass(isClass_) {}
		~TypeInfo();

		inline size_t TypeID() const { return typeID; }

		inline const std::string &GetName() const { return name; }

		inline size_t Size() const { return typeSz; }
		inline size_t Offset() const { return offset; }
		inline bool IsUnsigned() const { return unsig; }

		inline TypeInfo *GetParentClass() const { return parentClass; }
		bool IsBaseOf(const TypeInfo *type) const;
		inline bool IsClass() const { return isClass; }

		RespCode AddMember(const std::string &name, TypeInfo *type);
		inline std::optional<TypeInfo *> GetMember(const std::string &name) const {
			if (!members.contains(name)) return std::nullopt;
			return members.at(name);
		}

		RespCode AddMethod(const std::string &name, ScriptFunc *function);
		inline std::optional<ScriptFunc *> GetMethod(const std::string &name) const {
			if (!methods.contains(name)) return std::nullopt;
			return methods.at(name);
		}

		inline Engine *GetEngine() const { return engine; }
	};
	class ScriptObject final {
		public:
		enum class Modifier: int {
			CONST = (1 << 0),
			REFERENCE = (1 << 1)
		};

		private:
		TypeInfo *type = nullptr;
		Engine *engine;
		std::string identifier;
		void *ptr = nullptr;
		std::unordered_map<std::string, ScriptObject *> members;
		Scope *classScope = nullptr;
		bool shouldDealloc;
		size_t refCount = 1;
		Modifier modifiers;

		public:
		friend class Engine;
		friend class Module;
		friend class Scope;
		friend class TypeInfo;

		ScriptObject(Engine *engine, TypeInfo *type, Modifier mods = (Modifier)0, bool shouldAlloc = true);
		~ScriptObject();

		TypeInfo const *GetType() const { return type; }
		const std::string &GetName() const { return identifier; }

		std::optional<ScriptObject *> GetMember(const std::string &name) const;

		bool IsModifier(Modifier mod) const { return static_cast<int>(modifiers) & static_cast<int>(mod); }
		void SetType(Modifier mod) { modifiers = static_cast<Modifier>(static_cast<int>(modifiers) | static_cast<int>(mod)); }
		void UnsetType(Modifier mod) { modifiers = static_cast<Modifier>(static_cast<int>(modifiers) & ~static_cast<int>(mod)); }

		void SetAddress(void *ptr);
		void *GetAddressOfObj() const { return ptr; }

		Engine *GetEngine() const { return engine; }

		RespCode CallMethod(const std::string &name);
	};
	class ScriptFunc final {
		private:
		std::string name;
		size_t paramCount;

		FuncStmt *func = nullptr;
		ScriptObject *object = nullptr;

		TypeInfo *returnType;
		bool isMethod;

		public:
		friend class Engine;
		friend class Module;
		friend class Scope;

		ScriptFunc(const std::string &name, size_t params, FuncStmt *stmt = nullptr, TypeInfo *ret = nullptr, bool method = false)
			: name(name), paramCount(params), func(stmt), returnType(ret), isMethod(method) {}

		void SetClassObject(ScriptObject *obj);
		ScriptObject *GetScriptObject() const { return object; }

		const std::string &GetName() const { return name; }
		size_t GetParamCount() const { return paramCount; }
		FuncStmt *GetUnderlyingFunc() const { return func; }
	};
	class ScriptRval final {
		TypeInfo *valueType;
		void *data;

		public:
		friend class Engine;
		friend class Module;
		friend class Scope;
	};
	
	class Module final {
		private:
		struct Tokenizer {
			std::string code = "";
			int currRow = 1;
			int currCol = 1;
			size_t currIdx = 0;

			Tokenizer(const std::string &code_ = "") : code(code_) {}

			inline void AddCode(const std::string &code_) { code += code_; }
			Token Tokenize();
		};

		private:
		Tokenizer tokenizer;
		std::vector<Token> toks;
		size_t currTokIdx = 0;
		Token *currTok = nullptr;
		Engine *engine;
		RespCode errCode = RespCode::SUCCESS;

		std::string name = "";
		std::unique_ptr<BlockStmt> moduleStmts = nullptr;

		Token *NextToken();
		inline Token *GetToken() const { return currTok; }
		void GoToIndex(size_t idx) { currTokIdx = idx; currTok = &toks[idx]; }

		Expression *ParsePrimaryExpr();
		Expression *ParseExpression(int precedence = 0);

		std::optional<std::variant<ScriptFunc*, ScriptObject*>> NameResolution(const std::string &name);

		RespCode ParseClass();
		Statement *ParseBlock();
		std::vector<Statement *>ParseParams();
		Statement *ParseWhile();
		Statement *ParseFor();
		Statement *ParseIf();
		Statement *ParseFuncCall();
		Statement *ParseFuncDecl();
		Statement *ParseVarDecl();
		Statement *ParseVarAssign();
		Statement *ParseStatement();

		RespCode RunWhile(WhileStmt *stmt);
		RespCode RunFunc(FuncStmt *stmt, std::vector<Expression *> params);
		RespCode RunFuncCall(FuncCallStmt *stmt);
		RespCode RunReturn(ReturnStmt *stmt);
		RespCode RunIf(IfStmt *stmt);
		RespCode RunBlockStmt(BlockStmt *stmt);
		RespCode RunVarDeclStmt(VarDeclStmt *stmt);
		RespCode RunVarAssignStmt(VarAssignStmt *stmt);
		RespCode RunStmt(Statement *stmt);

		double EvaluateExpr(Scope *scope, Expression *expr);
		public:
		Module(Engine *engine, const std::string &name = "");

		inline void SetName(const std::string &name_) { name = name_; }
		inline const std::string &GetName() const { return name; }
		
		// Builds the AST
		RespCode Build();

		// Converts AST to scripting
		RespCode Run();

		RespCode AddSectionFromFile(const std::string &file);
		RespCode AddSectionFromMemory(const std::string &code);

		static RespCode CopyObjInto(ScriptObject *&dest, ScriptObject *src);
	};

	class Scope {
		public:
		enum class Type : int {
			FUNCTION = (1 << 0),
			LOOP = (1 << 1),
			CLASS = (1 << 2)
		};
		private:
		int scopeType;
		ScriptFunc *parentFunc = nullptr;
		std::vector<Scope *> children;
		std::vector<TypeInfo *> types;
		std::vector<ScriptObject *> objects;
		std::vector<ScriptFunc *> funcs;
		Engine *engine;
		Scope *parent;
		ScriptObject *returnObj = nullptr;

		public:
		friend class Module;
		friend class Engine;

		Scope(Engine *engine, Scope *parent = nullptr, int scopeType = 0);
		~Scope();

		Scope *AddChild(int type = 0);
		Scope *GetChild(size_t idx) const { return children[idx]; }
		size_t GetChildrenSize() const { return children.size(); }
		void DeleteChildScope(Scope *child);
		
		void SetParent(Scope *newParent) { parent = newParent; }
		Scope *GetParent() const { return parent; }

		void SetScopeType(Type type) { scopeType |= static_cast<int>(type); }
		bool IsOfType(Type type) const { return (scopeType & static_cast<int>(type)) != 0; }

		RespCode RegisterType(TypeInfo *type);
		RespCode RegisterObject(ScriptObject *obj);
		RespCode RegisterFunc(ScriptFunc *func);

		Response<TypeInfo*> FindTypeInfoByName(const std::string &name) const;
		Response<TypeInfo*> FindTypeInfoByID(size_t id) const;

		Response<ScriptObject*> FindObjectByName(const std::string &name) const;
		Response<ScriptFunc *> FindFuncByName(const std::string &name) const;

		void DebugPrint(int depth = 0) const;
	};
}
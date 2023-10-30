#include <marklang.h>
#include <iostream>

namespace mlang {
	Scope::Scope(Engine *engine, Scope *parent, int type)
		: engine(engine), parent(parent), scopeType(type), children() {}
	Scope::~Scope() {
		for (auto child : children) {
			delete child;
		}
		for (auto type : types) {
			delete type;
		}
		for (auto obj : objects) {
			delete obj;
		}
		for (auto func : funcs) {
			delete func;
		}
	}

	Scope *Scope::AddChild(int type) {
		children.push_back(new Scope(engine, this, scopeType | type));

		return children.back();
	}
	void Scope::DeleteChildScope(Scope *toFind) {
		for (auto child = children.cbegin(); child != children.cend(); ++child) {
			if (*child != toFind) continue;

			delete *child;
			children.erase(child);
			break;
		}
	}

	RespCode Scope::RegisterType(TypeInfo *type) {
		if (std::find(types.begin(), types.end(), type) != types.end()) {
			return mlang::RespCode::ERR;
		}
		types.push_back(type);

		return mlang::RespCode::SUCCESS;
	}
	RespCode Scope::RegisterObject(ScriptObject *obj) {
		if (std::find(objects.begin(), objects.end(), obj) != objects.end()) {
			return mlang::RespCode::ERR;
		}
		objects.push_back(obj);

		return mlang::RespCode::SUCCESS;
	}
	RespCode Scope::RegisterFunc(ScriptFunc *func) {
		funcs.push_back(func);

		return RespCode::SUCCESS;
	}

	Response<TypeInfo *> Scope::FindTypeInfoByName(const std::string &name) const {
		auto pos = std::find_if(types.begin(), types.end(), [name](const TypeInfo *type) {
			return type->GetName() == name;
		});

		if (pos != types.end()) {
			return Response(*pos, RespCode::SUCCESS);
		}

		if (parent) {
			return parent->FindTypeInfoByName(name);
		}

		return Response<TypeInfo *>(nullptr, RespCode::ERR);
	}
	Response<TypeInfo *> Scope::FindTypeInfoByID(size_t id) const {
		auto pos = std::find_if(types.begin(), types.end(), [id](const TypeInfo *type) {
			return type->TypeID() == id;
		});

		if (pos != types.end()) {
			return Response(*pos, RespCode::SUCCESS);
		}

		if (parent) {
			return parent->FindTypeInfoByID(id);
		}

		return Response<TypeInfo *>(nullptr, RespCode::ERR);
	}

	Response<ScriptObject *> Scope::FindObjectByName(const std::string &name) const {
		auto pos = std::find_if(objects.begin(), objects.end(), [name](const ScriptObject *obj) {
			return obj->GetName() == name;
		});

		if (pos != objects.end()) {
			return Response(*pos, RespCode::SUCCESS);
		}
		
		if (parent) {
			return parent->FindObjectByName(name);
		}

		return Response<ScriptObject *>(nullptr, RespCode::ERR);;
	}
	Response<ScriptFunc *> Scope::FindFuncByName(const std::string &name) const {
		auto pos = std::find_if(funcs.begin(), funcs.end(), [name](const ScriptFunc *func) {
			return func->GetName() == name;
		});

		if (pos != funcs.end()) {
			return Response(*pos, RespCode::SUCCESS);
		}

		if (parent) {
			return parent->FindFuncByName(name);
		}

		return Response<ScriptFunc *>(nullptr, RespCode::ERR);;
	}

	static void PrintTabs(int tabs) {
		while (tabs--) {
			std::cout << '\t';
		}
	}

	void Scope::DebugPrint(int depth) const {
		if (types.size()) {
			PrintTabs(depth);
			std::cout << "------TYPES------\n";
		}
		for (auto &type : types) {
			PrintTabs(depth);
			std::cout << "Type name: " << type->GetName() << ", size: " << type->Size() << "\n";
		}

		if (objects.size()) {
			PrintTabs(depth);
			std::cout << "------VARIABLES------\n";
			for (auto &obj : objects) {
				PrintTabs(depth);
				std::cout << "Object name: " << obj->GetName() << ", type name: '" << obj->GetType()->GetName() << "'\n";
			}
		}

		for (auto &child : children) {
			child->DebugPrint(depth + 1);
		}
	}
}
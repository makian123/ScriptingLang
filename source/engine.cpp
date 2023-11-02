#include <iostream>
#include <marklang.h>

namespace mlang {
	Engine::Engine() {
		globalScope = new Scope(this);
		currScope = globalScope;

		// Deals with primitives
		globalScope->RegisterType(new TypeInfo(this, GenerateTID(), "void", 0));

		globalScope->RegisterType(new TypeInfo(this, GenerateTID(), "char", 1));
		globalScope->RegisterType(new TypeInfo(this, GenerateTID(), "short", 2));
		globalScope->RegisterType(new TypeInfo(this, GenerateTID(), "int", 4));
		globalScope->RegisterType(new TypeInfo(this, GenerateTID(), "long", 8));

		globalScope->RegisterType(new TypeInfo(this, GenerateTID(), "unsigned char", 1, true));
		globalScope->RegisterType(new TypeInfo(this, GenerateTID(), "unsigned short", 2, true));
		globalScope->RegisterType(new TypeInfo(this, GenerateTID(), "unsigned int", 4, true));
		globalScope->RegisterType(new TypeInfo(this, GenerateTID(), "unsigned long", 8, true));

		globalScope->RegisterType(new TypeInfo(this, GenerateTID(), "float", 4));
		globalScope->RegisterType(new TypeInfo(this, GenerateTID(), "double", 8));
	}
	Engine::~Engine() {
		delete globalScope;
	}

	RespCode Engine::NewModule(const std::string &name) {
		if (modules.find(name) != modules.end()) {
			std::cout << "Module '" << name << "' already exists\n";
			return RespCode::ERR;
		}

		modules[name] = std::make_unique<Module>(this, name);

		return RespCode::SUCCESS;
	}
	Response<Module *> Engine::GetModule(const std::string &name) const {
		if (modules.find(name) == modules.end()) {
			std::cout << "Module '" << name << "' doesn't exist\n";

			return Response<Module *>(nullptr, RespCode::ERR);
		}

		return Response(modules.at(name).get(), RespCode::SUCCESS);
	}
	RespCode Engine::DestroyModule(const std::string &name) {
		if (modules.find(name) == modules.end()) {
			std::cout << "Module '" << name << "' doesn't exist\n";
			return RespCode::ERR;
		}

		modules.erase(name);
		return RespCode::SUCCESS;
	}

	Scope *Engine::GetScope() const {
		return currScope;
	}
	void Engine::SetScope(Scope *scope) {
		currScope = scope;
	}

	RespCode Engine::RegisterGlobalType(const std::string &name, size_t size, TypeInfo *classInfo, size_t offset) {
		if (globalScope->FindTypeInfoByName(name).code == RespCode::SUCCESS) {
			return RespCode::ERR;
		}

		globalScope->RegisterType(new TypeInfo(this, GenerateTID(), name, size, false, offset, classInfo));
		return RespCode::SUCCESS;
	}
	Response<size_t> Engine::GetTypeIdxByName(const std::string &name) const {
		auto data = globalScope->FindTypeInfoByName(name).data;
		if (!data) {
			return Response<size_t>(0, RespCode::ERR);
		}
		return Response<size_t>(data.value()->TypeID(), RespCode::SUCCESS);
	}
	Response<TypeInfo *> Engine::GetTypeInfoByIdx(size_t idx) const {
		return globalScope->FindTypeInfoByID(idx);
	}
}
#include <marklang.h>
#include <iostream>

namespace mlang {
	ScriptObject::ScriptObject(Engine *engine, TypeInfo *type, Modifier mods, bool alloc, ScriptObject *parentClass)
		: engine(engine), type(type), modifiers(mods), shouldDealloc(alloc), parentClass(parentClass) {
		if (!IsModifier(Modifier::REFERENCE)) {
			if (alloc) {
				ptr = new char[type->Size()];
			}

			if (type->IsClass()) {
				classScope = engine->GetScope()->AddChild(static_cast<int>(Scope::Type::CLASS));
				for (auto &[memberName, memberType] : type->members) {
					members[memberName] = new ScriptObject(engine, memberType, static_cast<Modifier>(0), false, parentClass);
					if (alloc) {
						members[memberName]->SetAddress(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) + memberType->Offset()));
					}
					members[memberName]->identifier = memberName;
					members[memberName]->parentClass = this;

					classScope->RegisterObject(members[memberName]);
				}
			}
			refCount = 1;
		}
	}
	ScriptObject::~ScriptObject() {
		if (refCount) { refCount--; }
		if (!IsModifier(Modifier::REFERENCE) && !refCount) {
			for (auto &[ident, member] : members) {
				//allocator.destroy(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) + member->type->Offset()));
			}
			if (shouldDealloc) {
				delete ptr;
			}
		}
	}

	void ScriptObject::SetAddress(void *newPtr) {
		ptr = newPtr;

		for (auto &[name, member] : members) {
			member->SetAddress(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + member->type->offset));
		}
	}

	std::optional<ScriptObject *> ScriptObject::GetMember(const std::string &name) const {
		if (!members.contains(name)) return std::nullopt;
		return members.at(name);
	}

	RespCode ScriptObject::CallMethod(const std::string &name) {
		auto method = type->GetMethod(name);
		if (!method) return RespCode::ERR;

		return RespCode::SUCCESS;
	}
}
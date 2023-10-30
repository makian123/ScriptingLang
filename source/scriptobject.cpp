#include <marklang.h>
#include <iostream>

namespace mlang {
	ScriptObject::ScriptObject(Engine *engine, TypeInfo *type, bool reference, bool alloc)
		: engine(engine), type(type), isRef(reference), shouldDealloc(alloc) {
		if (!isRef) {
			if (alloc) {
				ptr = new char[type->Size()];
			}

			if (type->IsClass()) {
				classScope = engine->GetScope()->AddChild(static_cast<int>(Scope::Type::CLASS));
				for (auto &[memberName, memberType] : type->members) {
					members[memberName] = new ScriptObject(engine, memberType, false, false);
					if (alloc) {
						members[memberName]->SetAddress(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) + memberType->Offset()));
					}
					members[memberName]->identifier = memberName;

					classScope->RegisterObject(members[memberName]);
				}
			}
			refCount = 1;
		}
	}
	ScriptObject::~ScriptObject() {
		if (refCount) { refCount--; }
		if (!isRef && !refCount) {
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
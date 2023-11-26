#include <marklang.h>
#include <cassert>
#include <iostream>
#include <cstring>

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
	ScriptObject::ScriptObject(Engine *engine, TypeInfo *type, ScriptRval &rvalue, Modifier mods, bool alloc, ScriptObject *parentClass)
		: ScriptObject(engine, type, mods, alloc, parentClass){
		SetVal(rvalue);
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

	RespCode ScriptObject::SetVal(ScriptRval &value) {
		if (type->isClass ^ value.valueType->isClass) {
			return RespCode::ERR;
		}

		if (value.reference) {
			if (IsModifier(ScriptObject::Modifier::REFERENCE)) {
				ptr = value.data;
				return RespCode::SUCCESS;
			}

			auto retVal = SetVal(reinterpret_cast<const ScriptObject *>(value.data));

			return retVal;
		}

		if (type->isClass) {
			for (auto &[name, type] : value.valueType->members) {
				if (members.at(name)->SetVal(reinterpret_cast<ScriptObject *>(reinterpret_cast<size_t>(value.data) + value.valueType->members.at(name)->offset)) != RespCode::SUCCESS) {
					return RespCode::ERR;
				}
			}

			return RespCode::SUCCESS;
		}

		bool thisFloat = type->name == "float" || type->name == "double";
		bool otherFloat = value.valueType->name == "float" || value.valueType->name == "double";

		if (thisFloat == otherFloat) {
			if (thisFloat) {
				double srcVal = 0.0;
				switch (value.valueType->Size()) {
					case 4:
						srcVal = static_cast<double>(*reinterpret_cast<float *>(value.data));
						break;
					case 8:
						srcVal = static_cast<double>(*reinterpret_cast<float *>(value.data));
						break;
				}

				switch (type->Size()) {
					case 4:
						*reinterpret_cast<float *>(ptr) = static_cast<float>(srcVal);
						break;
					case 8:
						*reinterpret_cast<double *>(ptr) = static_cast<double>(srcVal);
						break;
				}

				delete value.data;
				value.data = nullptr;
				return RespCode::SUCCESS;
			}

			uint64_t srcVal = 0;
			switch (value.valueType->Size()) {
				case 1:
					srcVal = *reinterpret_cast<int8_t *>(value.data);
					break;
				case 2:
					srcVal = *reinterpret_cast<int16_t *>(value.data);
					break;
				case 4:
					srcVal = *reinterpret_cast<uint32_t *>(value.data);
					break;
				case 8:
					srcVal = *reinterpret_cast<uint64_t *>(value.data);
					break;
			}

			switch (type->Size()) {
				case 1:
					*reinterpret_cast<uint8_t *>(ptr) = static_cast<uint8_t>(srcVal);
					break;
				case 2:
					*reinterpret_cast<uint16_t *>(ptr) = static_cast<uint16_t>(srcVal);
					break;
				case 4:
					*reinterpret_cast<uint32_t *>(ptr) = static_cast<uint32_t>(srcVal);
					break;
				case 8:
					*reinterpret_cast<uint64_t *>(ptr) = static_cast<uint64_t>(srcVal);
					break;
			}

			return RespCode::SUCCESS;
		}
		else if (otherFloat) {
			float *floatPtr = reinterpret_cast<float *>(value.data);
			double *doublePtr = reinterpret_cast<double *>(value.data);

			switch (this->type->Size()) {
				case 1:
					*reinterpret_cast<uint8_t *>(ptr) = static_cast<uint8_t>((value.valueType->Size() == 4 ? *floatPtr : *doublePtr));
					break;
				case 2:
					*reinterpret_cast<uint16_t *>(ptr) = static_cast<uint16_t>((value.valueType->Size() == 4 ? *floatPtr : *doublePtr));
					break;
				case 4:
					*reinterpret_cast<uint32_t *>(ptr) = static_cast<uint32_t>((value.valueType->Size() == 4 ? *floatPtr : *doublePtr));
					break;
				case 8:
					*reinterpret_cast<uint64_t *>(ptr) = static_cast<uint64_t>((value.valueType->Size() == 4 ? *floatPtr : *doublePtr));
					break;
			}
		}
		else {
			uint64_t intSrc = 0;
			switch (value.valueType->Size()) {
				case 1:
					intSrc = *reinterpret_cast<uint8_t *>(value.data);
					break;
				case 2:
					intSrc = *reinterpret_cast<uint16_t *>(value.data);
					break;
				case 4:
					intSrc = *reinterpret_cast<uint32_t *>(value.data);
					break;
				case 8:
					intSrc = *reinterpret_cast<uint64_t *>(value.data);
					break;
			}

			if (type->Size() == 4)
				*static_cast<float *>(ptr) = static_cast<float>(value.valueType->unsig ? intSrc : static_cast<int64_t>(intSrc));
			else
				*static_cast<double *>(ptr) = static_cast<double>(value.valueType->unsig ? intSrc : static_cast<int64_t>(intSrc));
		}
		
		return RespCode::SUCCESS;
	}
	RespCode ScriptObject::SetVal(const ScriptObject *value) {
		if (type->isClass ^ value->type->isClass) {
			return RespCode::ERR;
		}

		if (type->isClass) {
			for (auto &[name, obj] : value->members) {
				if ((members[name]->SetVal(obj) != RespCode::SUCCESS)) {
					return RespCode::ERR;
				}
			}
			return RespCode::SUCCESS;
		}

		bool thisFloat = type->name == "float" || type->name == "double";
		bool otherFloat = value->type->name == "float" || value->type->name == "double";

		if (thisFloat == otherFloat) {
			if (thisFloat) {
				double srcVal = 0.0;
				switch (value->type->Size()) {
					case 4:
						srcVal = static_cast<double>(*reinterpret_cast<float *>(value->ptr));
						break;
					case 8:
						srcVal = static_cast<double>(*reinterpret_cast<float *>(value->ptr));
						break;
				}

				switch (type->Size()) {
					case 4:
						*reinterpret_cast<float *>(ptr) = static_cast<float>(srcVal);
						break;
					case 8:
						*reinterpret_cast<double *>(ptr) = static_cast<double>(srcVal);
						break;
				}

				return RespCode::SUCCESS;
			}

			uint64_t srcVal = 0;
			switch (value->type->Size()) {
				case 1:
					srcVal = *reinterpret_cast<int8_t*>(value->ptr);
					break;
				case 2:
					srcVal = *reinterpret_cast<int16_t *>(value->ptr);
					break;
				case 4:
					srcVal = *reinterpret_cast<uint32_t *>(value->ptr);
					break;
				case 8:
					srcVal = *reinterpret_cast<uint64_t *>(value->ptr);
					break;
			}

			switch (type->Size()) {
				case 1:
					*reinterpret_cast<uint8_t *>(ptr) = static_cast<uint8_t>(srcVal);
					break;
				case 2:
					*reinterpret_cast<uint16_t *>(ptr) = static_cast<uint16_t>(srcVal);
					break;
				case 4:
					*reinterpret_cast<uint32_t *>(ptr) = static_cast<uint32_t>(srcVal);
					break;
				case 8:
					*reinterpret_cast<uint64_t *>(ptr) = static_cast<uint64_t>(srcVal);
					break;
			}

			return RespCode::SUCCESS;
		}
		else if (!thisFloat) {
			float *floatPtr = reinterpret_cast<float *>(value->ptr);
			double *doublePtr = reinterpret_cast<double *>(value->ptr);

			switch (this->type->Size()) {
				case 1:
					*reinterpret_cast<uint8_t *>(ptr) = static_cast<uint8_t>((value->type->Size() == 4 ? *floatPtr : *doublePtr));
					break;
				case 2:
					*reinterpret_cast<uint16_t *>(ptr) = static_cast<uint16_t>((value->type->Size() == 4 ? *floatPtr : *doublePtr));
					break;
				case 4:
					*reinterpret_cast<uint32_t *>(ptr) = static_cast<uint32_t>((value->type->Size() == 4 ? *floatPtr : *doublePtr));
					break;
				case 8:
					*reinterpret_cast<uint64_t *>(ptr) = static_cast<uint64_t>((value->type->Size() == 4 ? *floatPtr : *doublePtr));
					break;
			}
		}
		else if (otherFloat) {
			uint64_t intSrc = 0;
			switch (value->type->Size()) {
				case 1:
					intSrc = *reinterpret_cast<uint8_t *>(value->ptr);
					break;
				case 2:
					intSrc = *reinterpret_cast<uint16_t *>(value->ptr);
					break;
				case 4:
					intSrc = *reinterpret_cast<uint32_t *>(value->ptr);
					break;
				case 8:
					intSrc = *reinterpret_cast<uint64_t *>(value->ptr);
					break;
			}

			if (type->Size() == 4)
				*static_cast<float *>(ptr) = static_cast<float>(value->type->unsig ? intSrc : static_cast<int64_t>(intSrc));
			else
				*static_cast<double *>(ptr) = static_cast<double>(value->type->unsig ? intSrc : static_cast<int64_t>(intSrc));
		}


		return RespCode::ERR;
	}

	ScriptObject *ScriptObject::Clone(ScriptObject *original) {
		auto ret = new ScriptObject(original->engine, original->type, original->modifiers, original->shouldDealloc, original->parentClass);
		ret->classScope = original->classScope;

		if (original->type->name == "char" ||
			original->type->name == "short" ||
			original->type->name == "int" ||
			original->type->name == "long" ||
			original->type->name == "float" ||
			original->type->name == "double") {
			ret->SetAddress(new char[original->type->Size()]);
			std::memcpy(ret->GetAddressOfObj(), original->GetAddressOfObj(), original->type->Size());

			return ret;
		}

		for (auto &[name, member] : original->members) {
			ret->members[name] = Clone(member);
		}

		return ret;
	}
}
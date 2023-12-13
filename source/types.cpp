#include <marklang.h>

namespace mlang {
	TypeInfo::TypeInfo(const TypeInfo *other)
		:typeID(other->engine->GenerateTID()), name(other->name), typeSz(other->typeID),
		unsig(other->unsig), engine(other->engine), offset(other->offset), parentClass(other->parentClass),
		isClass(other->isClass), baseClasses(other->baseClasses),
		methods(other->methods), members(other->members), visibility(other->visibility) {}
	TypeInfo::~TypeInfo() {}

	bool TypeInfo::IsBaseOf(const TypeInfo *base) const {
		if (std::find(baseClasses.begin(), baseClasses.end(), base) != baseClasses.end()) {
			return true;
		}

		for (auto type : baseClasses) {
			if (type->IsBaseOf(base)) {
				return true;
			}
		}

		return false;
	}

	RespCode TypeInfo::AddMember(const std::string &name, TypeInfo *type) {
		if (!type) return RespCode::ERR;
		if (members.contains(name)) return RespCode::ERR;

		members[name] = type;

		return RespCode::SUCCESS;
	}
	RespCode TypeInfo::AddMethod(const std::string &name, ScriptFunc *type) {
		if (!type) return RespCode::ERR;
		if (methods.contains(name)) return RespCode::ERR;

		methods[name] = type;

		return RespCode::SUCCESS;
	}
}
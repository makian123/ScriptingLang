#include <marklang.h>

namespace mlang {
	TypeInfo::~TypeInfo() {
		for (auto &member : members) {
			delete member.second;
		}
	}

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
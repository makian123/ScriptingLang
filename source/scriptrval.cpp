#include <marklang.h>
#include <stdexcept>
#include <utility>

namespace mlang {
	ScriptRval ScriptRval::CreateFromLiteral(Engine *engine, const std::string &data) {
		ScriptRval ret(engine, false);

		if (data.find('.') != std::string::npos) {
			try {
				ret.valueType = engine->GetTypeInfoByName("float");
				ret.data = new float(std::stof(data));
			}
			catch (std::out_of_range&) {
				try {
					ret.valueType = engine->GetTypeInfoByName("double");
					ret.data = new double(std::stod(data));
				}
				catch (std::out_of_range&) {
					throw std::exception("Invalid literal size");
				}
			}
			catch (...) {
				throw std::exception("Invalid value found");
			}
		}
		else {
			try {
				ret.valueType = engine->GetTypeInfoByName("int");
				ret.data = new int32_t(std::stoi(data));
			}
			catch (std::out_of_range&) {
				try {
					ret.valueType = engine->GetTypeInfoByName("long");
					ret.data = new int64_t(std::stoll(data));
				}
				catch (std::out_of_range&) {
					throw std::exception("Invalid literal size");
				}
			}
		}

		return ret;
	}

	ScriptRval ScriptRval::operator+(const ScriptRval &other) const {
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		auto maxType = (valueType->Size() > other.valueType->Size() ? valueType : other.valueType);

		if (maxType->GetName() != "float" && maxType->GetName() != "double") {
			if (valueType->GetName() == "float" || valueType->GetName() == "double")
				maxType = valueType;
			else if (other.valueType->GetName() == "float" || other.valueType->GetName() == "double")
				maxType = other.valueType;
		}
		
		if (maxType->GetName() == "float" || maxType->GetName() == "double") {
			double value = 0.0f;

			if (valueType->GetName() == "float") {
				value = *reinterpret_cast<float*>(data);
			}
			else if (valueType->GetName() == "double") {
				value = *reinterpret_cast<double *>(data);
			}
			else {
				switch (valueType->Size()) {
					case 1:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
						break;
					case 2:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(data) : *reinterpret_cast<int16_t *>(data));
						break;
					case 4:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(data) : *reinterpret_cast<int32_t *>(data));
						break;
					case 8:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(data) : *reinterpret_cast<int64_t *>(data));
						break;
				}
			}

			if (other.valueType->GetName() == "float") {
				value += *reinterpret_cast<float *>(other.data);
			}
			else if (other.valueType->GetName() == "double") {
				value += *reinterpret_cast<double *>(other.data);
			}
			else {
				switch (other.valueType->Size()) {
					case 1:
						value += (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
						break;
					case 2:
						value += (other.valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(other.data) : *reinterpret_cast<int16_t *>(other.data));
						break;
					case 4:
						value += (other.valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(other.data) : *reinterpret_cast<int32_t *>(other.data));
						break;
					case 8:
						value += (other.valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(other.data) : *reinterpret_cast<int64_t *>(other.data));
						break;
				}
			}

			if (maxType->GetName() == "float") {
				return ScriptRval::Create<float>(engine, maxType, value);
			}
			return ScriptRval::Create<double>(engine, maxType, value);
		}
		
		uint64_t val = 0;
		int64_t valSigned = 0;

		switch (valueType->Size()) {
			case 1:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
			case 2:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
			case 4:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
			case 8:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
		}

		switch (other.valueType->Size()) {
			case 1:
				val += (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
			case 2:
				val += (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
			case 4:
				val += (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
			case 8:
				val += (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
		}

		if (maxType->IsUnsigned()) {
			switch (maxType->Size()) {
				case 1:
					return ScriptRval::Create<uint8_t>(engine, maxType, val);
				case 2:
					return ScriptRval::Create<uint16_t>(engine, maxType, val);
				case 4:
					return ScriptRval::Create<uint32_t>(engine, maxType, val);
				case 8:
					return ScriptRval::Create<uint64_t>(engine, maxType, val);
			}
		}

		switch (maxType->Size()) {
			case 1:
				return ScriptRval::Create<int8_t>(engine, maxType, val);
			case 2:
				return ScriptRval::Create<int16_t>(engine, maxType, val);
			case 4:
				return ScriptRval::Create<int32_t>(engine, maxType, val);
			case 8:
				return ScriptRval::Create<int64_t>(engine, maxType, val);
			default:
				throw std::exception("Invalid value type deduced");
		}
	}
	ScriptRval ScriptRval::operator-(const ScriptRval &other) const {
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		auto maxType = (valueType->Size() > other.valueType->Size() ? valueType : other.valueType);

		if (maxType->GetName() != "float" && maxType->GetName() != "double") {
			if (valueType->GetName() == "float" || valueType->GetName() == "double")
				maxType = valueType;
			else if (other.valueType->GetName() == "float" || other.valueType->GetName() == "double")
				maxType = other.valueType;
		}

		if (maxType->GetName() == "float" || maxType->GetName() == "double") {
			double value = 0.0f;

			if (valueType->GetName() == "float") {
				value = *reinterpret_cast<float *>(data);
			}
			else if (valueType->GetName() == "double") {
				value = *reinterpret_cast<double *>(data);
			}
			else {
				switch (valueType->Size()) {
					case 1:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
						break;
					case 2:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(data) : *reinterpret_cast<int16_t *>(data));
						break;
					case 4:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(data) : *reinterpret_cast<int32_t *>(data));
						break;
					case 8:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(data) : *reinterpret_cast<int64_t *>(data));
						break;
				}
			}

			if (other.valueType->GetName() == "float") {
				value -= *reinterpret_cast<float *>(other.data);
			}
			else if (other.valueType->GetName() == "double") {
				value -= *reinterpret_cast<double *>(other.data);
			}
			else {
				switch (other.valueType->Size()) {
					case 1:
						value -= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
						break;
					case 2:
						value -= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(other.data) : *reinterpret_cast<int16_t *>(other.data));
						break;
					case 4:
						value -= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(other.data) : *reinterpret_cast<int32_t *>(other.data));
						break;
					case 8:
						value -= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(other.data) : *reinterpret_cast<int64_t *>(other.data));
						break;
				}
			}

			if (maxType->GetName() == "float") {
				return ScriptRval::Create<float>(engine, maxType, value);
			}
			return ScriptRval::Create<double>(engine, maxType, value);
		}

		uint64_t val = 0;
		int64_t valSigned = 0;

		switch (valueType->Size()) {
			case 1:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
			case 2:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
			case 4:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
			case 8:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
		}

		switch (other.valueType->Size()) {
			case 1:
				val -= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
			case 2:
				val -= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
			case 4:
				val -= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
			case 8:
				val -= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
		}

		if (maxType->IsUnsigned()) {
			switch (maxType->Size()) {
				case 1:
					return ScriptRval::Create<uint8_t>(engine, maxType, val);
				case 2:
					return ScriptRval::Create<uint16_t>(engine, maxType, val);
				case 4:
					return ScriptRval::Create<uint32_t>(engine, maxType, val);
				case 8:
					return ScriptRval::Create<uint64_t>(engine, maxType, val);
			}
		}

		switch (maxType->Size()) {
			case 1:
				return ScriptRval::Create<int8_t>(engine, maxType, val);
			case 2:
				return ScriptRval::Create<int16_t>(engine, maxType, val);
			case 4:
				return ScriptRval::Create<int32_t>(engine, maxType, val);
			case 8:
				return ScriptRval::Create<int64_t>(engine, maxType, val);
			default:
				throw std::exception("Invalid value type deduced");
		}
	}
	ScriptRval ScriptRval::operator*(const ScriptRval &other) const {
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		auto maxType = (valueType->Size() > other.valueType->Size() ? valueType : other.valueType);

		if (maxType->GetName() != "float" && maxType->GetName() != "double") {
			if (valueType->GetName() == "float" || valueType->GetName() == "double")
				maxType = valueType;
			else if (other.valueType->GetName() == "float" || other.valueType->GetName() == "double")
				maxType = other.valueType;
		}

		if (maxType->GetName() == "float" || maxType->GetName() == "double") {
			double value = 0.0f;

			if (valueType->GetName() == "float") {
				value = *reinterpret_cast<float *>(data);
			}
			else if (valueType->GetName() == "double") {
				value = *reinterpret_cast<double *>(data);
			}
			else {
				switch (valueType->Size()) {
					case 1:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
						break;
					case 2:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(data) : *reinterpret_cast<int16_t *>(data));
						break;
					case 4:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(data) : *reinterpret_cast<int32_t *>(data));
						break;
					case 8:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(data) : *reinterpret_cast<int64_t *>(data));
						break;
				}
			}

			if (other.valueType->GetName() == "float") {
				value *= *reinterpret_cast<float *>(other.data);
			}
			else if (other.valueType->GetName() == "double") {
				value *= *reinterpret_cast<double *>(other.data);
			}
			else {
				switch (other.valueType->Size()) {
					case 1:
						value *= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
						break;
					case 2:
						value *= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(other.data) : *reinterpret_cast<int16_t *>(other.data));
						break;
					case 4:
						value *= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(other.data) : *reinterpret_cast<int32_t *>(other.data));
						break;
					case 8:
						value *= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(other.data) : *reinterpret_cast<int64_t *>(other.data));
						break;
				}
			}

			if (maxType->GetName() == "float") {
				return ScriptRval::Create<float>(engine, maxType, value);
			}
			return ScriptRval::Create<double>(engine, maxType, value);
		}

		uint64_t val = 0;
		int64_t valSigned = 0;

		switch (valueType->Size()) {
			case 1:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
			case 2:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
			case 4:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
			case 8:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
		}

		switch (other.valueType->Size()) {
			case 1:
				val *= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
			case 2:
				val *= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
			case 4:
				val *= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
			case 8:
				val *= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
		}

		if (maxType->IsUnsigned()) {
			switch (maxType->Size()) {
				case 1:
					return ScriptRval::Create<uint8_t>(engine, maxType, val);
				case 2:
					return ScriptRval::Create<uint16_t>(engine, maxType, val);
				case 4:
					return ScriptRval::Create<uint32_t>(engine, maxType, val);
				case 8:
					return ScriptRval::Create<uint64_t>(engine, maxType, val);
			}
		}

		switch (maxType->Size()) {
			case 1:
				return ScriptRval::Create<int8_t>(engine, maxType, val);
			case 2:
				return ScriptRval::Create<int16_t>(engine, maxType, val);
			case 4:
				return ScriptRval::Create<int32_t>(engine, maxType, val);
			case 8:
				return ScriptRval::Create<int64_t>(engine, maxType, val);
			default:
				throw std::exception("Invalid value type deduced");
		}
	}
	ScriptRval ScriptRval::operator/(const ScriptRval &other) const {
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		auto maxType = (valueType->Size() > other.valueType->Size() ? valueType : other.valueType);

		if (maxType->GetName() != "float" && maxType->GetName() != "double") {
			if (valueType->GetName() == "float" || valueType->GetName() == "double")
				maxType = valueType;
			else if (other.valueType->GetName() == "float" || other.valueType->GetName() == "double")
				maxType = other.valueType;
		}

		if (maxType->GetName() == "float" || maxType->GetName() == "double") {
			double value = 0.0f;

			if (valueType->GetName() == "float") {
				value = *reinterpret_cast<float *>(data);
			}
			else if (valueType->GetName() == "double") {
				value = *reinterpret_cast<double *>(data);
			}
			else {
				switch (valueType->Size()) {
					case 1:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
						break;
					case 2:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(data) : *reinterpret_cast<int16_t *>(data));
						break;
					case 4:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(data) : *reinterpret_cast<int32_t *>(data));
						break;
					case 8:
						value = (valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(data) : *reinterpret_cast<int64_t *>(data));
						break;
				}
			}

			if (other.valueType->GetName() == "float") {
				value /= *reinterpret_cast<float *>(other.data);
			}
			else if (other.valueType->GetName() == "double") {
				value /= *reinterpret_cast<double *>(other.data);
			}
			else {
				switch (other.valueType->Size()) {
					case 1:
						value /= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
						break;
					case 2:
						value /= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(other.data) : *reinterpret_cast<int16_t *>(other.data));
						break;
					case 4:
						value /= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(other.data) : *reinterpret_cast<int32_t *>(other.data));
						break;
					case 8:
						value /= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(other.data) : *reinterpret_cast<int64_t *>(other.data));
						break;
				}
			}

			if (maxType->GetName() == "float") {
				return ScriptRval::Create<float>(engine, maxType, value);
			}
			return ScriptRval::Create<double>(engine, maxType, value);
		}

		uint64_t val = 0;
		int64_t valSigned = 0;

		switch (valueType->Size()) {
			case 1:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
			case 2:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
			case 4:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
			case 8:
				valSigned = val = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
				break;
		}

		switch (other.valueType->Size()) {
			case 1:
				val /= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
			case 2:
				val /= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
			case 4:
				val /= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
			case 8:
				val /= (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
				valSigned = val;
				break;
		}

		if (maxType->IsUnsigned()) {
			switch (maxType->Size()) {
				case 1:
					return ScriptRval::Create<uint8_t>(engine, maxType, val);
				case 2:
					return ScriptRval::Create<uint16_t>(engine, maxType, val);
				case 4:
					return ScriptRval::Create<uint32_t>(engine, maxType, val);
				case 8:
					return ScriptRval::Create<uint64_t>(engine, maxType, val);
			}
		}

		switch (maxType->Size()) {
			case 1:
				return ScriptRval::Create<int8_t>(engine, maxType, val);
			case 2:
				return ScriptRval::Create<int16_t>(engine, maxType, val);
			case 4:
				return ScriptRval::Create<int32_t>(engine, maxType, val);
			case 8:
				return ScriptRval::Create<int64_t>(engine, maxType, val);
			default:
				throw std::exception("Invalid value type deduced");
		}
	}
	
	ScriptRval &ScriptRval::operator+=(const ScriptRval &other){
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		*this = *this + other;
		return *this;
	}
	ScriptRval &ScriptRval::operator-=(const ScriptRval &other){
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		*this = *this - other;
		return *this;
	}
	ScriptRval &ScriptRval::operator*=(const ScriptRval &other){
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		*this = *this * other;
		return *this;
	}
	ScriptRval &ScriptRval::operator/=(const ScriptRval &other){
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		*this = *this / other;
		return *this;
	}

	ScriptRval ScriptRval::operator<(const ScriptRval &other) const {
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		bool value = false;
		auto ret = ScriptRval::Create<bool>(engine, engine->GetTypeInfoByName("bool"), false);

		return ret;
	}
	ScriptRval ScriptRval::operator>(const ScriptRval &other) const {
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		bool value = false;
		auto ret = ScriptRval::Create<bool>(engine, engine->GetTypeInfoByName("bool"), false);

		return ret;
	}
	ScriptRval ScriptRval::operator<=(const ScriptRval &other) const {
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		bool value = false;
		auto ret = ScriptRval::Create<bool>(engine, engine->GetTypeInfoByName("bool"), false);

		return ret;
	}
	ScriptRval ScriptRval::operator>=(const ScriptRval &other) const {
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		bool value = false;
		auto ret = ScriptRval::Create<bool>(engine, engine->GetTypeInfoByName("bool"), false);

		return ret;
	}
	ScriptRval ScriptRval::operator!=(const ScriptRval &other) const {
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		bool value = false;
		auto ret = ScriptRval::Create<bool>(engine, engine->GetTypeInfoByName("bool"), false);

		if (valueType->GetName() == "float") {
			auto firstVal = *reinterpret_cast<float *>(data);

			if (other.valueType->GetName() == "float") {
				value = firstVal != *reinterpret_cast<float *>(other.data);
			}
			else if (other.valueType->GetName() == "double") {
				value = firstVal != *reinterpret_cast<double *>(other.data);
			}
			else {
				uint64_t secondVal = 0;
				int64_t secondValSigned = 0;

				switch (other.valueType->Size()) {
					case 1:
						secondVal = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
						secondValSigned = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
						break;
					case 2:
						secondVal = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(other.data) : *reinterpret_cast<int16_t *>(other.data));
						secondValSigned = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(other.data) : *reinterpret_cast<int16_t *>(other.data));
						break;
					case 4:
						secondVal = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(other.data) : *reinterpret_cast<int32_t *>(other.data));
						secondValSigned = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(other.data) : *reinterpret_cast<int32_t *>(other.data));
						break;
					case 8:
						secondVal = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(other.data) : *reinterpret_cast<int64_t *>(other.data));
						secondValSigned = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(other.data) : *reinterpret_cast<int64_t *>(other.data));
						break;
				}

				value = firstVal != (other.valueType->IsUnsigned() ? secondVal : secondValSigned);
			}
		}
		else if (valueType->GetName() == "double") {
			auto firstVal = *reinterpret_cast<double *>(data);

			if (other.valueType->GetName() == "float") {
				value = firstVal != *reinterpret_cast<float *>(other.data);
			}
			else if (other.valueType->GetName() == "double") {
				value = firstVal != *reinterpret_cast<double *>(other.data);
			}
			else {
				uint64_t secondVal = 0;
				int64_t secondValSigned = 0;

				switch (other.valueType->Size()) {
					case 1:
						secondVal = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
						secondValSigned = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
						break;
					case 2:
						secondVal = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(other.data) : *reinterpret_cast<int16_t *>(other.data));
						secondValSigned = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(other.data) : *reinterpret_cast<int16_t *>(other.data));
						break;
					case 4:
						secondVal = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(other.data) : *reinterpret_cast<int32_t *>(other.data));
						secondValSigned = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(other.data) : *reinterpret_cast<int32_t *>(other.data));
						break;
					case 8:
						secondVal = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(other.data) : *reinterpret_cast<int64_t *>(other.data));
						secondValSigned = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(other.data) : *reinterpret_cast<int64_t *>(other.data));
						break;
				}

				value = firstVal != (other.valueType->IsUnsigned() ? secondVal : secondValSigned);
			}
		}
		else {
			uint64_t firstVal = 0;
			int64_t firstValSigned = 0;
			
			switch (valueType->Size()) {
				case 1:
					firstVal = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
					firstValSigned = (valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(data) : *reinterpret_cast<int8_t *>(data));
					break;
				case 2:
					firstVal = (valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(data) : *reinterpret_cast<int16_t *>(data));
					firstValSigned = (valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(data) : *reinterpret_cast<int16_t *>(data));
					break;
				case 4:
					firstVal = (valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(data) : *reinterpret_cast<int32_t *>(data));
					firstValSigned = (valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(data) : *reinterpret_cast<int32_t *>(data));
					break;
				case 8:
					firstVal = (valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(data) : *reinterpret_cast<int64_t *>(data));
					firstValSigned = (valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(data) : *reinterpret_cast<int64_t *>(data));
					break;
			}

			if (other.valueType->GetName() == "float") {
				float firstFloat = static_cast<float>(valueType->IsUnsigned() ? firstVal : firstValSigned);

				value = firstFloat != *reinterpret_cast<float *>(other.data);
			}
			else if (other.valueType->GetName() == "double") {
				float firstDouble = static_cast<double>(valueType->IsUnsigned() ? firstVal : firstValSigned);

				value = firstDouble != *reinterpret_cast<double *>(other.data);
			}
			else {
				uint64_t secondVal = 0;
				int64_t secondValSigned = 0;

				switch (other.valueType->Size()) {
					case 1:
						secondVal = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
						secondValSigned = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint8_t *>(other.data) : *reinterpret_cast<int8_t *>(other.data));
						break;
					case 2:
						secondVal = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(other.data) : *reinterpret_cast<int16_t *>(other.data));
						secondValSigned = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint16_t *>(other.data) : *reinterpret_cast<int16_t *>(other.data));
						break;
					case 4:
						secondVal = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(other.data) : *reinterpret_cast<int32_t *>(other.data));
						secondValSigned = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint32_t *>(other.data) : *reinterpret_cast<int32_t *>(other.data));
						break;
					case 8:
						secondVal = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(other.data) : *reinterpret_cast<int64_t *>(other.data));
						secondValSigned = (other.valueType->IsUnsigned() ? *reinterpret_cast<uint64_t *>(other.data) : *reinterpret_cast<int64_t *>(other.data));
						break;
				}

				value = (valueType->IsUnsigned() ? firstVal : firstValSigned) != (other.valueType->IsUnsigned() ? secondVal : secondValSigned);
			}
		}

		*reinterpret_cast<bool*>(ret.data) = value;
		return ret;
	}
	ScriptRval ScriptRval::operator==(const ScriptRval &other) const {
		if (valueType->IsClass() || other.valueType->IsClass()) throw std::exception("Bad value type");
		auto ret = (*this != other);

		*reinterpret_cast<bool*>(ret.data) = !*reinterpret_cast<bool *>(ret.data);

		return ret;
	}

	ScriptRval::operator bool() const {
		switch (valueType->Size()) {
			case 1:
				return *reinterpret_cast<uint8_t*>(data) != 0;
			case 2:
				return *reinterpret_cast<uint16_t *>(data) != 0;
			case 4:
				if (valueType->GetName() == "float")
					return *reinterpret_cast<float *>(data) != 0;
				return *reinterpret_cast<uint32_t *>(data) != 0;
			case 8:
				if (valueType->GetName() == "double")
					return *reinterpret_cast<double *>(data) != 0;
				return *reinterpret_cast<uint64_t *>(data) != 0;
		}
		for (size_t i = 0; i < valueType->Size(); ++i) {
			if (reinterpret_cast<char *>(data)[i] != 0) {
				return true;
			}
		}

		return false;
	}
}
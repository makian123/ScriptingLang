#include <iostream>
#include <cassert>
#include <functional>
#include <marklang.h>

mlang::ScriptRval EmptyFunction(mlang::Engine *engine, std::vector<mlang::ScriptRval> var) {
	std::cout << "Callback successful\n";
	std::cout << "Param value: " << *reinterpret_cast<const int *>((var[0].GetValue()));

	return mlang::ScriptRval::CreateEmpty();
}
mlang::ScriptRval TestStringFunc(mlang::Engine *engine, std::vector<mlang::ScriptRval> var) {
	std::cout << "Calling get function successful\n";

	auto thisObj = engine->GetScope()->GetParentFunction()->GetScriptObject();
	thisObj->GetMember("testVar").value()->SetVal(mlang::ScriptRval::CreateFromLiteral(engine, "12"));

	return mlang::ScriptRval::Create(
		engine, 
		engine->GetTypeInfoByName("int"), 
		*reinterpret_cast<int*>(thisObj->GetMember("testVar").value()->GetAddressOfObj())
	);
}

mlang::ScriptRval CustomPrint(mlang::Engine *engine, std::vector<mlang::ScriptRval> params) {
	auto type = params[0].GetType();
	if (type->IsClass()) {
		std::cout << "Invalid variable type: " << type->GetName() << "\n";
		return mlang::ScriptRval::CreateEmpty();
	}

	if (type->GetName() != "float" && type->GetName() != "double") {
		switch (type->Size()) {
			case 1:
				if (type->IsUnsigned()) std::cout << *reinterpret_cast<const uint8_t *>(params[0].GetValue());
				else std::cout << *reinterpret_cast<const int8_t *>(params[0].GetValue());
				break;
			case 2:
				if (type->IsUnsigned()) std::cout << *reinterpret_cast<const uint16_t *>(params[0].GetValue());
				else std::cout << *reinterpret_cast<const int16_t *>(params[0].GetValue());
				break;
			case 4:
				if (type->IsUnsigned()) std::cout << *reinterpret_cast<const uint32_t *>(params[0].GetValue());
				else std::cout << *reinterpret_cast<const int32_t *>(params[0].GetValue());
				break;
			case 8:
				if(type->IsUnsigned()) std::cout << *reinterpret_cast<const uint64_t *>(params[0].GetValue());
				else std::cout << *reinterpret_cast<const int64_t *>(params[0].GetValue());
				break;
		}
	}
	else {
		if((type->GetName() == "float")) std::cout << *reinterpret_cast<const float*>(params[0].GetValue());
		else std::cout << *reinterpret_cast<const double *>(params[0].GetValue());
	}
	std::cout << std::endl;

	return mlang::ScriptRval::CreateEmpty();
}

int main() {
	using mlang::ScriptRval;
	mlang::Engine engine;

	// Creates a new module
	auto r = engine.NewModule("testModule"); assert(r == mlang::RespCode::SUCCESS);

	// Fetches the same module
	auto mod = engine.GetModule("testModule").data.value();

	engine.RegisterFunction(
		"TestFunction",
		std::vector<std::pair<mlang::TypeInfo *, std::string>>(),
		engine.GetTypeInfoByName("void"),
		std::function(EmptyFunction)
	);
	engine.RegisterFunction(
		"Print",
		std::vector<std::pair<mlang::TypeInfo *, std::string>>{
			{engine.GetTypeInfoByName("int"), "src" }
		},
		engine.GetTypeInfoByName("void"),
		std::function(CustomPrint)
	);

	// Adds code from file
	r = mod->AddSectionFromFile("script.mla"); assert(r == mlang::RespCode::SUCCESS);

	r = mod->Build(); assert(r == mlang::RespCode::SUCCESS);

	r = mod->Run(); assert(r == mlang::RespCode::SUCCESS);

	return 0;
}
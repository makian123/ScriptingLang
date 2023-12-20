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
	engine.RegisterGlobalType("Getter", 4, nullptr, 0, true);
	engine.GetTypeInfoByName("Getter")->AddMember("testVar", engine.GetTypeInfoByName("int"));
	engine.GetTypeInfoByName("Getter")->RegisterMethod("Get", engine.GetTypeInfoByName("int"), false, {}, std::function(TestStringFunc));

	// Adds code from file
	r = mod->AddSectionFromFile("script.mla"); assert(r == mlang::RespCode::SUCCESS);

	r = mod->Build(); assert(r == mlang::RespCode::SUCCESS);

	r = mod->Run(); assert(r == mlang::RespCode::SUCCESS);

	return 0;
}
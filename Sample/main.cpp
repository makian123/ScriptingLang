#include <iostream>
#include <cassert>
#include <functional>
#include <marklang.h>

int main() {
	mlang::Engine engine;

	/*auto rvalue = mlang::ScriptRval::CreateFromLiteral(&engine, "12");
	auto obj = new mlang::ScriptObject(&engine, engine.GetTypeInfoByName("int"));
	obj->SetVal(rvalue);

	std::cout << *reinterpret_cast<int *>(obj->GetAddressOfObj());

	delete obj;

	return 0;*/

	// Creates a new module
	auto r = engine.NewModule("testModule"); assert(r == mlang::RespCode::SUCCESS);

	// Fetches the same module
	auto mod = engine.GetModule("testModule").data.value();

	// Adds code from file
	r = mod->AddSectionFromFile("script.mla"); assert(r == mlang::RespCode::SUCCESS);

	r = mod->Build(); assert(r == mlang::RespCode::SUCCESS);

	r = mod->Run(); assert(r == mlang::RespCode::SUCCESS);

	return 0;
}
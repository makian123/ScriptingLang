#include <marklang.h>

namespace mlang {
	void ScriptFunc::SetClassObject(ScriptObject *obj) {
		object = obj;
	}
}
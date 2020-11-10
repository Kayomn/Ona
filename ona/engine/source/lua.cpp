#include "ona/engine/module.hpp"

#include "lua5.3/lua.hpp"

namespace Ona::Engine {
	ScriptVar ScriptVar::Call() {
		return ScriptVar{};
	}

	ScriptVar ScriptVar::ReadObject(String const & fieldName) {
		return ScriptVar{};
	}

	void ScriptVar::WriteObject(String const & fieldName, ScriptVar const & valueVar) {

	}

	LuaEngine::LuaEngine(Allocator * allocator) : state{luaL_newstate()} {

	}

	LuaEngine::~LuaEngine() {
		lua_close(this->state);
	}

	ScriptResult LuaEngine::ExecuteBinary(Slice<uint8_t> const & binary) {
		// TODO:
		return ScriptResult::Fail(String::From("Not implemented"));
	}

	ScriptResult LuaEngine::ExecuteSource(String const & script) {
		// TODO:
		return ScriptResult::Fail(String::From("Not implemented"));
	}

	ScriptVar LuaEngine::NewObject() {
		// TODO:
		return ScriptVar{};
	}

	ScriptVar LuaEngine::ReadGlobal(String const & name) {
		// TODO:
		return ScriptVar{};
	}

	void LuaEngine::WriteGlobal(String const & name, ScriptVar const & valueVar) {
		// TODO:
	}
}

#include "ona/engine/module.hpp"

namespace Ona::Engine {
	LuaEngine::~LuaEngine() {

	}

	String LuaEngine::ExecuteSourceFile(Slice<LuaVar> const & returnVars, File const & file) {
		// TODO
		return String{};
	}

	String LuaEngine::ExecuteSourceScript(
		Slice<LuaVar> const & returnVars,
		String const & scriptSource
	) {
		// TODO
		return String{};
	}

	LuaVar LuaEngine::GetGlobal(String const & globalName) {
		// TODO
		return LuaVar{};
	}

	LuaVar LuaEngine::GetField(LuaVar const & tableVar, String const & fieldName) {
		// TODO
		return LuaVar{};
	}

	String LuaEngine::VarString(LuaVar const & var) {
		// TODO
		return String{};
	}

	LuaType LuaEngine::VarType(LuaVar const & var) {
		// TODO
		return LuaType{};
	}
}

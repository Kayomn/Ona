#include "ona/engine/module.hpp"

#include "lua5.3/lua.hpp"

namespace Ona::Engine {
	LuaEngine::LuaEngine(Allocator * allocator) : allocator{allocator}, state{luaL_newstate()} {
		lua_newtable(this->state);
		lua_setglobal(this->state, "Ona");

		lua_pushcfunction(this->state, [](lua_State * state) -> int {
			Slice<char const> message = {.pointer = lua_tolstring(state, -1, (&message.length))};

			OutFile().Write(message.AsBytes());

			return 0;
		});

		lua_setglobal(this->state, "print");
	}

	LuaEngine::~LuaEngine() {
		lua_close(this->state);
	}

	void LuaEngine::CallInitializer() {
		lua_getglobal(this->state, "Ona");

		if (lua_type(this->state, -1) == LUA_TTABLE) {
			lua_getfield(this->state, -1, "init");

			if (lua_type(this->state, -1) == LUA_TFUNCTION) {
				lua_pcall(this->state, 0, 0, 0);
			}

			lua_pop(this->state, 1);
		}

		lua_pop(this->state, 1);
	}

	void LuaEngine::CallFinalizer() {
		lua_getglobal(this->state, "Ona");

		if (lua_type(this->state, -1) == LUA_TTABLE) {
			lua_getfield(this->state, -1, "exit");

			if (lua_type(this->state, -1) == LUA_TFUNCTION) {
				lua_pcall(this->state, 0, 0, 0);
			}

			lua_pop(this->state, 1);
		}

		lua_pop(this->state, 1);
	}

	ScriptState LuaEngine::ExecuteBinary(Slice<uint8_t const> const & data) {
		static lua_Reader const reader = [](
			lua_State * state,
			void * data,
			size_t * size
		) -> char const * {
			Slice<uint8_t const> const * bytes =
					reinterpret_cast<Slice<uint8_t const> const *>(data);

			(*size) = bytes->length;

			return reinterpret_cast<char const *>(bytes->pointer);
		};

		// Evil, fucky const-erasing cast only for it to be re-cast to a const inside "reader".
		switch (lua_load(this->state, reader, (Slice<uint8_t> *)&data, "ona", nullptr)) {
			case LUA_OK: {
				switch (lua_pcall(this->state, 0, LUA_MULTRET, 0)) {
					case LUA_OK: return ScriptState::Ok;

					case LUA_ERRRUN:
					case LUA_ERRERR: {
						Slice<char const> error = {
							.pointer = lua_tolstring(this->state, -1, (&error.length))
						};

						this->ReportError(error);

						return ScriptState::RuntimeError;
					}

					case LUA_ERRMEM: return ScriptState::OutOfMemory;

					default: return ScriptState::RuntimeError;
				}
			} break;

			case LUA_ERRSYNTAX: {
				Slice<char const> error = {
					.pointer = lua_tolstring(this->state, -1, (&error.length))
				};

				this->ReportError(error);

				return ScriptState::SyntaxError;
			}

			case LUA_ERRMEM: return ScriptState::OutOfMemory;

			default: return ScriptState::RuntimeError;
		}
	}

	ScriptState LuaEngine::ExecuteSource(String const & script) {
		switch (luaL_loadstring(this->state, script.ZeroSentineled().AsChars().pointer)) {
			case LUA_OK: {
				switch (lua_pcall(this->state, 0, LUA_MULTRET, 0)) {
					case LUA_OK: return ScriptState::Ok;

					case LUA_ERRRUN:
					case LUA_ERRERR: {
						Slice<char const> error = {
							.pointer = lua_tolstring(this->state, -1, (&error.length))
						};

						this->ReportError(error);

						return ScriptState::RuntimeError;
					}

					case LUA_ERRMEM: return ScriptState::OutOfMemory;

					default: return ScriptState::RuntimeError;
				}
			} break;

			case LUA_ERRSYNTAX: {
				Slice<char const> error = {
					.pointer = lua_tolstring(this->state, -1, (&error.length))
				};

				this->ReportError(error);

				return ScriptState::SyntaxError;
			}

			case LUA_ERRMEM: return ScriptState::OutOfMemory;

			default: return ScriptState::RuntimeError;
		}
	}

	void LuaEngine::ReportError(Chars const & message) {
		OutFile().Write(message.AsBytes());
	}
}

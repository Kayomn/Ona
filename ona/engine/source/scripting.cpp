#include "ona/engine/module.hpp"

#include "lua5.3/lua.hpp"

namespace Ona::Engine {
	struct LuaReference {
		lua_State * state;

		uint64_t count;

		int handle;
	};

	thread_local Index<LuaReference, uint32_t> luaReferences = {DefaultAllocator()};

	LuaVar::LuaVar(LuaVar const & that) {
		if (this->IsReferenceType()) {
			LuaReference * reference =  luaReferences.Lookup(this->value.reference);

			if (reference) reference->count += 1;
		}
	}

	LuaVar::~LuaVar() {
		if (this->IsReferenceType()) {
			LuaReference * reference =  luaReferences.Lookup(this->value.reference);

			if (reference) {
				uint64_t const refCount = (reference->count -= 1);

				if (refCount) {
					luaL_unref(reference->state, LUA_REGISTRYINDEX, reference->handle);
					luaReferences.Remove(this->value.reference);
				}
			}
		}
	}

	bool LuaVar::IsReferenceType() const {
		return (
			(this->type == LuaType::String) ||
			(this->type == LuaType::Function) ||
			(this->type == LuaType::Table)
		);
	}

	String LuaVar::ToString() const {
		switch (this->type) {
			case LuaType::Nil: return String{};

			case LuaType::Boolean: return (this->value.boolean ? String{"true"} : String{"false"});

			case LuaType::Integer: return String{"0"};

			case LuaType::Number: return String{"0.0"};

			case LuaType::Function:
			case LuaType::String:
			case LuaType::Table: {
				LuaReference * luaReference = luaReferences.Lookup(this->value.reference);

				if (luaReference) {
					lua_State * state = luaReference->state;

					lua_rawgeti(state, LUA_REGISTRYINDEX, luaReference->handle);

					String str = {lua_tostring(state, -1)};

					lua_pop(state, -1);

					return str;
				}

				return String{};
			}
		}
	}

	LuaEngine::LuaEngine(Allocator * allocator) : allocator{allocator}, state{luaL_newstate()} {
		lua_newtable(this->state);
		lua_setglobal(this->state, "Ona");
	}

	LuaEngine::~LuaEngine() {
		lua_close(this->state);
	}

	LuaVar LuaEngine::ExecuteScript(String const & script) {
		switch (luaL_loadstring(this->state, script.ZeroSentineled().Chars().pointer)) {
			case LUA_OK: {
				switch (lua_pcall(this->state, 0, LUA_MULTRET, 0)) {
					case LUA_OK: return this->PopVar(-1);

					case LUA_ERRRUN:
					case LUA_ERRERR: {
						Slice<char const> error = {
							.pointer = lua_tolstring(this->state, -1, (&error.length))
						};

						this->ReportError(error);

						return LuaVar{};
					}

					case LUA_ERRMEM: return LuaVar{};

					default: return LuaVar{};
				}
			} break;

			case LUA_ERRSYNTAX: {
				Slice<char const> error = {
					.pointer = lua_tolstring(this->state, -1, (&error.length))
				};

				this->ReportError(error);

				return LuaVar{};
			}

			case LUA_ERRMEM: return LuaVar{};

			default: return LuaVar{};
		}
	}

	LuaVar LuaEngine::PopVar(int const stackIndex) {
		static auto createReference = [](lua_State * state, LuaType const luaType) -> LuaVar {
			LuaVar luaVar;

			Result<uint32_t> allocatedReference = luaReferences.Insert(LuaReference{
				.state = state,
				.count = 1,
				.handle = luaL_ref(state, LUA_REGISTRYINDEX),
			});

			if (allocatedReference.IsOk()) {
				luaVar.value.reference = allocatedReference.Value();
				luaVar.type = luaType;
			} else {
				luaVar = LuaVar{};
			}

			return luaVar;
		};

		switch (lua_type(this->state, stackIndex)) {
			case LUA_TNIL: {
				lua_pop(this->state, -1);

				return LuaVar{};
			}

			case LUA_TBOOLEAN: {
				LuaVar luaVar = {static_cast<bool>(lua_toboolean(this->state, stackIndex))};

				lua_pop(this->state, -1);

				return luaVar;
			}

			case LUA_TNUMBER: {
				LuaVar luaVar = {lua_tonumber(this->state, stackIndex)};

				lua_pop(this->state, -1);

				return luaVar;
			}

			case LUA_TFUNCTION: return createReference(this->state, LuaType::Function);

			case LUA_TSTRING: return createReference(this->state, LuaType::String);

			case LUA_TTABLE: return createReference(this->state, LuaType::Table);

			default: return LuaVar{};
		}
	}

	void LuaEngine::PushVar(LuaVar const & var) {
		switch (var.type) {
			case LuaType::Nil: {
				lua_pushnil(this->state);
			} break;

			case LuaType::Boolean: {
				lua_pushboolean(this->state, var.value.boolean);
			} break;

			case LuaType::Integer: {
				lua_pushinteger(this->state, var.value.integer);
			} break;

			case LuaType::Number: {
				lua_pushnumber(this->state, var.value.number);
			} break;

			case LuaType::Function:
			case LuaType::String:
			case LuaType::Table: {
				LuaReference * luaReference = luaReferences.Lookup(var.value.reference);

				if (luaReference) {
					// TODO: Handle LuaVars from other virtual machines.
					lua_rawgeti(this->state, LUA_REGISTRYINDEX, luaReference->handle);
				} else {
					lua_pushnil(this->state);
				}
			} break;
		}
	}

	LuaVar LuaEngine::ReadTable(LuaVar const & tableVar, String const & fieldName) {
		this->PushVar(tableVar);
		lua_getfield(this->state, -1, fieldName.ZeroSentineled().Chars().pointer);

		LuaVar valueVar = this->PopVar(-1);

		lua_pop(this->state, -1);

		return valueVar;
	}

	void LuaEngine::ReportError(Chars const & message) {
		OutFile().Write(message.AsBytes());
	}

	LuaVar LuaEngine::VarIndex(LuaVar const & luaVar, size_t const index) {
		this->PushVar(luaVar);
		lua_geti(this->state, -1, static_cast<lua_Integer>(index + 1));

		LuaVar retrievedLuaVar = this->PopVar(-1);

		lua_pop(this->state, -1);

		return retrievedLuaVar;
	}

	size_t LuaEngine::VarLength(LuaVar const & luaVar) {
		this->PushVar(luaVar);

		size_t const length = static_cast<size_t>(luaL_len(this->state, -1));

		lua_pop(this->state, -1);

		return length;
	}
}

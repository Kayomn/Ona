#include "engine/exports.hpp"

#include "lua.hpp"

namespace Ona::Engine {
	void ConfigValue::Free() {
		this->config->FreeValue(*this);
	}

	LuaConfig::LuaConfig() : LuaConfig{ErrorReporter{}} {

	}

	LuaConfig::LuaConfig(
		Callable<void(String const &)> const & errorReporter
	) : state{luaL_newstate()}, errorReporter{errorReporter} {

	}

	LuaConfig::~LuaConfig() {
		lua_close(this->state);
	}

	void LuaConfig::FreeValue(ConfigValue & value) {
		if (this == value.config) {
			luaL_unref(this->state, LUA_REGISTRYINDEX, value.handle);

			value.handle = 0;
		}
	}

	bool LuaConfig::Load(String const & script) {
		switch (luaL_loadstring(this->state, script.ZeroSentineled().Chars().pointer)) {
			case LUA_OK: {
				switch (lua_pcall(this->state, 0, LUA_MULTRET, 0)) {
					case LUA_OK: {
						// Pop loaded chunk.
						lua_pop(this->state, -1);

						return true;
					}

					case LUA_ERRRUN:
					case LUA_ERRERR: {
						if (this->errorReporter.HasValue()) {
							// Needs to be placed on the stack as its own value because lua API
							// needs to write to an out parameter.
							Slice<char const> error = {
								.pointer = lua_tolstring(this->state, -1, (&error.length))
							};

							this->errorReporter.Invoke(String{error});
						}

						// Pop error message.
						lua_pop(this->state, -1);

						return false;
					}

					case LUA_ERRMEM: return false;

					default: return false;
				}
			} break;

			case LUA_ERRSYNTAX: {
				if (this->errorReporter.HasValue()) {
					// Needs to be placed on the stack as its own value because lua API needs to
					// write to an out parameter.
					Slice<char const> error = {
						.pointer = lua_tolstring(this->state, -1, (&error.length))
					};

					this->errorReporter.Invoke(String{error});
				}

				// Pop error message.
				lua_pop(this->state, -1);

				return false;
			}

			case LUA_ERRMEM: return true;

			default: return true;
		}
	}

	ConfigValue LuaConfig::ReadArray(ConfigValue const & array, uint32_t index) {
		lua_rawgeti(this->state, LUA_REGISTRYINDEX, static_cast<lua_Integer>(array.handle));
		lua_geti(this->state, -1, static_cast<lua_Integer>(index + 1));

		ConfigValue value = {
			.config = this,
			.handle = static_cast<int64_t>(luaL_ref(this->state, LUA_REGISTRYINDEX))
		};

		lua_pop(this->state, -1);

		return value;
	}

	ConfigValue LuaConfig::ReadGlobal(String const & name) {
		lua_getglobal(this->state, name.ZeroSentineled().Chars().pointer);

		return ConfigValue{
			.config = this,
			.handle = static_cast<int64_t>(luaL_ref(this->state, LUA_REGISTRYINDEX))
		};
	}

	int64_t LuaConfig::ValueInteger(ConfigValue const & value) {
		lua_rawgeti(this->state, LUA_REGISTRYINDEX, static_cast<lua_Integer>(value.handle));

		lua_Integer const integer = lua_tointeger(this->state, -1);

		lua_pop(this->state, -1);

		return static_cast<int64_t>(integer);
	}

	uint32_t LuaConfig::ValueLength(ConfigValue const & value) {
		lua_rawgeti(this->state, LUA_REGISTRYINDEX, static_cast<lua_Integer>(value.handle));

		lua_Integer const length = luaL_len(this->state, -1);

		lua_pop(this->state, -1);

		return static_cast<uint32_t>(length);
	}

	String LuaConfig::ValueString(ConfigValue const & value) {
		lua_rawgeti(this->state, LUA_REGISTRYINDEX, static_cast<lua_Integer>(value.handle));

		Chars text = {.pointer = lua_tolstring(this->state, -1, &text.length)};
		String str = {text};

		// Unsafe to pop the string off the stack while it still in the process of being read by
		// native.
		lua_pop(this->state, -1);

		return str;
	}

	ConfigType LuaConfig::ValueType(ConfigValue const & value) {
		lua_rawgeti(this->state, LUA_REGISTRYINDEX, static_cast<lua_Integer>(value.handle));

		ConfigType type;

		switch (lua_type(this->state, -1)) {
			case LUA_TNIL: {
				type = ConfigType::None;
			} break;

			case LUA_TBOOLEAN: {
				type = ConfigType::Boolean;
			} break;

			case LUA_TNUMBER: {
				type = ConfigType::Number;
			} break;

			case LUA_TFUNCTION: {
				type = ConfigType::Function;
			} break;

			case LUA_TSTRING: {
				type = ConfigType::String;
			} break;

			case LUA_TTABLE: {
				// Length operator will only return the number of indices in a table, therefore a
				// non-empty table is an array. In the case of Lua, this differentiation is
				// unimportant, but for other config languages this is not the case so the API must
				// respect the difference.
				lua_Integer const tableLength = luaL_len(this->state, -1);
				type = (tableLength ? ConfigType::Array : ConfigType::Table);
			} break;

			default: {
				type = ConfigType::Unsupported;
			} break;
		}

		lua_pop(this->state, -1);

		return type;
	}

	void LuaConfig::WriteArray(
		ConfigValue const & array,
		uint32_t index,
		ConfigValue const & value
	) {
		lua_rawgeti(this->state, LUA_REGISTRYINDEX, static_cast<lua_Integer>(array.handle));
		lua_rawgeti(this->state, LUA_REGISTRYINDEX, static_cast<lua_Integer>(value.handle));
		lua_seti(this->state, -1, static_cast<lua_Integer>(index + 1));
		lua_pop(this->state, -1);
	}

	void LuaConfig::WriteGlobal(String const & name, ConfigValue value) {
		lua_rawgeti(this->state, LUA_REGISTRYINDEX, static_cast<lua_Integer>(value.handle));
		lua_setglobal(this->state, name.ZeroSentineled().Chars().pointer);
	}
}

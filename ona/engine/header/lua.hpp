
struct lua_State;

namespace Ona::Engine {
	class LuaEngine : public Object, public ScriptEngine {
		Allocator * allocator;

		lua_State * state;

		public:
		LuaEngine(Allocator * allocator);

		~LuaEngine() override;

		ScriptResult ExecuteBinary(Slice<uint8_t> const & data) override;

		ScriptResult ExecuteSource(String const & script) override;

		ScriptVar NewObject() override;

		ScriptVar ReadGlobal(String const & name) override;

		void WriteGlobal(String const & name, ScriptVar const & valueVar) override;
	};
}

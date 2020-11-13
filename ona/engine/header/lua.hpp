
struct lua_State;

namespace Ona::Engine {
	enum class ScriptState {
		Ok,
		SyntaxError,
		RuntimeError,
		OutOfMemory,
	};

	class LuaEngine : public Object {
		Allocator * allocator;

		lua_State * state;

		public:
		LuaEngine(Allocator * allocator);

		~LuaEngine() override;

		void CallInitializer();

		void CallFinalizer();

		void CallProcessor(Events const & events);

		ScriptState ExecuteBinary(Slice<uint8_t const> const & data);

		ScriptState ExecuteSource(String const & script);

		void ReportError(Chars const & message);
	};
}


struct lua_State;

namespace Ona::Engine {
	enum class LuaType {
		Nil,
		Boolean,
		Integer,
		Number,
		String,
		Function,
		Table
	};

	struct LuaVar {
		LuaType type;

		union {
			bool boolean;

			int32_t integer;

			double number;

			uint32_t reference;
		} value;

		LuaVar() = default;

		LuaVar(bool value) : type{LuaType::Boolean}, value{.boolean = value} { }

		LuaVar(int32_t value) : type{LuaType::Integer}, value{.integer = value} { }

		LuaVar(double value) : type{LuaType::Number}, value{.number = value} { }

		LuaVar(LuaVar const & that);

		~LuaVar();

		bool IsReferenceType() const;

		String ToString() const;
	};

	class LuaEngine : public Object {
		Allocator * allocator;

		lua_State * state;

		void PushVar(LuaVar const & var);

		LuaVar PopVar(int const stackIndex);

		public:
		LuaEngine(Allocator * allocator);

		~LuaEngine() override;

		LuaVar ExecuteScript(String const & script);

		LuaVar ReadTable(LuaVar const & tableVar, String const & fieldName);

		void ReportError(Chars const & message);

		LuaVar VarIndex(LuaVar const & luaVar, size_t const index);

		size_t VarLength(LuaVar const & luaVar);
	};
}

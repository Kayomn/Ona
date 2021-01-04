
struct lua_State;

namespace Ona::Engine {
	using namespace Ona::Core;

	enum class ConfigType {
		None,
		Unsupported,
		Boolean,
		Integer,
		Number,
		String,
		Function,
		Array,
		Table
	};

	class Config;

	struct ConfigValue {
		Config * config;

		int64_t handle;

		void Free() {
			this->config->FreeValue(*this);
		}
	};

	class Config : public Object {
		public:
		virtual void FreeValue(ConfigValue & value) = 0;

		virtual bool Load(String const & script) = 0;

		virtual ConfigValue ReadArray(ConfigValue const & array, uint32_t index) = 0;

		virtual ConfigValue ReadGlobal(String const & name) = 0;

		virtual int64_t ValueInteger(ConfigValue const & value) = 0;

		virtual uint32_t ValueLength(ConfigValue const & value) = 0;

		virtual String ValueString(ConfigValue const & value) = 0;

		virtual ConfigType ValueType(ConfigValue const & value) = 0;

		virtual void WriteGlobal(String const & name, ConfigValue value) = 0;

		virtual void WriteArray(
			ConfigValue const & array,
			uint32_t index,
			ConfigValue const & value
		) = 0;
	};

	class LuaConfig : public Config {
		lua_State * state;

		public:
		using ErrorReporter = Callable<void(String const &)>;

		ErrorReporter errorReporter;

		LuaConfig();

		LuaConfig(ErrorReporter const & errorReporter);

		~LuaConfig() override;

		void FreeValue(ConfigValue & value) override;

		bool Load(String const & script) override;

		ConfigValue ReadArray(ConfigValue const & array, uint32_t index) override;

		ConfigValue ReadGlobal(String const & name) override;

		int64_t ValueInteger(ConfigValue const & value) override;

		uint32_t ValueLength(ConfigValue const & value) override;

		String ValueString(ConfigValue const & value) override;

		ConfigType ValueType(ConfigValue const & value) override;

		void WriteGlobal(String const & name, ConfigValue value) override;

		void WriteArray(
			ConfigValue const & array,
			uint32_t index,
			ConfigValue const & value
		) override;
	};
}

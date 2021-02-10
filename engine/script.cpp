#include "engine.hpp"

namespace Ona {
	struct ConfigEnvironment::Value {
		enum class Type {
			Object,
			Boolean,
			Integer,
			Floating,
			String,
			Vector2,
			Array,
		};

		enum {
			UserdataSize = (64 - sizeof(Type)),
		};

		Type type;

		uint8_t userdata[UserdataSize];

		static Value * Find(
			HashTable<String, Value> * object,
			std::initializer_list<String> const & path
		) {
			Value * value = nullptr;

			for (String const & pathNode : path) {
				if (object == nullptr) return nullptr;

				value = object->Lookup(pathNode);

				if (value && (value->type != Value::Type::Object)) {
					object = reinterpret_cast<HashTable<String, Value> *>(value->userdata);
				} else {
					object = nullptr;
				}
			}

			return value;
		}
	};

	ConfigEnvironment::ConfigEnvironment(Allocator * allocator) : globals{allocator} {

	}

	ConfigEnvironment::~ConfigEnvironment() {

	}

	uint32_t ConfigEnvironment::Count(std::initializer_list<String> const & path) {
		Value * value = Value::Find(&this->globals, path);

		if (value) switch (value->type) {
			case Value::Type::Array:
					return reinterpret_cast<DynamicArray<Value> *>(value->userdata)->Length();

			case Value::Type::Object:
					return reinterpret_cast<HashTable<String, Value> *>(value->userdata)->Count();

			default: break;
		}

		return 0;
	}

	ScriptError ConfigEnvironment::Parse(String const & source) {
		// TODO: Implement.
		return ScriptError::None;
	}

	bool ConfigEnvironment::ReadBoolean(
		std::initializer_list<String> const & path,
		int32_t index,
		bool fallback
	) {
		Value * value = Value::Find(&this->globals, path);

		if (value) {
			if ((index == 0) && (value->type == Value::Type::Boolean)) {
				return (*reinterpret_cast<bool *>(value->userdata));
			} else if (value->type == Value::Type::Array) {
				auto array = reinterpret_cast<DynamicArray<Value> *>(value->userdata);

				if ((index >= 0) && (index < array->Length())) {
					Value * value = &array->At(index);

					if (value->type == Value::Type::Boolean) {
						return (*reinterpret_cast<bool *>(value->userdata));
					}
				}
			}
		}

		return fallback;
	}

	int64_t ConfigEnvironment::ReadInteger(
		std::initializer_list<String> const & path,
		int32_t index,
		int64_t fallback
	) {
		Value * value = Value::Find(&this->globals, path);

		if (value) {
			if ((index == 0) && (value->type == Value::Type::Vector2)) {
				return (*reinterpret_cast<int64_t *>(value->userdata));
			} else if (value->type == Value::Type::Array) {
				auto array = reinterpret_cast<DynamicArray<Value> *>(value->userdata);

				if ((index >= 0) && (index < array->Length())) {
					Value * value = &array->At(index);

					if (value->type == Value::Type::Integer) {
						return (*reinterpret_cast<int64_t *>(value->userdata));
					}
				}
			}
		}

		return fallback;
	}

	double ConfigEnvironment::ReadFloating(
		std::initializer_list<String> const & path,
		int32_t index,
		double fallback
	) {
		Value * value = Value::Find(&this->globals, path);

		if (value) {
			if ((index == 0) && (value->type == Value::Type::Floating)) {
				return (*reinterpret_cast<double *>(value->userdata));
			} else if (value->type == Value::Type::Array) {
				auto array = reinterpret_cast<DynamicArray<Value> *>(value->userdata);

				if ((index >= 0) && (index < array->Length())) {
					Value * value = &array->At(index);

					if (value->type == Value::Type::Floating) {
						return (*reinterpret_cast<double *>(value->userdata));
					}
				}
			}
		}

		return fallback;
	}

	String ConfigEnvironment::ReadString(
		std::initializer_list<String> const & path,
		int32_t index,
		String const & fallback
	) {
		Value * value = Value::Find(&this->globals, path);

		if (value) {
			if ((index == 0) && (value->type == Value::Type::String)) {
				return (*reinterpret_cast<String *>(value->userdata));
			} else if (value->type == Value::Type::Array) {
				auto array = reinterpret_cast<DynamicArray<Value> *>(value->userdata);

				if ((index >= 0) && (index < array->Length())) {
					Value * value = &array->At(index);

					if (value->type == Value::Type::String) {
						return (*reinterpret_cast<String *>(value->userdata));
					}
				}
			}
		}

		return fallback;
	}

	Vector2 ConfigEnvironment::ReadVector2(
		std::initializer_list<String> const & path,
		int32_t index,
		Vector2 fallback
	) {
		Value * value = Value::Find(&this->globals, path);

		if (value) {
			if ((index == 0) && (value->type == Value::Type::Vector2)) {
				return (*reinterpret_cast<Vector2 *>(value->userdata));
			} else if (value->type == Value::Type::Array) {
				auto array = reinterpret_cast<DynamicArray<Value> *>(value->userdata);

				if ((index >= 0) && (index < array->Length())) {
					Value * value = &array->At(index);

					if (value->type == Value::Type::Vector2) {
						return (*reinterpret_cast<Vector2 *>(value->userdata));
					}
				}
			}
		}

		return fallback;
	}
}

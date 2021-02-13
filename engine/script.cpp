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

		void Free() {
			switch (this->type) {
				case Type::String: {
					reinterpret_cast<String *>(this->userdata)->~String();

					break;
				};

				case Type::Array: {
					auto array = reinterpret_cast<DynamicArray<Value> *>(this->userdata);

					for (auto & value : array->Values()) value.Free();

					array->~DynamicArray();

					break;
				}

				case Type::Object: {
					auto object = reinterpret_cast<HashTable<String, Value> *>(this->userdata);

					object->ForEach([](String & key, Value & value) {
						value.Free();
					});

					object->~HashTable();

					break;
				}

				case Type::Boolean:
				case Type::Integer:
				case Type::Floating:
				case Type::Vector2: break;
			}
		}
	};

	ConfigEnvironment::ConfigEnvironment(Allocator * allocator) : globals{allocator} {

	}

	ConfigEnvironment::~ConfigEnvironment() {
		this->globals.ForEach([](String & key, Value & value) {
			value.Free();
		});
	}

	uint32_t ConfigEnvironment::Count(std::initializer_list<String> const & path) {
		Value * value = Value::Find(&this->globals, path);

		if (value) switch (value->type) {
			case Value::Type::Array:
					return reinterpret_cast<DynamicArray<Value> *>(value->userdata)->Length();

			case Value::Type::Object:
					return reinterpret_cast<HashTable<String, Value> *>(value->userdata)->Count();

			case Value::Type::Boolean:
			case Value::Type::Integer:
			case Value::Type::Floating:
			case Value::Type::String:
			case Value::Type::Vector2:
					return 1;
		}

		return 0;
	}

	ScriptError ConfigEnvironment::Parse(String const & source) {
		enum class TokenType {
			Invalid,
			BracketLeft,
			BracketRight,
			ParenLeft,
			ParenRight,
			Colon,
			Comma,
			Period,
			Identifier,
			NumberLiteral,
			StringLiteral,
		};

		struct Token {
			String text;

			TokenType type;
		};

		PackedStack<Token> tokens = {this->globals.AllocatorOf()};
		Slice<char const> sourceChars = source.Chars();

		for (size_t i = 0; i < sourceChars.length;) {
			switch (sourceChars.At(i)) {
				case '\n':
				case '\t':
				case ' ':
				case '\r':
				case '\v':
				case '\f': {
					i += 1;

					break;
				}

				case '[': {
					tokens.Push(Token{
						.text = String{"["},
						.type = TokenType::BracketLeft,
					});

					i += 1;

					break;
				}

				case ']': {
					tokens.Push(Token{
						.text = String{"]"},
						.type = TokenType::BracketRight,
					});

					i += 1;

					break;
				}

				case '(': {
					tokens.Push(Token{
						.text = String{"("},
						.type = TokenType::ParenLeft,
					});

					i += 1;

					break;
				}

				case ')': {
					tokens.Push(Token{
						.text = String{")"},
						.type = TokenType::ParenRight,
					});

					i += 1;

					break;
				}

				case '.': {
					tokens.Push(Token{
						.text = String{"."},
						.type = TokenType::Period,
					});

					i += 1;

					break;
				}

				case ':': {
					tokens.Push(Token{
						.text = String{":"},
						.type = TokenType::Colon,
					});

					i += 1;

					break;
				}

				case ',': {
					tokens.Push(Token{
						.text = String{","},
						.type = TokenType::Comma,
					});

					i += 1;

					break;
				}

				case '"': {
					size_t const iNext = (i + 1);
					size_t j = iNext;

					while ((j < sourceChars.length) && sourceChars.At(j) != '"') j += 1;

					if (sourceChars.At(j) == '"') {
						tokens.Push(Token{
							.text = String{Slice<char const>{
								.length = (j - iNext),
								.pointer = (sourceChars.pointer + iNext)
							}},

							.type = TokenType::NumberLiteral,
						});

						j += 1;
					} else {
						tokens.Push(Token{
							.text = String{"Unexpected end of file before end of string literal"},
							.type = TokenType::Invalid,
						});
					}

					i = j;

					break;
				}

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9': {
					size_t j = (i + 1);

					while ((j < sourceChars.length) && IsNumeric(sourceChars.At(j))) j += 1;

					if ((j < sourceChars.length) && (sourceChars.At(j) == '.')) {
						j += 1;

						if ((j < sourceChars.length) && IsNumeric(sourceChars.At(j))) {
							// Handle decimal places.
							j += 1;

							while ((j < sourceChars.length) && IsNumeric(sourceChars.At(j))) j += 1;
						} else {
							// Just a regular period, go back.
							j -= 2;
						}
					}

					tokens.Push(Token{
						.text = String{Slice<char const>{
							.length = (j - i),
							.pointer = (sourceChars.pointer + i)
						}},

						.type = TokenType::NumberLiteral,
					});

					i = j;

					break;
				}

				default: {
					size_t j = (i + 1);

					while ((j < sourceChars.length) && IsAlpha(sourceChars.At(j))) j += 1;

					tokens.Push(Token{
						.text = String{Slice<char const>{
							.length = (j - i),
							.pointer = (sourceChars.pointer + i)
						}},

						.type = TokenType::Identifier,
					});

					i = j;

					break;
				}
			}
		}

		tokens.ForEach([](Token const & token) {
			Print(token.text);
			Print(String{"\n"});
		});

		return ScriptError::None;
	}

	bool ConfigEnvironment::ReadBoolean(
		std::initializer_list<String> const & path,
		int32_t index,
		bool fallback
	) {
		Value * value = Value::Find(&this->globals, path);

		if (value) {
			if ((value->type == Value::Type::Boolean) && (index == 0)) {
				// Single-value access.
				return (*reinterpret_cast<bool *>(value->userdata));
			}

			if (value->type == Value::Type::Array) {
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
			if ((value->type == Value::Type::Integer) && (index == 0)) {
				// Single-value access.
				return (*reinterpret_cast<int64_t *>(value->userdata));
			}

			if (value->type == Value::Type::Array) {
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
			if ((value->type == Value::Type::Floating) && (index == 0)) {
				// Single-value access.
				return (*reinterpret_cast<float *>(value->userdata));
			}

			if (value->type == Value::Type::Array) {
				auto array = reinterpret_cast<DynamicArray<Value> *>(value->userdata);

				if ((index >= 0) && (index < array->Length())) {
					Value * value = &array->At(index);

					if (value->type == Value::Type::Floating) {
						return (*reinterpret_cast<float *>(value->userdata));
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
			if ((value->type == Value::Type::String) && (index == 0)) {
				// Single-value access.
				return (*reinterpret_cast<String *>(value->userdata));
			}

			if (value->type == Value::Type::Array) {
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
			if ((value->type == Value::Type::Vector2) && (index == 0)) {
				// Single-value access.
				return (*reinterpret_cast<Vector2 *>(value->userdata));
			}

			if (value->type == Value::Type::Array) {
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

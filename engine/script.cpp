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
			Vector3,
			Vector4,
			Array,
		};

		enum {
			UserdataSize = (64 - sizeof(Type)),
		};

		Type type;

		uint8_t userdata[UserdataSize];

		static Value const * Find(
			HashTable<String, Value> const * object,
			std::initializer_list<String> const & path
		) {
			Value const * value = nullptr;

			for (String const & pathNode : path) {
				if (object == nullptr) return nullptr;

				value = object->Lookup(pathNode);

				if (value && (value->type != Value::Type::Object)) {
					object = reinterpret_cast<HashTable<String, Value> const *>(value->userdata);
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
				case Type::Vector2:
				case Type::Vector3:
				case Type::Vector4: break;
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
		Value const * value = Value::Find(&this->globals, path);

		if (value) switch (value->type) {
			case Value::Type::Array:
					return reinterpret_cast<DynamicArray<Value> const *>(value->userdata)->Length();

			case Value::Type::Object: return reinterpret_cast<HashTable<String, Value> const *>(
				value->userdata
			)->Count();

			case Value::Type::Boolean:
			case Value::Type::Integer:
			case Value::Type::Floating:
			case Value::Type::String:
			case Value::Type::Vector2:
			case Value::Type::Vector3:
			case Value::Type::Vector4:
					return 1;
		}

		return 0;
	}

	ScriptError ConfigEnvironment::Parse(String const & source, String * errorMessage) {
		enum class TokenType {
			Invalid,
			BraceLeft,
			BraceRight,
			ParenLeft,
			ParenRight,
			SemiColon,
			Comma,
			Period,
			Identifier,
			NumberLiteral,
			StringLiteral,
			Newline,
			EOF,
		};

		struct Token {
			String text;

			TokenType type;
		};

		Allocator * parsingAllocator = this->globals.AllocatorOf();
		Slice<char const> sourceChars = source.Chars();

		enum class ParseState {
			None,
			Declaration,
		};

		PackedStack<HashTable<String, Value> *> objectStack = {this->globals.AllocatorOf()};
		ParseState parseState = ParseState::None;
		Token declarationToken = {};

		// In order to avoid allocating memory, the tokenizer is a stateful functor that lazily
		// returns a new tokens from the string with each iteration. An end-of-file (EOF) token is
		// returned the indicate when the end of the string has been reached.
		auto eatToken = [&, i = (size_t)0]() mutable -> Token {
			while (i < sourceChars.length) {
				switch (sourceChars.At(i)) {
					case '\n': {
						i += 1;

						return Token{
							.text = String{"\n"},
							.type = TokenType::Newline,
						};
					}

					case '\t':
					case ' ':
					case '\r':
					case '\v':
					case '\f': {
						i += 1;

						break;
					}

					case '{': {
						i += 1;

						return Token{
							.text = String{"{"},
							.type = TokenType::BraceLeft,
						};
					}

					case '}': {
						i += 1;

						return Token{
							.text = String{"}"},
							.type = TokenType::BraceRight,
						};
					}

					case '(': {
						i += 1;

						return Token{
							.text = String{"("},
							.type = TokenType::ParenLeft,
						};
					}

					case ')': {
						i += 1;

						return Token{
							.text = String{")"},
							.type = TokenType::ParenRight,
						};
					}

					case '.': {
						i += 1;

						return Token{
							.text = String{"."},
							.type = TokenType::Period,
						};
					}

					case ',': {
						i += 1;

						return Token{
							.text = String{","},
							.type = TokenType::Comma,
						};
					}

					case '"': {
						size_t const iNext = (i + 1);
						size_t j = iNext;

						while ((j < sourceChars.length) && sourceChars.At(j) != '"') j += 1;

						if (sourceChars.At(j) == '"') {
							i = (j + 1);

							return Token{
								.text = String{Chars{
									.length = (j - iNext),
									.pointer = (sourceChars.pointer + iNext)
								}},

								.type = TokenType::StringLiteral,
							};
						}

						i = j;

						return Token{
							.text = String{"Unexpected end of file before end of string literal"},
							.type = TokenType::Invalid,
						};
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

						while ((j < sourceChars.length) && IsDigit(sourceChars.At(j))) j += 1;

						if ((j < sourceChars.length) && (sourceChars.At(j) == '.')) {
							j += 1;

							if ((j < sourceChars.length) && IsDigit(sourceChars.At(j))) {
								// Handle decimal places.
								j += 1;

								while ((j < sourceChars.length) && IsDigit(sourceChars.At(j))) j += 1;
							} else {
								// Just a regular period, go back.
								j -= 2;
							}
						}

						size_t const iOld = i;
						i = j;

						return Token{
							.text = String{Chars{
								.length = (j - iOld),
								.pointer = (sourceChars.pointer + iOld)
							}},

							.type = TokenType::NumberLiteral,
						};
					}

					default: {
						size_t j = (i + 1);

						while ((j < sourceChars.length) && IsAlpha(sourceChars.At(j))) j += 1;

						size_t const iOld = i;
						i = j;

						return Token{
							.text = String{Chars{
								.length = (j - iOld),
								.pointer = (sourceChars.pointer + iOld)
							}},

							.type = TokenType::Identifier,
						};
					}
				}
			}

			return Token{
				.text = String{},
				.type = TokenType::EOF,
			};
		};

		objectStack.Push(&this->globals);

		for (;;) {
			switch (parseState) {
				case ParseState::None: {
					declarationToken = eatToken();

					switch (declarationToken.type) {
						case TokenType::Identifier: {
							parseState = ParseState::Declaration;

							break;
						}

						case TokenType::BraceRight: {
							if (objectStack.Count() == 1) {
								if (errorMessage) {
									(*errorMessage) = String{"Unexpected closing brace"};
								}

								return ScriptError::ParsingSyntax;
							}

							objectStack.Pop(1);

							TokenType const endlineTokenType = eatToken().type;

							if (
								(endlineTokenType != TokenType::Newline) &&
								(endlineTokenType != TokenType::EOF)
							) {
								if (errorMessage) (*errorMessage) = String{
									"Expected newline or end of file after closing brace"
								};

								return ScriptError::ParsingSyntax;
							}

							parseState = ParseState::None;

							break;
						}

						case TokenType::EOF: {
							if (objectStack.Count() > 1) {
								if (errorMessage) {
									(*errorMessage) = String{"Expected closing brace"};
								}

								return ScriptError::ParsingSyntax;
							}

							return ScriptError::None;
						}

						// Ignore excess newline breaks.
						case TokenType::Newline: break;

						default: {
							if (errorMessage) (*errorMessage) = String{
								"Expected declaration identifier or closing brace"
							};

							return ScriptError::ParsingSyntax;
						}
					}

					break;
				}

				case ParseState::Declaration: {
					Token const assignmentToken = eatToken();

					switch (assignmentToken.type) {
						case TokenType::NumberLiteral: {
							Value integerValue = {
								.type = Value::Type::Integer,
							};

							// TODO: Implement number parsing.
							new (integerValue.userdata) int64_t{0};

							if (!objectStack.Peek()->Insert(declarationToken.text, integerValue)) {
								if (errorMessage) (*errorMessage) = String{"Out of memory"};

								return ScriptError::OutOfMemory;
							}

							break;
						}

						case TokenType::StringLiteral: {
							Value stringValue = {
								.type = Value::Type::String,
							};

							new (stringValue.userdata) String{assignmentToken.text};

							if (!objectStack.Peek()->Insert(declarationToken.text, stringValue)) {
								if (errorMessage) (*errorMessage) = String{"Out of memory"};

								return ScriptError::OutOfMemory;
							}

							break;
						}

						case TokenType::ParenLeft: {
							enum {
								ValuesMax = 4,
							};

							float valueBuffer[ValuesMax] = {};
							Token valueToken = {};
							uint8_t values = 0;

							do {
								valueToken = eatToken();

								if (valueToken.type != TokenType::NumberLiteral) {
									if (errorMessage) (*errorMessage) = String{
										"Unexpected syntax in vector literal"
									};

									return ScriptError::ParsingSyntax;
								}

								if (values == ValuesMax) {
									if (errorMessage) (*errorMessage) = String{
										"Vector literals cannot contain more than 4 numbers"
									};

									return ScriptError::ParsingSyntax;
								}

								double parsedFloating = 0;

								if (!ParseFloating(valueToken.text, parsedFloating)) {
									if (errorMessage) (*errorMessage) = String{
										"Vector component is not a valid floating point value"
									};

									return ScriptError::ParsingSyntax;
								}

								valueBuffer[values] = static_cast<float>(parsedFloating);
								values += 1;
								valueToken = eatToken();
							} while (valueToken.type == TokenType::Comma);

							if (valueToken.type != TokenType::ParenRight) {
								if (errorMessage) {
									(*errorMessage) = String{"Unexpected syntax in vector literal"};
								}

								// Unexpected symbol in vector expression.
								return ScriptError::ParsingSyntax;
							}

							switch (values) {
								case 2: {
									Value value = {
										.type = Value::Type::Vector2,
									};

									new (value.userdata) Vector2{valueBuffer[0], valueBuffer[1]};

									if (!objectStack.Peek()->Insert(declarationToken.text, value)) {
										if (errorMessage) (*errorMessage) = String{"Out of memory"};

										return ScriptError::OutOfMemory;
									}

									break;
								}

								case 3: {
									Value value = {
										.type = Value::Type::Vector3,
									};

									new (value.userdata) Vector3{
										valueBuffer[0],
										valueBuffer[1],
										valueBuffer[2]
									};

									if (!objectStack.Peek()->Insert(declarationToken.text, value)) {
										if (errorMessage) (*errorMessage) = String{"Out of memory"};

										return ScriptError::OutOfMemory;
									}

									break;
								}

								case 4: {
									Value value = {
										.type = Value::Type::Vector4,
									};

									new (value.userdata) Vector4{
										valueBuffer[0],
										valueBuffer[1],
										valueBuffer[2],
										valueBuffer[3]
									};

									if (!objectStack.Peek()->Insert(declarationToken.text, value)) {
										if (errorMessage) (*errorMessage) = String{"Out of memory"};

										return ScriptError::OutOfMemory;
									}

									break;
								}

								default: {
									if (errorMessage) (*errorMessage) = String{
										"Vector literals must contain 2, 3, or 4 numbers"
									};

									return ScriptError::ParsingSyntax;
								}
							}

							break;
						}

						case TokenType::BraceLeft: {
							HashTable<String, Value> * parentObject = objectStack.Peek();

							auto insertedObject = parentObject->Require(
								declarationToken.text,

								[parentObject]() -> Value {
									Value objectValue = {
										.type = Value::Type::Object,
									};

									// Can't just pass the result of this "new" to the object stack
									// because memory is referencing the local stack "objectValue",
									// and not the final resting place of the object inside the hash
									// table.
									new (objectValue.userdata) HashTable<String, Value>{
										parentObject->AllocatorOf()
									};

									return objectValue;
								}
							);

							if (!insertedObject) return ScriptError::OutOfMemory;

							if (!objectStack.Push(reinterpret_cast<HashTable<String, Value> *>(
								insertedObject->userdata
							))) {
								if (errorMessage) (*errorMessage) = String{"Out of memory"};

								return ScriptError::OutOfMemory;
							}

							parseState = ParseState::None;

							break;
						}

						default: {
							if (errorMessage) (*errorMessage) = String{
								"Unexpected syntax after declaration"
							};

							return ScriptError::ParsingSyntax;
						}
					}

					TokenType const endlineTokenType = eatToken().type;

					if (
						(endlineTokenType != TokenType::Newline) &&
						(endlineTokenType != TokenType::EOF)
					) {
						if (errorMessage) (*errorMessage) = String{
							"Expected newline or end of file after declaration"
						};

						return ScriptError::ParsingSyntax;
					}

					parseState = ParseState::None;

					break;
				}

				default: {
					if (errorMessage) {
						(*errorMessage) = String{"Unexpected expression after declaration"};
					}

					return ScriptError::ParsingSyntax;
				}
			}
		}

		return ScriptError::None;
	}

	bool ConfigEnvironment::ReadBoolean(
		std::initializer_list<String> const & path,
		int32_t index,
		bool fallback
	) {
		Value const * value = Value::Find(&this->globals, path);

		if (value) {
			if ((value->type == Value::Type::Boolean) && (index == 0)) {
				// Single-value access.
				return (*reinterpret_cast<bool const *>(value->userdata));
			}

			if (value->type == Value::Type::Array) {
				auto array = reinterpret_cast<DynamicArray<Value> const *>(value->userdata);

				if ((index >= 0) && (index < array->Length())) {
					Value const * value = &array->At(index);

					if (value->type == Value::Type::Boolean) {
						return (*reinterpret_cast<bool const *>(value->userdata));
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
		Value const * value = Value::Find(&this->globals, path);

		if (value) {
			if ((value->type == Value::Type::Integer) && (index == 0)) {
				// Single-value access.
				return (*reinterpret_cast<int64_t const *>(value->userdata));
			}

			if (value->type == Value::Type::Array) {
				auto array = reinterpret_cast<DynamicArray<Value> const *>(value->userdata);

				if ((index >= 0) && (index < array->Length())) {
					Value const * value = &array->At(index);

					if (value->type == Value::Type::Integer) {
						return (*reinterpret_cast<int64_t const *>(value->userdata));
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
		Value const * value = Value::Find(&this->globals, path);

		if (value) {
			if ((value->type == Value::Type::Floating) && (index == 0)) {
				// Single-value access.
				return (*reinterpret_cast<float const *>(value->userdata));
			}

			if (value->type == Value::Type::Array) {
				auto array = reinterpret_cast<DynamicArray<Value> const *>(value->userdata);

				if ((index >= 0) && (index < array->Length())) {
					Value const * value = &array->At(index);

					if (value->type == Value::Type::Floating) {
						return (*reinterpret_cast<float const *>(value->userdata));
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
		Value const * value = Value::Find(&this->globals, path);

		if (value) {
			if ((value->type == Value::Type::String) && (index == 0)) {
				// Single-value access.
				return (*reinterpret_cast<String const *>(value->userdata));
			}

			if (value->type == Value::Type::Array) {
				auto array = reinterpret_cast<DynamicArray<Value> const *>(value->userdata);

				if ((index >= 0) && (index < array->Length())) {
					Value const * value = &array->At(index);

					if (value->type == Value::Type::String) {
						return (*reinterpret_cast<String const *>(value->userdata));
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
		Value const * value = Value::Find(&this->globals, path);

		if (value) {
			if ((value->type == Value::Type::Vector2) && (index == 0)) {
				// Single-value access.
				return (*reinterpret_cast<Vector2 const *>(value->userdata));
			}

			if (value->type == Value::Type::Array) {
				auto array = reinterpret_cast<DynamicArray<Value> const *>(value->userdata);

				if ((index >= 0) && (index < array->Length())) {
					Value const * value = &array->At(index);

					if (value->type == Value::Type::Vector2) {
						return (*reinterpret_cast<Vector2 const *>(value->userdata));
					}
				}
			}
		}

		return fallback;
	}

	Vector3 ConfigEnvironment::ReadVector3(
		std::initializer_list<String> const & path,
		int32_t index,
		Vector3 const & fallback
	) {
		Value const * value = Value::Find(&this->globals, path);

		if (value) {
			if ((value->type == Value::Type::Vector3) && (index == 0)) {
				// Single-value access.
				return (*reinterpret_cast<Vector3 const *>(value->userdata));
			}

			if (value->type == Value::Type::Array) {
				auto array = reinterpret_cast<DynamicArray<Value> const *>(value->userdata);

				if ((index >= 0) && (index < array->Length())) {
					Value const * value = &array->At(index);

					if (value->type == Value::Type::Vector3) {
						return (*reinterpret_cast<Vector3 const *>(value->userdata));
					}
				}
			}
		}

		return fallback;
	}

	Vector4 ConfigEnvironment::ReadVector4(
		std::initializer_list<String> const & path,
		int32_t index,
		Vector4 const & fallback
	) {
		Value const * value = Value::Find(&this->globals, path);

		if (value) {
			if ((value->type == Value::Type::Vector4) && (index == 0)) {
				// Single-value access.
				return (*reinterpret_cast<Vector4 const *>(value->userdata));
			}

			if (value->type == Value::Type::Array) {
				auto array = reinterpret_cast<DynamicArray<Value> const *>(value->userdata);

				if ((index >= 0) && (index < array->Length())) {
					Value const * value = &array->At(index);

					if (value->type == Value::Type::Vector4) {
						return (*reinterpret_cast<Vector4 const *>(value->userdata));
					}
				}
			}
		}

		return fallback;
	}
}

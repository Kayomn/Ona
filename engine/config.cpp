#include "engine.hpp"

namespace Ona {
	struct Config::Value {
		enum class Type {
			Boolean,
			Integer,
			Floating,
			String,
			Vector2,
			Vector3,
			Vector4,
		};

		uint8_t buffer[48];

		Type type;

		static HashTable<String, Value> * MakeSection(Allocator * allocator) {
			uint8_t * allocation = allocator->Allocate(sizeof(HashTable<String, Value>));

			return (allocation ? new (allocation) HashTable<String, Value>{allocator} : nullptr);
		}

		static bool ParseNumber(String const & source, Value * parsedValue) {
			int64_t parsedSigned;

			if (ParseSigned(source, &parsedSigned) && (parsedSigned <= UINT32_MAX)) {
				if (parsedValue) (*parsedValue) = {.type = Value::Type::Integer};

				new (parsedValue->buffer) int32_t{static_cast<int32_t>(parsedSigned)};

				return true;
			}
			else
			{
				// Parsing as signed decimal failed, try float now.
				double parsedFloating;

				if (ParseFloating(source, &parsedFloating)) {
					(*parsedValue) = {.type = Value::Type::Floating};

					new (parsedValue->buffer) float{static_cast<float>(parsedFloating)};

					return true;
				}
			}

			return false;
		}
	};

	Config::Config(Allocator * allocator) : sections{allocator} {

	}

	Config::~Config() {

	}

	bool Config::Parse(String const & source, String * errorMessage) {
		enum class TokenType {
			Error,
			Identifier,
			BracketLeft,
			BracketRight,
			BraceLeft,
			BraceRight,
			Equals,
			StringLiteral,
			NumberLiteral,
			ParenLeft,
			ParenRight,
			Period,
			Comma,
			Newline,
			Eof,
		};

		struct Token {
			String text;

			TokenType type;
		};

		Chars sourceChars = source.Chars();
		uint32_t i = 0;

		auto eatToken = [&]() mutable -> Token {
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

					case '[': {
						i += 1;

						return Token{
							.text = String{"["},
							.type = TokenType::BracketLeft,
						};
					}

					case ']': {
						i += 1;

						return Token{
							.text = String{"]"},
							.type = TokenType::BracketRight,
						};
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

					case '=': {
						i += 1;

						return Token{
							.text = String{"="},
							.type = TokenType::Equals,
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
							.type = TokenType::Error,
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
				.type = TokenType::Eof,
			};
		};

		// Create the default section.
		Allocator * allocator = this->sections.AllocatorOf();
		HashTable<String, Value> * defaultSection = Value::MakeSection(allocator);

		HashTable<String, Value> * * currentSection = this->sections.Insert(
			String{},
			defaultSection
		);

		if (!currentSection) {
			if (errorMessage) (*errorMessage) = String{"Out of memory"};

			return false;
		}

		for (;;) {
			Token token = eatToken();

			switch (token.type) {
				case TokenType::Error: {
					if (errorMessage) (*errorMessage) = token.text;

					return false;
				}

				case TokenType::Eof: goto exit;

				case TokenType::BracketLeft: {
					token = eatToken();

					if (token.type == TokenType::BracketRight) {
						currentSection = (&defaultSection);
					} else {
						if (token.type != TokenType::Identifier) {
							if (errorMessage) (*errorMessage) = String{
								"Expected section name following start of section"
							};

							return false;
						}

						if (eatToken().type != TokenType::BracketRight) {
							if (errorMessage) (*errorMessage) = String{
								"Expected section end following name of section"
							};

							return false;
						}

						currentSection = this->sections.Require(token.text, [allocator]() {
							return Value::MakeSection(allocator);
						});
					}

					TokenType const endlineTokenType = eatToken().type;

					if (
						(endlineTokenType != TokenType::Newline) &&
						(endlineTokenType != TokenType::Eof)
					) {
						if (errorMessage) (*errorMessage) = String{
							"Expected end of line after key value declaration"
						};

						return false;
					}

					break;
				}

				case TokenType::Identifier: {
					if (eatToken().type != TokenType::Equals) {
						if (errorMessage) (*errorMessage) = String{
							"Expected assignment `=` after key"
						};

						return false;
					}

					Token valueToken = eatToken();

					switch (valueToken.type) {
						case TokenType::StringLiteral: {
							Value value = {.type = Value::Type::String};

							new (value.buffer) String{valueToken.text};

							(*currentSection)->Insert(token.text, value);

							break;
						}

						case TokenType::NumberLiteral: {
							Value parsedValue;

							if (Value::ParseNumber(valueToken.text, &parsedValue)) {
								(*currentSection)->Insert(token.text, parsedValue);
							} else {
								if (errorMessage) {
									(*errorMessage) = String{"Could not parse number literal"};
								}

								return false;
							}

							break;
						}

						case TokenType::ParenLeft: {
							constexpr size_t ParsedValueMax = 4;
							double parsedBuffer[ParsedValueMax];
							uint64_t parsedCount = 0;
							valueToken = eatToken();

							for (;;) {
								if (parsedCount == ParsedValueMax) {
									if (errorMessage) (*errorMessage) = Format(String{
										"Vector declarations cannot contain more than {} elements"
									}, {StringUnsigned(ParsedValueMax)});

									return false;
								}

								if (valueToken.type != TokenType::NumberLiteral) {
									if (errorMessage) (*errorMessage) = String{
										"Expected number literal after start of vector `(`"
									};

									return false;
								}

								if (!ParseFloating(valueToken.text, parsedBuffer + parsedCount)) {
									if (errorMessage) {
										(*errorMessage) = String{"Could not parse number literal"};
									}

									return false;
								}

								parsedCount += 1;
								valueToken = eatToken();

								if (valueToken.type == TokenType::Comma) {
									valueToken = eatToken();
								} else if (valueToken.type == TokenType::ParenRight) {
									goto constructArray;
								}
							}

							constructArray:
							Value value;

							switch (parsedCount) {
								default: {
									if (errorMessage) {
										(*errorMessage) = String{
											"A vector may only contain 2, 3, or 4 elements"
										};
									}

									return false;
								}

								case 2: {
									value = {.type = Value::Type::Vector2};

									new (value.buffer) Vector2{
										static_cast<float>(parsedBuffer[0]),
										static_cast<float>(parsedBuffer[1])
									};

									break;
								}

								case 3: {
									value = {.type = Value::Type::Vector3};

									new (value.buffer) Vector3{
										static_cast<float>(parsedBuffer[0]),
										static_cast<float>(parsedBuffer[1]),
										static_cast<float>(parsedBuffer[2])
									};

									break;
								}

								case 4: {
									value = {.type = Value::Type::Vector4};

									new (value.buffer) Vector4{
										static_cast<float>(parsedBuffer[0]),
										static_cast<float>(parsedBuffer[1]),
										static_cast<float>(parsedBuffer[2]),
										static_cast<float>(parsedBuffer[3])
									};

									break;
								}
							}

							(*currentSection)->Insert(token.text, value);

							break;
						}

						default: {
							if (errorMessage) (*errorMessage) = String{
								"Expected value literal after assignment `=`"
							};

							return false;
						}
					}

					TokenType const endlineTokenType = eatToken().type;

					if (
						(endlineTokenType != TokenType::Newline) &&
						(endlineTokenType != TokenType::Eof)
					) {
						if (errorMessage) (*errorMessage) = String{
							"Expected end of line after key value declaration"
						};

						return false;
					}

					break;
				}

				default: {
					if (errorMessage) {
						(*errorMessage) = Format("Unexpected token `{}` in source", {token.text});
					}

					return false;
				}
			}
		}

		exit:
		return true;
	}

	String Config::ReadString(
		String const & section,
		String const & key,
		String const & fallback
	) const {
		HashTable<String, Value> const * const * sectionMap = this->sections.Lookup(section);

		if (sectionMap) {
			Value const * value = (*sectionMap)->Lookup(key);

			if (value && (value->type == Value::Type::String)) {
				return (*(reinterpret_cast<String const *>(value->buffer)));
			}
		}

		return fallback;
	}

	Vector2 Config::ReadVector2(
		String const & section,
		String const & key,
		Vector2 fallback
	) const {
		HashTable<String, Value> const * const * sectionMap = this->sections.Lookup(section);

		if (sectionMap) {
			Value const * value = (*sectionMap)->Lookup(key);

			if (value && (value->type == Value::Type::Vector2)) {
				return (*(reinterpret_cast<Vector2 const *>(value->buffer)));
			}
		}

		return fallback;
	}
}

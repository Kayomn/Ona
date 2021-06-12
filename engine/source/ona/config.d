/**
 * Parsing and document object model for the Ona configuration syntax.
 */
module ona.config;

private import ona.collections.map, ona.functional, ona.math, ona.string, ona.system, ona.unicode;

/**
 * All possible type states that a `ConfigValue` may exist in.
 */
public enum ConfigType {
	nil,
	boolean,
	integral,
	floating,
	string_,
}

/**
 * Represents a dynamically typed value entry held by a `Config` instance, with types discriminated
 * at runtime via `ConfigType`.
 */
public struct ConfigValue {
	private union Store {
		bool boolean;

		int integral;

		double floating;

		string string_;
	}

	private ConfigType type = ConfigType.nil;

	private Store store;

	/**
	 * Stores `bool` as the held value.
	 */
	@nogc
	public this(in bool boolean) inout {
		this.type = ConfigType.boolean;
		this.store.boolean = boolean;
	}

	/**
	 * Stores `int` as the held value.
	 */
	@nogc
	public this(in int integral) inout {
		this.type = ConfigType.integral;
		this.store.integral = integral;
	}

	/**
	 * Stores `floating` as the held value.
	 */
	@nogc
	public this(in double floating) inout {
		this.type = ConfigType.floating;
		this.store.floating = floating;
	}

	/**
	 * Stores `str` as the held value.
	 */
	@nogc
	public this(in string str) inout {
		this.type = ConfigType.string_;
		this.store.string_ = str;
	}

	/**
	 * Attempts to access the held value as an `int`, returning it or nothing wrapped in an
	 * `Optional`.
	 */
	@nogc
	public Optional!int integerValue() const {
		if (this.type == ConfigType.integral) {
			return Optional!int(this.store.integral);
		}

		return Optional!int();
	}

	/**
	 * Attempts to access the held value as a `string`, returning it or nothing wrapped in an
	 * `Optional`.
	 */
	@nogc
	public Optional!string stringValue() const {
		if (this.type == ConfigType.string_) {
			return Optional!string(this.store.string_);
		}

		return Optional!string();
	}
}

/**
 * Represents a collection of opaque, dynamically typed configuration file data indexed keys under
 * sections.
 *
 * The Ona configuration format supports all data types representable by `ConfigValue`.
 */
public final class Config {
	private alias Section = HashTable!(string, ConfigValue);

	private auto sections = new HashTable!(string, Section);

	/**
	 * Looks up the configuration value associated with `keyName` under the section `sectionName`,
	 * returning the value or nothing wrapped in a `ConfigValue`.
	 *
	 * To look up values under the default section, pass an empty `string` to `sectionName`.
	 */
	@nogc
	public ConfigValue find(in string sectionName, in string keyName) const {
		const (Optional!Section) lookedUpSection = this.sections.lookup(sectionName);

		if (lookedUpSection.hasValue()) {
			immutable (Optional!ConfigValue) lookedUpValue =
				lookedUpSection.value().lookup(keyName);

			if (lookedUpValue.hasValue()) return lookedUpValue.value();
		}

		return ConfigValue();
	}

	/**
	 * Parses the contents of `source` as per the rules defined by the Ona configuration syntax,
	 * returning `true` if `source` was successfully parsed, otherwise `false` if an error was
	 * encountered.
	 *
	 * Syntax error messages are printed to standard output.
	 */
	public bool parse(in char[] source) {
		import std.ascii : isAlpha, isDigit;

		enum TokenType {
			error,
			identifier,
			bracketLeft,
			bracketRight,
			braceLeft,
			braceRight,
			equals,
			stringLiteral,
			numberLiteral,
			parenLeft,
			parenRight,
			period,
			comma,
			newline,
			eof,
		}

		struct Token {
			const (char)[] text = "";

			TokenType type = TokenType.eof;
		}

		size_t i = 0;

		Token eatToken() {
			while (i < source.length) {
				switch (source[i]) {
					case '\n': {
						i += 1;

						return Token("\n", TokenType.newline);
					}

					case ' ', '\t', '\r', '\f', '\v': {
						i += 1;

						break;
					}

					case '[': {
						i += 1;

						return Token("[", TokenType.bracketLeft);
					}

					case ']': {
						i += 1;

						return Token("]", TokenType.bracketRight);
					}

					case '{': {
						i += 1;

						return Token("{", TokenType.braceLeft);
					}

					case '}': {
						i += 1;

						return Token("}", TokenType.braceRight);
					}

					case '(': {
						i += 1;

						return Token("(", TokenType.parenLeft);
					}

					case ')': {
						i += 1;

						return Token(")", TokenType.parenRight);
					}

					case '.': {
						i += 1;

						return Token(".", TokenType.period);
					}

					case ',': {
						i += 1;

						return Token(",", TokenType.comma);
					}

					case '=': {
						i += 1;

						return Token("=", TokenType.equals);
					}

					case '"': {
						immutable (size_t) iNext = (i + 1);
						size_t j = iNext;

						while ((j < source.length) && source[j] != '"') j += 1;

						if (source[j] == '"') {
							i = (j + 1);

							return Token(source[iNext .. j], TokenType.stringLiteral);
						}

						i = j;

						return Token(
							"Unexpected end of file before end of string literal",
							TokenType.error
						);
					}

					case '0': .. case '9': {
						size_t j = (i + 1);

						while ((j < source.length) && isDigit(source[j])) j += 1;

						if ((j < source.length) && (source[j] == '.')) {
							j += 1;

							if ((j < source.length) && isDigit(source[j])) {
								// Handle decimal places.
								j += 1;

								while ((j < source.length) && isDigit(source[j])) j += 1;
							} else {
								// Just a regular period, go back.
								j -= 2;
							}
						}

						immutable (size_t) iOld = i;
						i = j;

						return Token(source[iOld .. j], TokenType.numberLiteral);
					}

					default: {
						size_t j = (i + 1);

						while ((j < source.length) && isAlpha(source[j])) j += 1;

						immutable (size_t) iOld = i;
						i = j;

						return Token(source[iOld .. j], TokenType.identifier);
					}
				}
			}

			return Token("", TokenType.eof);
		}

		Section requireSection(in string sectionName) {
			Optional!Section lookedUpSection = this.sections.lookup(sectionName);

			if (lookedUpSection.hasValue()) {
				return lookedUpSection.value();
			}

			auto section = new Section();

			this.sections.assign(sectionName, section);

			return section;
		}

		// Create the default section.
		Section defaultSection = requireSection("");
		Section currentSection = defaultSection;

		for (;;) {
			Token token = eatToken();

			tokenParse: switch (token.type) {
				case TokenType.error: {
					print(token.text);

					return false;
				}

				case TokenType.eof: goto exit;

				case TokenType.bracketLeft: {
					token = eatToken();

					if (token.type == TokenType.bracketRight) {
						currentSection = defaultSection;
					} else {
						if (token.type != TokenType.identifier) {
							print("Expected section name following start of section");

							return false;
						}

						if (eatToken().type != TokenType.bracketRight) {
							print("Expected section end following name of section");

							return false;
						}

						currentSection = requireSection(idup(token.text));
					}

					immutable (TokenType) endlTokenType = eatToken().type;

					if ((endlTokenType != TokenType.newline) && (endlTokenType != TokenType.eof)) {
						print("Expected end of line after key value declaration");

						return false;
					}
				} break tokenParse;

				case TokenType.identifier: {
					if (eatToken().type != TokenType.equals) {
						print("Expected assignment `=` after key");

						return false;
					}

					Token valueToken = eatToken();

					valueParse: switch (valueToken.type) {
						case TokenType.identifier: {
							constantParse: switch (valueToken.text) {
								case "true": {
									currentSection.assign(idup(token.text), ConfigValue(true));
								} break constantParse;

								case "false": {
									currentSection.assign(idup(token.text), ConfigValue(false));
								} break constantParse;

								default: {
									print("Expected keyword literal after assignment `=`");

									return false;
								}
							}
						} break valueParse;

						case TokenType.stringLiteral: {
							currentSection.assign(
								idup(token.text),
								ConfigValue(idup(valueToken.text))
							);
						} break valueParse;

						case TokenType.numberLiteral: {
							immutable (Optional!long) parsedInteger = parseInteger(valueToken.text);

							if (parsedInteger.hasValue()) {
								immutable (long) integer = parsedInteger.value();

								if (integer < uint.max) {
									currentSection.assign(
										idup(token.text),
										ConfigValue(cast(int)integer)
									);

									break valueParse;
								}
							}

							// Parsing as signed decimal failed, try float now.
							immutable (Optional!double) parsedFloat = parseFloat(valueToken.text);

							if (parsedFloat.hasValue()) {
								currentSection.assign(
									idup(token.text),
									ConfigValue(parsedFloat.value())
								);

								break valueParse;
							}

							print("Could not parse number literal");

							return false;
						}

						default: {
							print("Expected value literal after assignment `=`");

							return false;
						}
					}

					immutable (TokenType) endlTokenType = eatToken().type;

					if ((endlTokenType != TokenType.newline) && (endlTokenType != TokenType.eof)) {
						print("Expected end of line after key value declaration");

						return false;
					}
				} break tokenParse;

				default: {
					print("Unexpected token " ~ idup(token.text) ~ "in source");

					return false;
				}
			}
		}

		exit:
		return true;
	}
}

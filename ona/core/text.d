module ona.core.text;

private import
	ona.core.memory,
	ona.core.os,
	ona.core.types,
	std.algorithm,
	std.ascii;

/**
 * Generative state machine for extracting lexemes from textual data using a simple ruleset system.
 */
public struct Lexer {
	/**
	 * Source data to lex.
	 */
	const (char)[] source;

	/**
	 * Set of lexible symbol character tokens.
	 */
	const (char)[] symbols;

	/**
	 * Set of skippable characters that are ignored.
	 */
	const (char)[] skippable;

	/**
	 * Constructs a `Lexer` using `source` as the source data to lex, `symbols` as the set of symbol
	 * tokens, and `skippable` as the characters that are ignored.
	 *
	 * Note that both `symbols` and `skippable` will delimit identifier tokens. For example, a
	 * `Tokenizer` with a `symbols` value of `"."` and a `skippable` value of ` ` will tokenize
	 * `"my.print value"` as `["my", ".", "print", "value"]`.
	 */
	@nogc
	this(in char[] source, in char[] symbols, in char[] skippable) pure {
		this.source = source;
		this.symbols = symbols;
		this.skippable = skippable;
	}

	/**
	 * Checks if `c` is a skippable character.
	 */
	@nogc
	bool isSkippable(in char c) pure const {
		foreach (skip; this.skippable) if (c == skip) return true;

		return false;
	}

	/**
	 * Checks if `c` is a symbol token character.
	 */
	@nogc
	bool isSymbol(in char c) pure const {
		foreach (symbol; this.symbols) if (c == symbol) return true;

		return false;
	}

	/**
	 * Lexes `Lexer.source` and generatively produces each lexeme for `action` to consume according
	 * to the ruleset specified in `Lexer.symbols` and `Lexer.skippable`.
	 */
	@nogc
	int opApply(int delegate(in char[] token) @nogc action) const {
		size_t cursor;

		while (cursor < this.source.length) {
			if (isWhite(this.source[cursor])) {
				// Skip whitespace.
				cursor += 1;
			} else if (this.isSymbol(this.source[cursor])) {
				// Handle single character symbols directly.
				immutable tokenEnd = (cursor + 1);

				if (action(this.source[cursor .. tokenEnd])) return 1;

				cursor = tokenEnd;
			} else {
				// The smallest a token can be is 1 character.
				size_t tokenEnd = cursor;

				while (
					(tokenEnd < this.source.length) &&
					(!this.isSkippable(this.source[tokenEnd])) &&
					(!this.isSymbol(this.source[tokenEnd]))
				) {
					tokenEnd += 1;
				}

				if (action(this.source[cursor .. tokenEnd])) return 1;

				cursor = tokenEnd;
			}
		}

		return 0;
	}
}

/**
 * Reference-counted, UTF-8-encoded character sequence.
 */
public struct String {
	static assert(this.sizeof == 32, (typeof(this).stringof ~ " is not 32 bytes"));

	private enum staticBufferSize = 24;

	private align(1) union Data {
		ubyte* dynamic;

		ubyte[staticBufferSize] static_;
	}

	private uint size;

	private uint length;

	private Data data;

	/**
	 * Creates a `String` from the ASCII / UTF-8 `data`.
	 */
	@nogc
	public this(in char[] data) {
		if (data.length <= this.size.max) {
			ubyte[] buffer = this.createBuffer(cast(uint)data.length);

			foreach (c; data) this.length += ((c & 0xC0) != 0x80);

			copyMemory(buffer, cast(const (ubyte)[])data);
		}
	}

	@nogc
	public this(this) pure {
		if (this.isDynamic()) this.refCounter() += 1;
	}

	@nogc
	public ~this() {
		if (this.isDynamic()) {
			const (size_t) refCount = (this.refCounter() -= 1);

			if (refCount == 0) deallocate(this.data.dynamic);
		}
	}

	/**
	 * A weak reference view of the UTF-8 `String` contents in its raw byte interpretation.
	 */
	@nogc
	public const (ubyte)[] asBytes() const pure return {
		return (
			this.isDynamic() ?
			this.data.dynamic[size_t.sizeof .. this.size] :
			this.data.static_[0 .. this.size]
		);
	}

	/**
	 * A weak reference view of the UTF-8 `String` contents in its `Chars` interpretation.
	 */
	@nogc
	public const (char)[] asChars() const pure {
		return (cast(const (char[]))this.asBytes());
	}

	@nogc
	private ubyte[] createBuffer(in uint size) return {
		if (size > staticBufferSize) {
			this.size = size;
			this.data.dynamic = allocate(size_t.sizeof + clamp(size, 0, this.size.max)).ptr;

			if (this.data.dynamic) {
				this.refCounter() = 1;

				return (this.data.dynamic + size_t.sizeof)[0 .. this.size];
			} else {
				this.size = 0;
			}
		} else {
			return this.data.static_[0 .. size];
		}

		return null;
	}

	/**
	 * Checks whether or not the `String` is using dynamic memory.
	 */
	@nogc
	public bool isDynamic() const pure {
		return (this.size > staticBufferSize);
	}

	/**
	 * The number of unicode characters that compose the `String`.
	 *
	 * Note that `String` length and `String` size are not the same thing, due to UTF-8 encoding,
	 * which uses multiple bytes to express certain characters.
	 *
	 * `String.lengthOf` also doesn't include null terminators.
	 */
	@nogc
	public uint lengthOf() const pure {
		return this.length;
	}

	/**
	 * Retrieves the character pointer of the `String`.
	 */
	@nogc
	public const (char)* pointerOf() const pure return {
		return (
			this.isDynamic() ?
			(cast(const (char)*)this.data.dynamic) :
			(cast(const (char)*)this.data.static_.ptr)
		);
	}

	@nogc
	public bool opEquals(in String that) const {
		return (this.asBytes() == that.asBytes());
	}

	@nogc
	private ref size_t refCounter() pure {
		assert(this.isDynamic(), "Attempt to access ref counter of non-dynamic String");

		return (*(cast(size_t*)this.data.dynamic));
	}

	/**
	 * Creates a copy of `str` with a null-terminating sentinel character on the end.
	 */
	@nogc
	public static String sentineled(in String str) {
		String sentineledString;
		ubyte[] buffer = sentineledString.createBuffer(str.lengthOf() + 1);

		if (buffer) {
			sentineledString.length = str.length;

			copyMemory(buffer, str.asBytes());
		}

		return sentineledString;
	}

	@nogc
	public ulong toHash() pure const {
		ulong hash = 5381;

		foreach (c; this.asBytes()) hash = (((hash << 5) + hash) ^ c);

		return hash;
	}
}

/**
 * Creates a `String` representing a decimal from the integer `value`.
 */
@nogc
public String decString(Type)(in Type value) {
	static if (__traits(isIntegral, Type)) {
		if (value) {
			enum base = 10;
			char[28] buffer;
			size_t bufferCount;

			static if (__traits(isUnsigned, Type)) {
				// Unsigend types need not apply.
				Type n1 = value;
			} else {
				Type n1 = void;

				if (value < 0) {
					// Negative value.
					n1 = -value;
					buffer[0] = '-';
					bufferCount += 1;
				} else {
					// Positive value.
					n1 = value;
				}
			}

			while (n1) {
				buffer[bufferCount] = cast(char)((n1 % base) + '0');
				n1 = (n1 / base);
				bufferCount += 1;
			}

			foreach (i; 0 .. (bufferCount / 2)) {
				swap(buffer[i], buffer[bufferCount - i - 1]);
			}

			return String(buffer[0 .. bufferCount]);
		} else {
			// Return string of zero.
			return String("0");
		}
	} else static if (__traits(isFloating, Type)) {
		return String("0.0");
	}
}

/**
 * Creates a `String` representing a hexadecimal from the integer `value`.
 */
@nogc
public String hexString(Type)(in Type value) {
	static if (__traits(isIntegral, Type)) {
		if (value) {
			enum hexLength = (Type.sizeof << 1);
			enum hexPrefix = "0x";
			enum hexDigits = "0123456789ABCDEF";
			char[hexPrefix.length + hexLength] buffer;
			size_t bufferCount = hexPrefix.length;

			copyMemory(buffer, hexPrefix);

			for (size_t j = ((hexLength - 1) * 4); bufferCount < buffer.length; j -= 4) {
				buffer[bufferCount] = hexDigits[(value >> j) & 0x0f];
				bufferCount += 1;
			}

			return String(buffer[0 .. bufferCount]);
		} else {
			// Return string of zero.
			return String("0x0");
		}
	} else static if (__traits(isFloating, Type)) {
		return String("0x0");
	}
}


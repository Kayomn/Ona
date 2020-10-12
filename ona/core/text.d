module ona.core.text;

private import
	ona.core.memory,
	ona.core.os,
	ona.core.types,
	std.algorithm;

/**
 * Non-owning view into a UTF-8-encoded character sequence.
 */
public struct Chars {
	static assert((this.sizeof == 24), (typeof(this).stringof ~ " is not 24 bytes"));

	/**
	 * Raw ASCII view the `Chars`.
	 */
	private const (char)[] values;

	/**
	 * Number of UTF-8 / ASCII characters in the `Chars`.
	 */
	private size_t length;

	/**
	 * Parses `data` into a `Chars` as if it's UTF-8 encoded.
	 */
	@nogc
	public static Chars parseUTF8(const (char)[] data) pure {
		size_t length;

		foreach (c; data) length += ((c & 0xC0) != 0x80);

		return Chars(data, length);
	}

	@nogc
	public bool opEquals(Chars that) const {
		return (this.values == that.values);
	}

	@nogc
	public ulong toHash() pure const {
		ulong hash = 5381;

		foreach (c; this.asBytes()) hash = (((hash << 5) + hash) ^ c);

		return hash;
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
	public this(const (char)[] data) {
		this(Chars.parseUTF8(data));
	}

	/**
	 * Creates a `String` from the character data `chars`.
	 */
	@nogc
	public this(Chars chars) {
		if (chars.values.length <= this.size.max) {
			ubyte[] buffer = this.createBuffer(cast(uint)chars.values.length);
			this.length = cast(uint)chars.length;

			copyMemory(buffer, cast(const (ubyte)[])chars.values);
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
	public Chars chars() const pure {
		return Chars((cast(const (char[]))this.asBytes()), this.size);
	}

	@nogc
	private ubyte[] createBuffer(uint size) return {
		if (size > staticBufferSize) {
			this.data.dynamic = allocate(size_t.sizeof + clamp(size, 0, this.size.max)).ptr;

			if (this.data.dynamic) {
				this.refCounter() = 1;
				this.size = size;

				return (this.data.dynamic + size_t.sizeof)[0 .. this.size];
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
	public bool opEquals(String that) const {
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
	public static String sentineled(String str) {
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

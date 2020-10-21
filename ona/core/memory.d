module ona.core.memory;

private import
	std.algorithm,
	std.conv,
	std.traits,
	ona.core.types;

/**
 * Runtime-polymorphic allocator API.
 *
 * The API expressed in `Allocator` is very similar to that which is provided in `ona.core.os` with
 * `ona.core.os.allocate`, `ona.core.os.deallocate`, and `ona.core.os.reallocate`.
 */
public interface Allocator {
	/**
	 * Allocates `size` bytes of memory.
	 */
	@nogc
	ubyte[] allocate(size_t size);

	/**
	 * Deallocates the memory at `allocation` using the `Allocator`.
	 *
	 * If `allocation` is not an address allocated by the `Allocator` then the program will
	 * encounter a critical error.
	 */
	@nogc
	void deallocate(void* allocation);

	/**
	 * Destroys and deallocates `instance`.
	 *
	 * If `instance` is not an address allocated by the `Allocator` then the program will encounter
	 * a critical error.
	 */
	void free(Type)(ref Type instance) {
		destroy(instance);
		this.deallocate(cast(void*)instance);

		instance = null;
	}

	/**
	 * Allocates and initializes an instance of `Type` using `args`.
	 */
	Type make(Type, Args...)(
		auto ref Args args
	) if ((!is(Type == interface)) || (!isAbstractClass!Type)) {
		static if (is(Type == class)) {
			Type instance = (cast(Type)this.allocate(__traits(classInstanceSize, Type)).ptr);

			if (instance) return emplace(instance, args);
		} else {
			Type* instance = (cast(Type*)this.allocate(Type.sizeof).ptr);

			if (instnace) return emplace(instance, args);
		}

		return null;
	}

	/**
	 * Reallocates `allocation` to be `size` bytes using the global allocator.
	 *
	 * If `allocation` is greater than `size`, the memory contents will be truncated in order to fit the
	 * new size.
	 */
	@nogc
	ubyte[] reallocate(void* allocation, size_t size);
}

/**
 * An allocation strategy that behaves similarly to automatic memory. Data must be deallocated in
 * the same order that it is allocated or it cannot otherwise be deallocated and will be skipped.
 *
 * The allocation strategy very simply bumps the pointer until an allocation cannot fit on the
 * current page, at which point it will be allocated on a new page.
 *
 * The only reliable way to free all memory created by a `StackAllocator` is by calling
 * `StackAllocator.reset`, which also invalidates any memory allocated by the allocator. Be careful
 * when using `StackAllocator`, as dangling pointers are easily created using it.
 */
public final class StackAllocator : Allocator {
	private struct Page {
		Page* nextPage;

		ubyte[pageSize] memory;
	}

	private enum pageSize = 4096;

	private Allocator baseAllocator;

	private size_t cursor;

	private size_t pageCount;

	private Page headPage;

	private Page* currentPage;

	/**
	 * Constructs a `StackAllocator` that uses the global allocator as its backing allocation
	 * strategy for all initial stack page allocations.
	 */
	@nogc
	public this() pure {
		this.currentPage = (&this.headPage);
		this.pageCount = 1;
	}

	/**
	 * Constructs a `StackAllocator` that uses `baseAllocator` as the backing allocation strategy
	 * for all initial stack page allocations.
	 *
	 * Passing a `null` value is functionally equivalent to calling the default constructor.
	 */
	@nogc
	public this(Allocator baseAllocator) pure {
		this.baseAllocator = baseAllocator;

		this();
	}

	@nogc
	public ~this() {
		Page* page = (&this.headPage);

		if (this.baseAllocator) {
			while (page) {
				Page* nextPage = page.nextPage;

				this.baseAllocator.deallocate(page);

				page = nextPage;
			}
		} else {
			while (page) {
				Page* nextPage = page.nextPage;

				deallocate(page);

				page = nextPage;
			}
		}
	}

	@nogc
	override ubyte[] allocate(size_t size) {
		bool createPage() {
			if (this.currentPage.nextPage) {
				this.currentPage = this.currentPage.nextPage;

				return true;
			} else {
				Page* page = (cast(Page*)(
					this.baseAllocator ?
					this.baseAllocator.allocate(Page.sizeof) :
					allocate(Page.sizeof)
				).ptr);

				if (page) {
					(*page) = Page.init;

					if (this.currentPage) {
						this.currentPage.nextPage = page;
						this.currentPage = this.currentPage.nextPage;
						this.pageCount += 1;
					}

					return true;
				}
			}

			return false;
		}

		if (size >= ((pageSize * this.pageCount) - this.cursor)) {
			if (!createPage()) {
				// Allocation failure.
				return null;
			}
		}

		scope (exit) this.cursor += size;

		immutable relativeCursor = (this.cursor - ((this.pageCount - 1) * pageSize));

		return this.currentPage.memory[relativeCursor .. (relativeCursor + size)];
	}

	@nogc
	override void deallocate(void* allocation) {

	}

	@nogc
	override ubyte[] reallocate(void* allocation, size_t size) {
		return null;
	}

	/**
	 * Resets the `StackAllocator` cursor, making all memory allocated by it invalid.
	 */
	@nogc
	public void reset() pure {
		this.cursor = 0;
	}
}

/**
 * A view of the data in `value` as a raw byte interpretation with a range equal to `Type.sizeof`.
 */
@nogc
public const (ubyte)[] asBytes(Type)(ref const (Type) value) pure {
	return (cast(ubyte*)&value)[0 .. Type.sizeof];
}

/**
 * Copies the memory contents of `source` into `destination`, returning the number of bytes that
 * were actually copied.
 *
 * The number of copied bytes may not reflect the size of `source` when `destination` is smaller
 * than it.
 */
@nogc
public size_t copyMemory(ubyte[] destination, const (ubyte)[] source) pure {
	immutable (size_t) copiedBytes = min(destination.length, source.length);

	foreach (i; 0 .. copiedBytes) destination[i] = source[i];

	return copiedBytes;
}

/**
 * Creates an instance of `Type` in automatic memory on the stack, calling the constructor that
 * matches `args`.
 *
 * Be weary of dangling references to the scoped value, as once the value leaves scope it is
 * destroyed and its memory invalidated.
 */
public auto scoped(Type, Args...)(auto ref Args args) {
	static struct Scoped {
		alias __instance this;

		private ubyte[__traits(classInstanceSize, Type)] buffer;

		static if (hasElaborateDestructor!Type) {
			public ~this() {
				if (this.exists()) destroy(this.__instance());
			}
		}

		@nogc
		public Type __instance() pure {
			return (cast(Type)this.buffer.ptr);
		}
	}

	Scoped scoped = void;

	emplace(scoped.__instance(), args);

	return scoped;
}

/**
 * Writes `value` to every byte in the range of `destination`.
 */
@nogc
public void writeMemory(ubyte[] destination, const (ubyte) value) pure {
	ubyte* target = destination.ptr;
	const (ubyte)* boundary = (target + destination.length);

	while (target != boundary) {
		(*target) = value;
		target += 1;
	}
}

/**
 * Writes zero to every byte in the range of `destination`.
 */
@nogc
public void zeroMemory(ubyte[] destination) pure {
	writeMemory(destination, 0);
}

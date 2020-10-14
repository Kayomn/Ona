module ona.core.memory;

private import
	std.algorithm,
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
	 * Deallocates the memory at `allocation` using the global allocator.
	 *
	 * If `allocation` is not an address allocated by the global allocator then the program will
	 * encounter a critical error.
	 */
	@nogc
	void deallocate(void* allocation);

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

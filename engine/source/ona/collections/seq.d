module ona.collections.seq;

private import core.memory, ona.functional, std.traits;

/**
 * Densely packed stack of `ValueType` that exposes array-like random access semantics, ensuring
 * O(1) pushes, removal, and random access, as well as efficient value iteration.
 */
public final class PackedStack(ValueType) {
	public alias opDollar = count;

	private ValueType* valuePointer;

	private uint valueCapacity;

	private uint valueCount;

	/**
	 * Initializes with a capacity of `0` and performs no internal allocations.
	 */
	public this() {

	}

	/**
	 * Initializes with a capacity of `initialCapacity` with internal allocation.
	 */
	public this(in uint initialCapacity) {
		this.valuePointer = (cast(ValueType*)pureMalloc(ValueType.sizeof * initialCapacity));
		assert(this.valuePointer, typeof(this).stringof ~ " construction out of memory");
		this.valueCapacity = initialCapacity;

		static if (hasIndirections!ValueType) {
			GC.addRange(this.valuePointer, this.valueCapacity);
		}
	}

	public ~this() {
		static if (is(ValueType == struct) && hasElaborateDestructor!ValueType) {
			foreach (ref value; this.valueBuffer[0 .. this.valueCount]) {
				value.__xdtor();
			}
		}

		static if (hasIndirections!ValueType) {
			GC.removeRange(this.valuePointer);
		}

		pureFree(this.valuePointer);
	}

	/**
	 * Returns the pre-allocated capacity for more values.
	 */
	public uint capacity() const pure {
		return this.valueCapacity;
	}

	/**
	 * Returns the number of values contained.
	 */
	public uint count() const pure {
		return this.valueCount;
	}

	/**
	 * Clears all values, calling the elaborate destructors of any contained values in the process.
	 */
	public void clear() pure {
		static if (is(ValueType == struct) && hasElaborateDestructor!ValueType) {
			foreach (ref value; this.valuePointer[0 .. this.valueCount]) {
				value.__xdtor();
			}
		}

		this.valueCount = 0;
	}

	/**
	 * Returns the value at `index`, asserting if `index` is not less than [PackedStack.count].
	 */
	public ref inout (ValueType) opIndex(in uint index) inout pure {
		assert(index < this.valueCount, typeof(this).stringof ~ " index out of bounds");

		return this.valuePointer[index];
	}

	/**
	 * Returns a non-owning view of the internal contents.
	 *
	 * Any subsequent mutable function called on the [PackedStack] instance should be considered to
	 * render any existing views into its contents invalid. Because of this, it is highly advised
	 * that a handle into its memory contents not be saved anywhere but temporary storage.
	 */
	public inout (ValueType)[] opSlice() inout {
		return this.valuePointer[0 .. this.valueCount];
	}

	/**
	 * Returns a non-owning view of the internal contents for `b` elements from index `a`, asserting
	 * if `a .. b` is an invalid range.
	 *
	 * Any subsequent mutable function called on the [PackedStack] instance should be considered to
	 * render any existing views into its contents invalid. Because of this, it is highly advised
	 * that a handle into its memory contents not be saved anywhere but temporary storage.
	 */
	public const (ValueType)[] opSlice(in uint a, in uint b) inout {
		assert(
			(a < this.valueCount) && (b <= this.valueCount),
			typeof(this).stringof ~ " slice out of bounds"
		);

		return this.valuePointer[a .. b];
	}

	/**
	 * Attempts to pop the top-most value, returning it or [none] if [PackedStack.count] is `0`.
	 */
	public Optional!ValueType pop() {
		if (this.valueCount != 0) {
			this.valueCount -= 1;

			return Optional!ValueType(this.valuePointer[this.valueCount]);
		}

		return Optional!ValueType();
	}

	/**
	 * Pushes `value`, returning a reference to the inserted value and asserting if the limit of
	 * `uint.max` values is reached.
	 *
	 * If the current capacity is reached, an implicit call to [PackedStack.reserve] is made,
	 * growing by a factor of `2`.
	 */
	public ref ValueType push(ValueType value) {
		assert(this.valueCount < uint.max, typeof(this).stringof ~ " push overflow");

		immutable (uint) valueCapacity = this.valueCapacity;

		if (this.valueCount == valueCapacity) {
			this.reserve((valueCapacity == 0) ? 2 : valueCapacity);
		}

		scope (exit) this.valueCount += 1;

		return (this.valuePointer[this.valueCount] = value);
	}

	/**
	 * Grows the value capacity for more elements by `additionalCapacity`, asserting if the request
	 * for more memory failed.
	 */
	public void reserve(in uint additionalCapacity) {
		static if (hasIndirections!ValueType) {
			GC.removeRange(this.valuePointer);
		}

		this.valueCapacity += additionalCapacity;
		this.valuePointer = (cast(ValueType*)pureRealloc(this.valuePointer, this.valueCapacity));

		assert(this.valuePointer, typeof(this).stringof ~ " reservation out of memory");

		static if (hasIndirections!ValueType) {
			GC.addRange(this.valuePointer, this.valueCapacity);
		}
	}
}

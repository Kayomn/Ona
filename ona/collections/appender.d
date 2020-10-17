module ona.collections.appender;

import
	ona.core,
	std.traits;

/**
 * Sequential buffer of linearly allocate memory with `O(1)` random access that is designed for
 * small amounts of insertion.
 */
public class Appender(ValueType, IndexType = size_t) {
	private NotNull!Allocator allocator;

	private ValueType* values;

	private IndexType count;

	private IndexType capacity;

	/**
	 * Constructs an `Appender` with `allocator` as the `Allocator`.
	 */
	@nogc
	public this(NotNull!Allocator allocator) pure {
		this.allocator = allocator;
	}

	public ~this() {
		static if (hasElaborateDestructor!ValueType) {
			foreach (ref value; this.valuesOf()) destroy(value);
		}

		this.allocator.deallocate(this.values.ptr);
	}

	/**
	 * Attempts to retrieve the `Allocator` being used by the `Appender`.
	 */
	@nogc
	public inout (NotNull!Allocator) allocatorOf() inout pure {
		return this.allocator;
	}

	/**
	 * Attempts to append `value` to the back of the `Appender`, reallocating the `Appender` memory
	 * buffer if more space is needed to accomodate and returning the address of the appended value.
	 *
	 * Should `Appender.append` fail, `null` is returned instead.
	 */
	@nogc
	public ValueType* append(in ValueType value) {
		if (this.count >= this.capacity) {
			if (!this.reserve(this.count ? this.count : 2)) {
				// Allocation failure.
				return null;
			}
		}

		ValueType* bufferAddress = (this.values + this.count);
		this.count += 1;
		(*bufferAddress) = value;

		return bufferAddress;
	}

	/**
	 * Retrieves the value at index `index` by reference.
	 */
	@nogc
	public ref inout (ValueType) at(in IndexType index) inout pure {
		return this.values[index];
	}

	/**
	 * Retrieves the memory buffer capacity of the `Appender`.
	 *
	 * `Appender.capacityOf` is not to be confused with `Appender.countOf`.
	 *
	 * Typically, `Appender.capacityOf` is used to make informed decisions about the cost of
	 * appending operations, which will be relatively cheap if the `Appender` has enough memory in
	 * its existing buffer to store the additional values.
	 */
	@nogc
	public IndexType capacityOf() const pure {
		return this.capacity;
	}

	/**
	 * Clears all contents of the `Appender`, destroying values if they are destructible.
	 */
	public void clear() {
		static if (hasElaborateDestructor!ValueType) {
			foreach (ref value; this.valuesOf()) destroy(value);
		}

		this.count = 0;
	}

	/**
	 * Retrieves the number of values stored by the `Appender`.
	 *
	 * `Appender.countOf` is not to be confused with `Appender.capacityOf`.
	 */
	@nogc
	public IndexType countOf() const pure {
		return this.count;
	}

	/**
	 * Attempts to compress the `Appender` memory buffer, reallocating it to be just big enough to
	 * store the current contents.
	 *
	 * Subsequent calls to `Appender.append` directly after calling `Appender.compress` are
	 * guaranteed to reallocate.
	 *
	 * Failure to compress the buffer will result in `false` being returned, otherwise `true`.
	 */
	@nogc
	public bool compress() {
		this.values = (cast(ValueType*)this.allocator.reallocate(
			this.values,
			(this.count * ValueType.sizeof)
		).ptr);

		return (this.values != null);
	}

	/**
	 * Attempts to reserve `capacity` more elements of space in the `Appender` memory buffer by
	 * reallocating it.
	 *
	 * `Appender.reserve` is a useful way to avoid unnecessary reallocations when the required
	 * storage space can be reasoned about ahead of time.
	 *
	 * Failure to reserve space for the buffer will result in `false` being returned, otherwise
	 * `true`.
	 */
	@nogc
	public bool reserve(in IndexType capacity) {
		immutable newCapacity = (this.capacity + capacity);

		this.values = (cast(ValueType*)this.allocator.reallocate(
			this.values,
			(newCapacity * ValueType.sizeof)
		).ptr);

		if (this.values) {
			this.capacity = newCapacity;

			return true;
		}

		return false;
	}

	/**
	 * Truncates `n` values from the end of the `Appender`, destroying them if they are
	 * destructible.
	 */
	@nogc
	public void truncate(in IndexType n) {
		assert((n < this.count), "Invalid range");

		static if (hasElaborateDestructor!ValueType) {
			foreach (ref value; this.values[(this.count - n), this.count]) destroy(value);
		}

		this.count -= n;
	}

	/**
	 * Returns a view into the values contained in the `Appender`.
	 *
	 * Note that the returned value is a weak reference, and subsequent calls to `Appender` may
	 * invalidate it.
	 */
	@nogc
	public inout (ValueType)[] valuesOf() inout pure {
		return this.values[0 .. this.count];
	}
}

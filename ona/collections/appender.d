module ona.collections.appender;

import
	ona.core,
	std.traits;

/**
 * Sequential buffer of linearly allocate memory with `O(1)` random access.
 */
public struct Appender(Type) {
	private Allocator allocator;

	private size_t count;

	private Type[] values;

	/**
	 * Constructs an `Appender` with `allocator` as the `Allocator`.
	 *
	 * Passing a `null` `Allocator` will have the same result as initializing with `Appender.init`.
	 */
	@nogc
	public this(Allocator allocator) pure {
		this.allocator = allocator;
	}

	public ~this() {
		static if (hasElaborateDestructor!Type) {
			foreach (ref value; this.valuesOf()) destroy(value);
		}

		if (this.allocator) {
			this.allocator.deallocate(this.values.ptr);
		} else {
			deallocate(this.values.ptr);
		}
	}

	/**
	 * Attempts to retrieve the `Allocator` being used by the `Appender`.
	 *
	 * If the `Appender` is using the global allocator, `null` is returned instead.
	 */
	@nogc
	public inout (Allocator) allocatorOf() inout pure {
		return this.allocator;
	}

	/**
	 * Attempts to append `value` to the back of the `Appender`, reallocating the `Appender` memory
	 * buffer if more space is needed to accomodate and returning the address of the appended value.
	 *
	 * Should `Appender.append` fail, `null` is returned instead.
	 */
	@nogc
	public Type* append(Type value) {
		if (this.count >= this.values.length) {
			if (!this.reserve(this.count ? this.count : 2)) {
				// Allocation failure.
				return nil!(Type*);
			}
		}

		Type* bufferAddress = (this.values.ptr + this.count);
		this.count += 1;
		(*bufferAddress) = value;

		return bufferAddress;
	}

	/**
	 * Retrieves the value at index `index` by reference.
	 */
	@nogc
	public ref inout (Type) at(const (size_t) index) inout pure {
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
	public size_t capacityOf() const pure {
		return this.values.length;
	}

	/**
	 * Clears all contents of the `Appender`, destroying values if they are destructible.
	 */
	public void clear() {
		static if (hasElaborateDestructor!Type) {
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
	public size_t countOf() const pure {
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
		if (this.allocator) {
			this.values = cast(Type[])this.allocator.reallocate(
				this.values.ptr,
				(this.count * Type.sizeof)
			);
		} else {
			this.values = cast(Type[])reallocate(this.values.ptr, (this.count * Type.sizeof));
		}

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
	public bool reserve(const (size_t) capacity) {
		if (this.allocator) {
			this.values = cast(Type[])this.allocator.reallocate(
				this.values.ptr,
				((this.values.length + capacity) * Type.sizeof)
			);
		} else {
			this.values = cast(Type[])reallocate(
				this.values.ptr,
				((this.values.length + capacity) * Type.sizeof)
			);
		}

		return (this.values != null);
	}

	/**
	 * Truncates `n` values from the end of the `Appender`, destroying them if they are
	 * destructible.
	 */
	@nogc
	public void truncate(const (size_t) n) {
		assert((n < this.count), "Invalid range");

		static if (hasElaborateDestructor!Type) {
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
	public inout (Type[]) valuesOf() inout {
		return this.values[0 .. this.count];
	}
}

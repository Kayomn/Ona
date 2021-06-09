/**
 * Type and logic abstractions for facilitating functional programming concepts.
 */
module ona.functional;

private import std.traits;

/**
 * Lightweight wrapper around `Type`, which provides semantics for checking the existence of a
 * value in a type safe manner.
 *
 * For reference types, such as classes and pointers, the struct will utilize `null` to express an
 * empty state, allowing it to avoid any additional data packing.
 */
public struct Optional(Type) {
	private Type payload;

	static if (isPointer!Type || is(Type == class)) {
		/**
		 * Assigns a value of `value`, with a `null` value leaving the contents empty.
		 */
		public this(inout (Type) value) inout {
			this.payload = value;
		}

		/**
		 * Returns `true` if a value is contained, otherwise `false`.
		 */
		public bool hasValue() const {
			return (this.payload !is null);
		}
	} else {
		private bool hasValueType = false;

		/**
		 * Assigns a value of `value`.
		 */
		public this(inout (Type) value) inout {
			this.hasValueType = true;
			this.payload = value;
		}

		/**
		 * Returns `true` if a value is contained, otherwise `false`.
		 */
		public bool hasValue() const {
			return this.hasValueType;
		}
	}

	/**
	 * Attempts to return the contained value, asserting if it does not exist.
	 *
	 * It is recommended that [Optional.hasValue] be called first if the presence of a value
	 * cannot be assumed.
	 */
	public ref inout (Type) value() inout {
		assert(this.hasValue(), "Attempt to access empty Optional.");

		return this.payload;
	}

	/**
	 * Attempts to return the contained value or `fallback` if it does not exist.
	 *
	 * Unlike [Optional.value], this function will always return some sort of value, meaning that it
	 * is not required to call [Optional.hasValue].
	 */
	public inout (Type) or(inout (Type) fallback) inout {
		return this.or(fallback);
	}

	/**
	 * Attempts to return the contained value by reference or `fallback` by reference if it does not
	 * exist.
	 *
	 * Unlike [Optional.value], this function will always return some sort of value, meaning that it
	 * is not required to call [Optional.hasValue].
	 */
	@safe
	public ref inout (Type) or(ref inout (Type) fallback) inout {
		if (this.hasValue()) {
			return this.payload;
		}

		return fallback;
	}
}

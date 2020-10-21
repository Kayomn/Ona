module ona.core.types;

private import
	std.algorithm,
	std.conv,
	std.traits;

/**
 * A type wrapper for a value *or* an error.
 *
 * `Result` is commonly used for functions that may return a result or fail, in which case the error
 * data may be returned in the `Result` instead.
 */
public struct Result(ValueType, ErrorType = void) {
	private enum hasErrorType = !is(ErrorType == void);

	private enum okFlagIndex = max(ValueType.sizeof, ErrorType.sizeof);

	private ubyte[max(ValueType.sizeof, ErrorType.sizeof) + 1] store;

	/**
	 * Calls the copy constructor of the contents of `Result`.
	 */
	public this(ref Result that) {
		if (this) {
			static if (hasElaborateCopyConstructor!ValueType) {
				this.valueOf() = that.valueOf();
			}
		} else {
			static if (hasElaborateCopyConstructor!ErrorType) {
				this.errorOf() = that.errorOf();
			}
		}
	}

	public ~this() {
		if (this) {
			static if (hasElaborateDestructor!ValueType) {
				destroy(this.valueOf());
			}
		} else {
			static if (hasElaborateDestructor!ValueType) {
				destroy(this.errorOf());
			}
		}
	}

	static if (hasErrorType) {
		/**
		 * Creates a `Result` containing an error value of `error`, indicating a failed operation.
		 */
		@nogc
		public static Result fail(ErrorType error) {
			Result res;

			emplace((cast(ErrorType*)res.store.ptr), error);

			return res;
		}
	} else {
		/**
		 * Creates a `Result` containing an error, indicating a failed operation.
		 */
		@nogc
		public static Result fail() pure {
			return Result();
		}
	}

	/**
	 * Creates a `Result` containing the value of a successful operation.
	 */
	@nogc
	public static Result ok(ValueType value) pure {
		Result res;
		res.store[okFlagIndex] = 1;

		emplace((cast(ValueType*)res.store.ptr), value);

		return res;
	}

	/**
	 * Checks that the `Result` is ok.
	 */
	@nogc
	public Type opCast(Type)() const pure if (is(Type == bool)) {
		return (cast(bool)this.store[okFlagIndex]);
	}

	/**
	 * Retrieves the value contained in the `Result` from a successful operation.
	 *
	 * Attempting to retrieve a value from a failed `Result` will cause critically erroneous
	 * behavior. Because of this, it is important to check that the `Result` returned from a
	 * successful operation before attempting to access it.
	 */
	@nogc
	public ref inout (ValueType) valueOf() inout pure {
		assert(cast(bool)this, "Result is erroneous");

		return (*(cast(inout (ValueType)*)this.store));
	}

	static if (hasErrorType) {
		/**
		 * Retrieves the error contained in the `Result` from a failed operation.
		 *
		 * Attempting to retrieve an error from a successful `Result` will cause critically erroneous
		 * behavior. Because of this, it is important to check that the `Result` returned from a
		 * failed operation before attempting to access it.
		 */
		@nogc
		public ref inout (ErrorType) errorOf() inout {
			assert((!cast(bool)this), "Result is ok");

			return (*(cast(inout (ErrorType)*)this.store));
		}
	}
}

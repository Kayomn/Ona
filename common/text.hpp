
namespace Ona {
	/**
	 * Checks if `c` is an alphabetical character, returning `true` if it is and `false` if it
	 * isn't.
	 */
	constexpr bool IsAlpha(int const c) {
		return (((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')));
	}

	/**
	 * Checks if `c` is a digit character, returning `true` if it is and `false` if it isn't.
	 */
	constexpr bool IsDigit(int const c) {
		return ((c >= '0') && (c <= '9'));
	}

	/**
	 * Creates a `String` containing `value` as signed decimal-encoded text.
	 */
	String StringSigned(int64_t value);

	/**
	 * Creates a `String` containing `value` as unsigned decimal-encoded text.
	 */
	String StringUnsigned(uint64_t value);

	/**
	 * Creates a new `String` from `string` with the format arguments in `values`.
	 *
	 * Failure to supply a value for a format placeholder in the `string` will result in that
	 * placeholder being left in the produced `String`.
	 *
	 * Excess format specifiers are ignored.
	 */
	String Format(String const & string, std::initializer_list<String> const & values);

	/**
	 * Attempts to parse `string` as if it were a signed decimal integer, returning `true` if it
	 * could be parsed from `string`, otherwise `false` if the operation failed.
	 *
	 * If `output` is not `nullptr` then the parsing result is written to it.
	 */
	bool ParseSigned(String const & string, int64_t * output);

	/**
	 * Attempts to parse `string` as if it were an unsigned decimal integer, returning `true` if it
	 * could be parsed from `string`, otherwise `false` if the operation failed.
	 *
	 * If `output` is not `nullptr` then the parsing result is written to it.
	 */
	bool ParseUnsigned(String const & string, uint64_t * output);

	/**
	 * Attempts to parse `string` as if it were a floating point number, returning `true` if it
	 * could be parsed from `string`, otherwise `false` if the operation failed.
	 *
	 * If `output` is not `nullptr` then the parsing result is written to it.
	 */
	bool ParseFloating(String const & string, double * output);

	bool LoadText(Stream * stream, Allocator * allocator, String * text);
}

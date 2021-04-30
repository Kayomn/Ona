#include "common.hpp"

namespace Ona {
	String Format(String const & string, std::initializer_list<String> const & values) {
		// TODO: Implement.
		return string;
	}

	String StringSigned(int64_t value) {
		if (value) {
			enum {
				Base = 10,
			};

			FixedArray<char, 32> buffer;
			size_t bufferCount = 0;

			if (value < 0) {
				// Negative value.
				value = (-value);
				buffer.At(0) = '-';
				bufferCount += 1;
			}

			while (value) {
				buffer.At(bufferCount) = static_cast<char>((value % Base) + '0');
				value /= Base;
				bufferCount += 1;
			}

			size_t const bufferCountHalf = (bufferCount / 2);

			for (size_t i = 0; i < bufferCountHalf; i += 1) {
				size_t const iReverse = (bufferCount - i - 1);
				char const temp = buffer.At(i);
				buffer.At(i) = buffer.At(iReverse);
				buffer.At(iReverse) = temp;
			}

			return String(buffer.Sliced(0, bufferCount));
		}

		return String{"0"};
	}

	String StringUnsigned(uint64_t value) {
		if (value) {
			enum {
				Base = 10,
			};

			FixedArray<char, 32> buffer = {};
			size_t bufferCount = 0;

			while (value) {
				buffer.At(bufferCount) = static_cast<char>((value % Base) + '0');
				value /= Base;
				bufferCount += 1;
			}

			size_t const bufferCountHalf = (bufferCount / 2);

			for (size_t i = 0; i < bufferCountHalf; i += 1) {
				size_t const iReverse = (bufferCount - i - 1);
				char const temp = buffer.At(i);
				buffer.At(i) = buffer.At(iReverse);
				buffer.At(iReverse) = temp;
			}

			return String{buffer.Sliced(0, bufferCount)};
		}

		return String{"0"};
	}

	bool ParseSigned(String const & string, int64_t * output) {
		if (string.Length()) {
			Chars chars = string.Chars();
			int64_t result = 0;
			int64_t fact = 1;

			if (chars.At(0) == '-') {
				// Ignore the sign in the string for now, we'll come back to it tho.
				chars = chars.Sliced(1, chars.length);
				fact = -1;
			}

			// Length may have changed if a sign was removed.
			if (chars.length && (chars.At(0) > '0') && (chars.At(0) < ':')) {
				for (size_t i = 0; i < chars.length; i += 1) {
					char const c = chars.At(chars.length - (i + 1));

					if (!IsDigit(c)) return false;

					result += static_cast<int64_t>((c - '0') * Pow(10, i));
				}

				if (output) (*output) = (result * fact);

				return true;
			}
		}

		return false;
	}

	bool ParseUnsigned(String const & string, uint64_t * output) {
		if (string.Length()) {
			Chars chars = string.Chars();
			uint64_t result = 0;

			// Length may have changed if a sign was removed.
			if (chars.length && (chars.At(0) > '0') && (chars.At(0) < ':')) {
				for (size_t i = 0; i < chars.length; i += 1) {
					char const c = chars.At(chars.length - (i + 1));

					if (!IsDigit(c)) return false;

					result += static_cast<uint64_t>((c - '0') * Pow(10, i));
				}

				if (output) (*output) = result;

				return true;
			}
		}

		return false;
	}

	bool ParseFloating(String const & string, double * output) {
		if (string.Length()) {
			double result = 0;
			Chars chars = string.Chars();
			double fact = 1;
			bool hasDecimal = false;

			if (chars.At(0) == '-') {
				chars = chars.Sliced(1, chars.length);
				fact = -1;
			}

			// Length may have changed if a sign was removed.
			if (chars.length) {
				if (chars.At(0) == '.') {
					hasDecimal = true;
				} else if (chars.At(0) && (chars.length > 1) && (chars.At(1) == '0')) {
					// A floating point value cannot begin with sequential zeroes.
					return false;
				}

				for (auto c : chars) {
					if (c == '.') {
						if (hasDecimal) {
							// Multiple decimal places are not valid.
							return false;
						} else {
							hasDecimal = true;
						}
					} else if (IsDigit(c)) {
						int const d = (c - '0');

						if ((d >= 0) && (d <= 9)) {
							if (hasDecimal) fact /= 10.0f;

							result = ((result * 10.0f) + static_cast<double>(d));
						}
					}
				}
			}

			if (output) (*output) = (result * fact);

			return true;
		}

		return false;
	}

	bool LoadText(Stream * stream, Allocator * allocator, String * text) {
		DynamicArray<char> buffer{allocator};
		uint64_t textSize;

		while ((textSize = stream->AvailableBytes()) != 0) {
			if (!buffer.Resize(textSize)) return false;

			if (stream->ReadUtf8(buffer.Values()) != textSize) return false;
		}

		(*text) = String{buffer.Values()};

		return true;
	}
}

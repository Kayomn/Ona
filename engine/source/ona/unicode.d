/**
 * Parsing and analysis of text encoded under the unicode format standard.
 */
module ona.unicode;

private import ona.functional, std.ascii, std.math;

/**
 * Attempts to parse and return the result as a `double` if one could be parsed from `source`,
 * otherwise nothing if the operation failed.
 */
public Optional!double parseFloat(const (char)[] source) {
	if (source.length) {
		double result = 0;
		double fact = 1;
		bool hasDecimal = false;

		if (source[0] == '-') {
			source = source[1 .. source.length];
			fact = -1;
		}

		// Length may have changed if a sign was removed.
		foreach (c; source) {
			if (c == '.') {
				if (hasDecimal) {
					// Multiple decimal places are not valid.
					return Optional!double();
				}

				hasDecimal = true;
			} else if (!isDigit(c)) {
				return Optional!double();
			}

			immutable (int) d = (c - '0');

			if ((d >= 0) && (d <= 9)) {
				if (hasDecimal) fact /= 10.0f;

				result = ((result * 10.0f) + (cast(double)d));
			}
		}

		return Optional!double(result * fact);
	}

	return Optional!double();
}

unittest {
	import std.math : isClose;

	foreach (invalid; ["de.8", "e8", "8z", "xc", "89.j", "99.0i"]) {
		assert(!parseFloat("de.8").hasValue(), invalid ~ " is valid when it shouldn't be");
	}

	{
		Optional!double parsedFloat = parseFloat("3.14");

		assert(parsedFloat.hasValue());
		assert(isClose(parsedFloat.value(), 3.14));
	}
}

/**
 * Attempts to parse and return the result as a `long` if one could be parsed from `source`,
 * otherwise nothing if the operation failed.
 */
public Optional!long parseInteger(const (char)[] source) {
	if (source.length) {
		long result = 0;
		long fact = 1;

		if (source[0] == '-') {
			// Ignore the sign in the string for now, we'll come back to it tho.
			source = source[1 .. source.length];
			fact = (-1);
		}

		// Length may have changed if a sign was removed.
		if (source.length && (source[0] > '0') && (source[0] < ':')) {
			foreach (i; 0 .. source.length) {
				immutable (char) c = source[source.length - (i + 1)];

				if (!isDigit(c)) return Optional!long();

				result += (cast(long)((c - '0') * pow(10, i)));
			}

			return Optional!long(result * fact);
		}
	}

	return Optional!long();
}

unittest {
	foreach (invalid; ["de.8", "e8", "8z", "xc", "89.j", "99.0i"]) {
		assert(!parseFloat(invalid).hasValue());
	}

	{
		Optional!double parsedFloat = parseFloat("3.14");

		assert(parsedFloat.hasValue());
		assert(isClose(parsedFloat.value(), 3.14));
	}
}

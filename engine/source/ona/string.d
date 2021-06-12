/**
 * Creation and manipulation of tightly packed text sequences.
 */
module ona.string;

/**
 * Creates and returns a `string` representation of `value`.
 */
public string unsignedString(long value) {
	if (value) {
		enum base = 10;
		char[32] buffer = 0;
		size_t bufferCount = 0;

		while (value) {
			buffer[bufferCount] = (cast(char)((value % base) + '0'));
			value /= base;
			bufferCount += 1;
		}

		foreach (i; 0 .. (bufferCount / 2)) {
			immutable (size_t) iReverse = (bufferCount - i - 1);
			immutable (char) temp = buffer[i];
			buffer[i] = buffer[iReverse];
			buffer[iReverse] = temp;
		}

		return idup(buffer[0 .. bufferCount]);
	}

	return "0";
}

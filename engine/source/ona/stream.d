/**
 * Input and output data streaming programming interface.
 */
module ona.stream;

private import ona.functional, std.algorithm;

/**
 * Bit flags used for indicating the access state of a [Stream].
 */
public enum StreamAccess {
	unknown = 0,
	read = 1,
	write = 2,
}

/**
 * Scrollable stream of bytes to be read to and written from depending on its access rules.
 */
public interface Stream {
	/**
	 * Seeks the available number of bytes that the stream currently contains.
	 *
	 * Reading from and writing to the stream may modify the result, so it is advised that it be
	 * used in a loop in case any further bytes become available after any initial reads.
	 */
	@nogc
	ulong availableBytes();

	/**
	 * Attempts to read as many bytes as will fit in `input` from the stream, returning the number
	 * of bytes actually read.
	 */
	@system
	ulong readBytes(ubyte[] input);

	/**
	 * Repositions the stream cursor to `offset` bytes relative to the beginning of the stream.
	 */
	@system
	long seekHead(in long offset);

	/**
	 * Repositions the stream cursor to `offset` bytes relative to the end of the stream.
	 */
	@system
	long seekTail(in long offset);

	/**
	 * Moves the cursor `offset` bytes from its current position.
	 */
	@system
	long skip(in long offset);

	/**
	 * Attempts to write as many bytes to the stream as there are in `output`, returning the number
	 * of bytes actually written.
	 */
	@system
	ulong writeBytes(in ubyte[] output);
}

/**
 * A [Stream] abstraction for arbitrary buffers of bytes.
 */
public final class MemoryStream : Stream {
	private size_t cursor = 0;

	private ubyte[] buffer = [];

	/**
	 * Assigns `buffer` as the memory to be read from and written to by the stream operations.
	 */
	@nogc
	public this(ubyte[] buffer) {
		this.buffer = buffer;
	}

	/**
	 * Seeks the available number of bytes that the memory currently contains.
	 *
	 * Reading from and writing to the memory **will** modify the result, so it is advised that it
	 * be used in a loop in case any further bytes become available after any initial reads.
	 */
	@nogc
	public ulong availableBytes() {
		return (this.buffer.length - this.cursor);
	}

	/**
	 * Attempts to read as many bytes as will fit in `input` from the memory, returning the number
	 * of bytes actually read.
	 */
	@nogc
	public ulong readBytes(ubyte[] input) {
		size_t bytesRead = 0;

		while ((this.cursor < this.buffer.length) && (bytesRead < input.length)) {
			input[bytesRead] = this.buffer[this.cursor];
			this.cursor += 1;
			bytesRead += 1;
		}

		return (cast(ulong)bytesRead);
	}

	/**
	 * Repositions the stream cursor to `offset` bytes relative to the beginning of the memory,
	 * returning `0` if `offset` is an invalid location in the memory buffer.
	 */
	@nogc
	public long seekHead(in long offset) {
		if ((offset < 0) || (offset > this.buffer.length)) {
			this.cursor = 0;
		} else {
			this.cursor = offset;
		}

		return this.cursor;
	}

	/**
	 * Repositions the stream cursor to `offset` bytes relative to the end of the memory, returning
	 * `0` if `offset` is an invalid location in the memory buffer.
	 */
	@nogc
	public long seekTail(in long offset) {
		if ((offset > 0) || (offset < (-this.buffer.length))) {
			this.cursor = 0;
		} else {
			this.cursor = offset;
		}

		return this.cursor;
	}

	/**
	 * Moves the cursor `offset` bytes from its current position.
	 */
	@nogc
	public long skip(in long offset) {
		this.cursor = clamp(this.cursor + offset, 0, this.cursor);

		return this.cursor;
	}

	/**
	 * Attempts to write as many bytes to the memory as there are in `output`, returning the number
	 * of bytes actually written.
	 */
	@nogc
	public ulong writeBytes(in ubyte[] output) {
		size_t bytesWritten = 0;

		while ((this.cursor < this.buffer.length) && (bytesWritten < output.length)) {
			this.buffer[this.cursor] = output[bytesWritten];
			this.cursor += 1;
			bytesWritten += 1;
		}

		return bytesWritten;
	}
}

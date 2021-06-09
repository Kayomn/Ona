/**
 * System-level abstraction layer.
 */
module ona.system;

private import
	sys = core.sys.linux.unistd,
	fcntl = core.sys.posix.fcntl,
	ona.functional,
	ona.stream,
	std.string;

/**
 * Opaque handle to a file-like system resource held in a persistent data storage.
 */
public final class SystemStream : Stream {
	private static SystemStream outputStream;

	private string systemPath = "";

	private int handle = -1;

	private StreamAccess accessFlags = StreamAccess.unknown;

	static this() {
		outputStream = new SystemStream(sys.STDOUT_FILENO, "stdout", StreamAccess.write);
	}

	/**
	 * Initializes a stream that holds no file.
	 */
	@safe @nogc
	public this() {

	}

	@safe @nogc
	private this(in int handle, in string systemPath, in StreamAccess accessFlags) {
		this.handle = handle;
		this.systemPath = systemPath;
		this.accessFlags = accessFlags;
	}

	@trusted @nogc
	public ~this() {
		this.close();
	}

	/**
	 * Seeks the available number of bytes that the file is known to currently contain.
	 *
	 * Reading from and writing to the file may modify the result, so it is advised that this
	 * function be called to update the available bytes with every stream mutation.
	 */
	@trusted @nogc
	public ulong availableBytes() {
		immutable (long) cursor = this.skip(0);
		immutable (long) length = (this.seekTail(0) - cursor);

		this.seekHead(cursor);

		return length;
	}

	/**
	 * Attempts to close the stream, silently failing if there is nothing to close.
	 */
	@trusted @nogc
	public void close() {
		this.flush();
		sys.close(this.handle);
	}

	/**
	 * Flushes any buffered write operations to the file.
	 */
	@trusted @nogc
	public void flush() {

	}

	/**
	 * Reads and interprets the contents of the data at `systemPath` into a newly allocated `ubyte`
	 * buffer, returning it if all available data was successfully read, otherwise nothing, wrapped
	 * in an [Optional].
	 */
	@trusted
	public static Optional!(ubyte[]) load(in string systemPath) {
		scope stream = new SystemStream();

		if (stream.open(systemPath, StreamAccess.read)) {
			ulong availableBytes = stream.availableBytes();
			size_t cursor = 0;
			ubyte[] readBuffer = [];

			do {
				readBuffer.length += availableBytes;

				if (stream.readBytes(readBuffer[cursor .. availableBytes]) != availableBytes) {
					return Optional!(ubyte[])();
				}

				cursor += availableBytes;
				availableBytes = stream.availableBytes();
			} while (availableBytes);

			return Optional!(ubyte[])(readBuffer);
		}

		return Optional!(ubyte[])();
	}

	/**
	 * Attempts to allocate and open a [SystemStream] on `systemPath` with the access rules defined
	 * by `accessFlags`, returning it if the operation was successful, otherwise nothing, wrapped in
	 * an [Optional].
	 */
	@trusted
	public bool open(in string systemPath, in StreamAccess accessFlags) {
		int unixAccessFlags = 0;

		if (accessFlags & StreamAccess.read) unixAccessFlags |= fcntl.O_RDONLY;

		if (accessFlags & StreamAccess.write) unixAccessFlags |= (fcntl.O_WRONLY | fcntl.O_CREAT);

		this.handle = fcntl.open(
			toStringz(systemPath),
			unixAccessFlags,
			(fcntl.S_IRUSR | fcntl.S_IRGRP | fcntl.S_IROTH | fcntl.S_IWUSR)
		);

		if (this.handle > 0) {
			this.systemPath = systemPath;

			return true;
		}

		return false;
	}

	/**
	 * Returns the standard output [SystemStream].
	 */
	@safe @nogc
	public static SystemStream output() {
		return outputStream;
	}

	/**
	 * Attempts to read as many bytes as will fit in `input` from the file, returning the number of
	 * bytes actually read.
	 */
	@trusted @nogc
	public ulong readBytes(ubyte[] input) {
		immutable (sys.ssize_t) bytesRead = sys.read(this.handle, input.ptr, input.length);

		return ((bytesRead > -1) ? (cast(ulong)bytesRead) : 0);
	}

	/**
	 * Repositions the file cursor to `offset` bytes relative to the beginning of the stream,
	 * returning the number of bytes that the cursor has moved by relative to its previous position.
	 */
	@trusted @nogc
	public long seekHead(in long offset) {
		immutable (sys.off_t) bytesSought = sys.lseek(this.handle, cast(sys.off_t)offset, 0);

		return ((bytesSought > -1) ? bytesSought : 0);
	}

	/**
	 * Repositions the file cursor to `offset` bytes relative to the end of the stream, returning
	 * the number of bytes that the cursor has moved by relative to its previous position.
	 */
	@trusted @nogc
	public long seekTail(in long offset) {
		immutable (sys.off_t) bytesSought = sys.lseek(this.handle, cast(sys.off_t)offset, 2);

		return ((bytesSought > -1) ? bytesSought : 0);
	}

	/**
	 * Repositions the file cursor to `offset` bytes relative to its current position, returning the
	 * number of bytes that the cursor has moved by relative to its previous position.
	 */
	@trusted @nogc
	public long skip(in long offset) {
		immutable (sys.off_t) bytesSought = sys.lseek(this.handle, cast(sys.off_t)offset, 1);

		return ((bytesSought > -1) ? bytesSought : 0);
	}

	@safe @nogc
	public override string toString() const {
		return this.systemPath;
	}

	/**
	 * Attempts to write as many bytes to the file as there are in `output`, returning the number of
	 * bytes actually written.
	 */
	@trusted @nogc
	public ulong writeBytes(in ubyte[] output) {
		immutable (sys.ssize_t) bytesWritten =
			sys.write(this.handle, output.ptr, output.length);

		return ((bytesWritten > -1) ? (cast(ulong)bytesWritten) : 0);
	}
}

/**
 * Prints `message` to the system standard output.
 */
@trusted @nogc
public void print(in char[] message) {
	auto outputStream = SystemStream.output();
	ubyte[1] endline = ['\n'];

	outputStream.writeBytes(cast(ubyte[])message);
	outputStream.writeBytes(endline);
	outputStream.flush();
}

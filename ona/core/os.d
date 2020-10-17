module ona.core.os;

private import
	core.stdc.errno,
	core.stdc.stdlib,
	core.sys.posix.unistd,
	core.sys.posix.fcntl,
	core.sys.posix.dlfcn,
	ona.core.memory,
	ona.core.text,
	ona.core.types;

/**
 * File resource type used for handling file-like resources such as local system files, virtual
 * files, network-mapped I/O, and other things.
 */
public struct File {
	/**
	 * Flags used to desciminate the access state of a `File`.
	 */
	enum OpenFlags {
		unknown,
		read = 0x1,
		write = 0x2
	}

	/**
	 * Table of file interaction operations defined by the implementation of the `File` or nothing.
	 *
	 * `null` will cause any `File` function to act as a NO-OP.
	 */
	const (FileOperations)* operations;

	/**
	 * Opaque, platform-agnostic file descriptor handle.
	 */
	FileDescriptor descriptor;

	/**
	 * Attempts to close the handle of the `File` and clears the operations table.
	 */
	@nogc
	void free() {
		if (this.operations) this.operations.closer(this.descriptor);

		this.descriptor.clear();

		this.operations = null;
	}

	/**
	 * Attempts to print the UTF-8 encoded text in `str` to `File`.
	 *
	 * The number of bytes actually printed is returned.
	 */
	@nogc
	void print(String str) {
		if (this.operations) this.operations.writer(this.descriptor, str.asBytes());
	}

	/**
	 * Attempts to read the contents of the `File` into `output`.
	 *
	 * The number of bytes actually read is returned.
	 */
	@nogc
	size_t read(ubyte[] output) {
		return (this.operations ? this.operations.reader(this.descriptor, output) : 0);
	}

	/**
	 * Attempts to seek from the beginning of the `File` by `offset` bytes.
	 *
	 * The number of bytes actually sought is returned.
	 */
	@nogc
	long seekHead(long offset) {
		if (this.operations) {
			return this.operations.seeker(this.descriptor, FileOperations.SeekBase.head, offset);
		}

		return 0;
	}

	/**
	 * Attempts to seek from the end of the `File` by `offset` bytes.
	 *
	 * The number of bytes actually sought is returned.
	 */
	@nogc
	long seekTail(long offset) {
		if (this.operations) {
			return this.operations.seeker(this.descriptor, FileOperations.SeekBase.tail, offset);
		}

		return 0;
	}

	/**
	 * Attempts to skip `offset` number of bytes in the `File`.
	 *
	 * The number of bytes actually skipped is returned.
	 */
	@nogc
	long skip(long offset) {
		if (this.operations) {
			return this.operations.seeker(this.descriptor, FileOperations.SeekBase.current, offset);
		}

		return 0;
	}

	/**
	 * Attempts to read the current cursor position in the `File`.
	 */
	@nogc
	long tell() {
		if (this.operations) {
			return this.operations.seeker(this.descriptor, FileOperations.SeekBase.current, 0);
		}

		return 0;
	}

	/**
	 * Attempts to write `input` to the `File`.
	 *
	 * The number of bytes actually written is returned.
	 */
	@nogc
	size_t write(const (void)[] input) {
		return (this.operations ? this.operations.writer(this.descriptor, input) : 0);
	}
}

/**
 * Function table of operations supported by an implementor of `File`.
 */
public struct FileOperations {
	/**
	 * File cursor base positions to start seeking from.
	 */
	enum SeekBase {
		head,
		current,
		tail
	}

	/**
	 * Function used to close `descriptor`.
	 */
	@nogc
	void function(FileDescriptor descriptor) closer;

	/**
	 * Function used to seek through the bytes of the file at `descriptor`.
	 */
	@nogc
	long function(FileDescriptor descriptor, SeekBase seekBase, long offset) seeker;

	/**
	 * Function used to read bytes of the file at `descriptor`.
	 */
	@nogc
	size_t function(FileDescriptor descriptor, ubyte[] output) reader;

	/**
	 * Function used to write bytes of the file at `descriptor`.
	 */
	@nogc
	size_t function(FileDescriptor descriptor, const (void)[] input) writer;
}

/**
 * Library resource type used for handling runtime-dynamic code.
 */
public struct Library {
	private void* context;

	/**
	 * Attempts to find a symbol in the `Library` matching `symbolName`.
	 *
	 * `Library.findSymbol` has no type safety guarantees, and must therefore be assured by the
	 * caller instead.
	 */
	@nogc
	public void* findSymbol(String symbolName) {
		return dlsym(this.context, String.sentineled(symbolName).pointerOf());
	}

	/**
	 * Frees the resources associated with the `Library`.
	 */
	@nogc
	public void free() {
		dlclose(this.context);
	}
}

/**
 * Platform-agnostic file descriptor sum type.
 */
public union FileDescriptor {
	/**
	 * Unix-style 32-bit handle values.
	 */
	int unixHandle;

	/**
	 * Windows-style opaque userdata addresses.
	 */
	void* userdata;

	/**
	 * Clears the `FileDescriptor` to zero.
	 */
	@nogc
	void clear() {
		this = this.init;
	}

	/**
	 * Checks whether or not the `FileDescriptor` is zero.
	 */
	@nogc
	bool isEmpty() const pure {
		return (this.userdata != null);
	}
}

/**
 * Errors produced when a file fails to be opened.
 */
public enum FileOpenError {
	badAccess,
	notFound
}

/**
 * Errors produced when a file fails to be loaded.
 */
public enum FileLoadError {
	badAccess,
	notFound,
	outOfMemory
}

/**
 * Errors produced when a library fails to be accessed.
 */
public enum LibraryError {
	cantLoad
}

private FileOperations osFileOperations = {
	(FileDescriptor descriptor) {
		close(descriptor.unixHandle);
	},

	(FileDescriptor descriptor, FileOperations.SeekBase seekBase, ptrdiff_t offset)  {
		const (long) bytesSought = lseek(descriptor.unixHandle, offset, seekBase);

		return ((bytesSought > -1) ? cast(size_t)bytesSought : 0);
	},

	(FileDescriptor descriptor, ubyte[] output) {
		const (ptrdiff_t) bytesRead = read(descriptor.unixHandle, output.ptr, output.length);

		return ((bytesRead > -1) ? cast(size_t)bytesRead : 0);
	},

	(FileDescriptor descriptor, const (void)[] input) {
		const (ptrdiff_t) bytesWritten = write(descriptor.unixHandle, input.ptr, input.length);

		return ((bytesWritten > -1) ? cast(size_t)bytesWritten : 0);
	}
};

/**
 * Retrieves the global allocator.
 */
@nogc
public Allocator globalAllocator() {
	final class GlobalAllocator : Allocator {
		@nogc
		override ubyte[] allocate(size_t size) {
			ubyte* allocation = cast(ubyte*)malloc(size);
			size *= (allocation != null);

			return allocation[0 .. size];
		}

		@nogc
		override void deallocate(void* allocation) {
			free(allocation);
		}

		@nogc
		override ubyte[] reallocate(void* allocation, size_t size) {
			allocation = realloc(allocation, size);
			size *= (allocation != null);

			return (cast(ubyte*)allocation)[0 .. size];
		}
	}

	__gshared Allocator allocator = new GlobalAllocator();

	return allocator;
}

/**
 * Checks if a file at `filePath` is accessible to the process.
 */
@nogc
public bool checkFile(String filePath) {
	return (access(String.sentineled(filePath).pointerOf(), F_OK) != -1);
}

// public Type make(Args...)(NotNull!Allocator allocator, auto ref Args args) {
// 	enum classSize = __traits(classInstanceSize, Type);
// 	Type instance = allocator.allocate(classSize).ptr;

// 	if (instance) emplace(instance, args);

// 	return instance;
// }

/**
 * Attempts to open a file at `filePath`.
 *
 * The access rules given to the file are passed using `flags`. See `File.OpenFlags` for more
 * information.
 */
@nogc
public Result!(File, FileOpenError) openFile(String filePath, File.OpenFlags flags) {
	alias Res = Result!(File, FileOpenError);
	int unixAccessFlags;

	if (flags & File.OpenFlags.read) {
		unixAccessFlags |= O_RDONLY;
	}

	if (flags & File.OpenFlags.write) {
		unixAccessFlags |= (O_WRONLY | O_CREAT);
	}

	/**
	*         Read Write Execute
	*        -------------------
	* Owner | yes  yes   no
	* Group | yes  no    no
	* Other | yes  no    no
	*/
	const (int) handle = open(
		String.sentineled(filePath).pointerOf(),
		unixAccessFlags,
		(S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR)
	);

	if (handle == -1) {
		switch (errno) {
			case ENOENT: return Res.fail(FileOpenError.notFound);

			default: return Res.fail(FileOpenError.badAccess);
		}
	}

	return Res.ok(File((&osFileOperations), FileDescriptor(handle)));
}

/**
 * Attempts to open a library at `libraryPath`.
 */
@nogc
public Result!(Library, LibraryError) openLibrary(String libraryPath) {
	alias Res = Result!(Library, LibraryError);
	Library library = {context: dlopen(String.sentineled(libraryPath).pointerOf(), RTLD_NOW)};

	return (library.context ? Res.ok(library) : Res.fail(LibraryError.cantLoad));
}

/**
 * The global output file used for writing general information to.
 */
@nogc
public File outFile() {
	return File((&osFileOperations), FileDescriptor(STDOUT_FILENO));
}

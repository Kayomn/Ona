#include "ona/core/header.hpp"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>

namespace Ona::Core {
	FileOperations osFileOperations = {
		FileOperations::Closer{[](FileDescriptor descriptor) -> void {
			close(descriptor.unixHandle);
		}},

		FileOperations::Seeker{[](
			FileDescriptor descriptor,
			FileOperations::SeekBase seekBase,
			int64_t offset
		) -> int64_t {
			int64_t const bytesSought = lseek(descriptor.unixHandle, offset, seekBase);

			return ((bytesSought > -1) ? static_cast<size_t>(bytesSought) : 0);
		}},

		FileOperations::Reader{[](FileDescriptor descriptor, Slice<uint8_t> output) -> size_t {
			ssize_t const bytesRead = read(descriptor.unixHandle, output.pointer, output.length);

			return ((bytesRead > -1) ? static_cast<size_t>(bytesRead) : 0);
		}},

		FileOperations::Writer{[](FileDescriptor descriptor, Slice<uint8_t const> input) -> size_t {
			ssize_t const bytesWritten = write(descriptor.unixHandle, input.pointer, input.length);

			return ((bytesWritten > -1) ? static_cast<size_t>(bytesWritten) : 0);
		}},
	};

	Slice<uint8_t> Allocate(size_t size) {
		uint8_t * allocationAddress = reinterpret_cast<uint8_t *>(malloc(size));
		size *= (allocationAddress != nullptr);

		return SliceOf(allocationAddress, size);
	}

	void Deallocate(void * allocation) {
		free(allocation);
	}

	Result<Array<uint8_t>, FileLoadError> LoadFile(
		Optional<Allocator *> allocator,
		String const & filePath
	) {
		using Res = Result<Array<uint8_t>, FileLoadError>;
		FileOpenError openError = {};
		let openedFile = OpenFile(filePath, File::OpenRead);

		if (openedFile.IsOk()) {
			let file = openedFile.Value();
			int64_t const fileSize = file.SeekTail(0);

			file.SeekHead(0);

			let fileContents = Array<uint8_t>::Init(allocator, static_cast<size_t>(fileSize));

			if (fileContents.Count() == fileSize) {
				return Res::Ok(fileContents);
			}

			return Res::Fail(FileLoadError::Resources);
		} else switch (openedFile.Error()) {
			case FileOpenError::NotFound: return Res::Fail(FileLoadError::NotFound);

			case FileOpenError::BadAccess: return Res::Fail(FileLoadError::BadAccess);
		}
	}

	Result<File, FileOpenError> OpenFile(String const & filePath, File::OpenFlags flags) {
		using Res = Result<File, FileOpenError>;
		int unixAccessFlags = 0;

		if (flags & File::OpenRead) {
			unixAccessFlags |= O_RDONLY;
		}

		if (flags & File::OpenWrite) {
			unixAccessFlags |= (O_WRONLY | O_CREAT);
		}

		/**
		*         Read Write Execute
		*        -------------------
		* Owner | yes  yes   no
		* Group | yes  no    no
		* Other | yes  no    no
		*/
		int const handle = open(
			String::Sentineled(filePath).AsChars().pointer,
			unixAccessFlags,
			(S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR)
		);

		if (handle == -1) {
			switch (errno) {
				case ENOENT: return Res::Fail(FileOpenError::NotFound);

				default: return Res::Fail(FileOpenError::BadAccess);
			}
		}

		return Res::Ok(File{
			Optional<FileOperations const *>{&osFileOperations},
			FileDescriptor{handle}
		});
	}

	Slice<uint8_t> Reallocate(void * allocation, size_t size) {
		uint8_t * allocationAddress = reinterpret_cast<uint8_t *>(realloc(allocation, size));
		size *= (allocationAddress != nullptr);

		return SliceOf(allocationAddress, size);
	}

	void FileDescriptor::Clear() {
		Slice<uint8_t> memory = SliceOf(
			reinterpret_cast<uint8_t *>(this),
			sizeof(FileDescriptor)
		);

		ZeroMemory(memory);
	}

	File & OutFile() {
		static let file = File{
			Optional<FileOperations const *>{&osFileOperations},
			FileDescriptor{STDOUT_FILENO}
		};

		return file;
	}

	bool CheckFile(String const & filePath) {
		return (access(String::Sentineled(filePath).AsChars().pointer, F_OK) != -1);
	}

	void File::Free() {
		if (this->operations.HasValue() && this->operations->closer.HasValue()) {
			this->operations->closer.Value()(this->descriptor);
		}
	}

	void File::Print(String const & string) {
		if (this->operations.HasValue() && this->operations->writer.HasValue()) {
			this->operations->writer.Value()(this->descriptor, string.AsBytes());
		}
	}

	size_t File::Read(Slice<uint8_t> const & output) {
		if (this->operations.HasValue() && this->operations->reader.HasValue()) {
			return this->operations->reader.Value()(this->descriptor, output);
		}

		return 0;
	}

	int64_t File::SeekHead(int64_t offset) {
		if (this->operations.HasValue() && this->operations->seeker.HasValue()) {
			return this->operations->seeker.Value()(
				this->descriptor,
				FileOperations::SeekBaseHead,
				offset
			);
		}

		return 0;
	}

	int64_t File::SeekTail(int64_t offset) {
		if (this->operations.HasValue() && this->operations->seeker.HasValue()) {
			return this->operations->seeker.Value()(
				this->descriptor,
				FileOperations::SeekBaseTail,
				offset
			);
		}

		return 0;
	}

	int64_t File::Skip(int64_t offset) {
		if (this->operations.HasValue() && this->operations->seeker.HasValue()) {
			return this->operations->seeker.Value()(
				this->descriptor,
				FileOperations::SeekBaseCurrent,
				offset
			);
		}

		return 0;
	}

	int64_t File::Tell() {
		if (this->operations.HasValue() && this->operations->seeker.HasValue()) {
			return this->operations->seeker.Value()(
				this->descriptor,
				FileOperations::SeekBaseCurrent,
				0
			);
		}

		return 0;
	}

	size_t File::Write(Slice<uint8_t const> const & input) {
		if (this->operations.HasValue() && this->operations->writer.HasValue()) {
			return this->operations->writer.Value()(this->descriptor, input);
		}

		return 0;
	}

	Optional<void *> Library::FindSymbol(String const & symbolName) {
		return dlsym(this->context, String::Sentineled(symbolName).AsChars().pointer);
	}

	void Library::Free() {
		dlclose(this->context);
	}

	Library OpenLibrary(String const & filePath, LibraryError * error) {
		Library library = {dlopen(String::Sentineled(filePath).AsChars().pointer, RTLD_NOW)};

		if ((!library.context) && error) {
			(*error) = LibraryError::CantLoad;
		}

		return library;
	}
}

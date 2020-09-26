#include "ona/core.hpp"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>

namespace Ona::Core {
	FileOperations osFileOperations = {
		[](FileDescriptor descriptor) -> void {
			close(descriptor.unixHandle);
		},

		[](
			FileDescriptor descriptor,
			FileOperations::SeekBase seekBase,
			int64_t offset
		) -> int64_t {
			int64_t const bytesSought = lseek(descriptor.unixHandle, offset, seekBase);

			return ((bytesSought > -1) ? static_cast<size_t>(bytesSought) : 0);
		},

		[](FileDescriptor descriptor, Slice<uint8_t> output) -> size_t {
			ssize_t const bytesRead = read(descriptor.unixHandle, output.pointer, output.length);

			return ((bytesRead > -1) ? static_cast<size_t>(bytesRead) : 0);
		},

		[](FileDescriptor descriptor, Slice<uint8_t const> input) -> size_t {
			ssize_t const bytesWritten = write(descriptor.unixHandle, input.pointer, input.length);

			return ((bytesWritten > -1) ? static_cast<size_t>(bytesWritten) : 0);
		},
	};

	Slice<uint8_t> Allocate(size_t size) {
		uint8_t * allocationAddress = reinterpret_cast<uint8_t *>(malloc(size));
		size *= (allocationAddress != nullptr);

		return SliceOf(allocationAddress, size);
	}

	void Assert(bool expression, Chars const & message) {
		if (!expression) {
			OutFile().Write(message.AsBytes());
			std::abort();
		}
	}

	void Deallocate(uint8_t * allocation) {
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

		return Res::Ok(File{&osFileOperations, FileDescriptor{handle}});
	}

	Slice<uint8_t> Reallocate(uint8_t * allocation, size_t size) {
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
		static File file = {&osFileOperations, FileDescriptor{STDOUT_FILENO}};

		return file;
	}

	bool CheckFile(String const & filePath) {
		return (access(String::Sentineled(filePath).AsChars().pointer, F_OK) != -1);
	}

	void File::Free() {
		this->operations->closer(this->descriptor);
	}

	void File::Print(String const & string) {
		this->operations->writer(this->descriptor, string.AsBytes());
	}

	size_t File::Read(Slice<uint8_t> const & output) {
		return this->operations->reader(this->descriptor, output);
	}

	int64_t File::SeekHead(int64_t offset) {
		return this->operations->seeker(this->descriptor, FileOperations::SeekBaseHead, offset);
	}

	int64_t File::SeekTail(int64_t offset) {
		return this->operations->seeker(this->descriptor, FileOperations::SeekBaseTail, offset);
	}

	int64_t File::Skip(int64_t offset) {
		return this->operations->seeker(this->descriptor, FileOperations::SeekBaseCurrent, offset);
	}

	int64_t File::Tell() {
		return this->operations->seeker(this->descriptor, FileOperations::SeekBaseCurrent, 0);
	}

	size_t File::Write(Slice<uint8_t const> const & input) {
		return this->operations->writer(this->descriptor, input);
	}

	void * Library::FindSymbol(String const & symbolName) {
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

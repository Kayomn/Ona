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
			int64_t offset,
			FileIoError * error
		) -> int64_t {
			int64_t const bytesSought = lseek(descriptor.unixHandle, offset, seekBase);

			if (bytesSought > -1) {
				return bytesSought;
			} else if (error) {
				(*error) = FileIoError::BadAccess;
			}

			return 0;
		},

		[](FileDescriptor descriptor, Slice<uint8_t> output, FileIoError * error) -> size_t {
			ssize_t const bytesRead = read(descriptor.unixHandle, output.pointer, output.length);

			if (bytesRead > -1) {
				return static_cast<size_t>(bytesRead);
			} else if (error) {
				(*error) = FileIoError::BadAccess;
			}

			return 0;
		},

		[](FileDescriptor descriptor, Slice<uint8_t const> input, FileIoError * error) -> size_t {
			ssize_t const bytesWritten = write(descriptor.unixHandle, input.pointer, input.length);

			if (bytesWritten > -1) {
				return static_cast<size_t>(bytesWritten);
			} else if (error) {
				(*error) = FileIoError::BadAccess;
			}

			return 0;
		},
	};

	Slice<uint8_t> Allocate(size_t size) {
		uint8_t * allocationAddress = reinterpret_cast<uint8_t *>(malloc(size));
		size *= (allocationAddress != nullptr);

		return SliceOf(allocationAddress, size);
	}

	void Assert(bool expression, Chars const & message) {
		if (!expression) {
			OutFile().Write(message.AsBytes(), nullptr);
			std::abort();
		}
	}

	void Deallocate(uint8_t * allocation) {
		free(allocation);
	}

	Array<uint8_t> LoadFile(Allocator * allocator, String const & filePath, FileLoadError * error) {
		static auto readToLoadError = [](FileIoError readError, FileLoadError * error) {
			if (error) switch (readError) {
				case FileIoError::None: break;

				case FileIoError::BadAccess: {
					(*error) = FileLoadError::BadAccess;
				} break;
			}
		};

		FileOpenError openError = {};
		File file = OpenFile(filePath, File::OpenRead, &openError);

		if (openError == FileOpenError::None) {
			FileIoError readError = {};
			int64_t const fileSize = file.SeekTail(0, &readError);

			file.SeekHead(0, &readError);

			if (readError == FileIoError::None) {
				readToLoadError(readError, error);
			} else if (fileSize > SIZE_MAX) {
				// Handles 32-bit platforms where files can be bigger than addressable memory.
				if (error) (*error) = FileLoadError::Resources;
			} else {
				auto fileContents = Array<uint8_t>::New(allocator, static_cast<size_t>(fileSize));

				if (fileContents.Count()) {
					file.Read(fileContents.Values(), &readError);

					if (readError == FileIoError::None) {
						return fileContents;
					}

					readToLoadError(readError, error);
				} else if (error) (*error) = FileLoadError::Resources;
			}
		} else if (error) switch (openError) {
			case FileOpenError::None: break;

			case FileOpenError::NotFound: {
				(*error) = FileLoadError::NotFound;
			} break;

			case FileOpenError::BadAccess: {
				(*error) = FileLoadError::BadAccess;
			} break;
		}

		return Array<uint8_t>{};
	}

	File OpenFile(String const & filePath, File::OpenFlags flags, FileOpenError * error) {
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

		if ((handle == -1) || error) {
			switch (errno) {
				case ENOENT: {
					(*error) = FileOpenError::NotFound;
				} break;

				default: {
					(*error) = FileOpenError::BadAccess;
				} break;
			}
		} else {
			return File{&osFileOperations, FileDescriptor{handle}};
		}

		return File::Bad();
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

	void File::Print(String const & string, FileIoError * error) {
		this->operations->writer(this->descriptor, string.AsBytes(), error);
	}

	size_t File::Read(Slice<uint8_t> const & output, FileIoError * error) {
		return this->operations->reader(this->descriptor, output, error);
	}

	int64_t File::SeekHead(int64_t offset, FileIoError * error) {
		return this->operations->seeker(
			this->descriptor,
			FileOperations::SeekBaseHead,
			offset,
			error
		);
	}

	int64_t File::SeekTail(int64_t offset, FileIoError * error) {
		return this->operations->seeker(
			this->descriptor,
			FileOperations::SeekBaseTail,
			offset,
			error
		);
	}

	int64_t File::Skip(int64_t offset, FileIoError * error) {
		return this->operations->seeker(
			this->descriptor,
			FileOperations::SeekBaseCurrent,
			offset,
			error
		);
	}

	int64_t File::Tell(FileIoError * error) {
		return this->operations->seeker(
			this->descriptor,
			FileOperations::SeekBaseCurrent,
			0,
			error
		);
	}

	size_t File::Write(Slice<uint8_t const> const & input, FileIoError * error) {
		return this->operations->writer(this->descriptor, input, error);
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

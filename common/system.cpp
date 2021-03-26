#include "common.hpp"

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <dirent.h>

static int UserdataToHandle(void * userdata) {
	return static_cast<int>(reinterpret_cast<size_t>(userdata));
};

namespace Ona {
	void FileContents::Free() {
		if (this->allocator && this->raw.pointer) this->allocator->Deallocate(this->raw.pointer);
	}

	String FileContents::ToString() const {
		return String{Slice<char const>{
			.length = this->raw.length,
			.pointer = reinterpret_cast<char const *>(this->raw.pointer),
		}};
	}

	bool OpenFile(String const & filePath, File::OpenFlags openFlags, File & file) {
		static FileOperations const unixFileOperations = {
			.closer = [](void * handle) {
				close(UserdataToHandle(handle));
			},

			.reader = [](void * handle, Slice<uint8_t> input) -> size_t {
				ssize_t const bytesRead = read(
					UserdataToHandle(handle),
					input.pointer,
					input.length
				);

				return ((bytesRead > -1) ? static_cast<size_t>(bytesRead) : 0);
			},

			.seeker = [](void * handle, int64_t offset, FileOperations::SeekMode mode) -> int64_t {
				int64_t bytesSought;

				switch (mode) {
					case FileOperations::SeekMode::Cursor: {
						bytesSought = lseek(UserdataToHandle(handle), offset, SEEK_CUR);
					} break;

					case FileOperations::SeekMode::Head: {
						bytesSought = lseek(UserdataToHandle(handle), offset, SEEK_SET);
					} break;

					case FileOperations::SeekMode::Tail: {
						bytesSought = lseek(UserdataToHandle(handle), offset, SEEK_END);
					} break;
				}

				return ((bytesSought > -1) ? static_cast<size_t>(bytesSought) : 0);
			},

			.writer = [](void * handle, Slice<uint8_t const> const & output) -> size_t {
				ssize_t const bytesWritten = write(
					UserdataToHandle(handle),
					output.pointer,
					output.length
				);

				return ((bytesWritten > -1) ? static_cast<size_t>(bytesWritten) : 0);
			},
		};

		int unixAccessFlags = 0;

		if (openFlags & File::OpenRead) unixAccessFlags |= O_RDONLY;

		if (openFlags & File::OpenWrite) unixAccessFlags |= (O_WRONLY | O_CREAT);

		/**
		*         Read Write Execute
		*        -------------------
		* Owner | yes  yes   no
		* Group | yes  no    no
		* Other | yes  no    no
		*/
		int const handle = (open(
			filePath.ZeroSentineled().Chars().pointer,
			unixAccessFlags,
			(S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR)
		));

		if (handle > 0) {
			file = File{&unixFileOperations, reinterpret_cast<void *>(handle)};

			return true;
		}

		return false;
	}

	FileLoadError LoadFile(
		Allocator * allocator,
		String const & filePath,
		FileContents & fileContents
	) {
		Owned<File> file;

		if (OpenFile(filePath, File::OpenRead, file.value)) {
			int64_t const fileLength = file.value.SeekTail(0);

			if ((fileLength > -1) && (fileLength < SIZE_MAX)) {
				Slice<uint8_t> fileBuffer = {
					.pointer = allocator->Allocate(static_cast<size_t>(fileLength))
				};

				fileBuffer.length = (fileLength * (fileBuffer.pointer != nullptr));

				file.value.SeekHead(0);
				file.value.Read(fileBuffer);

				fileContents = FileContents{
					.allocator = allocator,
					.raw = fileBuffer
				};

				return FileLoadError::None;
			}

			return FileLoadError::OutOfMemory;
		}

		return FileLoadError::FileError;
	}

	uint64_t EnumerateFiles(String const & directoryPath, Callable<void(String const &)> action) {
		DIR * directory = opendir(directoryPath.ZeroSentineled().Chars().pointer);

		if (directory) {
			uint64_t filesEnumerated = 0;
			dirent * directoryEntry;

			while ((directoryEntry = readdir(directory)) != nullptr) {
				char const * entryName = directoryEntry->d_name;

				if ((*entryName) != '.') {
					action.Invoke(String{entryName});

					filesEnumerated += 1;
				}
			}

			closedir(directory);

			return filesEnumerated;
		}

		return 0;
	}

	void * Library::FindSymbol(String const & symbol) {
		return dlsym(this->handle, symbol.ZeroSentineled().Chars().pointer);
	}

	void Library::Free() {
		dlclose(this->handle);
	}

	bool OpenLibrary(String const & libraryPath, Library & result) {
		Library library = {
			.handle = dlopen(libraryPath.ZeroSentineled().Chars().pointer, RTLD_LAZY)
		};

		if (library.handle) {
			result = library;

			return true;
		}

		return false;
	}

	void Print(String const & message) {
		String zeroSentineledMessage = message.ZeroSentineled();
		Slice<uint8_t const> const zeroSentineledBytes = zeroSentineledMessage.Bytes();

		write(STDOUT_FILENO, zeroSentineledBytes.pointer, zeroSentineledBytes.length);
	}

	bool PathExists(String const & path) {
		return (access(path.ZeroSentineled().Chars().pointer, F_OK) != -1);
	}

	String PathExtension(String const & path) {
		Chars const pathChars = path.Chars();
		uint32_t pathCharsStartIndex = (pathChars.length - 1);
		uint32_t extensionLength = 0;

		while (pathChars.At(pathCharsStartIndex) != '.') {
			pathCharsStartIndex -= 1;
			extensionLength += 1;
		}

		pathCharsStartIndex += 1;

		return String{pathChars.Sliced(pathCharsStartIndex, extensionLength)};
	}

	size_t CopyMemory(Slice<uint8_t> destination, Slice<uint8_t const> const & source) {
		size_t const size = (
			(destination.length < source.length) ?
			destination.length :
			source.length
		);

		for (size_t i = 0; i < size; i += 1) {
			destination.At(i) = source.At(i);
		}

		return size;
	}

	Slice<uint8_t> WriteMemory(Slice<uint8_t> destination, uint8_t value) {
		uint8_t * target = destination.pointer;
		uint8_t const * boundary = (target + destination.length);

		while (target != boundary) {
			(*target) = value;
			target += 1;
		}

		return destination;
	}

	Slice<uint8_t> ZeroMemory(Slice<uint8_t> destination) {
		for (size_t i = 0; i < destination.length; i += 1) {
			destination.At(i) = 0;
		}

		return destination;
	}

	Allocator * DefaultAllocator() {
		class Mallocator final : public Allocator {
			public:
			uint8_t * Allocate(size_t size) override {
				return reinterpret_cast<uint8_t *>(malloc(size));
			}

			void Deallocate(void * allocation) override {
				free(allocation);
			}

			uint8_t * Reallocate(void * allocation, size_t size) override {
				return reinterpret_cast<uint8_t *>(realloc(allocation, size));
			}
		};

		static Mallocator allocator = {};

		return &allocator;
	}
}

#include "ona/core/module.hpp"

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
			.operations = &osFileOperations,
			.descriptor = FileDescriptor{handle}
		});
	}

	void FileDescriptor::Clear() {
		Slice<uint8_t> memory = SliceOf(
			reinterpret_cast<uint8_t *>(this),
			sizeof(FileDescriptor)
		);

		ZeroMemory(memory);
	}

	Allocator * DefaultAllocator() {
		static class : public Object, public Allocator {
			Slice<uint8_t> Allocate(size_t size) override {
				uint8_t * allocationAddress = reinterpret_cast<uint8_t *>(malloc(size));
				size *= (allocationAddress != nullptr);

				return SliceOf(allocationAddress, size);
			}

			void Deallocate(void * allocation) override {
				free(allocation);
			}

			Slice<uint8_t> Reallocate(void * allocation, size_t size) override {
				uint8_t * allocationAddress = reinterpret_cast<uint8_t *>(
					realloc(allocation, size)
				);

				size *= (allocationAddress != nullptr);

				return SliceOf(allocationAddress, size);
			}
		} allocator;

		return &allocator;
	}

	File OutFile() {
		return File{
			.operations = &osFileOperations,
			.descriptor = FileDescriptor{STDOUT_FILENO}
		};
	}

	bool CheckFile(String const & filePath) {
		return (access(String::Sentineled(filePath).AsChars().pointer, F_OK) != -1);
	}

	void File::Free() {
		if (this->operations && this->operations->closer) {
			this->operations->closer(this->descriptor);
		}
	}

	void File::Print(String const & string) {
		if (this->operations && this->operations->writer) {
			this->operations->writer(this->descriptor, string.AsBytes());
		}
	}

	size_t File::Read(Slice<uint8_t> const & output) {
		if (this->operations && this->operations->reader) {
			return this->operations->reader(this->descriptor, output);
		}

		return 0;
	}

	int64_t File::SeekHead(int64_t offset) {
		if (this->operations && this->operations->seeker) {
			return this->operations->seeker(
				this->descriptor,
				FileOperations::SeekBaseHead,
				offset
			);
		}

		return 0;
	}

	int64_t File::SeekTail(int64_t offset) {
		if (this->operations && this->operations->seeker) {
			return this->operations->seeker(
				this->descriptor,
				FileOperations::SeekBaseTail,
				offset
			);
		}

		return 0;
	}

	int64_t File::Skip(int64_t offset) {
		if (this->operations && this->operations->seeker) {
			return this->operations->seeker(
				this->descriptor,
				FileOperations::SeekBaseCurrent,
				offset
			);
		}

		return 0;
	}

	int64_t File::Tell() {
		if (this->operations && this->operations->seeker) {
			return this->operations->seeker(
				this->descriptor,
				FileOperations::SeekBaseCurrent,
				0
			);
		}

		return 0;
	}

	size_t File::Write(Slice<uint8_t const> const & input) {
		if (this->operations && this->operations->writer) {
			return this->operations->writer(this->descriptor, input);
		}

		return 0;
	}

	Result<String, FileOpenError> LoadText(String const & filePath) {
		// TODO.
		return Result<String, FileOpenError>::Fail(FileOpenError::NotFound);
	}

	void * Library::FindSymbol(String const & symbolName) {
		return dlsym(this->context, String::Sentineled(symbolName).AsChars().pointer);
	}

	void Library::Free() {
		dlclose(this->context);
	}

	Result<Library> OpenLibrary(String const & filePath) {
		using Res = Result<Library>;
		Library library = {dlopen(String::Sentineled(filePath).AsChars().pointer, RTLD_NOW)};

		return (library.context ? Res::Ok(library) : Res::Fail());
	}
}

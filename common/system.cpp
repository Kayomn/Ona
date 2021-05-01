#include "common.hpp"

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>

namespace Ona {
	SystemStream::~SystemStream() {
		Close();
	}

	uint64_t SystemStream::AvailableBytes() {
		int64_t const cursor = this->Skip(0);
		int64_t const length = (this->SeekTail(0) - cursor);

		this->SeekHead(cursor);

		return length;
	}

	void SystemStream::Close() {
		close(this->handle);
	}

	String SystemStream::ID() {
		return this->systemPath;
	}

	bool SystemStream::Open(String const & systemPath, Stream::AccessFlags accessFlags) {
		int unixAccessFlags = 0;

		if (accessFlags & Stream::AccessRead) unixAccessFlags |= O_RDONLY;

		if (accessFlags & Stream::AccessWrite) unixAccessFlags |= (O_WRONLY | O_CREAT);

		/**
		*         Read Write Execute
		*        -------------------
		* Owner | yes  yes   no
		* Group | yes  no    no
		* Other | yes  no    no
		*/
		this->handle = static_cast<uint64_t>(open(
			systemPath.ZeroSentineled().Chars().pointer,
			unixAccessFlags,
			(S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR)
		));

		if (this->handle > 0) {
			this->systemPath = systemPath;

			return true;
		}

		return false;
	}

	uint64_t SystemStream::ReadBytes(Slice<uint8_t> input) {
		ssize_t const bytesRead = read(
			static_cast<int32_t>(this->handle),
			input.pointer,
			input.length
		);

		return ((bytesRead > -1) ? static_cast<size_t>(bytesRead) : 0);
	}

	uint64_t SystemStream::ReadUtf8(Slice<char> input) {
		ssize_t const bytesRead = read(
			static_cast<int32_t>(this->handle),
			input.pointer,
			input.length
		);

		return ((bytesRead > -1) ? static_cast<uint64_t>(bytesRead) : 0);
	}

	int64_t SystemStream::SeekHead(int64_t offset) {
		int32_t const bytesSought = lseek(static_cast<int32_t>(this->handle), offset, SEEK_SET);

		return ((bytesSought > -1) ? bytesSought : 0);
	}

	int64_t SystemStream::SeekTail(int64_t offset) {
		int32_t const bytesSought = lseek(static_cast<int32_t>(this->handle), offset, SEEK_END);

		return ((bytesSought > -1) ? bytesSought : 0);
	}

	int64_t SystemStream::Skip(int64_t offset) {
		int64_t const bytesSought = lseek(static_cast<int32_t>(this->handle), offset, SEEK_CUR);

		return ((bytesSought > -1) ? bytesSought : 0);
	}

	uint64_t SystemStream::WriteBytes(Slice<uint8_t const> const & output) {
		ssize_t const bytesWritten = write(this->handle, output.pointer, output.length);

		return ((bytesWritten > -1) ? static_cast<size_t>(bytesWritten) : 0);
	}

	uint32_t EnumeratePath(String const & systemPath, Callable<void(String const &)> action) {
		DIR * directory = opendir(systemPath.ZeroSentineled().Chars().pointer);

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

	void Print(String const & message) {
		String zeroSentineledMessage = message.ZeroSentineled();
		Slice<uint8_t const> const zeroSentineledBytes = zeroSentineledMessage.Bytes();

		write(STDOUT_FILENO, zeroSentineledBytes.pointer, zeroSentineledBytes.length);
	}

	bool PathExists(String const & systemPath) {
		return (access(systemPath.ZeroSentineled().Chars().pointer, F_OK) != -1);
	}

	String PathExtension(String const & systemPath) {
		Chars const pathChars = systemPath.Chars();
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

	uint8_t * Allocate(Allocator allocator, size_t size) {
		switch (allocator) {
			case Allocator::Default: return reinterpret_cast<uint8_t *>(malloc(size));
		}
	}

	uint8_t * Reallocate(Allocator allocator, void * allocation, size_t size) {
		switch (allocator) {
			case Allocator::Default: return reinterpret_cast<uint8_t *>(realloc(allocation, size));
		}
	}

	void Deallocate(Allocator allocator, void * allocation) {
		switch (allocator) {
			case Allocator::Default: free(allocation);
		}
	}
}

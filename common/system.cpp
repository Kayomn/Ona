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

	MutexError CreateMutex(Mutex & result) {
		pthread_mutex_t * mutex = new pthread_mutex_t{};

		if (mutex) switch (pthread_mutex_init(mutex, nullptr)) {
			case 0: {
				result = Mutex{.handle = mutex};

				return MutexError::None;
			}

			case ENOMEM: return MutexError::OutOfMemory;

			case EAGAIN: return MutexError::ResourceLimit;

			default: return MutexError::OSFailure;
		}

		return MutexError::ResourceLimit;
	}

	void Mutex::Free() {
		if (this->handle) {
			auto mutexHandle = reinterpret_cast<pthread_mutex_t *>(this->handle);

			// TODO: Destroy is a request, not guaranteed. Need to handle this more gracefully.
			pthread_mutex_destroy(mutexHandle);

			delete mutexHandle;

			this->handle = nullptr;
		}
	}

	void Mutex::Lock() {
		if (this->handle) pthread_mutex_lock(reinterpret_cast<pthread_mutex_t *>(this->handle));
	}

	void Mutex::Unlock() {
		if (this->handle) pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t *>(this->handle));
	}

	ConditionError CreateCondition(Condition & result) {
		auto condition = new pthread_cond_t{};

		if (condition) switch (pthread_cond_init(condition, nullptr)) {
			case 0: {
				result = Condition{.handle = condition};

				return ConditionError::None;
			}

			case ENOMEM: return ConditionError::OutOfMemory;

			case EAGAIN: return ConditionError::ResourceLimit;

			default: return ConditionError::OSFailure;
		}

		return ConditionError::ResourceLimit;
	}

	void Condition::Free() {
		if (this->handle) {
			auto conditionHandle = reinterpret_cast<pthread_cond_t *>(this->handle);

			// TODO: Destroy is a request, not guaranteed. Need to handle this more gracefully.
			pthread_cond_destroy(conditionHandle);

			delete conditionHandle;

			this->handle = nullptr;
		}
	}

	void Condition::Signal() {
		if (this->handle) pthread_cond_signal(reinterpret_cast<pthread_cond_t *>(this->handle));
	}

	void Condition::Wait(Mutex & mutex) {
		if (this->handle && mutex.handle) {
			pthread_cond_wait(
				reinterpret_cast<pthread_cond_t *>(this->handle),
				reinterpret_cast<pthread_mutex_t *>(mutex.handle)
			);
		}
	}

	ThreadError AcquireThread(String const & name, Callable<void()> action, Thread & result) {
		if (action.HasValue()) {
			pthread_t thread;

			switch (pthread_create(&thread, nullptr, [](void * userdata) -> void * {
				reinterpret_cast<Callable<void()> *>(userdata)->Invoke();

				return nullptr;
			}, &action)) {
				case 0: {
					FixedArray<char, 16> threadNameBuffer = {};

					size_t const tailIndex = CopyMemory(
						threadNameBuffer.Values().Bytes(),
						name.Chars().Bytes()
					);

					// Zero sentinel.
					if (tailIndex > threadNameBuffer.FixedLength) {
						threadNameBuffer.At(threadNameBuffer.FixedLength - 1) = 0;
					} else {
						threadNameBuffer.At(tailIndex) = 0;
					}

					pthread_setname_np(thread, threadNameBuffer.Pointer());

					result = Thread{
						.handle = reinterpret_cast<void *>(thread),
					};

					return ThreadError::None;
				}

				case EAGAIN: return ThreadError::ResourceLimit;

				default: break;
			}
		}

		return ThreadError::OSFailure;
	}

	void Thread::Join() {
		if (this->handle) pthread_join(reinterpret_cast<pthread_t>(this->handle), nullptr);
	}

	uint32_t CountHardwareConcurrency() {
		return static_cast<uint32_t>(sysconf(_SC_NPROCESSORS_ONLN));
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
		static class : public Object, public Allocator {
			uint8_t * Allocate(size_t size) override {
				return reinterpret_cast<uint8_t *>(malloc(size));
			}

			void Deallocate(void * allocation) override {
				free(allocation);
			}

			uint8_t * Reallocate(void * allocation, size_t size) override {
				return reinterpret_cast<uint8_t *>(realloc(allocation, size));
			}
		} allocator;

		return &allocator;
	}
}

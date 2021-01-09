#include "ona/engine/module.hpp"

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>

namespace Ona::Engine {
	using namespace Ona::Core;

	void * Library::FindSymbol(String const & symbol) {
		return dlsym(this->handle, symbol.ZeroSentineled().Chars().pointer);
	}

	void Library::Free() {
		dlclose(this->handle);
	}

	bool LoadLibrary(String const & libraryPath, Library & result) {
		Library library = {
			.handle = dlopen(libraryPath.ZeroSentineled().Chars().pointer, RTLD_LAZY)
		};

		if (library.handle) {
			result = library;

			return true;
		}

		return false;
	}

	Error<MutexError> CreateMutex(Mutex & result) {
		using Err = Error<MutexError>;
		pthread_mutex_t * mutex = new pthread_mutex_t{};

		if (mutex) switch (pthread_mutex_init(mutex, nullptr)) {
			case 0: {
				result = Mutex{.handle = mutex};

				return Err{};
			}

			case ENOMEM: return Err{MutexError::OutOfMemory};

			case EAGAIN: Err{MutexError::ResourceLimit};

			default: return Err{MutexError::OSFailure};
		}

		return Err{MutexError::ResourceLimit};
	}

	void Mutex::Free() {
		if (this->handle) {
			pthread_mutex_destroy(reinterpret_cast<pthread_mutex_t *>(this->handle));
			free(this->handle);

			this->handle = nullptr;
		}
	}

	void Mutex::Lock() {
		if (this->handle) pthread_mutex_lock(reinterpret_cast<pthread_mutex_t *>(this->handle));
	}

	void Mutex::Unlock() {
		if (this->handle) pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t *>(this->handle));
	}

	Error<ConditionError> CreateCondition(Condition & result) {
		using Err = Error<ConditionError>;
		auto condition = new pthread_cond_t{};

		if (condition) switch (pthread_cond_init(condition, nullptr)) {
			case 0: {
				result = Condition{.handle = condition};

				return Err{};
			}

			case ENOMEM: return Err{ConditionError::OutOfMemory};

			case EAGAIN: return Err{ConditionError::ResourceLimit};

			default: return Err{ConditionError::OSFailure};
		}

		return Err{ConditionError::ResourceLimit};
	}

	void Condition::Free() {
		if (this->handle) {
			pthread_cond_destroy(reinterpret_cast<pthread_cond_t *>(this->handle));
			free(this->handle);

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

	Error<ThreadError> AcquireThread(
		String const & name,
		ThreadProperties const & properties,
		Callable<void()> const & action,
		Thread & result
	) {
		using Err = Error<ThreadError>;

		struct ThreadData {
			Callable<void()> action;

			ThreadProperties properties;
		};

		if (!action.IsEmpty()) {
			pthread_t thread;

			ThreadData threadData = {
				.action = action,
				.properties = properties,
			};

			switch (pthread_create(&thread, nullptr, [](void * userdata) -> void * {
				auto threadData = reinterpret_cast<ThreadData *>(userdata);

				if (threadData->properties.isCancellable) {
					pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
					pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
				}

				threadData->action.Invoke();

				return nullptr;
			}, &threadData)) {
				case 0: {
					pthread_setname_np(
						thread,
						name.Substring(0, 16).ZeroSentineled().Chars().pointer
					);

					result = Thread{.handle = reinterpret_cast<void *>(thread)};

					return Err{};
				}

				case EAGAIN: return Err{ThreadError::ResourceLimit};

				default: break;
			}
		}

		return Err{ThreadError::OSFailure};
	}

	bool Thread::Cancel() {
		return (pthread_cancel(reinterpret_cast<pthread_t>(this->handle)) == 0);
	}

	uint32_t CountHardwareConcurrency() {
		return static_cast<uint32_t>(sysconf(_SC_NPROCESSORS_ONLN));
	}

	FileServer * LoadFilesystem() {
		static auto userdataToHandle = [](void * userdata) {
			return static_cast<int>(reinterpret_cast<size_t>(userdata));
		};

		static auto seekImpl = [](
			FileServer * fileServer,
			File & file,
			int seek,
			int64_t offset
		) -> size_t {
			if (file.server == fileServer) {
				int64_t const bytesSought = lseek(userdataToHandle(file.userdata), offset, seek);

				if (bytesSought > -1) return static_cast<size_t>(bytesSought);
			}

			return 0;
		};

		static class FileSystem final : public FileServer {
			public:
			bool CheckFile(String const & filePath) override {
				return (access(filePath.ZeroSentineled().Chars().pointer, F_OK) != -1);
			}

			void CloseFile(File & file) override {
				if (file.server == this) {
					close(userdataToHandle(file.userdata));

					file.userdata = nullptr;
				}
			}

			bool OpenFile(
				String const & filePath,
				File & file,
				File::OpenFlags openFlags
			) override {
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
					file = File{
						.server = this,
						.userdata = reinterpret_cast<void *>(handle)
					};

					return true;
				}

				return false;
			}

			void Print(File & file, String const & string) override {
				this->Write(file, string.Bytes());
			}

			size_t Read(File & file, Slice<uint8_t> output) override {
				if (file.server == this) {
					ssize_t const bytesRead = read(
						userdataToHandle(file.userdata),
						output.pointer,
						output.length
					);

					if (bytesRead > -1) return static_cast<size_t>(bytesRead);
				}

				return 0;
			}

			int64_t SeekHead(File & file, int64_t offset) override {
				return seekImpl(this, file, SEEK_SET, offset);
			}

			int64_t SeekTail(File & file, int64_t offset) override {
				return seekImpl(this, file, SEEK_END, offset);
			}

			int64_t Skip(File & file, int64_t offset) override {
				return seekImpl(this, file, SEEK_CUR, offset);
			}

			size_t Write(File & file, Slice<uint8_t const> const & input) override {
				if (file.server == this) {
					ssize_t const bytesWritten = write(
						userdataToHandle(file.userdata),
						input.pointer,
						input.length
					);

					if (bytesWritten > -1) return static_cast<size_t>(bytesWritten);
				}

				return 0;
			}
		} fileSystem = {};

		return &fileSystem;
	}

	void Print(String const & message) {
		String zeroSentineledMessage = message.ZeroSentineled();
		Slice<uint8_t const> const zeroSentineledBytes = zeroSentineledMessage.Bytes();

		write(STDOUT_FILENO, zeroSentineledBytes.pointer, zeroSentineledBytes.length);
	}
}

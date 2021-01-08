#include "ona/engine/module.hpp"

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

namespace Ona::Engine {
	class PosixMutex final : public Mutex {
		public:
		pthread_mutex_t value;

		void Lock() override {
			pthread_mutex_lock(&this->value);
		}

		void Unlock() override {
			pthread_mutex_unlock(&this->value);
		}
	};

	Mutex * AllocateMutex() {
		auto mutex = new PosixMutex{};

		if (mutex) {
			pthread_mutex_init(&mutex->value, nullptr);

			return mutex;
		}

		return nullptr;
	}

	void DestroyMutex(Mutex * & mutex) {
		pthread_mutex_destroy(&reinterpret_cast<PosixMutex *>(mutex)->value);

		delete mutex;

		mutex = nullptr;
	}

	class PosixCondition final : public Condition {
		public:
		pthread_cond_t value;

		void Signal() override {
			pthread_cond_signal(&this->value);
		}

		void Wait(Mutex * mutex) override {
			pthread_cond_wait(&this->value, (&reinterpret_cast<PosixMutex *>(mutex)->value));
		}
	};

	Condition * AllocateCondition() {
		auto condition = new PosixCondition{};

		if (condition) {
			pthread_cond_init(&condition->value, nullptr);

			return condition;
		}

		return nullptr;
	}

	void DestroyCondition(Condition * & condition) {
		pthread_cond_destroy(&reinterpret_cast<PosixCondition *>(condition)->value);

		delete condition;

		condition = nullptr;
	}

	bool ThreadHandle::Cancel() {
		if (this->id && this->canceller) {
			bool const result = this->canceller(this->id);
			this->id = 0;

			return result;
		}

		return false;
	}

	uint32_t CountThreads() {
		return static_cast<uint32_t>(sysconf(_SC_NPROCESSORS_ONLN));
	}

	ThreadHandle AcquireThread(
		String const & name,
		ThreadProperties const & properties,
		Callable<void()> const & action
	) {
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

			if (pthread_create(&thread, nullptr, [](void * userdata) -> void * {
				auto threadData = reinterpret_cast<ThreadData *>(userdata);

				if (threadData->properties.isCancellable) {
					pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
					pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
				}

				threadData->action.Invoke();

				return nullptr;
			}, &threadData) == 0) {
				pthread_setname_np(thread, name.Substring(0, 16).ZeroSentineled().Chars().pointer);

				return ThreadHandle{
					.id = thread,

					.canceller = [](ThreadID threadID) {
						return (pthread_cancel(threadID) == 0);
					},
				};
			}
		}

		return ThreadHandle{};
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

#include "ona/engine/module.hpp"

#include <unistd.h>
#include <fcntl.h>

namespace Ona::Engine {
	ThreadServer * LoadThreadServer(float hardwarePriority) {
		static class PosixThreadServer final : public ThreadServer {
			private:
			pthread_cond_t taskCondition;

			pthread_mutex_t taskMutex;

			DynamicArray<pthread_t> threads;

			PackedQueue<Task> tasks;

			AtomicU32 taskCount;

			public:
			PosixThreadServer(
				float hardwarePriority
			) :
				tasks{DefaultAllocator()},

				threads{
					DefaultAllocator(),
					static_cast<size_t>(sysconf(_SC_NPROCESSORS_ONLN) * hardwarePriority)
				}
			{

			}

			void Execute(Task const & task) override {
				pthread_mutex_lock(&this->taskMutex);
				this->tasks.Enqueue(task);
				pthread_mutex_unlock(&this->taskMutex);
				// Signal to the listener that there's work to be done.
				pthread_cond_signal(&this->taskCondition);
			}

			bool Start() override {
				static auto worker = [](void * arg) -> void * {
					auto threadServer = reinterpret_cast<PosixThreadServer *>(arg);

					pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
					pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);

					for (;;) {
						// Halt all but the first worker thread at this impasse so they
						// don't fight over access to the list, letting only one of the
						// threads proceed.
						pthread_mutex_lock(&threadServer->taskMutex);

						while (threadServer->tasks.Count() == 0) {
							// While there's no work, just listen for some and wait.
							pthread_cond_wait(
								&threadServer->taskCondition,
								&threadServer->taskMutex
							);
						}

						// Ok, work acquired - let the next thread in on the action and let
						// this one deal with the task it has acquired.
						Task task = threadServer->tasks.Dequeue();

						pthread_mutex_unlock(&threadServer->taskMutex);
						task.Invoke();
					}

					// Because pthreads return void pointers (in case you want to return
					// some product from the operation).
					return nullptr;
				};

				if (
					(pthread_cond_init(&this->taskCondition, nullptr) == 0) &&
					(pthread_mutex_init(&this->taskMutex, nullptr) == 0)
				) {
					size_t threadsInitialized = 0;

					this->taskCount.Store(0);

					for (size_t i = 0; i < this->threads.Length(); i += 1) {
						// TODO: Better names for threads. Keep in mind that max name length is 16.
						if (
							(pthread_create(&this->threads.At(i), nullptr, worker, this) == 0) &&
							(pthread_setname_np(this->threads.At(i), "ona.thread[0]") == 0)
						) {
							threadsInitialized += 1;
						}
					}

					return (threadsInitialized == this->threads.Length());
				}

				return false;
			}

			void Stop() override {
				// Signal exit and wait for all threads to end.
				for (size_t i = 0; i < this->threads.Length(); i += 1) {
					pthread_cancel(this->threads.At(i));
				}

				pthread_mutex_destroy(&this->taskMutex);
				pthread_cond_destroy(&this->taskCondition);
			}

			void Wait() override {
				while (this->taskCount.Load() != 0);
			}
		} threadServer = {hardwarePriority};

		return &threadServer;
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

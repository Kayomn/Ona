#include "ona/engine/module.hpp"

#include <unistd.h>
#include <fcntl.h>

namespace Ona::Engine {
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

		static class : public FileServer {
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

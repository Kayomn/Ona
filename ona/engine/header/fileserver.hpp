
namespace Ona::Engine {
	using namespace Ona::Core;

	class FileServer;

	struct File {
		/**
		 * Bitflags used for indicating the access state of a `File`.
		 */
		enum OpenFlags {
			OpenUnknown = 0,
			OpenRead = 0x1,
			OpenWrite = 0x2
		};

		FileServer * server;

		void * userdata;

		void Free();
	};

	class FileServer : public Object {
		public:
		/**
		 * Checks that the file at `filePath` is accessible to the program.
		 *
		 * An inaccessible file may not exist or may be outside of the process permissions to read.
		 */
		virtual bool CheckFile(String const & filePath) = 0;

		/**
		 * Closes the file access referenced by `file`.
		 *
		 * Closing a closed `file` or `file` that does not belong to the `FileServer` does nothing.
		 */
		virtual void CloseFile(File & file) = 0;

		/**
		 * Attempts to open the file at `filePath` into `file` using `openFlags` for access
		 * permissions.
		 */
		virtual bool OpenFile(String const & filePath, File & file, File::OpenFlags openFlags) = 0;

		/**
		 * Attempts to print the contents of `string` to the file at `file`.
		 *
		 * Printing to a closed `file` or `file` that does not belong to the `FileServer` does
		 * nothing.
		 *
		 * While the operation may fail, the API deems failure to print `String`s as unimportant and
		 * therefore does not expose any error handling for it.
		 */
		virtual void Print(File & file, String const & string) = 0;

		/**
		 * Attempts to read the contents of the file at `file` into the buffer pointer to in
		 * `output`.
		 *
		 * Reading from a closed `file` or `file` that does not belong to the `FileServer` does
		 * nothing.
		 *
		 * The number of bytes actually read into `output` are returned.
		 */
		virtual size_t Read(File & file, Slice<uint8_t> output) = 0;

		/**
		 * Attempts to seek `offset` bytes into the file at `file` from the file beginning.
		 *
		 * Seeking a closed `file` or `file` that does not belong to the `FileServer` does nothing.
		 *
		 * The number of bytes actually sought are returned.
		 */
		virtual int64_t SeekHead(File & file, int64_t offset) = 0;

		/**
		 * Attempts to seek `offset` bytes into the file at `file` from the file end.
		 *
		 * Seeking a closed `file` or `file` that does not belong to the `FileServer` does nothing.
		 *
		 * The number of bytes actually sought are returned.
		 */
		virtual int64_t SeekTail(File & file, int64_t offset) = 0;

		/**
		 * Attempts to seek `offset` bytes into the file at `file` from the current file cursor
		 * position.
		 *
		 * Seeking a closed `file` or `file` that does not belong to the `FileServer` does nothing.
		 *
		 * The number of bytes actually sought are returned.
		 */
		virtual int64_t Skip(File & file, int64_t offset) = 0;

		/**
		 * Attempts to write the contents of the buffer pointed to in `input` into the contents of
		 * `file`.
		 *
		 * Writing to a closed `file` or `file` that does not belong to the `FileServer` does
		 * nothing.
		 *
		 * The number of bytes actually written to the file are returned.
		 */
		virtual size_t Write(File & file, Slice<uint8_t const> const & input) = 0;
	};
}

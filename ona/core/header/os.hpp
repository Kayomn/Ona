
namespace Ona::Core {
	bool CheckFile(String const & filePath);

	struct FileContents {
		Allocator * allocator;

		Slice<uint8_t> bytes;

		void Free();

		String ToString() const;
	};

	FileContents LoadFile(Allocator * allocator, String const & filePath);

	union FileDescriptor {
		int unixHandle;

		void * userdata;

		void Clear();

		constexpr bool IsEmpty() const {
			return (this->userdata == nullptr);
		}
	};

	struct FileOperations {
		enum SeekBase {
			SeekBaseHead,
			SeekBaseCurrent,
			SeekBaseTail
		};

		void(*closer)(FileDescriptor descriptor);

		int64_t(*seeker)(FileDescriptor descriptor, SeekBase seekBase, int64_t offset);

		size_t(*reader)(FileDescriptor descriptor, Slice<uint8_t> output);

		size_t(*writer)(FileDescriptor descriptor, Slice<uint8_t const> input);
	};

	struct File {
		enum OpenFlags {
			OpenUnknown = 0,
			OpenRead = 0x1,
			OpenWrite = 0x2
		};

		FileOperations const * operations;

		FileDescriptor descriptor;

		void Free();

		bool IsOpen() const;

		void Print(String const & string);

		size_t Read(Slice<uint8_t> output);

		int64_t SeekHead(int64_t offset);

		int64_t SeekTail(int64_t offset);

		int64_t Skip(int64_t offset);

		int64_t Tell();

		size_t Write(Slice<uint8_t const> const & input);
	};

	File OutFile();

	Allocator * DefaultAllocator();

	File OpenFile(String const & filePath, File::OpenFlags flags);

	struct Library {
		void * context;

		void * FindSymbol(String const & symbolName);

		bool IsOpen() const;

		void Free();
	};

	Library OpenLibrary(String const & filePath);
}

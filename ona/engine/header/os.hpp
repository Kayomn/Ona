
namespace Ona::Engine {
	class ThreadPool : public Object {
		public:
		virtual void Enqueue() = 0;

		virtual void Execute() = 0;
	};

	/**
	 * Attempts to load access to the operating system filesystem as a `FileServer`.
	 */
	FileServer * LoadFilesystem();

	/**
	 * Prints `message` to the operating system standard output.
	 */
	void Print(String const & message);
}

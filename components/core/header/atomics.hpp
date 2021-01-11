
namespace Ona::Core {
	struct AtomicU32 {
		private:
		volatile $atomic uint32_t value;

		public:
		AtomicU32() = default;

		AtomicU32(AtomicU32 const & that);

		uint32_t FetchAdd(uint32_t amount);

		uint32_t FetchSub(uint32_t amount);

		uint32_t Load() const;

		void Store(uint32_t value);
	};
}

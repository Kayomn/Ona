
namespace Ona::Core {
	class Random : public Object {
		public:
		virtual uint32_t NextU32() = 0;

		virtual uint64_t NextU64() = 0;
	};

	/**
	 * A very predictable but trivial and fast pseudo-random number generator.
	 */
	class XorShifter final : public Random {
		public:
		uint64_t seed;

		XorShifter(uint64_t seed) : seed{seed} {

		}

		uint32_t NextU32() override;

		uint64_t NextU64() override;
	};
}

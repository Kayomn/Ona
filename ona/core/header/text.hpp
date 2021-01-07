
namespace Ona::Core {
	/**
	 * Reference-counted, UTF-8-encoded character sequence.
	 */
	struct String {
		private:
		enum { StaticBufferSize = 24 };

		uint32_t size;

		uint32_t length;

		union {
			uint8_t * dynamic;

			uint8_t static_[StaticBufferSize];
		} buffer;

		public:
		String() = default;

		String(char const * data);

		String(Chars const & chars);

		String(char const c, uint32_t const count);

		String(String const & that);

		~String();

		Slice<uint8_t const> Bytes() const;

		Chars Chars() const;

		bool Equals(String const & that) const;

		static String Concat(std::initializer_list<String> const & args);

		constexpr bool IsDynamic() const {
			return (this->size > StaticBufferSize);
		}

		constexpr uint32_t Length() const {
			return this->length;
		}

		uint64_t ToHash() const;

		String ToString() const;

		String ZeroSentineled() const;
	};
}

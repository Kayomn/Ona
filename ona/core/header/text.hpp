
namespace Ona::Core {
	enum class FormatType {
		Null,
		Text,
		Uint8,
		Uint16,
		Uint32,
		Uint64,
		Int8,
		Int16,
		Int32,
		Int64,
		Object,
	};

	class Object;

	struct String;

	struct Formater {
		union {
			String const * text;

			uint8_t uint8;

			uint16_t uint16;

			uint32_t uint32;

			uint64_t uint64;

			int8_t int8;

			int16_t int16;

			int32_t int32;

			int64_t int64;

			Object const * object;
		} value;

		FormatType type;

		Formater() = default;

		Formater(String const & text) : value{.text = &text}, type{FormatType::Text} {

		}

		Formater(uint8_t uint8) : value{.uint8 = uint8}, type{FormatType::Uint8} {

		}

		Formater(uint16_t uint16) : value{.uint16 = uint16}, type{FormatType::Uint16} {

		}

		Formater(uint32_t uint32) : value{.uint32 = uint32}, type{FormatType::Uint32} {

		}

		Formater(uint64_t uint64) : value{.uint64 = uint64}, type{FormatType::Uint64} {

		}

		Formater(int8_t int8) : value{.int8 = int8}, type{FormatType::Int8} {

		}

		Formater(int16_t int16) : value{.int16 = int16}, type{FormatType::Int16} {

		}

		Formater(int32_t int32) : value{.int32 = int32}, type{FormatType::Int32} {

		}

		Formater(int64_t int64) : value{.int64 = int64}, type{FormatType::Int64} {

		}

		Formater(Object const * object) : value{.object = object}, type{FormatType::Object} {

		}
	};

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

		static String Concat(std::initializer_list<String> const & args);

		bool Equals(String const & that) const;

		static String Format(String const & string, std::initializer_list<Formater> const & values);

		constexpr bool IsDynamic() const {
			return (this->size > StaticBufferSize);
		}

		constexpr uint32_t Length() const {
			return this->length;
		}

		String Substring(uint32_t startIndex, size_t length) const;

		uint64_t ToHash() const;

		String ToString() const;

		String ZeroSentineled() const;
	};

	String DecStringSigned(int64_t value);

	String DecStringUnsigned(uint64_t value);
}

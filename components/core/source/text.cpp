#include "components/core/exports.hpp"

namespace Ona::Core {
	String::String(char const * data) : size{0}, length{0} {
		for (char const * c = (data + this->size); (*c) != 0; c += 1) {
			this->size += 1;
			this->length += (((*c) & 0xC0) != 0x80);
		}

		if (this->size > StaticBufferSize) {
			this->buffer.dynamic = DefaultAllocator()->Allocate(sizeof(AtomicU32) + this->size);

			if (this->buffer.dynamic) {
				reinterpret_cast<AtomicU32 *>(this->buffer.dynamic)->Store(1);

				CopyMemory(Slice<uint8_t>{
					.length = this->size,
					.pointer = (this->buffer.dynamic + sizeof(AtomicU32)),
				}, Slice<uint8_t const>{
					.length = this->size,
					.pointer = reinterpret_cast<uint8_t const *>(data)
				});
			}
		} else {
			CopyMemory(Slice<uint8_t>{
				.length = this->size,
				.pointer = this->buffer.static_,
			}, Slice<uint8_t const>{
				.length = this->size,
				.pointer = reinterpret_cast<uint8_t const *>(data)
			});
		}
	}

	String::String(Core::Chars const & chars) {
		if (chars.length > StaticBufferSize) {
			this->buffer.dynamic = DefaultAllocator()->Allocate(sizeof(AtomicU32) + chars.length);

			if (this->buffer.dynamic) {
				reinterpret_cast<AtomicU32 *>(this->buffer.dynamic)->Store(1);
				this->size = chars.length;
				this->length = 0;

				for (size_t i = 0; (i < chars.length); i += 1) {
					this->length += ((chars.At(i) & 0xC0) != 0x80);
				}

				CopyMemory(Slice<uint8_t>{
					.length = this->size,
					.pointer = (this->buffer.dynamic + sizeof(AtomicU32)),
				}, chars.Bytes());
			}
		} else {
			this->size = chars.length;
			this->length = 0;

			for (size_t i = 0; (i < chars.length); i += 1) {
				this->length += ((chars.At(i) & 0xC0) != 0x80);
			}

			CopyMemory(Slice<uint8_t>{
				.length = this->size,
				.pointer = this->buffer.static_,
			}, chars.Bytes());
		}
	}

	String::String(char const c, uint32_t const count) {
		if (count > StaticBufferSize) {
			this->buffer.dynamic = DefaultAllocator()->Allocate(sizeof(AtomicU32) + count);

			if (this->buffer.dynamic) {
				reinterpret_cast<AtomicU32 *>(this->buffer.dynamic)->Store(1);
				this->size = count;
				this->length = count;

				WriteMemory(Slice<uint8_t>{
					.length = this->size,
					.pointer = (this->buffer.dynamic + sizeof(AtomicU32)),
				}, c);
			}
		} else {
			this->size = count;
			this->length = count;

			WriteMemory(Slice<uint8_t>{
				.length = this->size,
				.pointer = this->buffer.static_,
			}, c);
		}
	}

	String::String(String const & that) {
		this->length = that.length;
		this->size = that.size;
		this->buffer = that.buffer;

		if (this->IsDynamic()) {
			reinterpret_cast<AtomicU32 *>(this->buffer.dynamic)->FetchAdd(1);
		}
	}

	String::~String() {
		if (this->IsDynamic()) {
			if (reinterpret_cast<AtomicU32 *>(this->buffer.dynamic)->FetchSub(1) == 0) {
				DefaultAllocator()->Deallocate(this->buffer.dynamic);
			}
		}
	}

	Slice<uint8_t const> String::Bytes() const {
		return Slice<uint8_t const>{
			.length = this->size,

			.pointer = (
				this->IsDynamic() ?
				(this->buffer.dynamic + sizeof(AtomicU32)) :
				this->buffer.static_
			)
		};
	}

	Chars String::Chars() const {
		return Core::Chars{
			.length = this->size,

			.pointer = reinterpret_cast<char const *>(
				this->IsDynamic() ?
				(this->buffer.dynamic + sizeof(AtomicU32)) :
				this->buffer.static_
			)
		};
	}

	String String::Concat(std::initializer_list<String> const & args) {
		uint32_t length = 0;
		uint32_t offset = 0;

		for (String const & arg : args) length += arg.Length();

		String str = {'\0', length};

		if (str.IsDynamic()) {
			for (String const & arg : args) {
				uint32_t const argLength = arg.Length();

				CopyMemory(Slice<uint8_t>{
					.length = argLength,
					.pointer = (str.buffer.dynamic + offset)
				}, arg.Bytes());

				offset += argLength;
			}
		} else {
			for (String const & arg : args) {
				uint32_t const argLength = arg.Length();

				CopyMemory(Slice<uint8_t>{
					.length = argLength,
					.pointer = (str.buffer.static_ + offset)
				}, arg.Bytes());

				offset += argLength;
			}
		}

		return str;
	}

	bool String::Equals(String const & that) const {
		return this->Bytes().Equals(that.Bytes());
	}

	String String::Format(String const & string, std::initializer_list<Formater> const & values) {
		// TODO: Implement.
		return string;
	}

	String String::Substring(uint32_t startIndex, size_t length) const {
		return String{this->Chars().Sliced(startIndex, length)};
	}

	uint64_t String::ToHash() const {
		uint64_t hash = 5381;

		for (auto c : this->Chars()) hash = (((hash << 5) + hash) ^ c);

		return hash;
	}

	String String::ToString() const {
		return *this;
	}

	String String::ZeroSentineled() const {
		if (this->Chars().At(this->size - 1) != 0) {
			// String is not zero-sentineled, so make a copy that is.
			String sentineledString = {'\0', (this->size + 1)};

			if (sentineledString.size) {
				sentineledString.length = this->length;

				if (this->IsDynamic()) {
					CopyMemory(Slice<uint8_t>{
						.length = sentineledString.length,
						.pointer = (sentineledString.buffer.dynamic + sizeof(AtomicU32))
					}, this->Bytes());
				} else {
					CopyMemory(Slice<uint8_t>{
						.length = sentineledString.length,
						.pointer = sentineledString.buffer.static_
					}, this->Bytes());
				}
			}

			return sentineledString;
		}

		return *this;
	}

	String DecStringSigned(int64_t value) {
		if (value) {
			enum {
				Base = 10,
			};

			FixedArray<char, 32> buffer;
			size_t bufferCount;

			if (value < 0) {
				// Negative value.
				value = (-value);
				buffer.At(0) = '-';
				bufferCount += 1;
			}

			while (value) {
				buffer.At(bufferCount) = static_cast<char>((value % Base) + '0');
				value /= Base;
				bufferCount += 1;
			}

			size_t const bufferCountHalf = (bufferCount / 2);

			for (size_t i = 0; i < bufferCountHalf; i += 1) {
				size_t const iReverse = (bufferCount - i - 1);
				char const temp = buffer.At(i);
				buffer.At(i) = buffer.At(iReverse);
				buffer.At(iReverse) = temp;
			}

			return String(buffer.Sliced(0, bufferCount));
		}

		return String{"0"};
	}

	String DecStringUnsigned(uint64_t value) {
		if (value) {
			enum {
				Base = 10,
			};

			FixedArray<char, 32> buffer = {};
			size_t bufferCount = 0;

			while (value) {
				buffer.At(bufferCount) = static_cast<char>((value % Base) + '0');
				value /= Base;
				bufferCount += 1;
			}

			size_t const bufferCountHalf = (bufferCount / 2);

			for (size_t i = 0; i < bufferCountHalf; i += 1) {
				size_t const iReverse = (bufferCount - i - 1);
				char const temp = buffer.At(i);
				buffer.At(i) = buffer.At(iReverse);
				buffer.At(iReverse) = temp;
			}

			return String{buffer.Sliced(0, bufferCount)};
		}

		return String{"0"};
	}
}

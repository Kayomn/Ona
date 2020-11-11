#include "ona/core/module.hpp"

namespace Ona::Core {
	String::String(String const & that) {
		this->length = that.length;
		this->size = that.size;
		this->buffer = that.buffer;

		if (this->IsDynamic()) {
			(*(reinterpret_cast<size_t *>(this->buffer.dynamic))) += 1;
		}
	}

	String::~String() {
		if (this->IsDynamic()) {
			size_t const refCount = ((*(reinterpret_cast<size_t *>(this->buffer.dynamic))) -= 1);

			if (refCount == 0) {
				DefaultAllocator()->Deallocate(this->buffer.dynamic);
			}
		}
	}

	Chars String::AsChars() const {
		return Chars{
			.length = this->size,

			.pointer = reinterpret_cast<char const *>(
				this->IsDynamic() ?
				this->buffer.dynamic :
				this->buffer.static_
			)
		};
	}

	Slice<uint8_t> String::CreateBuffer(size_t size) {
		if (size > staticBufferSize) {
			this->buffer.dynamic = DefaultAllocator()->Allocate(sizeof(size_t) + size).pointer;

			if (this->buffer.dynamic) {
				(*(reinterpret_cast<size_t *>(this->buffer.dynamic))) = 1;
				this->size = size;

				return Slice<uint8_t>{
					.length = size,
					.pointer = (this->buffer.dynamic + sizeof(size_t)),
				};
			}
		} else {
			this->size = size;

			return Slice<uint8_t>{
				.length = size,
				.pointer = this->buffer.static_,
			};
		}

		return Slice<uint8_t>{};
	}

	bool String::Equals(String const & that) const {
		return this->AsBytes().Equals(that.AsBytes());
	}

	String String::From(char const * data) {
		size_t size = 0;

		while (*(data + size)) size += 1;

		return From(SliceOf(data, size));
	}

	String String::From(Chars const & data) {
		String string = String{};
		Slice<uint8_t> buffer = string.CreateBuffer(data.length);

		if (buffer.length) {
			for (size_t i = 0; (i < data.length); i += 1) {
				string.length += ((data.At(i) & 0xC0) != 0x80);
			}

			CopyMemory(buffer, data.AsBytes());
		}

		return string;
	}

	String String::Sentineled(String const & string) {
		String sentineledString = String{};
		Slice<uint8_t> buffer = sentineledString.CreateBuffer(string.Length() + 1);

		if (buffer.length) {
			sentineledString.length = string.length;

			CopyMemory(buffer.AsBytes(), string.AsBytes());
		}

		return sentineledString;
	}

	uint64_t String::ToHash() const {
		uint64_t hash = 5381;

		for (auto c : this->AsChars()) hash = (((hash << 5) + hash) ^ c);

		return hash;
	}

	String String::ZeroSentineled() const {
		String sentineledString = String{};
		Slice<uint8_t> buffer = sentineledString.CreateBuffer(this->Length() + 1);

		if (buffer.length) {
			sentineledString.length = this->length;

			CopyMemory(buffer.AsBytes(), this->AsBytes());
		}

		return sentineledString;
	}
}

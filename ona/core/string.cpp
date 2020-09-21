#include "ona/core.hpp"

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
				Deallocate(this->buffer.dynamic);
			}
		}
	}

	Chars String::AsChars() const {
		return Chars::Of(
			reinterpret_cast<char const *>(
				this->IsDynamic() ?
				this->buffer.dynamic :
				this->buffer.static_
			),
			this->size
		);
	}

	Slice<uint8_t> String::CreateBuffer(size_t size) {
		if (size > staticBufferSize) {
			this->buffer.dynamic = Allocate(size).pointer;

			if (this->buffer.dynamic) {
				(*(reinterpret_cast<size_t *>(this->buffer.dynamic))) = 1;
				this->size = size;

				return Slice<uint8_t>::Of((this->buffer.dynamic + sizeof(size_t)), size);
			}
		} else {
			this->size = size;

			return Slice<uint8_t>::Of(this->buffer.static_, size);
		}

		return Slice<uint8_t>{};
	}

	bool String::Equals(String const & that) const {
		return this->AsBytes().Equals(that.AsBytes());
	}

	String String::From(char const * data) {
		size_t size = 0;

		while (*(data + size)) size += 1;

		return From(Chars::Of(data, size));
	}

	String String::From(Chars const & data) {
		String string = {};
		Slice<uint8_t> buffer = string.CreateBuffer(data.length);

		if (buffer) {
			for (size_t i = 0; (i < data.length); i += 1) {
				string.length += ((data(i) & 0xC0) != 0x80);
			}

			CopyMemory(buffer, data.AsBytes());
		}

		return string;
	}

	String String::Sentineled(String const & string) {
		String sentineledString = {};
		Slice<uint8_t> buffer = sentineledString.CreateBuffer(string.Length() + 1);

		if (buffer) {
			sentineledString.length = string.length;

			CopyMemory(buffer.AsBytes(), string.AsBytes());
		}

		return sentineledString;
	}
}

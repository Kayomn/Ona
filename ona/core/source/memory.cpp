#include "ona/core/header.hpp"

namespace Ona::Core {
	size_t CopyMemory(Slice<uint8_t> destination, Slice<uint8_t const> source) {
		size_t const size = (
			(destination.length < source.length) ?
			destination.length :
			source.length
		);

		for (size_t i = 0; i < size; i += 1) {
			destination(i) = source(i);
		}

		return size;
	}

	void WriteMemory(Slice<uint8_t> destination, uint8_t value) {
		uint8_t * target = destination.pointer;
		uint8_t const * boundary = (target + destination.length);

		while (target != boundary) {
			(*target) = value;
			target += 1;
		}
	}

	void ZeroMemory(Slice<uint8_t> destination) {
		for (size_t i = 0; i < destination.length; i += 1) {
			destination(i) = 0;
		}
	}
}

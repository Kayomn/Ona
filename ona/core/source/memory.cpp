#include "ona/core/module.hpp"

namespace Ona::Core {
	size_t CopyMemory(Slice<uint8_t> destination, Slice<uint8_t const> source) {
		size_t const size = (
			(destination.length < source.length) ?
			destination.length :
			source.length
		);

		for (size_t i = 0; i < size; i += 1) {
			destination.At(i) = source.At(i);
		}

		return size;
	}

	Slice<uint8_t> WriteMemory(Slice<uint8_t> destination, uint8_t value) {
		uint8_t * target = destination.pointer;
		uint8_t const * boundary = (target + destination.length);

		while (target != boundary) {
			(*target) = value;
			target += 1;
		}

		return destination;
	}

	Slice<uint8_t> ZeroMemory(Slice<uint8_t> destination) {
		for (size_t i = 0; i < destination.length; i += 1) {
			destination.At(i) = 0;
		}

		return destination;
	}
}

void * operator new(size_t count, Ona::Core::Allocator * allocator) {
	return allocator->Allocate(count).pointer;
}

void operator delete(void * pointer, Ona::Core::Allocator * allocator) {
	allocator->Deallocate(pointer);
}

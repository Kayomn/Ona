#include "ona/engine/header.hpp"

using Ona::Core::Color;
using Ona::Core::Vector4;
using Ona::Core::Slice;

namespace Ona::Engine {
	size_t Attribute::ByteSize() const {
		size_t size;

		switch (this->type) {
			case TypeDescriptor::Byte:
			case TypeDescriptor::UnsignedByte: {
				size = 1;
			} break;

			case TypeDescriptor::Short:
			case TypeDescriptor::UnsignedShort: {
				size = 2;
			} break;

			case TypeDescriptor::Int:
			case TypeDescriptor::UnsignedInt: {
				size = 4;
			} break;

			case TypeDescriptor::Float: {
				size = 4;
			} break;

			case TypeDescriptor::Double: {
				size = 8;
			} break;
		}

		return (size * this->components);
	}

	size_t Layout::MaterialSize() const {
		constexpr size_t attributeAlignment = 4;
		size_t size = 0;

		// This needs to be aligned as per the standards that OpenGL and similar APIs conform to.
		for (let & attribute : this->Attributes()) {
			size_t const attributeSize = attribute.ByteSize();
			size_t const remainder = (attributeSize % attributeAlignment);
			// Avoid branching where possible. This will blast through the loop with a more
			// consistent speed if its just straight arithmetic operations.
			size += (attributeSize + ((attributeAlignment - remainder) * (remainder != 0)));
		}

		return size;
	}

	bool Layout::ValidateMaterialData(Slice<uint8_t const> const & data) const {
		size_t const materialSize = this->MaterialSize();

		return ((materialSize && (data.length == materialSize) && ((data.length % 4) == 0)));
	}

	bool Layout::ValidateVertexData(Slice<uint8_t const> const & data) const {
		size_t const vertexSize = this->VertexSize();

		return (vertexSize && ((data.length % vertexSize) == 0));
	}

	size_t Layout::VertexSize() const {
		size_t size = 0;

		for (let & attribute : this->Attributes()) {
			size += attribute.ByteSize();
		}

		return size;
	}

	Vector4 NormalizeColor(Color const & color) {
		return Vector4{
			(color.r / (static_cast<float>(0xFF))),
			(color.g / (static_cast<float>(0xFF))),
			(color.b / (static_cast<float>(0xFF))),
			(color.a / (static_cast<float>(0xFF)))
		};
	}
}

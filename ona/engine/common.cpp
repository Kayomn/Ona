#include "ona/engine.hpp"

using Ona::Core::Color;
using Ona::Core::Vector4;
using Ona::Core::Slice;

namespace Ona::Engine {
	size_t TypeDescriptorSize(TypeDescriptor typeDescriptor) {
		switch (typeDescriptor) {
			case TypeDescriptor::Byte:
			case TypeDescriptor::UnsignedByte: return 1;

			case TypeDescriptor::Short:
			case TypeDescriptor::UnsignedShort: return 2;

			case TypeDescriptor::Int:
			case TypeDescriptor::UnsignedInt: return 4;

			case TypeDescriptor::Float: return 4;

			case TypeDescriptor::Double: return 8;
		}
	}

	size_t MaterialLayout::BufferSize() const {
		constexpr size_t propertyAlignment = 4;
		size_t size = 0;

		// This needs to be aligned as per the standards that OpenGL and similar APIs conform to.
		for (let & property : this->properties) {
			size_t const propertySize = (TypeDescriptorSize(property.type) * property.components);
			size_t const remainder = (propertySize % propertyAlignment);
			// Avoid branching where possible. This will blast through the loop with a more
			// consistent speed if its just straight arithmetic operations.
			size += (propertySize + ((propertyAlignment - remainder) * (remainder != 0)));
		}

		return size;
	}

	bool MaterialLayout::Validate(Slice<uint8_t const> const & data) const {
		size_t propertiesSize = 0;

		for (auto & property : this->properties) {
			propertiesSize += (TypeDescriptorSize(property.type) * property.components);
		}

		return (data.length == propertiesSize);
	}

	Vector4 NormalizeColor(Color const & color) {
		return Vector4{
			(color.r / (static_cast<float>(Color::channelMax))),
			(color.g / (static_cast<float>(Color::channelMax))),
			(color.b / (static_cast<float>(Color::channelMax))),
			(color.a / (static_cast<float>(Color::channelMax)))
		};
	}

	bool VertexLayout::Validate(Slice<uint8_t const> const & data) const {
		if (this->vertexSize && ((data.length % this->vertexSize) == 0)) {
			size_t propertiesSize = 0;

			for (auto & attribute : this->attributes) {
				propertiesSize += (TypeDescriptorSize(attribute.type) * attribute.components);
			}

			return (data.length == propertiesSize);
		}

		return false;
	}
}

#include <cmath>

namespace Ona::Core {
	constexpr float Max(float const a, float const b) {
		return ((a > b) ? a : b);
	}

	constexpr float Min(float const a, float const b) {
		return ((a < b) ? a : b);
	}

	constexpr float Clamp(float const value, float const lower, float const upper) {
		return Max(lower, Min(value, upper));
	}

	constexpr float Floor(float const value) {
		return std::floor(value);
	}

	struct Matrix {
		static constexpr size_t dimensions = 4;

		float elements[dimensions * dimensions];

		constexpr float & At(size_t row, size_t column) {
			return this->elements[column + (row * dimensions)];
		}

		static constexpr Matrix Identity() {
			Matrix identity = Matrix{};

			for (size_t i = 0; i < dimensions; i += 1) identity.At(i, i) = 1.f;

			return identity;
		}
	};

	constexpr Matrix OrthographicMatrix(
		float left,
		float right,
		float bottom,
		float top,
		float near,
		float far
	) {
		Matrix result = Matrix::Identity();
		result.elements[0 + 0 * 4] = 2.0f / (right - left);
		result.elements[1 + 1 * 4] = 2.0f / (top - bottom);
		result.elements[2 + 2 * 4] = 2.0f / (near - far);
		result.elements[3 + 0 * 4] = (left + right) / (left - right);
		result.elements[3 + 1 * 4] = (bottom + top) / (bottom - top);
		result.elements[3 + 2 * 4] = (far + near) / (far - near);

		return result;
	}

	constexpr Matrix TranslationMatrix(float x, float y, float z) {
		Matrix result = Matrix::Identity();
		result.At(0, 3) = x;
		result.At(1, 3) = y;
		result.At(2, 3) = z;

		return result;
	}

	struct Vector2 {
		float x, y;

		constexpr Vector2 Add(Vector2 const & that) const {
			return Vector2{(this->x + that.x), (this->y + that.y)};
		}

		constexpr Vector2 Sub(Vector2 const & that) const {
			return Vector2{(this->x - that.x), (this->y - that.y)};
		}

		constexpr Vector2 Mul(Vector2 const & that) const {
			return Vector2{(this->x * that.x), (this->y * that.y)};
		}

		constexpr Vector2 Div(Vector2 const & that) const {
			return Vector2{(this->x / that.x), (this->y / that.y)};
		}

		constexpr Vector2 Floor() const {
			return Vector2{Ona::Core::Floor(this->x), Ona::Core::Floor(this->y)};
		}

		constexpr Vector2 Normalized() const {
			return Vector2{Clamp(this->x, -1.f, 1.f), Clamp(this->y, -1.f, 1.f)};
		}
	};

	struct Vector3 {
		float x, y, z;

		constexpr Vector3 Add(Vector3 const & that) const {
			return Vector3{(this->x + that.x), (this->y + that.y), (this->z + that.z)};
		}

		constexpr Vector3 Sub(Vector3 const & that) const {
			return Vector3{(this->x - that.x), (this->y - that.y), (this->z - that.z)};
		}

		constexpr Vector3 Mul(Vector3 const & that) const {
			return Vector3{(this->x * that.x), (this->y * that.y), (this->z * that.z)};
		}

		constexpr Vector3 Div(Vector3 const & that) const {
			return Vector3{(this->x / that.x), (this->y / that.y), (this->z / that.z)};
		}
	};

	struct Vector4 {
		float x, y, z, w;

		constexpr Vector4 Add(Vector4 const & that) const {
			return Vector4{
				(this->x + that.x),
				(this->y + that.y),
				(this->z + that.z),
				(this->w + that.w)
			};
		}

		constexpr Vector4 Sub(Vector4 const & that) const {
			return Vector4{
				(this->x - that.x),
				(this->y - that.y),
				(this->z - that.z),
				(this->w - that.w)
			};
		}

		constexpr Vector4 Mul(Vector4 const & that) const {
			return Vector4{
				(this->x * that.x),
				(this->y * that.y),
				(this->z * that.z),
				(this->w * that.w)
			};
		}

		constexpr Vector4 Div(Vector4 const & that) const {
			return Vector4{
				(this->x / that.x),
				(this->y / that.y),
				(this->z / that.z),
				(this->w / that.w)
			};
		}
	};

	struct Point2 {
		int32_t x, y;

		constexpr int64_t Area() const {
			return (this->x * this->y);
		}
	};
}

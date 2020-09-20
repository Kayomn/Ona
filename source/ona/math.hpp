#ifndef ONA_MATH_H
#define ONA_MATH_H

#include <cmath>

namespace Ona::Math {
	struct Vector2 {
		float x, y;

		constexpr Vector2 operator+(Vector2 const & that) const {
			return Vector2{(this->x + that.x), (this->y + that.y)};
		}

		constexpr Vector2 operator-(Vector2 const & that) const {
			return Vector2{(this->x - that.x), (this->y - that.y)};
		}

		constexpr Vector2 operator*(Vector2 const & that) const {
			return Vector2{(this->x * that.x), (this->y * that.y)};
		}

		constexpr Vector2 operator/(Vector2 const & that) const {
			return Vector2{(this->x / that.x), (this->y / that.y)};
		}
	};

	struct Vector3 {
		float x, y, z;

		constexpr Vector3 operator+(Vector3 const & that) const {
			return Vector3{(this->x + that.x), (this->y + that.y), (this->z + that.z)};
		}

		constexpr Vector3 operator-(Vector3 const & that) const {
			return Vector3{(this->x - that.x), (this->y - that.y), (this->z - that.z)};
		}

		constexpr Vector3 operator*(Vector3 const & that) const {
			return Vector3{(this->x * that.x), (this->y * that.y), (this->z * that.z)};
		}

		constexpr Vector3 operator/(Vector3 const & that) const {
			return Vector3{(this->x / that.x), (this->y / that.y), (this->z / that.z)};
		}
	};

	struct Vector4 {
		float x, y, z, w;

		constexpr Vector4 operator+(Vector4 const & that) const {
			return Vector4{
				(this->x + that.x),
				(this->y + that.y),
				(this->z + that.z),
				(this->w + that.w)
			};
		}

		constexpr Vector4 operator-(Vector4 const & that) const {
			return Vector4{
				(this->x - that.x),
				(this->y - that.y),
				(this->z - that.z),
				(this->w - that.w)
			};
		}

		constexpr Vector4 operator*(Vector4 const & that) const {
			return Vector4{
				(this->x * that.x),
				(this->y * that.y),
				(this->z * that.z),
				(this->w * that.w)
			};
		}

		constexpr Vector4 operator/(Vector4 const & that) const {
			return Vector4{
				(this->x / that.x),
				(this->y / that.y),
				(this->z / that.z),
				(this->w / that.w)
			};
		}
	};
}

#endif

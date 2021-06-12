/**
 * Computational implementations of types derived directly from mathematics.
 */
module ona.math;

/**
 * 16-element column-major matrix backed by floating-point arithmetic.
 *
 * A default-initialized [Matrix] is populated with `float.nan`, as per the initialization rules for
 * primitives in D.
 */
public struct Matrix {
	/**
	 * A [Matrix] initialized with `1` along the main diagnonal and `0` everywhere else.
	 */
	enum identity = Matrix([
		Vector4(1, 0, 0, 0),
		Vector4(0, 1, 0, 0),
		Vector4(0, 0, 1, 0),
		Vector4(0, 0, 0, 1)
	]);

	/**
	 * Matrix columns.
	 */
	Vector4[4] columns;

	/**
	 * Computes the arithmetic product of the current value by `that`, returning the product.
	 *
	 * In the case of multiplication, order matters.
	 */
	@nogc
	Matrix opBinary(string op)(in Matrix that) const pure if (op == "*") {
		immutable (Vector4) lhs0 = this.columns[0];
		immutable (Vector4) lhs1 = this.columns[1];
		immutable (Vector4) lhs2 = this.columns[2];
		immutable (Vector4) lhs3 = this.columns[3];

		immutable (Vector4) rhs0 = that.columns[0];
		immutable (Vector4) rhs1 = that.columns[1];
		immutable (Vector4) rhs2 = that.columns[2];
		immutable (Vector4) rhs3 = that.columns[3];

		return Matrix([
			Vector4(
				(lhs0.x * rhs0.x) + (lhs0.y * rhs1.x) + (lhs0.z * rhs2.x) + (lhs0.w * rhs3.x),
				(lhs0.x * rhs0.y) + (lhs0.y * rhs1.y) + (lhs0.z * rhs2.y) + (lhs0.w * rhs3.y),
				(lhs0.x * rhs0.z) + (lhs0.y * rhs1.z) + (lhs0.z * rhs2.z) + (lhs0.w * rhs3.z),
				(lhs0.x * rhs0.w) + (lhs0.y * rhs1.w) + (lhs0.z * rhs2.w) + (lhs0.w * rhs3.w)
			),

			Vector4(
				(lhs1.x * rhs0.x) + (lhs1.y * rhs1.x) + (lhs1.z * rhs2.x) + (lhs1.w * rhs3.x),
				(lhs1.x * rhs0.y) + (lhs1.y * rhs1.y) + (lhs1.z * rhs2.y) + (lhs1.w * rhs3.y),
				(lhs1.x * rhs0.z) + (lhs1.y * rhs1.z) + (lhs1.z * rhs2.z) + (lhs1.w * rhs3.z),
				(lhs1.x * rhs0.w) + (lhs1.y * rhs1.w) + (lhs1.z * rhs2.w) + (lhs1.w * rhs3.w)
			),

			Vector4(
				(lhs2.x * rhs0.x) + (lhs2.y * rhs1.x) + (lhs2.z * rhs2.x) + (lhs2.w * rhs3.x),
				(lhs2.x * rhs0.y) + (lhs2.y * rhs1.y) + (lhs2.z * rhs2.y) + (lhs2.w * rhs3.y),
				(lhs2.x * rhs0.z) + (lhs2.y * rhs1.z) + (lhs2.z * rhs2.z) + (lhs2.w * rhs3.z),
				(lhs2.x * rhs0.w) + (lhs2.y * rhs1.w) + (lhs2.z * rhs2.w) + (lhs2.w * rhs3.w)
			),

			Vector4(
				(lhs3.x * rhs0.x) + (lhs3.y * rhs1.x) + (lhs3.z * rhs2.x) + (lhs3.w * rhs3.x),
				(lhs3.x * rhs0.y) + (lhs3.y * rhs1.y) + (lhs3.z * rhs2.y) + (lhs3.w * rhs3.y),
				(lhs3.x * rhs0.z) + (lhs3.y * rhs1.z) + (lhs3.z * rhs2.z) + (lhs3.w * rhs3.z),
				(lhs3.x * rhs0.w) + (lhs3.y * rhs1.w) + (lhs3.z * rhs2.w) + (lhs3.w * rhs3.w)
			),
		]);
	}

	/**
	 * Returns an identity [Matrix] with a 3D scale transform of `x`, `y`, and `z` along its x, y,
	 * and z axes respectively.
	 */
	@nogc
	static Matrix scale(in float x, in float y, in float z) pure {
		return Matrix([
			Vector4(x, 0, 0, 0),
			Vector4(0, y, 0, 0),
			Vector4(0, 0, z, 0),
			Vector4(0, 0, 0, 1)
		]);
	}

	/**
	 * Returns an identity [Matrix] with a 3D scale transform of `xyz` along its x, y, and z axes.
	 */
	@nogc
	static Matrix scaleXyz(in float xyz) pure {
		return scale(xyz, xyz, xyz);
	}

	/**
	 * Returns an identity [Matrix] with a 3D translation transform of `x`, `y`, and `z` along its
	 * x, y, and z axes respectively.
	 */
	@nogc
	static Matrix translation(in float x, in float y, in float z) pure {
		return Matrix([
			Vector4(1, 0, 0, x),
			Vector4(0, 1, 0, y),
			Vector4(0, 0, 1, z),
			Vector4(0, 0, 0, 1)
		]);
	}
}

/**
 * Two-dimensional point in space backed by integer arithmetic.
 */
public struct Point2 {
	/**
	 * A [Point2] with all components assigned `1`.
	 */
	enum one = Point2(1, 1);

	/**
	 * Point components.
	 */
	int x, y;
}

/**
 * Four-component quaternion backed by floating-point arithmetic.
 *
 * A default-initialized [Quaternion] is populated with `float.nan`, as per the initialization rules
 * for primitives in D.
 */
public struct Quaternion {
	/**
	 * A [Quaternion] with the x, y, z components assigned `0` and the w component assigned `1`.
	 */
	enum identity = Quaternion(0, 0, 0, 1);

	/**
	 * Quaternion components.
	 */
	float x, y, z, w;

	/**
	 * Creates and returns a rotation [Matrix] from the current data.
	 */
	@nogc
	Matrix matrix() const {
		immutable (float) a2 = (this.x * this.x);
		immutable (float) b2 = (this.y * this.y);
		immutable (float) c2 = (this.z * this.z);
		immutable (float) ac = (this.x * this.z);
		immutable (float) ab = (this.x * this.y);
		immutable (float) bc = (this.y * this.z);
		immutable (float) ad = (this.w * this.x);
		immutable (float) bd = (this.w * this.y);
		immutable (float) cd = (this.w * this.z);

		return Matrix([
			Vector4(1 - (2 * (b2 + c2)), 2 * (ab + cd), 2 * (ac - bd), 0),
			Vector4(2 * (ab - cd), 1 - (2 * (a2 + c2)), 2 * (bc + ad), 0),
			Vector4(2 * (ac + bd), 2 * (bc - ad), 1 - (2 * (a2 + b2)), 0),
			Vector4.zero,
		]);
	}
}

/**
 * 2D rectangle backed by floating-point arithmetic.
 *
 * A default-initialized [Rect] is populated with `float.nan`, as per the initialization rules for
 * primitives in D.
 */
public struct Rect {
	/**
	 * Top-left origin point.
	 */
	Vector2 origin;

	/**
	 * Size relative to [Rect.origin], typically descending toward the bottom-right.
	 */
	Vector2 extent;

	/**
	 * Assigns `origin` as the origin point and `extent` as the extent size.
	 */
	@nogc
	this(in Vector2 origin, in Vector2 extent) {
		this.origin = origin;
		this.extent = extent;
	}

	/**
	 * Assigns `x` and `y` as the origin point components, with `width` and `height` as the extent
	 * size components.
	 */
	@nogc
	this(in float x, in float y, in float width, in float height) {
		this.origin = Vector2(x, y);
		this.extent = Vector2(width, height);
	}
}

/**
 * Two-component vector backed by floating-point arithmetic.
 *
 * A default-initialized [Vector2] is populated with `float.nan`, as per the initialization rules
 * for primitives in D.
 */
public struct Vector2 {
	/**
	 * A [Vector2] with all components assigned `0`.
	 */
	enum zero = Vector2(0, 0);

	/**
	 * Vector components.
	 */
	float x, y;

	/**
	 * Initializes the x and y components to be equal to the values in `point`.
	 */
	@nogc
	this(in Point2 point) pure {
		this.x = (cast(float)point.x);
		this.y = (cast(float)point.y);
	}

	/**
	 * Initializes the x and y components to `x` and `y` respectively.
	 */
	@nogc
	this(in float x, in float y) pure {
		this.x = x;
		this.y = y;
	}

	/**
	 * Linearly interpolates between the current value and `to` by `step`, returning the product.
	 */
	@nogc
	Vector2 lerp(in Vector2 to, in float step) const pure {
		return Vector2(
			this.x + ((to.x - this.x) * step),
			this.y + ((to.y - this.y) * step)
		);
	}

	/**
	 * Computes the arithmetic product of the current value by `that`, returning the product.
	 */
	@nogc
	Vector2 opBinary(string op)(in float that) const pure {
		mixin("return Vector2(this.x " ~ op ~ " that, this.y " ~ op ~ " that);");
	}

	/**
	 * Computes the arithmetic product of the current value by `that`, returning the product.
	 */
	@nogc
	Vector2 opBinary(string op)(in Vector2 that) const pure {
		mixin("return Vector2(this.x " ~ op ~ " that.x, this.y " ~ op ~ " that.y);");
	}

	/**
	 * Decomposes and returns the vector components in an array ordered `x`, `y`.
	 */
	@nogc
	float[2] values() const pure {
		return [this.x, this.y];
	}
}


/**
 * Three-component vector backed by floating-point arithmetic.
 *
 * A default-initialized [Vector3] is populated with `float.nan`, as per the initialization rules
 * for primitives in D.
 */
public struct Vector3 {
	/**
	 * A [Vector3] with all components assigned `1`.
	 */
	enum one = Vector3(1, 1, 1);

	/**
	 * A [Vector3] with all components assigned `0`.
	 */
	enum zero = Vector3(0, 0, 0);

	/**
	 * Vector components.
	 */
	float x, y, z;

	/**
	 * Linearly interpolates between the current value and `to` by `step`, returning the product.
	 */
	@nogc
	Vector3 lerp(in Vector3 to, in float step) const pure {
		return Vector3(
			this.x + ((to.x - this.x) * step),
			this.y + ((to.y - this.y) * step),
			this.z + ((to.z - this.z) * step)
		);
	}

	/**
	 * Returns a [Vector3] with `value` assigned to all components.
	 */
	@nogc
	static Vector3 of(in float value) pure {
		return Vector3(value, value, value);
	}

	/**
	 * Computes the arithmetic product of the current value by `that`, returning the product.
	 */
	@nogc
	Vector3 opBinary(string op)(in float that) const pure {
		mixin("return Vector3(this.x " ~
			op ~ " that, this.y " ~ op ~ " that, this.z " ~ op ~ " that);");
	}

	/**
	 * Computes the arithmetic product of the current value by `that`, returning the product.
	 */
	@nogc
	Vector3 opBinary(string op)(in Vector3 that) const pure {
		mixin("return Vector3(this.x " ~
			op ~ " that.x, this.y " ~ op ~ " that.y, this.z " ~ op ~ " that.z);");
	}

	/**
	 * Decomposes and returns the vector components in an array ordered `x`, `y`, `z`.
	 */
	@nogc
	float[3] values() const pure {
		return [this.x, this.y, this.z];
	}
}

/**
 * Four-component vector backed by floating-point arithmetic.
 *
 * A default-initialized [Vector4] is populated with `float.nan`, as per the initialization rules
 * for primitives in D.
 */
public struct Vector4 {
	/**
	 * A [Vector4] with all components assigned `1`.
	 */
	enum one = Vector4(1, 1, 1, 1);

	/**
	 * A [Vector4] with all components assigned `0`.
	 */
	enum zero = Vector4(0, 0, 0, 0);

	/**
	 * Vector components.
	 */
	float x, y, z, w;

	/**
	 * Linearly interpolates between the current value and `to` by `step`, returning the product.
	 */
	@nogc
	Vector4 lerp(in Vector4 to, in float step) const pure {
		return Vector4(
			this.x + ((to.x - this.x) * step),
			this.y + ((to.y - this.y) * step),
			this.z + ((to.z - this.z) * step),
			this.w + ((to.w - this.w) * step)
		);
	}

	/**
	 * Returns a [Vector4] with `value` assigned to all components.
	 */
	@nogc
	static Vector4 of(in float value) pure {
		return Vector4(value, value, value, value);
	}

	/**
	 * Computes the arithmetic product of the current value by `that`, returning the product.
	 */
	@nogc
	Vector4 opBinary(string op)(in float that) const pure {
		mixin("return Vector3(this.x " ~ op ~ " that, this.y " ~
				op ~ " that, this.z " ~ op ~ " that, this.w " ~ op ~ " that);");
	}

	/**
	 * Computes the arithmetic product of the current value by `that`, returning the product.
	 */
	@nogc
	Vector4 opBinary(string op)(in Vector4 that) const pure {
		mixin("return Vector3(this.x " ~ op ~ " that.x, this.y " ~
				op ~ " that.y, this.z " ~ op ~ " that.z, this.w " ~ op ~ " that.w);");
	}

	/**
	 * Decomposes and returns the vector components in an array ordered `x`, `y`, `z`, `w`.
	 */
	@nogc
	float[4] values() const pure {
		return [this.x, this.y, this.z, this.w];
	}
}


module ona.core.math;

/**
 * 4x4 mathematical matrix.
 */
public struct Matrix {
	/**
	 * The number of dimensions that compose the `Matrix`
	 */
	enum dimensions = 4;

	/**
	 * Matrix elements.
	 */
	float[dimensions * dimensions] elements = 0;

	/**
	 * Constructs a `Matrix` from `elements`.
	 */
	@nogc
	this(typeof(elements) elements...) pure {
		this.elements = elements;
	}

	/**
	 * Creates an identity `Matrix` - a zeroed `Matrix` with a main diagonal of `1` values.
	 */
	@nogc
	static Matrix identity() pure {
		Matrix matrix = of(0);

		foreach (i; 0 .. dimensions) matrix[i, i] = 1f;

		return matrix;
	}

	/**
	 * Creates a `Point2` with all components assigned `value`.
	 */
	@nogc
	static Matrix of(float value) pure {
		return Matrix(
			value, value, value, value,
			value, value, value, value,
			value, value, value, value,
			value, value, value, value
		);
	}

	/**
	 * Returns the scalar value at row `row` and column `column` by reference.
	 */
	@nogc
	ref inout (float) opIndex(const (size_t) row, const (size_t) column) pure inout {
		return this.elements[(dimensions * row) + column];
	}
}

/**
 * 2-component point.
 */
public struct Point2 {
	/**
	 * Point components.
	 */
	int x, y;

	/**
	 * Creates a `Point2` with all components assigned `value`.
	 */
	@nogc
	static Point2 of(int value) pure {
		return Point2(value, value);
	}
}

/**
 * 3-component point.
 */
public struct Point3 {
	/**
	 * Point components.
	 */
	int x, y, z;

	/**
	 * Creates a `Point3` with all components assigned `value`.
	 */
	@nogc
	static Point3 of(int value) pure {
		return Point3(value, value, value);
	}
}

/**
 * 4-component point.
 */
public struct Point4 {
	/**
	 * Point components.
	 */
	int x, y, z, w;

	/**
	 * Creates a `Point4` with all components assigned `value`.
	 */
	@nogc
	static Point4 of(int value) pure {
		return Point4(value, value, value, value);
	}
}

/**
 * 2-component vector tailored for 2D graphics programming.
 */
public struct Vector2 {
	/**
	 * Vector components.
	 */
	float x, y;

	/**
	 * Creates a `Vector2` with all components assigned `value`.
	 */
	@nogc
	static Vector2 of(float value) pure {
		return Vector2(value, value);
	}
}

/**
 * 3-component vector tailored for 3D graphics programming.
 */
public struct Vector3 {
	/**
	 * Vector components.
	 */
	float x, y, z;

	/**
	 * Creates a `Vector3` with all components assigned `value`.
	 */
	@nogc
	static Vector3 of(float value) pure {
		return Vector3(value, value, value);
	}
}

/**
 * 4-component mathematical vector.
 */
public struct Vector4 {
	/**
	 * Vector components.
	 */
	float x, y, z, w;

	/**
	 * Creates a `Vector4` with all components assigned `value`.
	 */
	@nogc
	static Vector4 of(float value) pure {
		return Vector4(value, value, value, value);
	}
}

/**
 * Creates an orthographic `Matrix` - a `Matrix` typically used for representing a flat projection
 * or space.
 */
@nogc
public Matrix orthographicMatrix(
	float left,
	float right,
	float bottom,
	float top,
	float near,
	float far
) pure {
	Matrix result = Matrix.identity;
	result.elements[0 + 0 * 4] = 2.0f / (right - left);
	result.elements[1 + 1 * 4] = 2.0f / (top - bottom);
	result.elements[2 + 2 * 4] = 2.0f / (near - far);
	result.elements[3 + 0 * 4] = (left + right) / (left - right);
	result.elements[3 + 1 * 4] = (bottom + top) / (bottom - top);
	result.elements[3 + 2 * 4] = (far + near) / (far - near);

	return result;
}

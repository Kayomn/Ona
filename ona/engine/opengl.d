module ona.engine.opengl;

version (OpenGL) {
	private import
		ona.collections,
		ona.core,
		ona.engine.graphics;

	@nogc
	private size_t calculateAttributeSize(in Property property) pure {
		size_t size = void;

		typeCheck: final switch (property.type) with (PropertyType) {
			case integral: {
				size = 4;
			} break typeCheck;

			case floating: {
				size = 4;
			} break typeCheck;

			case matrix: {
				size = 32;
			} break typeCheck;

			case vector2: {
				size = 8;
			} break typeCheck;

			case vector3: {
				size = 12;
			} break typeCheck;

			case vector4: {
				size = 16;
			} break typeCheck;
		}

		return (size * property.components);
	}

	@nogc
	private size_t calculateUserdataSize(in Property[] properties) pure {
		enum attributeAlignment = 4;
		size_t size;

		// This needs to be aligned as per the standards that OpenGL and similar APIs conform to.
		foreach (ref property; properties) {
			immutable attributeSize = calculateAttributeSize(property);
			immutable remainder = (attributeSize % attributeAlignment);
			// Avoid branching where possible. This will blast through the loop with a more consistent
			// speed if its just straight arithmetic operations.
			size += (attributeSize + ((attributeAlignment - remainder) * (remainder != 0)));
		}

		return size;
	}

	@nogc
	private size_t calculateVertexSize(in Property[] properties) pure {
		size_t size = 0;

		foreach (ref property; properties) size += calculateAttributeSize(property);

		return size;
	}

	@nogc
	private bool glslCompileFragment(SourceBuilder glsl, in ShaderAST ast) {
		return true;
	}

	@nogc
	private bool glslCompileHeader(
		SourceBuilder glsl,
		in Property[] vertexProperties,
		in Property[] rendererProperties,
		in Property[] materialProperties
	) {
		if (!glsl.write("#version 430 core\n")) return false;

		static const (char)[] propertyTypeToGLName(in PropertyType type) pure {
			final switch (type) with (PropertyType) {
				case floating: return "float";

				case integral: return "int";

				case vector2: return "vec2";

				case vector3: return "vec3";

				case vector4: return "vec4";

				case matrix: return "mat4x4";
			}
		}

		static bool writeUserdataLayout(ref SourceBuilder glsl, in Property[] layout) {
			if (!glsl.write("layout (std140, row_major) uniform Renderer {\n")) return false;

			foreach (ref property; layout) {
				if (
					(!glsl.write('\t')) ||
					(!glsl.write(propertyTypeToGLName(property.type))) ||
					(!glsl.write(' ')) ||
					(!glsl.write(property.name))
				) {
					return false;
				}

				if (property.components > 1) {
					if (
						(!glsl.write('[')) ||
						(!glsl.write(decString(property.components).asChars())) ||
						(!glsl.write(']'))
					) {
						return false;
					}
				}

				if (!glsl.write(";\n")) return false;
			}

			if (!glsl.write("};\n")) return false;

			return true;
		}

		foreach (ref property; vertexProperties) {
			if (
				(!glsl.write("in ")) ||
				(!glsl.write(propertyTypeToGLName(property.type))) ||
				(!glsl.write(' ')) ||
				(!glsl.write(property.name))
			) {
				return false;
			}

			if (property.components > 1) {
				if (
					(!glsl.write('[')) ||
					(!glsl.write(decString(property.components).asChars())) ||
					(!glsl.write(']')) ||
					(!glsl.write(";\n"))
				) {
					return false;
				}
			}
		}

		if (!writeUserdataLayout(glsl, rendererProperties)) return false;

		if (!writeUserdataLayout(glsl, materialProperties)) return false;

		return true;
	}

	@nogc
	private bool glslCompileVertex(SourceBuilder glsl, ShaderAST ast) {
		static final class GLSLParser : ShaderStatementVisitor, ShaderExpressionVisitor {
			private SourceBuilder glsl;

			@nogc
			public this(SourceBuilder glsl) pure {
				this.glsl = glsl;
			}

			@nogc
			String visitAssignExpression(ShaderExpression.Assign assign) {
				this.glsl.write('=');

				return assign.expression.accept(this);
			}

			@nogc
			override String visitDiscardStatement(ShaderStatement.Discard discard) {
				this.glsl.write("discard;");

				return String();
			}

			@nogc
			override String visitFunctionStatement(ShaderStatement.Function function_) {
				this.glsl.write(function_.typename);
				this.glsl.write(' ');
				this.glsl.write(function_.identifier);
				this.glsl.write("{\n");

				foreach (statement; function_.bodyOf().valuesOf()) {
					String error = statement.accept(this);

					if (error) return error;
				}

				this.glsl.write("}\n");

				return String();
			}

			@nogc
			override String visitVariableStatement(ShaderStatement.Variable variable) {
				if (variable.hasFlag(ShaderStatement.Variable.Flags.isGlobal)) {
					this.glsl.write("out ");

					if (variable.hasFlag(ShaderStatement.Variable.Flags.isConst)) {
						return String("Global variables cannot be constant");
					}
				}

				this.glsl.write(variable.typename);
				this.glsl.write(' ');
				this.glsl.write(variable.identifier);

				if (variable.expression) {
					String error = variable.expression.accept(this);

					if (error) return error;
				}

				this.glsl.write(";\n");

				return String();
			}

			@nogc
			override String visitIfStatement(ShaderStatement.If if_) {
				this.glsl.write("if(");
				if_.expression.accept(this);
				this.glsl.write("){\n");

				foreach (statement; if_.bodyOf().valuesOf()) {
					String error = statement.accept(this);

					if (!error) return error;
				}

				this.glsl.write("}\n");

				return String();
			}

			@nogc
			override String visitLiteralExpression(ShaderExpression.Literal literal) {
				this.glsl.write(literal.value);

				return String();
			}

			@nogc
			override String visitReturnStatement(ShaderStatement.Return return_) {
				this.glsl.write("return ");

				scope (exit) this.glsl.write(";\n");

				return return_.expression.accept(this);
			}
		}

		auto visitor = scoped!GLSLParser(glsl);

		foreach (statement; ast.statements.valuesOf()) statement.accept(visitor);

		return true;
	}

	public GraphicsServer loadOpenGL(in String title, int width, int height) {
		final class OpenGLServer : GraphicsServer {
			private struct Material {
				ResourceID rendererId;

				uint textureHandle;

				uint userdataBufferHandle;
			}

			private struct Poly {
				ResourceID rendererId;

				uint vertexBufferHandle;

				uint vertexArrayHandle;

				int vertexCount;
			}

			private struct Renderer {
				uint shaderProgramHandle;

				uint userdataBufferHandle;

				const (Property)[] vertexProperties;

				const (Property)[] rendererProperties;

				const (Property)[] materialProperties;
			}

			private OpenGL* openGL;

			private Sloter!(Material, uint) materials;

			private Sloter!(Poly, uint) polys;

			override void clear() {
				openGLClear();
			}

			override void coloredClear(in Color color) {
				immutable normalizedColor = color.normalized();

				openGLClearColored(
					normalizedColor.x,
					normalizedColor.y,
					normalizedColor.z,
					normalizedColor.w
				);
			}

			override bool readEvents(ref Events events) {
				return openGLReadEvents(this.openGL, &events);
			}

			override void update() {
				openGLUpdate(this.openGL);
			}

			override ResourceID createRenderer(
				in Property[] vertexProperties,
				in Property[] rendererProperties,
				in Property[] materialProperties,
				in char[] shaderSource
			) {
				enum bufferLimit = ptrdiff_t.max;
				immutable userdataSize = calculateUserdataSize(rendererProperties);
				immutable materialSize = calculateUserdataSize(materialProperties);

				if ((userdataSize < bufferLimit) && (materialSize < bufferLimit)) {
					SourceBuilder vertexGLSL;
					SourceBuilder fragmentGLSL;

					if (glslCompileHeader(
						vertexGLSL, vertexProperties, rendererProperties, materialProperties
					) && glslCompileHeader(
						fragmentGLSL, vertexProperties, rendererProperties, materialProperties
					)) {
						auto astAllocator = scoped!StackAllocator();

						Result!(ShaderAST, String) parsedShaderAST = parseShaderAST(
							astAllocator,
							shaderSource
						);

						if (
							parsedShaderAST &&
							glslCompileVertex(vertexGLSL, parsedShaderAST.valueOf()) &&
							glslCompileFragment(fragmentGLSL, parsedShaderAST.valueOf())
						) {
							// TODO: Create buffers.
						} else {
							outFile().print(parsedShaderAST.errorOf());
						}
					}
				}

				return ResourceID.init;
			}

			override ResourceID createMaterial(
				ResourceID rendererID,
				in Image mainTexture,
				scope const (void)[] materialData
			) {
				return ResourceID.init;
			}

			override ResourceID createPoly(
				ResourceID rendererID,
				scope const (void)[] vertexData
			) {
				return ResourceID.init;
			}

			override void renderPolyInstanced(
				ResourceID rendererID,
				ResourceID polyID,
				ResourceID materialID,
				size_t count
			) {

			}

			override void updateRendererUserdata(
				ResourceID rendererID,
				scope const (void)[] rendererData
			) {

			}
		}

		static OpenGLServer graphicsServer;

		if (!graphicsServer) {
			graphicsServer = new OpenGLServer();

			if (graphicsServer) {
				graphicsServer.openGL = openGLLoad(
					String.sentineled(title).pointerOf(),
					width,
					height
				);

				if (graphicsServer.openGL) return graphicsServer;
			}
		}

		return null;
	}

	private extern (C) {
		struct OpenGL;

		@nogc
		OpenGL* openGLLoad(const (char)* title, int width, int height);

		@nogc
		void openGLClear();

		@nogc
		void openGLClearColored(float r, float g, float b, float a);

		@nogc
		bool openGLReadEvents(OpenGL* openGL, Events* events);

		@nogc
		void openGLUpdate(OpenGL* openGL);
	}
}

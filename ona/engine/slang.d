module ona.engine.slang;

private import
	ona.collections,
	ona.core;

public struct ShaderAST {
	private Allocator allocator;

	public Appender!ShaderStatement statements;

	public void free() {
		if (this.allocator) {
			foreach (statement; this.statements.valuesOf()) {
				statement.free();
				this.allocator.free(statement);
			}

			this.allocator.free(this.statements);
		}
	}
}

private struct Lexeme {
	Token token;

	const (char)[] value;

	@nogc
	bool isSymbol(char symbol) pure const {
		return ((this.token == Token.symbol) && (this.value[0] == symbol));
	}
}

private struct ShaderLexer {
	private const (char)[] source;

	private size_t cursor;

	@nogc
	this(in char[] source) pure {
		this.source = source;
	}

	@nogc
	bool next(out Lexeme lexeme) pure {
		static bool isLogicalChar(in char c) pure {
			switch (c) {
				case '<', '>', '=', '!': return true;

				default: return false;
			}
		}

		static bool isDelimiter(in char c) pure {
			if (isLogicalChar(c)) return true;

			switch (c) {
				case ',', '.', '+', ';', '/', '*', '(', ')', '{', '}', '[', ']':
					return true;

				default: return false;
			}
		}

		while (this.cursor < this.source.length) {
			char atCursor = this.source[this.cursor];

			if (isWhitespace(atCursor)) {
				// Skip whitespace.
				this.cursor += 1;
			} else if (isDelimiter(atCursor)) {
				// Symbols.
				size_t tokenEnd = (this.cursor + 1);

				if (tokenEnd < this.source.length) {
					if (isLogicalChar(this.source[tokenEnd])) {
						// Logical operator symbol (==, !=, <=, >=).
						tokenEnd += 1;
					} else if (this.source[tokenEnd] == '/') {
						// Single-line comment.
						tokenEnd += 1;

						while (
							(tokenEnd < this.source.length) &&
							(this.source[tokenEnd] != '\n')
						) {
							tokenEnd += 1;
						}
					} else if (this.source[tokenEnd] == '*') {
						// Multi-line comment.
						bool endOfComment;
						tokenEnd += 1;

						while ((tokenEnd < this.source.length) && (!endOfComment)) {
							if (this.source[tokenEnd] == '*') {
								tokenEnd += 1;

								if (this.source[tokenEnd] == '/') {
									tokenEnd += 1;
									endOfComment = true;
								}
							} else {
								tokenEnd += 1;
							}
						}
					}
				}

				lexeme = Lexeme(Token.symbol, this.source[this.cursor .. tokenEnd]);
				this.cursor = tokenEnd;

				return true;
			} else analysis: switch (atCursor) {
				case '0': .. case '9': {
					// Number literals.
					size_t tokenEnd = (this.cursor + 1);

					while (
						(tokenEnd < this.source.length) &&
						isDigit(this.source[tokenEnd])
					) {
						tokenEnd += 1;
					}

					if ((tokenEnd < this.source.length) && (this.source[tokenEnd] == '.')) {
						// Decimal place encountered.
						immutable decimalIndex = tokenEnd;
						tokenEnd += 1;

						// Read second half of number.
						while (
							(tokenEnd < this.source.length) &&
							isDigit(this.source[tokenEnd])
						) {
							tokenEnd += 1;
						}

						// The decimal is not part of the number, it is just a random period
						// after the number.
						if (tokenEnd == decimalIndex) tokenEnd -= 1;

						lexeme = Lexeme(Token.numberLiteral, this.source[this.cursor .. tokenEnd]);
					} else {
						lexeme = Lexeme(Token.integerLiteral, this.source[this.cursor .. tokenEnd]);
					}

					this.cursor = tokenEnd;

					return true;
				} break analysis;

				default: {
					// Identifiers and keywords.
					size_t tokenEnd = (this.cursor + 1);

					while (
						(tokenEnd < this.source.length) &&
						(!isWhitespace(this.source[tokenEnd])) &&
						(!isDelimiter(this.source[tokenEnd]))
					) {
						tokenEnd += 1;
					}

					const value = this.source[this.cursor .. tokenEnd];

					identifierCheck: switch (value) {
						case "matrix", "vector2", "vector3", "vector4", "float": {
							lexeme = Lexeme(Token.typename, value);
						} break identifierCheck;

						case "int", "const", "if", "else", "return", "instance": {
							lexeme = Lexeme(Token.keyword, value);
						} break identifierCheck;

						default: {
							lexeme = Lexeme(Token.identifier, value);
						} break identifierCheck;
					}

					this.cursor = tokenEnd;

					return true;
				} break analysis;
			}
		}

		return false;
	}
}

public interface ShaderStatement {
	static final class Discard : ShaderStatement {
		@nogc
		override String accept(ShaderStatementVisitor visitor) {
			return visitor.visitDiscardStatement(this);
		}

		@nogc
		void free() {

		}
	}

	static final class Function : ShaderStatement {
		private Appender!ShaderStatement body;

		public const (char)[] typename;

		public const (char)[] identifier;

		@nogc
		public inout (Appender!ShaderStatement) bodyOf() inout pure {
			return this.body;
		}

		@nogc
		override String accept(ShaderStatementVisitor visitor) {
			return visitor.visitFunctionStatement(this);
		}

		@nogc
		void free() {

		}
	}

	static final class If : ShaderStatement {
		private Appender!ShaderStatement body;

		public ShaderExpression expression;

		public ShaderStatement statementTrue;

		public ShaderStatement statementFalse;

		@nogc
		public inout (Appender!ShaderStatement) bodyOf() inout pure {
			return this.body;
		}

		@nogc
		override String accept(ShaderStatementVisitor visitor) {
			return visitor.visitIfStatement(this);
		}

		@nogc
		void free() {

		}
	}

	static final class Return : ShaderStatement {
		ShaderExpression expression;

		@nogc
		override String accept(ShaderStatementVisitor visitor) {
			return visitor.visitReturnStatement(this);
		}

		@nogc
		void free() {

		}
	}

	static final class Variable : ShaderStatement {
		public enum Flags {
			none = 0,
			isGlobal = 0x1,
			isConst = 0x2
		}

		public const (char)[] typename;

		public const (char)[] identifier;

		public ShaderExpression expression;

		public Flags flags;

		@nogc
		public this(in char[] typename, in char[] identifier) pure {
			this.typename = typename;
			this.identifier = identifier;
		}

		@nogc
		public this(in char[] typename, in char[] identifier, ShaderExpression expression) pure {
			this.typename = typename;
			this.identifier = identifier;
			this.expression = expression;
		}

		@nogc
		override String accept(ShaderStatementVisitor visitor) {
			return visitor.visitVariableStatement(this);
		}

		@nogc
		void free() {

		}

		@nogc
		public bool hasFlag(in Flags flags) pure {
			return ((this.flags & flags) != 0);
		}
	}

	@nogc
	String accept(ShaderStatementVisitor visitor);

	@nogc
	void free();
}

public interface ShaderStatementVisitor {
	@nogc
	String visitIfStatement(ShaderStatement.If if_);

	@nogc
	String visitFunctionStatement(ShaderStatement.Function function_);

	@nogc
	String visitDiscardStatement(ShaderStatement.Discard discard);

	@nogc
	String visitVariableStatement(ShaderStatement.Variable variable);

	@nogc
	String visitReturnStatement(ShaderStatement.Return return_);
}

public interface ShaderExpression {
	static final class Assign : ShaderExpression {
		public ShaderExpression expression;

		@nogc
		public this(ShaderExpression expression) {
			this.expression = expression;
		}

		@nogc
		override String accept(ShaderExpressionVisitor visitor) {
			return visitor.visitAssignExpression(this);
		}

		@nogc
		void free() {

		}
	}

	static final class Literal : ShaderExpression {
		public const (char)[] value;

		@nogc
		public this(in char[] value) {
			this.value = value;
		}

		@nogc
		override String accept(ShaderExpressionVisitor visitor) {
			return visitor.visitLiteralExpression(this);
		}

		@nogc
		void free() {

		}
	}

	@nogc
	String accept(ShaderExpressionVisitor visitor);

	@nogc
	void free();
}

public interface ShaderExpressionVisitor {
	@nogc
	String visitAssignExpression(ShaderExpression.Assign assign);

	@nogc
	String visitLiteralExpression(ShaderExpression.Literal literal);
}

private enum Token {
	symbol,
	identifier,
	typename,
	keyword,
	numberLiteral,
	integerLiteral,
	eof
}

private alias ParseResult(Type) = Result!(Type, String);

@nogc
private ParseResult!ShaderStatement parseDeclaration(
	Allocator allocator,
	in Lexeme typename,
	ref ShaderLexer lexer
) {
	alias Res = ParseResult!ShaderStatement;
	Lexeme identifier;

	if ((!lexer.next(identifier)) || (identifier.token != Token.identifier)) {
		return Res.fail(String("Expected identifier after typename"));
	}

	Lexeme symbol;

	if ((!lexer.next(symbol)) || (symbol.token != Token.symbol)) {
		return Res.fail(String("Expected symbol after identifier"));
	}

	declarationType: switch (symbol.value[0]) {
		case ';': {
			auto globalVariable = allocator.make!(ShaderStatement.Variable)(
				typename.value,
				identifier.value
			);

			return (globalVariable ? Res.ok(globalVariable) : Res.fail(String("Out of memory")));
		} break declarationType;

		case '=': {
			Lexeme expression;

			if (!lexer.next(expression)) return Res.fail(String("Unexpected end of line"));

			ParseResult!ShaderExpression parsed = parseExpression(allocator, expression, lexer);

			if (!parsed) return Res.fail(parsed.errorOf());

			auto assign = allocator.make!(ShaderExpression.Assign)(parsed.valueOf());

			if (!assign) return Res.fail(String("Out of memory"));

			auto globalVariable = allocator.make!(ShaderStatement.Variable)(
				typename.value,
				identifier.value,
				assign
			);

			return (globalVariable ? Res.ok(globalVariable) : Res.fail(String("Out of memory")));
		} break declarationType;

		case '(': break declarationType;

		default: break declarationType;
	}

	return Res.ok(null);
}

@nogc
private ParseResult!ShaderExpression parseExpression(
	Allocator allocator,
	in Lexeme typename,
	ref ShaderLexer lexer
) {
	alias Res = ParseResult!ShaderExpression;

	return Res.ok(null);
}

@nogc
public ParseResult!ShaderAST parseShaderAST(Allocator allocator, in char[] source) {
	alias Res = ParseResult!ShaderAST;
	ShaderLexer lexer = source;

	ShaderAST ast = {
		allocator: allocator,
		statements: allocator.make!(Appender!ShaderStatement)(globalAllocator())
	};

	if (!ast.statements) return Res.fail(String("Out of memory"));

	Lexeme declaration;

	// A source file with zero lexemes should be valid.
	while (lexer.next(declaration)) {
		if (declaration.token != Token.typename) {
			return Res.fail(String("Expected typename at beginning of declaration"));
		}

		ParseResult!ShaderStatement parsed = parseDeclaration(allocator, declaration, lexer);

		if (parsed) return Res.fail(parsed.errorOf());

		if (!ast.statements.append(parsed.valueOf())) return Res.fail(String("Out of memory"));
	}

	return Res.ok(ast);
}

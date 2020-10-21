module ona.engine.slang;

private import
	ona.collections,
	ona.core;

public interface ShaderStatement {
	final class Discard : ShaderStatement {
		@nogc
		override String accept(ShaderStatementVisitor visitor) {
			return visitor.visitDiscardStatement(this);
		}
	}

	final class Function : ShaderStatement {
		private ShaderAST body;

		public const (char)[] typename;

		public const (char)[] identifier;

		@nogc
		public inout (ShaderAST) bodyOf() inout pure {
			return this.body;
		}

		@nogc
		override String accept(ShaderStatementVisitor visitor) {
			return visitor.visitFunctionStatement(this);
		}
	}

	final class If : ShaderStatement {
		private ShaderAST body;

		public ShaderExpression expression;

		public ShaderStatement statementTrue;

		public ShaderStatement statementFalse;

		@nogc
		public inout (ShaderAST) bodyOf() inout pure {
			return this.body;
		}

		@nogc
		override String accept(ShaderStatementVisitor visitor) {
			return visitor.visitIfStatement(this);
		}
	}

	final class Return : ShaderStatement {
		ShaderExpression expression;

		@nogc
		override String accept(ShaderStatementVisitor visitor) {
			return visitor.visitReturnStatement(this);
		}
	}

	final class Variable : ShaderStatement {
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
		public bool hasFlag(in Flags flags) pure {
			return ((this.flags & flags) != 0);
		}
	}

	@nogc
	String accept(ShaderStatementVisitor visitor);
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
	final class Assign : ShaderExpression {
		public ShaderExpression expression;

		@nogc
		public this(ShaderExpression expression) {
			this.expression = expression;
		}

		@nogc
		override String accept(ShaderExpressionVisitor visitor) {
			return visitor.visitAssignExpression(this);
		}
	}

	final class Literal : ShaderExpression {
		public const (char)[] value;

		@nogc
		public this(in char[] value) {
			this.value = value;
		}

		@nogc
		override String accept(ShaderExpressionVisitor visitor) {
			return visitor.visitLiteralExpression(this);
		}
	}

	@nogc
	String accept(ShaderExpressionVisitor visitor);
}

public interface ShaderExpressionVisitor {
	@nogc
	String visitAssignExpression(ShaderExpression.Assign assign);

	@nogc
	String visitLiteralExpression(ShaderExpression.Literal literal);
}

public alias ShaderAST = Appender!ShaderStatement;

@nogc
public Result!(ShaderAST, String) parseShaderAST(Allocator allocator, in char[] source) {
	alias Res = Result!(ShaderAST, String);

	return Res.fail(String("Not implemented"));
}

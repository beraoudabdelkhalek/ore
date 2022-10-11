#pragma once

#include <vector>

#include "Interpreter.h"
#include "Runtime/Object.h"
#include "Runtime/Value.h"

#define ENUMERATE_BINARY_OPS              \
  __ENUM_BI_OP(Add, "+")                  \
  __ENUM_BI_OP(Sub, "-")                  \
  __ENUM_BI_OP(Mult, "*")                 \
  __ENUM_BI_OP(Div, "/")                  \
  __ENUM_BI_OP(Equals, "==")              \
  __ENUM_BI_OP(NotEquals, "!=")           \
  __ENUM_BI_OP(GreaterThan, ">")          \
  __ENUM_BI_OP(LessThan, "<")             \
  __ENUM_BI_OP(GreaterThanOrEquals, ">=") \
  __ENUM_BI_OP(LessThanOrEquals, "<=")    \
  __ENUM_BI_OP(And, "and")                \
  __ENUM_BI_OP(Or, "or")                  \
  __ENUM_BI_OP(Xor, "xor")                \
  __ENUM_BI_OP(StringConcat, "..")

namespace Ore::AST {

class ASTNode {
  public:
  virtual ~ASTNode() = default;
  void dump() const { dump_impl(0); }
  virtual char const* class_name() const = 0;
  virtual void dump_impl(int indent) const = 0;
  virtual Value execute(Interpreter&) = 0;

  virtual bool is_identifier() const { return false; }
  virtual bool is_member_expression() const { return false; }
};

class ScopeNode : public ASTNode {
  public:
  virtual char const* class_name() const override { return "ScopeNode"; };
  virtual void dump_impl(int indent) const override;
  virtual Value execute(Interpreter&) override;

  std::vector<std::shared_ptr<ASTNode>> children() const { return m_children; };

  template<typename T, typename... Args>
  void append(Args&&... args)
  {
    auto child = std::make_shared<T>(std::forward<Args>(args)...);
    m_children.push_back(std::move(child));
  }

  private:
  std::vector<std::shared_ptr<ASTNode>> m_children;
};

class Program : public ScopeNode {
  public:
  virtual char const* class_name() const override { return "Program"; };
  virtual void dump_impl(int indent) const override;
};

class Statement : public ASTNode {
};

class Expression : public Statement {
};

class Literal : public Expression {
  public:
  Literal(Value value)
      : m_value(value)
  {
  }

  Value value() { return m_value; }
  const Value value() const { return m_value; }

  virtual char const* class_name() const override { return "Literal"; }
  virtual void dump_impl(int indent) const override;
  virtual Value execute(Interpreter&) override;

  private:
  Value m_value;
};

class FunctionDeclaration : public Statement {
  public:
  FunctionDeclaration(std::string const& name, std::shared_ptr<ScopeNode> body)
      : m_name(name)
      , m_body(body)
  {
  }

  std::string name() const { return m_name; }
  std::shared_ptr<ScopeNode> body() const { return m_body; }

  virtual char const* class_name() const override { return "FunctionDeclaration"; }
  virtual void dump_impl(int indent) const override;
  virtual Value execute(Interpreter&) override;

  private:
  std::string m_name;
  std::shared_ptr<ScopeNode> m_body;
};

class CallExpression : public Expression {
  public:
  CallExpression(std::string const& name)
      : m_name(name)
  {
  }

  std::string const& name() const { return m_name; }
  virtual char const* class_name() const override { return "CallExpression"; }
  virtual void dump_impl(int indent) const override;
  virtual Value execute(Interpreter&) override;

  private:
  std::string m_name;
};

class ReturnStatement : public Statement {
  public:
  ReturnStatement()
      : m_argument(std::make_unique<Literal>(Value()))
  {
  }

  ReturnStatement(std::unique_ptr<Expression> argument)
      : m_argument(std::move(argument))
  {
  }

  Expression& argument() const { return *m_argument; }
  virtual char const* class_name() const override { return "ReturnStatement"; }
  virtual void dump_impl(int indent) const override;
  virtual Value execute(Interpreter&) override;

  private:
  std::unique_ptr<Expression> m_argument;
};

class IfStatement : public Statement {
  public:
  IfStatement(std::unique_ptr<Expression> test, std::unique_ptr<ScopeNode> consequent, std::unique_ptr<ScopeNode> alternate)
      : m_test(std::move(test))
      , m_consequent(std::move(consequent))
      , m_alternate(std::move(alternate))
  {
  }

  Expression& test() const { return *m_test; }
  ScopeNode& consequent() const { return *m_consequent; }
  ScopeNode& alternate() const { return *m_alternate; }

  virtual char const* class_name() const override { return "IfStatement"; }
  virtual void dump_impl(int indent) const override;
  virtual Value execute(Interpreter&) override;

  private:
  std::unique_ptr<Expression> m_test;
  std::unique_ptr<ScopeNode> m_consequent, m_alternate;
};

class WhileStatement : public Statement {
  public:
  WhileStatement(std::unique_ptr<Expression> test, std::unique_ptr<ScopeNode> body)
      : m_test(std::move(test))
      , m_body(std::move(body))
  {
  }

  Expression& test() const { return *m_test; }
  ScopeNode& body() const { return *m_body; }

  virtual char const* class_name() const override { return "WhileStatement"; }
  virtual void dump_impl(int indent) const override;
  virtual Value execute(Interpreter&) override;

  private:
  std::unique_ptr<Expression> m_test;
  std::unique_ptr<ScopeNode> m_body;
};

class Identifier : public Expression {
  public:
  Identifier(std::string const& name)
      : m_name(name)
  {
  }

  virtual char const* class_name() const override { return "Identifier"; }
  virtual void dump_impl(int indent) const override;
  virtual Value execute(Interpreter&) override;

  virtual bool is_identifier() const override { return true; }

  std::string name() const { return m_name; }

  private:
  std::string m_name;
};

class AssignmentExpression : public Expression {
  public:
  AssignmentExpression(std::unique_ptr<ASTNode> lhs, std::unique_ptr<Expression> rhs)
      : m_lhs(std::move(lhs))
      , m_rhs(std::move(rhs))
  {
  }

  virtual char const* class_name() const override { return "AssignmentExpression"; }
  virtual void dump_impl(int indent) const override;
  virtual Value execute(Interpreter&) override;

  private:
  std::unique_ptr<ASTNode> m_lhs;
  std::unique_ptr<Expression> m_rhs;
};

class UnaryExpression : public Expression {
  public:
  enum class Op {
    Not,
    Length
  };

  UnaryExpression(Op op, std::unique_ptr<Expression> operand)
      : m_op(op)
      , m_operand(std::move(operand))
  {
  }

  virtual char const* class_name() const override { return "UnaryExpression"; }
  virtual void dump_impl(int indent) const override;
  virtual Value execute(Interpreter&) override;

  private:
  Op m_op;
  std::unique_ptr<Expression> m_operand;
};

class BinaryExpression : public Expression {
  public:
  enum class Op {
#define __ENUM_BI_OP(op, sym) op,
    ENUMERATE_BINARY_OPS
#undef __ENUM_BI_OP
  };

  BinaryExpression(std::unique_ptr<Expression> lhs, Op op, std::unique_ptr<Expression> rhs)
      : m_lhs(std::move(lhs))
      , m_op(op)
      , m_rhs(std::move(rhs))
  {
  }

  virtual char const* class_name() const override { return "BinaryExpression"; }
  virtual void dump_impl(int indent) const override;
  virtual Value execute(Interpreter&) override;

  private:
  std::unique_ptr<Expression> m_lhs, m_rhs;
  Op m_op;
};

class MemberExpression : public Expression {
  public:
  explicit MemberExpression(std::unique_ptr<Expression> object, std::unique_ptr<Identifier> id)
      : m_object(std::move(object))
      , m_id(std::move(id))
  {
  }

  Expression& object() { return *m_object; }
  Expression const& object() const { return *m_object; }
  Identifier& id() { return *m_id; }
  Identifier const& id() const { return *m_id; }

  virtual char const* class_name() const override { return "MemberExpression"; }
  virtual void dump_impl(int indent) const override;
  virtual Value execute(Interpreter&) override;

  virtual bool is_member_expression() const override { return true; }

  private:
  std::unique_ptr<Expression> m_object;
  std::unique_ptr<Identifier> m_id;
};

}

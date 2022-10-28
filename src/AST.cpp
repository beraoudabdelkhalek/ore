#include "AST.h"
#include "Runtime/ArrayObject.h"
#include "Runtime/FunctionObject.h"
#include "Runtime/NativeFunction.h"

static void print_indent(int indent)
{
  for (int i = 0; i < indent; i++)
    putchar(' ');
}

namespace Ore::AST {

void Literal::dump_impl(int indent) const
{
  print_indent(indent);
  std::cout << "\033[35m" << class_name() << " \033[33m@ {" << this << "} \033[34m" << value() << "\033[0m" << std::endl;
}

Value Literal::execute(Interpreter& interpreter)
{
  return value();
}

void BlockStatement::dump_impl(int indent) const
{
  for (auto& child : children())
    child->dump_impl(indent);
}

void Program::dump_impl(int indent) const
{
  print_indent(indent);
  printf("\033[32m%s \033[33m@ {%p}\033[0m\n", class_name(), this);
  for (auto& child : children())
    child->dump_impl(indent + 1);
}

Value BlockStatement::execute(Interpreter& interpreter)
{
  return interpreter.run(*this);
}

void FunctionDeclaration::dump_impl(int indent) const
{
  print_indent(indent);
  std::cout << "\033[32m" << class_name() << " \033[33m@ {" << this << "} \033[34m" << name().value_or("(anonymous)") << " (";

  for (auto& parameter : parameters())
    std::cout << parameter << ",";

  printf(")\033[0m\n");

  body()->dump_impl(indent + 1);
}

Value FunctionDeclaration::execute(Interpreter& interpreter)
{
  auto* function = interpreter.heap().allocate<FunctionObject>(name(), body(), parameters());

  if (name().has_value())
    interpreter.set_variable(name().value(), function);

  return Value(function);
}

void CallExpression::dump_impl(int indent) const
{
  print_indent(indent);
  printf("\033[35m%s \033[33m@ {%p}\033[0m\n", class_name(), this);

  m_callee->dump_impl(indent + 1);

  for (auto& argument : m_arguments)
    argument->dump_impl(indent + 1);
}

Value CallExpression::execute(Interpreter& interpreter)
{
  Value value;
  if (m_callee->is_identifier()) {
    value = interpreter.get_variable(static_cast<Identifier&>(*m_callee).name());
  } else if (m_callee->is_member_expression()) {
    value = static_cast<MemberExpression&>(*m_callee).execute(interpreter);
  } else
    __builtin_unreachable();

  assert(value.is_object());

  auto* callee = value.as_object();

  if (callee->is_function()) {
    auto& function = static_cast<FunctionObject&>(*callee);

    assert(function.parameters().size() == m_arguments.size());

    std::map<std::string, Value> passed_arguments;

    for (size_t i = 0; i < m_arguments.size(); i++) {
      auto value = m_arguments[i]->execute(interpreter);
      passed_arguments[function.parameters()[i]] = value;
    }

    return interpreter.run(*function.body(), Interpreter::ScopeType::Function, passed_arguments);

  } else if (callee->is_native_function()) {
    auto& function = static_cast<NativeFunction&>(*callee);

    std::vector<Value> passed_arguments;
    for (auto& argument : m_arguments)
      passed_arguments.push_back(argument->execute(interpreter));

    return function.native_function()(passed_arguments);
  }

  __builtin_unreachable();
}

void ReturnStatement::dump_impl(int indent) const
{
  print_indent(indent);
  printf("\033[32m%s \033[33m@ {%p}\033[0m\n", class_name(), this);
  argument().dump_impl(indent + 1);
}

Value ReturnStatement::execute(Interpreter& interpreter)
{
  auto argument_value = argument().execute(interpreter);
  interpreter.do_return();
  return argument_value;
}

void IfStatement::dump_impl(int indent) const
{
  print_indent(indent);
  printf("\033[32m%s \033[33m@ {%p}\033[0m\n", class_name(), this);
  test().dump_impl(indent + 1);

  consequent().dump_impl(indent + 1);

  print_indent(indent);
  printf("\033[32mElse\033[0m\n");

  alternate().dump_impl(indent + 1);
}

Value IfStatement::execute(Interpreter& interpreter)
{
  if (test().execute(interpreter).to_boolean())
    return consequent().execute(interpreter);
  else
    return alternate().execute(interpreter);
}

void WhileStatement::dump_impl(int indent) const
{
  print_indent(indent);
  printf("\033[32m%s \033[33m@ {%p}\033[0m<\n", class_name(), this);
  test().dump_impl(indent + 1);
  body().dump_impl(indent + 1);
}

Value WhileStatement::execute(Interpreter& interpreter)
{

  Value return_value;

  while (test().execute(interpreter).to_boolean())
    return_value = body().execute(interpreter);

  return return_value;
}

void Identifier::dump_impl(int indent) const
{
  print_indent(indent);
  printf("\033[35m%s \033[33m@ {%p} \033[34m%s \033[0m\n", class_name(), this, name().c_str());
}

Value Identifier::execute(Interpreter& interpreter)
{
  return interpreter.get_variable(name());
}

void AssignmentExpression::dump_impl(int indent) const
{
  print_indent(indent);
  printf("\033[32m%s \033[33m@ {%p}\033[0m\n", class_name(), this);
  m_lhs->dump_impl(indent + 1);
  m_rhs->dump_impl(indent + 1);
}

Value AssignmentExpression::execute(Interpreter& interpreter)
{
  auto v = m_rhs->execute(interpreter);

  if (m_lhs->is_identifier()) {
    auto id = static_cast<Identifier&>(*m_lhs);
    interpreter.set_variable(id.name(), v);
  } else if (m_lhs->is_member_expression()) {
    auto& member_expression = static_cast<MemberExpression&>(*m_lhs);

    auto object_value = member_expression.object().execute(interpreter);
    auto* object = object_value.to_object(interpreter.heap());

    if (member_expression.is_computed()) {
      auto computed_value = member_expression.property().execute(interpreter);
      if (computed_value.is_number())
        object->put(computed_value.as_number(), v);
      else if (computed_value.is_string())
        object->put(computed_value.as_string()->string(), v);
      __builtin_unreachable();
    } else {
      assert(member_expression.property().is_identifier());
      auto& id = static_cast<Identifier&>(member_expression.property());

      object->put(id.name(), v);
    }

  } else {
    __builtin_unreachable();
  }

  return v;
}

void UnaryExpression::dump_impl(int indent) const
{
  print_indent(indent);
  printf("\033[35m%s \033[33m@ {%p} \033[34m", class_name(), this);
  switch (m_op) {
  case Op::Not:
    printf("not");
    break;
  case Op::Length:
    putchar('#');
    break;
  }
  printf("\033[0m\n");
  m_operand->dump_impl(indent + 1);
}

Value UnaryExpression::execute(Interpreter& interpreter)
{
  auto value = m_operand->execute(interpreter);
  switch (m_op) {
  case Op::Not:
    return !value;
  case Op::Length:
    return Value::length(value);
  default:
    __builtin_unreachable();
  }
}

void BinaryExpression::dump_impl(int indent) const
{
  print_indent(indent);
  printf("\033[35m%s \033[33m@ {%p} \033[34m", class_name(), this);
  switch (m_op) {
#define __ENUM_BI_OP(op, sym) \
  case Op::op:                \
    printf(sym);              \
    break;
    ENUMERATE_BINARY_OPS
#undef __ENUM_BI_OP
  }
  printf("\033[0m\n");
  m_lhs->dump_impl(indent + 1);
  m_rhs->dump_impl(indent + 1);
}

Value BinaryExpression::execute(Interpreter& interpreter)
{
  auto lhs_value = m_lhs->execute(interpreter);
  auto rhs_value = m_rhs->execute(interpreter);

  switch (m_op) {
  case Op::Add:
    return lhs_value + rhs_value;
  case Op::Sub:
    return lhs_value - rhs_value;
  case Op::Mult:
    return lhs_value * rhs_value;
  case Op::Div:
    return lhs_value / rhs_value;
  case Op::Equals:
    return lhs_value == rhs_value;
  case Op::NotEquals:
    return lhs_value != rhs_value;
  case Op::GreaterThan:
    return lhs_value > rhs_value;
  case Op::LessThan:
    return lhs_value < rhs_value;
  case Op::GreaterThanOrEquals:
    return lhs_value >= rhs_value;
  case Op::LessThanOrEquals:
    return lhs_value <= rhs_value;
  case Op::StringConcat:
    return Value::string_concat(lhs_value, rhs_value, interpreter.heap());
  case Op::And:
    return Value::logical_and(lhs_value, rhs_value);
  case Op::Or:
    return Value::logical_or(lhs_value, rhs_value);
  case Op::Xor:
    return Value::logical_xor(lhs_value, rhs_value);
  default:
    __builtin_unreachable();
  }
}

void MemberExpression::dump_impl(int indent) const
{
  print_indent(indent);
  printf("\033[32m%s \033[33m@ {%p} \033[34m{computed: %s}\033[0m\n", class_name(), this, is_computed() ? "true" : "false");
  object().dump_impl(indent + 1);
  property().dump_impl(indent + 1);
}

Value MemberExpression::execute(Interpreter& interpreter)
{
  auto obj = object().execute(interpreter).to_object(interpreter.heap());

  PropertyKey key;

  if (is_computed()) {
    auto computed_value = property().execute(interpreter);
    if (computed_value.is_number())
      key = PropertyKey(computed_value.as_number());
    else if (computed_value.is_string())
      key = PropertyKey(computed_value.as_string()->string());
    else
      __builtin_unreachable();

  } else {
    assert(property().is_identifier());
    auto id = static_cast<Identifier&>(property());
    key = PropertyKey(id.name());
  }

  if (obj->contains(key))
    return obj->get(key);

  return ore_nil();
}

void ObjectExpression::dump_impl(int indent) const
{
  print_indent(indent);
  printf("\033[32m%s \033[33m@ {%p}\033[0m\n", class_name(), this);
  for (auto& [key, value] : m_properties) {
    print_indent(indent + 1);
    printf("\033[34mKey: %s\033[0m\n", key.c_str());
    value->dump_impl(indent + 2);
  }
}

Value ObjectExpression::execute(Interpreter& interpreter)
{

  auto* object = interpreter.heap().allocate<Object>();

  for (auto& [key, value] : m_properties)
    object->put(key, value->execute(interpreter));

  return Value(object);
}

void ArrayExpression::dump_impl(int indent) const
{
  print_indent(indent);
  printf("\033[32m%s \033[33m@ {%p}\033[0m\n", class_name(), this);
  for (auto& element : m_elements)
    element->dump_impl(indent + 1);
}

Value ArrayExpression::execute(Interpreter& interpreter)
{
  std::vector<Value> values;
  for (auto& element : m_elements)
    values.push_back(element->execute(interpreter));

  return interpreter.heap().allocate<ArrayObject>(values);
}

}

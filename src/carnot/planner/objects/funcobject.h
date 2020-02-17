#pragma once
#include <memory>
#include <string>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <pypa/ast/ast.hh>

#include "src/carnot/planner/ast/ast_visitor.h"
#include "src/carnot/planner/ir/ir_nodes.h"
#include "src/carnot/planner/objects/qlobject.h"

namespace pl {
namespace carnot {
namespace planner {
namespace compiler {

/**
 * @brief NameToNode is a struct to store string, node pairs. This enables Arg data structures to
 * preserve input order of these arguments which is probably expected by the user and gives
 * deterministic guarantees that hashmaps can't.
 *
 */
struct NameToNode {
  NameToNode(std::string_view n, QLObjectPtr nd) : name(n), node(nd) {}
  std::string name;
  QLObjectPtr node;
};

struct ArgMap {
  // Kwargs is a vector because we want to preserve the input order for display of the tables.
  std::vector<NameToNode> kwargs;
  std::vector<QLObjectPtr> args;
};
/**
 * @brief Data structure that contains parsed arguments. This separates the concerns of parsing
 * arguments from the actual implementation by storing evaluated arguments into a map.
 */
class ParsedArgs {
 public:
  void AddArg(std::string_view arg_name, QLObjectPtr node) {
    DCHECK(!HasArgOrKwarg(arg_name));
    args_[arg_name] = node;
  }

  Status AddArg(std::string_view arg_name, IRNode* node) {
    PL_ASSIGN_OR_RETURN(auto obj, QLObject::FromIRNode(node));
    AddArg(arg_name, obj);
    return Status::OK();
  }

  void SubDefaultArg(std::string_view arg_name, QLObjectPtr node) {
    default_subbed_args_.emplace(arg_name);
    AddArg(arg_name, node);
  }

  bool HasArgOrKwarg(std::string_view arg_name) { return HasArg(arg_name) || HasKwarg(arg_name); }

  QLObjectPtr GetArg(std::string_view arg_name) const {
    DCHECK(args_.contains(arg_name)) << absl::Substitute("arg '$0' not found", arg_name);
    return args_.find(arg_name)->second;
  }

  void AddKwarg(std::string_view arg_name, QLObjectPtr node) {
    DCHECK(!HasArgOrKwarg(arg_name));
    kwargs_.emplace_back(arg_name, node);
  }

  Status AddKwarg(std::string_view arg_name, IRNode* node) {
    PL_ASSIGN_OR_RETURN(auto obj, QLObject::FromIRNode(node));
    AddKwarg(arg_name, obj);
    return Status::OK();
  }

  void AddVariableArg(QLObjectPtr node) { variable_args_.push_back(node); }

  const std::vector<NameToNode>& kwargs() const { return kwargs_; }
  const absl::flat_hash_map<std::string, QLObjectPtr>& args() const { return args_; }
  const std::vector<QLObjectPtr>& variable_args() const { return variable_args_; }
  const absl::flat_hash_set<std::string>& default_subbed_args() const {
    return default_subbed_args_;
  }

 private:
  bool HasArg(std::string_view arg_name) { return args_.contains(arg_name); }
  bool HasKwarg(std::string_view kwarg) {
    for (const auto& kw : kwargs_) {
      if (kw.name == kwarg) {
        return true;
      }
    }
    return false;
  }

  // The mapping of named, non-variable arguments to the ir representation.
  absl::flat_hash_map<std::string, QLObjectPtr> args_;
  // Holder for extra kw args if the function has a **kwargs argument.
  std::vector<NameToNode> kwargs_;
  // Variable arguments that are passed in.
  std::vector<QLObjectPtr> variable_args_;
  // The set of arguments that wer substituted with defaults.
  absl::flat_hash_set<std::string> default_subbed_args_;
};

using FunctionType = std::function<StatusOr<QLObjectPtr>(const pypa::AstPtr&, const ParsedArgs&)>;

class FuncObject : public QLObject {
 public:
  static constexpr TypeDescriptor FuncType = {
      /* name */ "Function",
      /* type */ QLObjectType::kFunction,
  };
  // The default type. I haven't completely decided this API so using an alias for now.
  using DefaultType = std::string;

  static StatusOr<std::shared_ptr<FuncObject>> Create(
      std::string_view name, const std::vector<std::string>& arguments,
      const absl::flat_hash_map<std::string, DefaultType>& defaults, bool has_variable_len_args,
      bool has_variable_len_kwargs, FunctionType impl);

  /**
   * @brief Construct a new Python Function.
   *
   * @param name the name of the function.
   * @param arguments the list of all argument names.
   * @param defaults the list of all defaults. Each key must be a part of arguments, otherwise will
   * fail.
   * @param has_variable_len_args whether or not this supports generic positional arguments.
   * @param has_variable_len_kwargs whether or not this supports generic keyword arguments.
   * @param impl the implementation of the function.
   */
  FuncObject(std::string_view name, const std::vector<std::string>& arguments,
             const absl::flat_hash_map<std::string, DefaultType>& defaults,
             bool has_variable_len_args, bool has_variable_len_kwargs, FunctionType impl);

  /**
   * @brief Call this function with the args.
   *
   * @param args the args to pass into the function.
   * @param ast the ast object where this function is called. Used for reporting errors accurately.
   * @return The return type of the object or an error if something goes wrong during function
   * processing.
   */
  StatusOr<QLObjectPtr> Call(const ArgMap& args, const pypa::AstPtr& ast, ASTVisitor* ast_visitor);
  const std::string& name() const { return name_; }

  const std::vector<std::string>& arguments() const { return arguments_; }

  // Exposing this publicly to enable testing of default arguments.
  const absl::flat_hash_map<std::string, DefaultType>& defaults() const { return defaults_; }

 private:
  StatusOr<ParsedArgs> PrepareArgs(const ArgMap& args, const pypa::AstPtr& ast,
                                   ASTVisitor* ast_visitor);

  StatusOr<QLObjectPtr> GetDefault(std::string_view arg, ASTVisitor* ast_visitor);
  bool HasDefault(std::string_view arg);

  std::string FormatArguments(const absl::flat_hash_set<std::string> args);

  int64_t NumArgs() const { return arguments_.size(); }
  int64_t NumPositionalArgs() const { return NumArgs() - defaults_.size(); }

  std::string name_;
  std::vector<std::string> arguments_;
  absl::flat_hash_map<std::string, DefaultType> defaults_;
  FunctionType impl_;

  // Whether the function takes **kwargs as an argument.
  bool has_variable_len_kwargs_;
  // Whether the function takes *args as an argument.
  bool has_variable_len_args_;
};

template <typename TIRNode>
StatusOr<TIRNode*> GetArgAs(QLObjectPtr arg, std::string_view arg_name) {
  if (!arg->HasNode()) {
    return error::InvalidArgument("Could not get IRNode from arg '$0'", arg_name);
  }
  return AsNodeType<TIRNode>(arg->node(), arg_name);
}

template <typename TIRNode>
StatusOr<TIRNode*> GetArgAs(const ParsedArgs& args, std::string_view arg_name) {
  return GetArgAs<TIRNode>(args.GetArg(arg_name), arg_name);
}

}  // namespace compiler
}  // namespace planner
}  // namespace carnot
}  // namespace pl

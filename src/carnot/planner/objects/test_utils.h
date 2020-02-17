#pragma once

#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/carnot/planner/compiler/ast_visitor.h"
#include "src/carnot/planner/compiler/test_utils.h"
#include "src/carnot/planner/compiler_state/compiler_state.h"
#include "src/carnot/planner/objects/collection_object.h"
#include "src/carnot/planner/objects/dataframe.h"
#include "src/carnot/planner/objects/expr_object.h"

namespace pl {
namespace carnot {
namespace planner {
namespace compiler {

class QLObjectTest : public OperatorTests {
 protected:
  void SetUp() override {
    OperatorTests::SetUp();

    auto info = std::make_shared<RegistryInfo>();
    udfspb::UDFInfo info_pb;
    PL_CHECK_OK(info->Init(info_pb));
    auto compiler_state =
        std::make_shared<CompilerState>(std::make_unique<RelationMap>(), info.get(), 0);
    // Graph is set in OperatorTests.
    ast_visitor = ASTVisitorImpl::Create(graph.get(), compiler_state.get(), /*flag values*/ {})
                      .ConsumeValueOrDie();
  }

  ArgMap MakeArgMap(const std::vector<std::pair<std::string, IRNode*>>& kwargs,
                    const std::vector<IRNode*>& args) {
    std::vector<NameToNode> converted_kwargs;
    std::vector<QLObjectPtr> converted_args;
    for (const auto& p : kwargs) {
      converted_kwargs.push_back({p.first, QLObject::FromIRNode(p.second).ConsumeValueOrDie()});
    }
    for (IRNode* node : args) {
      converted_args.push_back(QLObject::FromIRNode(node).ConsumeValueOrDie());
    }
    return ArgMap{converted_kwargs, converted_args};
  }

  std::shared_ptr<ASTVisitor> ast_visitor;
};

}  // namespace compiler
}  // namespace planner
}  // namespace carnot
}  // namespace pl

#include "src/carnot/planner/compiler/var_table.h"
#include "src/carnot/planner/compiler/test_utils.h"
#include "src/carnot/planner/objects/dataframe.h"

namespace pl {
namespace carnot {
namespace planner {
namespace compiler {

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using VarTableTest = OperatorTests;
TEST_F(VarTableTest, test_parent_var_table) {
  auto var_table = VarTable::Create();
  std::string var_name = "foo";
  EXPECT_FALSE(var_table->HasVariable(var_name));
  auto mem_src = MakeMemSource();

  auto dataframe_object = Dataframe::Create(mem_src).ConsumeValueOrDie();
  var_table->Add(var_name, dataframe_object);

  EXPECT_TRUE(var_table->HasVariable(var_name));
  EXPECT_EQ(var_table->Lookup(var_name), dataframe_object);
}

TEST_F(VarTableTest, test_nested_var_table_lookup) {
  auto parent_table = VarTable::Create();
  auto child_table = parent_table->CreateChild();

  std::string foo = "foo";
  EXPECT_FALSE(parent_table->HasVariable(foo));
  EXPECT_FALSE(child_table->HasVariable(foo));
  auto mem_src = MakeMemSource();

  auto dataframe_object = Dataframe::Create(mem_src).ConsumeValueOrDie();
  parent_table->Add(foo, dataframe_object);

  EXPECT_TRUE(parent_table->HasVariable(foo));
  EXPECT_TRUE(child_table->HasVariable(foo));
  EXPECT_EQ(parent_table->Lookup(foo), dataframe_object);
  EXPECT_EQ(child_table->Lookup(foo), dataframe_object);

  // Child table doesn't leak into parent.
  std::string bar = "bar";
  EXPECT_FALSE(parent_table->HasVariable(bar));
  EXPECT_FALSE(child_table->HasVariable(bar));

  auto dataframe_object2 = Dataframe::Create(mem_src).ConsumeValueOrDie();
  child_table->Add(bar, dataframe_object2);

  EXPECT_FALSE(parent_table->HasVariable(bar));
  EXPECT_TRUE(child_table->HasVariable(bar));
  EXPECT_EQ(child_table->Lookup(bar), dataframe_object2);
}

}  // namespace compiler
}  // namespace planner
}  // namespace carnot
}  // namespace pl

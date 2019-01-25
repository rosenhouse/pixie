#include <gtest/gtest.h>
#include <iostream>

#include "src/carnot/plan/operators.h"
#include "src/carnot/proto/plan.pb.h"
#include "src/carnot/proto/test_proto.h"

namespace pl {
namespace carnot {
namespace plan {

class DummyTestUDF : udf::ScalarUDF {
 public:
  udf::Int64Value Exec(udf::FunctionContext*, udf::BoolValue, udf::Float64Value) { return 0; }
};

class OperatorTest : public ::testing::Test {
 public:
  OperatorTest()
      : state_(std::make_shared<udf::ScalarUDFRegistry>("test"),
               std::make_shared<udf::UDARegistry>("udaTest")) {
    state_.udf_registry()->RegisterOrDie<DummyTestUDF>("testUdf");
    Relation rel0;
    rel0.AddColumn(carnotpb::INT64, "col0");
    rel0.AddColumn(carnotpb::FLOAT64, "col1");

    Relation rel1;
    rel1.AddColumn(carnotpb::INT64, "col0");
    rel1.AddColumn(carnotpb::FLOAT64, "col1");

    schema_.AddRelation(0, rel0);
    schema_.AddRelation(1, rel1);
  }
  virtual ~OperatorTest() {}

 protected:
  Schema schema_;
  CompilerState state_;
};

TEST_F(OperatorTest, from_proto_map) {
  auto map_pb = carnotpb::testutils::CreateTestMap1PB();
  auto map_op = Operator::FromProto(map_pb, 1);
  EXPECT_EQ(1, map_op->id());
  EXPECT_TRUE(map_op->is_initialized());
  EXPECT_EQ(carnotpb::OperatorType::MAP_OPERATOR, map_op->op_type());
}

TEST_F(OperatorTest, from_proto_src) {
  auto src_pb = carnotpb::testutils::CreateTestSource1PB();
  auto src_op = Operator::FromProto(src_pb, 1);
  EXPECT_EQ(1, src_op->id());
  EXPECT_TRUE(src_op->is_initialized());
  EXPECT_EQ(carnotpb::OperatorType::MEMORY_SOURCE_OPERATOR, src_op->op_type());
}

TEST_F(OperatorTest, from_proto_sink) {
  auto sink_pb = carnotpb::testutils::CreateTestSink1PB();
  auto sink_op = Operator::FromProto(sink_pb, 1);
  EXPECT_EQ(1, sink_op->id());
  EXPECT_TRUE(sink_op->is_initialized());
  EXPECT_EQ(carnotpb::OperatorType::MEMORY_SINK_OPERATOR, sink_op->op_type());
}

TEST_F(OperatorTest, from_proto_blocking_agg) {
  auto agg_pb = carnotpb::testutils::CreateTestBlockingAgg1PB();
  auto agg_op = Operator::FromProto(agg_pb, 1);
  EXPECT_EQ(1, agg_op->id());
  EXPECT_TRUE(agg_op->is_initialized());
  EXPECT_EQ(carnotpb::OperatorType::BLOCKING_AGGREGATE_OPERATOR, agg_op->op_type());
}

TEST_F(OperatorTest, output_relation_source) {
  auto src_pb = carnotpb::testutils::CreateTestSource1PB();
  auto src_op = Operator::FromProto(src_pb, 1);

  auto rel = src_op->OutputRelation(schema_, state_, std::vector<int64_t>());
  EXPECT_EQ(1, rel.ValueOrDie().NumColumns());
  EXPECT_EQ(carnotpb::DataType::FLOAT64, rel.ValueOrDie().GetColumnType(0));
  EXPECT_EQ("usage", rel.ValueOrDie().GetColumnName(0));
}

TEST_F(OperatorTest, output_relation_source_inputs) {
  auto src_pb = carnotpb::testutils::CreateTestSource1PB();
  auto src_op = Operator::FromProto(src_pb, 1);

  auto rel = src_op->OutputRelation(schema_, state_, std::vector<int64_t>({1}));
  EXPECT_FALSE(rel.ok());
  EXPECT_EQ(rel.msg(), "Source operator cannot have any inputs");
}

TEST_F(OperatorTest, output_relation_sink) {
  auto sink_pb = carnotpb::testutils::CreateTestSink1PB();
  auto sink_op = Operator::FromProto(sink_pb, 1);
  auto rel = sink_op->OutputRelation(schema_, state_, std::vector<int64_t>());
  EXPECT_EQ(0, rel.ValueOrDie().NumColumns());
}

TEST_F(OperatorTest, output_relation_map) {
  auto map_pb = carnotpb::testutils::CreateTestMap1PB();
  auto map_op = Operator::FromProto(map_pb, 1);
  auto rel = map_op->OutputRelation(schema_, state_, std::vector<int64_t>({1}));
  EXPECT_EQ(1, rel.ValueOrDie().NumColumns());
  EXPECT_EQ(carnotpb::DataType::INT64, rel.ValueOrDie().GetColumnType(0));
}

TEST_F(OperatorTest, output_relation_map_no_input) {
  auto map_pb = carnotpb::testutils::CreateTestMap1PB();
  auto map_op = Operator::FromProto(map_pb, 1);
  auto rel = map_op->OutputRelation(schema_, state_, std::vector<int64_t>({}));
  EXPECT_FALSE(rel.ok());
  EXPECT_EQ(rel.msg(), "Map operator must have exactly one input");
}

TEST_F(OperatorTest, output_relation_map_missing_rel) {
  auto map_pb = carnotpb::testutils::CreateTestMap1PB();
  auto map_op = Operator::FromProto(map_pb, 1);
  auto rel = map_op->OutputRelation(schema_, state_, std::vector<int64_t>({3}));
  EXPECT_FALSE(rel.ok());
  EXPECT_EQ(rel.msg(), "Missing relation (3) for input of Map");
}

TEST_F(OperatorTest, output_relation_blocking_agg_no_input) {
  auto agg_pb = carnotpb::testutils::CreateTestBlockingAgg1PB();
  auto agg_op = Operator::FromProto(agg_pb, 1);
  auto rel = agg_op->OutputRelation(schema_, state_, std::vector<int64_t>({}));
  EXPECT_FALSE(rel.ok());
  EXPECT_EQ(rel.msg(), "BlockingAgg operator must have exactly one input");
}

TEST_F(OperatorTest, output_relation_blocking_agg_missing_rel) {
  auto agg_pb = carnotpb::testutils::CreateTestBlockingAgg1PB();
  auto agg_op = Operator::FromProto(agg_pb, 1);
  auto rel = agg_op->OutputRelation(schema_, state_, std::vector<int64_t>({3}));
  EXPECT_FALSE(rel.ok());
  EXPECT_EQ(rel.msg(), "Missing relation (3) for input of BlockingAggregateOperator");
}

}  // namespace plan
}  // namespace carnot
}  // namespace pl

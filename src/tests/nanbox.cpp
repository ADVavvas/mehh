#include <gtest/gtest.h>
#include "value.hpp"

class ValueTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(ValueTest, DefaultConstructionIsNil) {
    value_t val;
    EXPECT_TRUE(val.isNil());
}

TEST_F(ValueTest, ConstructWithNumber) {
    value_t val(42.5);
    EXPECT_TRUE(val.isNumber());
    EXPECT_FALSE(val.isNil());
    EXPECT_FALSE(val.isBool());
    EXPECT_DOUBLE_EQ(val.asNumber(), 42.5);
}

TEST_F(ValueTest, ConstructWithBoolTrue) {
    value_t val(true);
    EXPECT_TRUE(val.isBool());
    EXPECT_FALSE(val.isNil());
    EXPECT_FALSE(val.isNumber());
    EXPECT_TRUE(val.asBool());
}

TEST_F(ValueTest, ConstructWithBoolFalse) {
    value_t val(false);
    EXPECT_TRUE(val.isBool());
    EXPECT_FALSE(val.isNil());
    EXPECT_FALSE(val.isNumber());
    EXPECT_FALSE(val.asBool());
}

TEST_F(ValueTest, ConstructWithObject) {
    Obj obj;
    value_t val(&obj);
    EXPECT_TRUE(val.isObj());
    EXPECT_FALSE(val.isNil());
    EXPECT_FALSE(val.isBool());
    EXPECT_FALSE(val.isNumber());
    EXPECT_EQ(val.asObj(), &obj);
}

TEST_F(ValueTest, SetNumber) {
    value_t val;
    val.setNumber(123.45);
    EXPECT_TRUE(val.isNumber());
    EXPECT_DOUBLE_EQ(val.asNumber(), 123.45);
}

TEST_F(ValueTest, SetBoolTrue) {
    value_t val{false};
    val.setBool(true);
    EXPECT_TRUE(val.isBool());
    EXPECT_TRUE(val.asBool());
}

TEST_F(ValueTest, SetBoolFalse) {
    value_t val;
    val.setBool(false);
    EXPECT_TRUE(val.isBool());
    EXPECT_FALSE(val.asBool());
}

TEST_F(ValueTest, SetObject) {
    Obj obj;
    value_t val;
    val.setObj(&obj);
    EXPECT_TRUE(val.isObj());
    EXPECT_EQ(val.asObj(), &obj);
}

TEST_F(ValueTest, IsNilCheck) {
    value_t val;
    EXPECT_TRUE(val.isNil());
    val.setBool(true);  // Change the value
    EXPECT_FALSE(val.isNil());
}


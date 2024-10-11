#include <gtest/gtest.h>
#include "value.hpp"

class ValueTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(ValueTest, DefaultConstructionIsNil) {
    Value val;
    EXPECT_TRUE(val.isNil());
}

TEST_F(ValueTest, ConstructWithNumber) {
    Value val(42.5);
    EXPECT_TRUE(val.isNumber());
    EXPECT_FALSE(val.isNil());
    EXPECT_FALSE(val.isBool());
    EXPECT_DOUBLE_EQ(val.asNumber(), 42.5);
}

TEST_F(ValueTest, ConstructWithBoolTrue) {
    Value val(true);
    EXPECT_TRUE(val.isBool());
    EXPECT_FALSE(val.isNil());
    EXPECT_FALSE(val.isNumber());
    EXPECT_TRUE(val.asBool());
}

TEST_F(ValueTest, ConstructWithBoolFalse) {
    Value val(false);
    EXPECT_TRUE(val.isBool());
    EXPECT_FALSE(val.isNil());
    EXPECT_FALSE(val.isNumber());
    EXPECT_FALSE(val.asBool());
}

TEST_F(ValueTest, ConstructWithObject) {
    std::string s = "test";
    StringObj obj(s);
    Value val(&obj);
    EXPECT_TRUE(val.isObj());
    EXPECT_FALSE(val.isNil());
    EXPECT_FALSE(val.isBool());
    EXPECT_FALSE(val.isNumber());
    EXPECT_EQ(val.asObj(), &obj);
}

TEST_F(ValueTest, SetNumber) {
    Value val;
    val.setNumber(123.45);
    EXPECT_TRUE(val.isNumber());
    EXPECT_DOUBLE_EQ(val.asNumber(), 123.45);
}

TEST_F(ValueTest, SetBoolTrue) {
    Value val{false};
    val.setBool(true);
    EXPECT_TRUE(val.isBool());
    EXPECT_TRUE(val.asBool());
}

TEST_F(ValueTest, SetBoolFalse) {
    Value val;
    val.setBool(false);
    EXPECT_TRUE(val.isBool());
    EXPECT_FALSE(val.asBool());
}

TEST_F(ValueTest, SetObject) {
    std::string s = "test";
    StringObj obj(s);
    Value val;
    val.setObj(&obj);
    EXPECT_TRUE(val.isObj());
    EXPECT_EQ(val.asObj(), &obj);
}

TEST_F(ValueTest, IsNilCheck) {
    Value val;
    EXPECT_TRUE(val.isNil());
    val.setBool(true);  // Change the value
    EXPECT_FALSE(val.isNil());
}


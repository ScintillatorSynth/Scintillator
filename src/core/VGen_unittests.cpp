#include "gtest/gtest.h"

#include "core/AbstractVGen.hpp"
#include "core/Intrinsic.hpp"
#include "core/VGen.hpp"

namespace scin {

TEST(VGenTest, InvalidInputCounts) {
    std::shared_ptr<const AbstractVGen> noInputs(
        new AbstractVGen("noInput", {}, { "out" }, { {} }, { { 1 } }, "@out = @time * 0.5f;"));
    VGen someInputs(noInputs);
    someInputs.addConstantInput(1.0f);
    EXPECT_FALSE(someInputs.validate());

    std::shared_ptr<const AbstractVGen> twoInputs(
        new AbstractVGen("twoInputs", { "in1", "in2" }, { "out" }, { { 1, 1 } }, { { 1 } }, "@out = @in1 + @in2;"));
    VGen incrementalInputs(twoInputs);
    EXPECT_FALSE(incrementalInputs.validate());
    incrementalInputs.addVGenInput(0, 0, 1);
    EXPECT_FALSE(incrementalInputs.validate());
    incrementalInputs.addConstantInput(-45.0f);
    EXPECT_TRUE(incrementalInputs.validate());
    incrementalInputs.addVGenInput(1, 0, 1);
    EXPECT_FALSE(incrementalInputs.validate());
}

TEST(VGenTest, InputDimensionsInvalid) {}

TEST(VGenTest, InputValuesAndTypesRetained) {
    std::shared_ptr<const AbstractVGen> threeInputs(new AbstractVGen(
        "threeInputs", { "a", "b", "c" }, { "out" }, { { 1, 1, 1 } }, { { 1 } }, "@out = @a + @b + @c;"));
    VGen allConstants(threeInputs);
    allConstants.addConstantInput(-1.0f);
    allConstants.addConstantInput(2.0f);
    allConstants.addConstantInput(45.0f);
    float constantValue = 0.0f;
    int vgenIndex = 0;
    int outputIndex = 0;
    EXPECT_TRUE(allConstants.getInputConstantValue(0, constantValue));
    EXPECT_EQ(-1.0f, constantValue);
    EXPECT_FALSE(allConstants.getInputVGenIndex(0, vgenIndex, outputIndex));
    EXPECT_EQ(0, vgenIndex);
    EXPECT_EQ(0, outputIndex);
    constantValue = 0.0f;
    EXPECT_TRUE(allConstants.getInputConstantValue(1, constantValue));
    EXPECT_EQ(2.0f, constantValue);
    EXPECT_FALSE(allConstants.getInputVGenIndex(1, vgenIndex, outputIndex));
    EXPECT_EQ(0, vgenIndex);
    EXPECT_EQ(0, outputIndex);
    constantValue = 0.0f;
    EXPECT_TRUE(allConstants.getInputConstantValue(2, constantValue));
    EXPECT_EQ(45.0f, constantValue);
    EXPECT_FALSE(allConstants.getInputVGenIndex(2, vgenIndex, outputIndex));
    EXPECT_EQ(0, vgenIndex);
    EXPECT_EQ(0, outputIndex);
    constantValue = 0.0f;
    EXPECT_FALSE(allConstants.getInputConstantValue(3, constantValue));
    EXPECT_EQ(0.0f, constantValue);
    EXPECT_FALSE(allConstants.getInputVGenIndex(3, vgenIndex, outputIndex));
    EXPECT_EQ(0, vgenIndex);
    EXPECT_EQ(0, outputIndex);
    EXPECT_TRUE(allConstants.validate());

    VGen allVGens(threeInputs);
    allVGens.addVGenInput(2, 1, 1);
    allVGens.addVGenInput(3, 4, 2);
    allVGens.addVGenInput(0, 0, 3);
    constantValue = 0.0f;
    vgenIndex = -1;
    outputIndex = -2;
    EXPECT_FALSE(allVGens.getInputConstantValue(0, constantValue));
    EXPECT_EQ(0.0f, constantValue);
    EXPECT_TRUE(allVGens.getInputVGenIndex(0, vgenIndex, outputIndex));
    EXPECT_EQ(2, vgenIndex);
    EXPECT_EQ(1, outputIndex);
    vgenIndex = -1;
    outputIndex = -2;
    EXPECT_FALSE(allVGens.getInputConstantValue(1, constantValue));
    EXPECT_EQ(0.0f, constantValue);
    EXPECT_TRUE(allVGens.getInputVGenIndex(1, vgenIndex, outputIndex));
    EXPECT_EQ(3, vgenIndex);
    EXPECT_EQ(4, outputIndex);
    vgenIndex = -1;
    outputIndex = -2;
    EXPECT_FALSE(allVGens.getInputConstantValue(2, constantValue));
    EXPECT_EQ(0.0f, constantValue);
    EXPECT_TRUE(allVGens.getInputVGenIndex(2, vgenIndex, outputIndex));
    EXPECT_EQ(0, vgenIndex);
    EXPECT_EQ(0, outputIndex);
    vgenIndex = -1;
    outputIndex = -2;
    EXPECT_FALSE(allVGens.getInputConstantValue(3, constantValue));
    EXPECT_EQ(0.0f, constantValue);
    EXPECT_FALSE(allVGens.getInputVGenIndex(3, vgenIndex, outputIndex));
    EXPECT_EQ(-1, vgenIndex);
    EXPECT_EQ(-2, outputIndex);
    EXPECT_TRUE(allVGens.validate());

    VGen pickAndMix(threeInputs);
    pickAndMix.addConstantInput(-23.7f);
    pickAndMix.addVGenInput(14, 7, 3);
    pickAndMix.addConstantInput(12224.3);
    constantValue = 0.0f;
    vgenIndex = -1;
    outputIndex = -2;
    EXPECT_TRUE(pickAndMix.getInputConstantValue(0, constantValue));
    EXPECT_EQ(-23.7f, constantValue);
    EXPECT_FALSE(pickAndMix.getInputVGenIndex(0, vgenIndex, outputIndex));
    EXPECT_EQ(-1, vgenIndex);
    EXPECT_EQ(-2, outputIndex);
    constantValue = 0.0f;
    vgenIndex = -1;
    outputIndex = -2;
    EXPECT_FALSE(pickAndMix.getInputConstantValue(1, constantValue));
    EXPECT_EQ(0.0f, constantValue);
    EXPECT_TRUE(pickAndMix.getInputVGenIndex(1, vgenIndex, outputIndex));
    EXPECT_EQ(14, vgenIndex);
    EXPECT_EQ(7, outputIndex);
    constantValue = 0.0f;
    vgenIndex = -1;
    outputIndex = -2;
    EXPECT_TRUE(pickAndMix.getInputConstantValue(2, constantValue));
    EXPECT_EQ(12224.3f, constantValue);
    EXPECT_FALSE(pickAndMix.getInputVGenIndex(2, vgenIndex, outputIndex));
    EXPECT_EQ(-1, vgenIndex);
    EXPECT_EQ(-2, outputIndex);
    EXPECT_TRUE(pickAndMix.validate());
}

} // namespace scin

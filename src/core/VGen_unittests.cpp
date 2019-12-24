#include "gtest/gtest.h"

#include "core/AbstractVGen.hpp"
#include "core/VGen.hpp"

namespace scin {

TEST(VGenTest, InvalidInputCounts) {
    std::shared_ptr<const AbstractVGen> noInputs(
        new AbstractVGen("noInput", "@out = @time * 0.5f;", {}, { "time" }, {}));
    VGen someInputs(noInputs);
    someInputs.addConstantInput(1.0f);
    EXPECT_FALSE(someInputs.validate());

    std::shared_ptr<const AbstractVGen> twoInputs(
        new AbstractVGen("twoInputs", "@out = @in1 + @in2;", { "in1", "in2" }, {}, {}));
    VGen incrementalInputs(twoInputs);
    EXPECT_FALSE(incrementalInputs.validate());
    incrementalInputs.addVGenInput(0);
    EXPECT_FALSE(incrementalInputs.validate());
    incrementalInputs.addConstantInput(-45.0f);
    EXPECT_TRUE(incrementalInputs.validate());
    incrementalInputs.addVGenInput(1);
    EXPECT_FALSE(incrementalInputs.validate());
}

TEST(VGenTest, InputValuesAndTypesRetained) {
    std::shared_ptr<const AbstractVGen> threeInputs(
        new AbstractVGen("threeInputs", "@out = @a + @b + @c;", { "a", "b", "c" }, {}, {}));
    VGen allConstants(threeInputs);
    allConstants.addConstantInput(-1.0f);
    allConstants.addConstantInput(2.0f);
    allConstants.addConstantInput(45.0f);
    float constantValue = 0.0f;
    int vgenIndex = 0;
    EXPECT_TRUE(allConstants.getInputConstantValue(0, constantValue));
    EXPECT_EQ(-1.0f, constantValue);
    EXPECT_FALSE(allConstants.getInputVGenIndex(0, vgenIndex));
    EXPECT_EQ(0, vgenIndex);
    constantValue = 0.0f;
    EXPECT_TRUE(allConstants.getInputConstantValue(1, constantValue));
    EXPECT_EQ(2.0f, constantValue);
    EXPECT_FALSE(allConstants.getInputVGenIndex(1, vgenIndex));
    EXPECT_EQ(0, vgenIndex);
    constantValue = 0.0f;
    EXPECT_TRUE(allConstants.getInputConstantValue(2, constantValue));
    EXPECT_EQ(45.0f, constantValue);
    EXPECT_FALSE(allConstants.getInputVGenIndex(2, vgenIndex));
    EXPECT_EQ(0, vgenIndex);
    constantValue = 0.0f;
    EXPECT_FALSE(allConstants.getInputConstantValue(3, constantValue));
    EXPECT_EQ(0.0f, constantValue);
    EXPECT_FALSE(allConstants.getInputVGenIndex(3, vgenIndex));
    EXPECT_EQ(0, vgenIndex);
    EXPECT_TRUE(allConstants.validate());

    VGen allVGens(threeInputs);
    allVGens.addVGenInput(2);
    allVGens.addVGenInput(3);
    allVGens.addVGenInput(0);
    constantValue = 0.0f;
    vgenIndex = -1;
    EXPECT_FALSE(allVGens.getInputConstantValue(0, constantValue));
    EXPECT_EQ(0.0f, constantValue);
    EXPECT_TRUE(allVGens.getInputVGenIndex(0, vgenIndex));
    EXPECT_EQ(2, vgenIndex);
    vgenIndex = -1;
    EXPECT_FALSE(allVGens.getInputConstantValue(1, constantValue));
    EXPECT_EQ(0.0f, constantValue);
    EXPECT_TRUE(allVGens.getInputVGenIndex(1, vgenIndex));
    EXPECT_EQ(3, vgenIndex);
    vgenIndex = -1;
    EXPECT_FALSE(allVGens.getInputConstantValue(2, constantValue));
    EXPECT_EQ(0.0f, constantValue);
    EXPECT_TRUE(allVGens.getInputVGenIndex(2, vgenIndex));
    EXPECT_EQ(0, vgenIndex);
    vgenIndex = -1;
    EXPECT_FALSE(allVGens.getInputConstantValue(3, constantValue));
    EXPECT_EQ(0.0f, constantValue);
    EXPECT_FALSE(allVGens.getInputVGenIndex(3, vgenIndex));
    EXPECT_EQ(-1, vgenIndex);
    EXPECT_TRUE(allVGens.validate());

    VGen pickAndMix(threeInputs);
    pickAndMix.addConstantInput(-23.7f);
    pickAndMix.addVGenInput(14);
    pickAndMix.addConstantInput(12224.3);
    constantValue = 0.0f;
    vgenIndex = -1;
    EXPECT_TRUE(pickAndMix.getInputConstantValue(0, constantValue));
    EXPECT_EQ(-23.7f, constantValue);
    EXPECT_FALSE(pickAndMix.getInputVGenIndex(0, vgenIndex));
    EXPECT_EQ(-1, vgenIndex);
    constantValue = 0.0f;
    vgenIndex = -1;
    EXPECT_FALSE(pickAndMix.getInputConstantValue(1, constantValue));
    EXPECT_EQ(0.0f, constantValue);
    EXPECT_TRUE(pickAndMix.getInputVGenIndex(1, vgenIndex));
    EXPECT_EQ(14, vgenIndex);
    constantValue = 0.0f;
    vgenIndex = -1;
    EXPECT_TRUE(pickAndMix.getInputConstantValue(2, constantValue));
    EXPECT_EQ(12224.3f, constantValue);
    EXPECT_FALSE(pickAndMix.getInputVGenIndex(2, vgenIndex));
    EXPECT_EQ(-1, vgenIndex);
    EXPECT_TRUE(pickAndMix.validate());
}

} // namespace scin

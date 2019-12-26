#include "gtest/gtest.h"

#include "core/AbstractVGen.hpp"
#include "core/Intrinsic.hpp"

#include <string>
#include <vector>

namespace scin {

TEST(AbstractVGenTest, DuplicateParameterNames) {
    AbstractVGen dupIn("dupIn", { "in1", "in1" }, { "out" }, { { 1, 1 } }, { { 1 } }, "@out = @in1 + @in1;");
    EXPECT_FALSE(dupIn.prepareTemplate());
    EXPECT_FALSE(dupIn.valid());

    AbstractVGen dupOut("dupOut", {}, { "out", "out" }, { {} }, { { 1, 1 } }, "@out = 2.0f;");
    EXPECT_FALSE(dupOut.prepareTemplate());
    EXPECT_FALSE(dupOut.valid());

    AbstractVGen inCrossOut("inCrossOut", { "cross" }, { "cross" }, { { 1 } }, { { 1 } }, "@cross = @cross;");
    EXPECT_FALSE(inCrossOut.prepareTemplate());
    EXPECT_FALSE(inCrossOut.valid());
}

TEST(AbstractVGenTest, ReservedParameterNames) {
    AbstractVGen timeInput("timeInput", { "time" }, { "out" }, { { 1 } }, { { 1 } }, "@out = @time;");
    EXPECT_FALSE(timeInput.prepareTemplate());
    EXPECT_FALSE(timeInput.valid());
}

TEST(AbstractVGenTest, UnknownParameterNames) {
    AbstractVGen noInputParams("noParams", {}, { "out" }, { {} }, { { 1 } }, "@out = sin(@freq * 2 * pi * @time);");
    EXPECT_FALSE(noInputParams.prepareTemplate());
    EXPECT_FALSE(noInputParams.valid());

    AbstractVGen absentInputParam("absentiInputParam", { "a" }, { "out" }, { { 1, 1 } }, { { 1 } },
                                  "@out = @time * (@a + @b);");
    EXPECT_FALSE(absentInputParam.prepareTemplate());
    EXPECT_FALSE(absentInputParam.valid());

    AbstractVGen noOutputParam("absentOutputParam", { "a" }, {}, { { 1 } }, { {} }, "@out = 1.0f;");
    EXPECT_FALSE(noOutputParam.prepareTemplate());
    EXPECT_FALSE(noOutputParam.valid());
}

TEST(AbstractVGenTest, DimensionMismatches) {}

TEST(AbstractVGenTest, ParameterizeInvalid) {
    AbstractVGen invalid("invalid", {}, {}, {}, {}, "@out = 1.0f;");
    EXPECT_FALSE(invalid.prepareTemplate());
    EXPECT_FALSE(invalid.valid());
    EXPECT_EQ("", invalid.parameterize({}, {}, {}));

    AbstractVGen mismatch("mistmatchInput", { "in1", "in2" }, { "out" }, { { 1, 1 } }, { { 1 } },
                          "@out = @in1 + @in2 / 2.0;");
    EXPECT_TRUE(mismatch.prepareTemplate());
    EXPECT_TRUE(mismatch.valid());
    EXPECT_EQ("", mismatch.parameterize({ "onlyOne" }, {}, { "out" }));
}

TEST(AbstractVGenTest, ParameterizeValid) {
    AbstractVGen constant("constant", {}, { "out" }, { {} }, { { 1 } }, "@out = 2.0f;");
    EXPECT_TRUE(constant.prepareTemplate());
    EXPECT_TRUE(constant.valid());
    EXPECT_EQ("subOut = 2.0f;", constant.parameterize({}, {}, { "subOut" }));

    AbstractVGen passthrough("passthrough", { "in" }, { "out" }, { { 1 } }, { { 1 } }, "@out = @in;");
    EXPECT_TRUE(passthrough.prepareTemplate());
    EXPECT_TRUE(passthrough.valid());
    EXPECT_EQ("passthrough_11_out = passthrough_11_in;",
              passthrough.parameterize({ "passthrough_11_in" }, {}, { "passthrough_11_out" }));

    AbstractVGen moreComplex("moreComplex", { "freq", "phase", "mul", "add" }, { "out" }, { { 1, 1, 1, 1 } }, { { 4 } },
                             "float a = @add + @mul * (sin((@time * 2.0 * pi * @freq) + @phase));\n"
                             "@out = float4(a, a, a, 1.0);\n");
    EXPECT_TRUE(moreComplex.prepareTemplate());
    EXPECT_TRUE(moreComplex.valid());
    EXPECT_EQ("float temp = 0.5f + ubo.moreComplex_mul * (sin((time * 2.0 * pi * otherVGen_out) + normPos.x));\n"
              "gl_FragColor = float4(temp, temp, temp, 1.0);\n",
              moreComplex.parameterize({ "otherVGen_out", "normPos.x", "ubo.moreComplex_mul", "0.5f" },
                                       { { Intrinsic::kTime, "time" } }, { "gl_FragColor" }));
}

} // namespace scin

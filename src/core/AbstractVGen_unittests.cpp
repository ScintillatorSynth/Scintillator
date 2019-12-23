#include "gtest/gtest.h"

#include "core/AbstractVGen.hpp"

#include <string>
#include <vector>

namespace scin {

TEST(AbstractVGenTest, DuplicateParameterNames) {
    AbstractVGen dupFrag("dupFrag", "@out = @in1 + @in1;", { "in1", "in1" });
    EXPECT_FALSE(dupFrag.prepareTemplate());
    EXPECT_FALSE(dupFrag.valid());

    AbstractVGen dupFragIntrinsic("dupFragIntrinsic", "@out = @time * 2.0 + @squids;", { "squids", "time" },
                                  { "time" });
    EXPECT_FALSE(dupFragIntrinsic.prepareTemplate());
    EXPECT_FALSE(dupFragIntrinsic.valid());

    AbstractVGen dupIntermediates("dupIntermediates", "@temp = 1.0;\n@out = @temp;", {}, {}, { "temp", "temp" });
    EXPECT_FALSE(dupIntermediates.prepareTemplate());
    EXPECT_FALSE(dupIntermediates.valid());
}

TEST(AbstractVGenTest, ReservedParameterNames) {
    AbstractVGen outInput("outInput", "@out = @out", { "out" });
    EXPECT_FALSE(outInput.prepareTemplate());
    EXPECT_FALSE(outInput.valid());
}

TEST(AbstractVGenTest, UnknownParameterNames) {
    AbstractVGen noParams("noParams", "@out = sin(@freq * 2 * pi);");
    EXPECT_FALSE(noParams.prepareTemplate());
    EXPECT_FALSE(noParams.valid());

    AbstractVGen absentParam("absentParam", "@out = @time * @undefined;", {}, { "time" });
    EXPECT_FALSE(absentParam.prepareTemplate());
    EXPECT_FALSE(absentParam.valid());
}

TEST(AbstractVGenTest, MissingOrMultipleOutputs) {
    AbstractVGen noOutput("noOutput", "float @temp = @in * 2.0;\n@notOut = sin(@temp);\n", { "in" }, {}, { "temp" });
    EXPECT_FALSE(noOutput.prepareTemplate());
    EXPECT_FALSE(noOutput.valid());

    AbstractVGen twoOut("twoOut", "float @temp = @out / 3.0;\n@out = 656.0;", {}, {}, { "temp" });
    EXPECT_FALSE(twoOut.prepareTemplate());
    EXPECT_FALSE(twoOut.valid());
}

TEST(AbstractVGenTest, ParameterizeInvalid) {
    AbstractVGen invalid("invalid", "@out = @out * @out + @out;");
    EXPECT_FALSE(invalid.prepareTemplate());
    EXPECT_FALSE(invalid.valid());
    EXPECT_EQ("", invalid.parameterize({}, {}, {}, "out"));

    AbstractVGen mismatch("mistmatchInput", "@out = @in1 + @in2 / 2.0;", { "in1", "in2" });
    EXPECT_TRUE(mismatch.prepareTemplate());
    EXPECT_TRUE(mismatch.valid());
    EXPECT_EQ("", mismatch.parameterize({ "onlyOne" }, {}, {}, "out"));
}

TEST(AbstractVGenTest, ParameterizeValid) {
    AbstractVGen constant("constant", "@out = 2.0f;");
    EXPECT_TRUE(constant.prepareTemplate());
    EXPECT_TRUE(constant.valid());
    EXPECT_EQ("subOut = 2.0f;", constant.parameterize({}, {}, {}, "subOut"));

    AbstractVGen passthrough("passthrough", "@out = @in;", { "in" });
    EXPECT_TRUE(passthrough.prepareTemplate());
    EXPECT_TRUE(passthrough.valid());
    EXPECT_EQ("passthrough_11_out = passthrough_11_in;",
              passthrough.parameterize({ "passthrough_11_in" }, {}, {}, "passthrough_11_out"));

    AbstractVGen moreComplex("moreComplex",
                             "float @sin = @add + @mul * (sin((@time * 2.0 * pi * @freq) + @phase));\n"
                             "@out = float4(@sin, @sin, @sin, 1.0);\n",
                             { "freq", "phase", "mul", "add" }, { "time" }, { "sin" });
    EXPECT_TRUE(moreComplex.prepareTemplate());
    EXPECT_TRUE(moreComplex.valid());
    EXPECT_EQ("float temp = 0.5f + ubo.moreComplex_mul * (sin((time * 2.0 * pi * otherVGen_out) + normPos.x));\n"
              "gl_FragColor = float4(temp, temp, temp, 1.0);\n",
              moreComplex.parameterize({ "otherVGen_out", "normPos.x", "ubo.moreComplex_mul", "0.5f" }, { "time" },
                                       { "temp" }, "gl_FragColor"));
}

} // namespace scin

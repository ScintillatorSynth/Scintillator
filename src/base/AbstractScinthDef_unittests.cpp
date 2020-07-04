#include "gtest/gtest.h"

#include "base/AbstractScinthDef.hpp"
#include "base/Archetypes.hpp"
#include "base/VGen.hpp"

#include <memory>
#include <unordered_set>
#include <vector>

namespace scin { namespace base { namespace {

class AbstractScinthDefTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_EQ(6,
                  m_arch.parseAbstractVGensFromString("---\n"
                                                      "name: Double\n"
                                                      "rates: [ frame, shape, pixel ]\n"
                                                      "inputs: [ a ]\n"
                                                      "outputs: [ out ]\n"
                                                      "dimensions:\n"
                                                      "    - inputs: 1\n"
                                                      "      outputs: 1\n"
                                                      "    - inputs: 2\n"
                                                      "      outputs: 2\n"
                                                      "    - inputs: 3\n"
                                                      "      outputs: 3\n"
                                                      "    - inputs: 4\n"
                                                      "      outputs: 4\n"
                                                      "shader: \"@out = 2.0 * @a;\"\n"
                                                      "---\n"
                                                      "name: FragOut\n"
                                                      "rates: [ pixel ]\n"
                                                      "inputs: [ a ]\n"
                                                      "outputs: [ out ]\n"
                                                      "dimensions:\n"
                                                      "    - inputs: 1\n"
                                                      "      outputs: 4\n"
                                                      "shader: \"@out = vec4(@a, @a, @a, 1.0);\"\n"
                                                      "---\n"
                                                      "name: NormPos\n"
                                                      "rates: [ shape, pixel ]\n"
                                                      "outputs: [ out ]\n"
                                                      "dimensions:\n"
                                                      "    - outputs: 2\n"
                                                      "shader: \"@out = @normPos;\"\n"
                                                      "---\n"
                                                      "name: TexPos\n"
                                                      "rates: [ shape, pixel ]\n"
                                                      "outputs: [ out ]\n"
                                                      "dimensions:\n"
                                                      "    - outputs: 2\n"
                                                      "shader: \"@out = @texPos;\"\n"
                                                      "---\n"
                                                      "name: TexPos\n"
                                                      "rates: [ shape, pixel ]\n"
                                                      "outputs: [ out ]\n"
                                                      "dimensions:\n"
                                                      "    - outputs: 2\n"
                                                      "shader: \"@out = @texPos;\"\n"
                                                      "---\n"
                                                      "name: VX\n"
                                                      "rates: [ frame, shape, pixel ]\n"
                                                      "inputs:\n"
                                                      "    - vec\n"
                                                      "outputs:\n"
                                                      "    - out\n"
                                                      "dimensions:\n"
                                                      "    - inputs: 1\n"
                                                      "      outputs: 1\n"
                                                      "    - inputs: 2\n"
                                                      "      outputs: 2\n"
                                                      "    - inputs: 3\n"
                                                      "      outputs: 3\n"
                                                      "    - inputs: 4\n"
                                                      "      outputs: 4\n"
                                                      "shader: \"@out = @vec.x;\"\n"));
    }

    Archetypes m_arch;
};

TEST_F(AbstractScinthDefTest, InvalidRatesFailBuild) {
    // Rate flow pixel => frame => pixel invalid.
    std::shared_ptr<AbstractScinthDef> def = m_arch.parseOnly("name: a\n"
                                                              "vgens:\n"
                                                              "    - className: Double\n"
                                                              "      rate: pixel\n"
                                                              "      inputs:\n"
                                                              "          - type: constant\n"
                                                              "            dimension: 1\n"
                                                              "            value: 1.0\n"
                                                              "      outputs:\n"
                                                              "          - dimension: 1\n"
                                                              "    - className: Double\n"
                                                              "      rate: frame\n"
                                                              "      inputs:\n"
                                                              "          - type: vgen\n"
                                                              "            dimension: 1\n"
                                                              "            vgenIndex: 0\n"
                                                              "            outputIndex: 0\n"
                                                              "      outputs:\n"
                                                              "          - dimension: 1\n"
                                                              "    - className: FragOut\n"
                                                              "      rate: pixel\n"
                                                              "      inputs:\n"
                                                              "          - type: vgen\n"
                                                              "            dimension: 1\n"
                                                              "            vgenIndex: 1\n"
                                                              "            outputIndex: 0\n"
                                                              "      outputs:\n"
                                                              "          - dimension: 4\n");
    ASSERT_NE(nullptr, def);
    EXPECT_FALSE(def->build());

    // Rate flow shape => frame => pixel invalid.
    def = m_arch.parseOnly("name: a\n"
                           "vgens:\n"
                           "    - className: Double\n"
                           "      rate: shape\n"
                           "      inputs:\n"
                           "          - type: constant\n"
                           "            dimension: 1\n"
                           "            value: 1.0\n"
                           "      outputs:\n"
                           "          - dimension: 1\n"
                           "    - className: Double\n"
                           "      rate: frame\n"
                           "      inputs:\n"
                           "          - type: vgen\n"
                           "            dimension: 1\n"
                           "            vgenIndex: 0\n"
                           "            outputIndex: 0\n"
                           "      outputs:\n"
                           "          - dimension: 1\n"
                           "    - className: FragOut\n"
                           "      rate: pixel\n"
                           "      inputs:\n"
                           "          - type: vgen\n"
                           "            dimension: 1\n"
                           "            vgenIndex: 1\n"
                           "            outputIndex: 0\n"
                           "      outputs:\n"
                           "          - dimension: 4\n");
    ASSERT_NE(nullptr, def);
    EXPECT_FALSE(def->build());
}

TEST_F(AbstractScinthDefTest, SinglePixelRateVGen) {
    std::shared_ptr<AbstractScinthDef> def = m_arch.parseOnly("name: singleOut\n"
                                                              "vgens:\n"
                                                              "    - className: FragOut\n"
                                                              "      rate: pixel\n"
                                                              "      inputs:\n"
                                                              "          - type: constant\n"
                                                              "            dimension: 1\n"
                                                              "            value: 0.5\n"
                                                              "      outputs:\n"
                                                              "          - dimension: 4\n");
    ASSERT_NE(nullptr, def);
    ASSERT_TRUE(def->build());

    EXPECT_EQ("singleOut", def->name());
    EXPECT_EQ(0, def->parameters().size());
    EXPECT_EQ(1, def->instances().size());
    EXPECT_EQ(0, def->computeFixedImages().size());
    EXPECT_EQ(0, def->drawFixedImages().size());
    EXPECT_EQ(0, def->computeParameterizedImages().size());
    EXPECT_EQ(0, def->drawParameterizedImages().size());
    EXPECT_FALSE(def->hasComputeStage());

    // Fragment and uniform manifests should be empty.
    EXPECT_EQ(0, def->fragmentManifest().numberOfElements());
    EXPECT_EQ(0, def->uniformManifest().numberOfElements());
    // Vertex manifest should contain a single element, the position.
    ASSERT_EQ(1, def->vertexManifest().numberOfElements());
    EXPECT_EQ(Intrinsic::kPosition, def->vertexManifest().intrinsicForElement(0));
}

TEST_F(AbstractScinthDefTest, NormPosPixelRate) {
    std::shared_ptr<AbstractScinthDef> def = m_arch.parseOnly("name: normPosDef\n"
                                                              "vgens:\n"
                                                              "    - className: NormPos\n"
                                                              "      rate: pixel\n"
                                                              "      outputs:\n"
                                                              "         - dimension: 2\n"
                                                              "    - className: VX\n"
                                                              "      rate: pixel\n"
                                                              "      inputs:\n"
                                                              "          - type: vgen\n"
                                                              "            vgenIndex: 0\n"
                                                              "            outputIndex: 0\n"
                                                              "            dimension: 2\n"
                                                              "      outputs:\n"
                                                              "          - dimension: 1\n"
                                                              "    - className: FragOut\n"
                                                              "      rate: pixel\n"
                                                              "      inputs:\n"
                                                              "          - type: vgen\n"
                                                              "            vgenIndex: 1\n"
                                                              "            outputIndex: 0\n"
                                                              "            dimension: 1\n"
                                                              "      outputs:\n"
                                                              "          - dimension: 4\n");
    ASSERT_NE(nullptr, def);
    ASSERT_TRUE(def->build());

    EXPECT_EQ("normPosDef", def->name());
    EXPECT_EQ(0, def->parameters().size());
    EXPECT_EQ(3, def->instances().size());
    EXPECT_EQ(0, def->computeFixedImages().size());
    EXPECT_EQ(0, def->drawFixedImages().size());
    EXPECT_EQ(0, def->computeParameterizedImages().size());
    EXPECT_EQ(0, def->drawParameterizedImages().size());
    EXPECT_FALSE(def->hasComputeStage());

    // Fragment manifest should contain a single element, the NormPos intrinsic.
    ASSERT_EQ(1, def->fragmentManifest().numberOfElements());
    EXPECT_EQ(Intrinsic::kNormPos, def->fragmentManifest().intrinsicForElement(0));

    EXPECT_EQ(0, def->uniformManifest().numberOfElements());

    // Vertex manifest should contain the position and the NormPos.
    std::unordered_set<Intrinsic> vertexIntrinsics { Intrinsic::kPosition, Intrinsic::kNormPos };
    ASSERT_EQ(2, def->vertexManifest().numberOfElements());
    for (auto i = 0; i < def->vertexManifest().numberOfElements(); ++i) {
        vertexIntrinsics.erase(def->vertexManifest().intrinsicForElement(i));
    }
    EXPECT_EQ(0, vertexIntrinsics.size());
}

TEST_F(AbstractScinthDefTest, TexPosPixelRate) {
    std::shared_ptr<AbstractScinthDef> def = m_arch.parseOnly("name: normPosDef\n"
                                                              "vgens:\n"
                                                              "    - className: TexPos\n"
                                                              "      rate: pixel\n"
                                                              "      outputs:\n"
                                                              "         - dimension: 2\n"
                                                              "    - className: VX\n"
                                                              "      rate: pixel\n"
                                                              "      inputs:\n"
                                                              "          - type: vgen\n"
                                                              "            vgenIndex: 0\n"
                                                              "            outputIndex: 0\n"
                                                              "            dimension: 2\n"
                                                              "      outputs:\n"
                                                              "          - dimension: 1\n"
                                                              "    - className: FragOut\n"
                                                              "      rate: pixel\n"
                                                              "      inputs:\n"
                                                              "          - type: vgen\n"
                                                              "            vgenIndex: 1\n"
                                                              "            outputIndex: 0\n"
                                                              "            dimension: 1\n"
                                                              "      outputs:\n"
                                                              "          - dimension: 4\n");
    ASSERT_NE(nullptr, def);
    ASSERT_TRUE(def->build());

    EXPECT_EQ("normPosDef", def->name());
    EXPECT_EQ(0, def->parameters().size());
    EXPECT_EQ(3, def->instances().size());
    EXPECT_EQ(0, def->computeFixedImages().size());
    EXPECT_EQ(0, def->drawFixedImages().size());
    EXPECT_EQ(0, def->computeParameterizedImages().size());
    EXPECT_EQ(0, def->drawParameterizedImages().size());
    EXPECT_FALSE(def->hasComputeStage());

    // Fragment manifest should contain a single element, the NormPos intrinsic.
    ASSERT_EQ(1, def->fragmentManifest().numberOfElements());
    EXPECT_EQ(Intrinsic::kTexPos, def->fragmentManifest().intrinsicForElement(0));

    EXPECT_EQ(0, def->uniformManifest().numberOfElements());

    // Vertex manifest should contain the position and the NormPos.
    std::unordered_set<Intrinsic> vertexIntrinsics { Intrinsic::kPosition, Intrinsic::kTexPos };
    ASSERT_EQ(2, def->vertexManifest().numberOfElements());
    for (auto i = 0; i < def->vertexManifest().numberOfElements(); ++i) {
        vertexIntrinsics.erase(def->vertexManifest().intrinsicForElement(i));
    }
    EXPECT_EQ(0, vertexIntrinsics.size());
}

} // namespace
} // namespace base
} // namesapce base

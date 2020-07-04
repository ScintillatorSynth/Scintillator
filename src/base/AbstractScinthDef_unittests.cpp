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
        ASSERT_EQ(7,
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
                                                      "name: BWOut\n"
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
                                                      "shader: \"@out = @vec.x;\"\n"
                                                      "---\n"
                                                      "name: Time\n"
                                                      "rates: [ frame, shape, pixel ]\n"
                                                      "outputs: [ out ]\n"
                                                      "dimensions:\n"
                                                      "    - outputs: 1\n"
                                                      "shader: \"@out = @time;\"\n"));
    }

    Archetypes m_arch;
};

TEST_F(AbstractScinthDefTest, BuildFailsWithInvalidRates) {
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
                                                              "    - className: BWOut\n"
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
                           "    - className: BWOut\n"
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

TEST_F(AbstractScinthDefTest, BuildWithSinglePixelRateVGen) {
    std::shared_ptr<AbstractScinthDef> def = m_arch.parseOnly("name: singleOut\n"
                                                              "vgens:\n"
                                                              "    - className: BWOut\n"
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
    EXPECT_EQ(0, def->drawUniformManifest().numberOfElements());
    // Vertex manifest should contain a single element, the position.
    ASSERT_EQ(1, def->vertexManifest().numberOfElements());
    EXPECT_EQ(Intrinsic::kPosition, def->vertexManifest().intrinsicForElement(0));
}

TEST_F(AbstractScinthDefTest, BuildWithNormPosPixelRate) {
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
                                                              "    - className: BWOut\n"
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

    EXPECT_EQ(0, def->drawUniformManifest().numberOfElements());

    // Vertex manifest should contain the position and the NormPos.
    std::unordered_set<Intrinsic> vertexIntrinsics { Intrinsic::kPosition, Intrinsic::kNormPos };
    ASSERT_EQ(2, def->vertexManifest().numberOfElements());
    for (auto i = 0; i < def->vertexManifest().numberOfElements(); ++i) {
        vertexIntrinsics.erase(def->vertexManifest().intrinsicForElement(i));
    }
    EXPECT_EQ(0, vertexIntrinsics.size());
}

TEST_F(AbstractScinthDefTest, BuildWithTexPosPixelRate) {
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
                                                              "    - className: BWOut\n"
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

    EXPECT_EQ(0, def->drawUniformManifest().numberOfElements());

    // Vertex manifest should contain the position and the NormPos.
    std::unordered_set<Intrinsic> vertexIntrinsics { Intrinsic::kPosition, Intrinsic::kTexPos };
    ASSERT_EQ(2, def->vertexManifest().numberOfElements());
    for (auto i = 0; i < def->vertexManifest().numberOfElements(); ++i) {
        vertexIntrinsics.erase(def->vertexManifest().intrinsicForElement(i));
    }
    EXPECT_EQ(0, vertexIntrinsics.size());
}

TEST_F(AbstractScinthDefTest, BuildWithTimePixelRate) {
    std::shared_ptr<AbstractScinthDef> def = m_arch.parseOnly("name: timeTest\n"
                                                              "vgens:\n"
                                                              "    - className: Time\n"
                                                              "      rate: pixel\n"
                                                              "      outputs:\n"
                                                              "          - dimension: 1\n"
                                                              "    - className: BWOut\n"
                                                              "      rate: pixel\n"
                                                              "      inputs:\n"
                                                              "          - type: vgen\n"
                                                              "            vgenIndex: 0\n"
                                                              "            outputIndex: 0\n"
                                                              "            dimension: 1\n"
                                                              "      outputs:\n"
                                                              "          - dimension: 4\n");
    ASSERT_NE(nullptr, def);
    ASSERT_TRUE(def->build());

    EXPECT_EQ("timeTest", def->name());
    EXPECT_EQ(0, def->parameters().size());
    EXPECT_EQ(2, def->instances().size());
    EXPECT_EQ(0, def->computeFixedImages().size());
    EXPECT_EQ(0, def->drawFixedImages().size());
    EXPECT_EQ(0, def->computeParameterizedImages().size());
    EXPECT_EQ(0, def->drawParameterizedImages().size());
    EXPECT_FALSE(def->hasComputeStage());

    // Empty fragment manifest.
    EXPECT_EQ(0, def->fragmentManifest().numberOfElements());

    // One element in the draw uniform manifest, the time intrinsic.
    ASSERT_EQ(1, def->drawUniformManifest().numberOfElements());
    EXPECT_EQ(Intrinsic::kTime, def->drawUniformManifest().intrinsicForElement(0));

    // Vertex manifest should contain only the position.
    ASSERT_EQ(1, def->vertexManifest().numberOfElements());
    EXPECT_EQ(Intrinsic::kPosition, def->vertexManifest().intrinsicForElement(0));
}

TEST_F(AbstractScinthDefTest, BuildPixelRateParams) {
    std::shared_ptr<AbstractScinthDef> def = m_arch.parseOnly("name: fragParams\n"
                                                              "parameters:\n"
                                                              "    - name: a\n"
                                                              "      defaultValue: 0.5\n"
                                                              "vgens:\n"
                                                              "    - className: BWOut\n"
                                                              "      rate: pixel\n"
                                                              "      inputs:\n"
                                                              "          - type: parameter\n"
                                                              "            index:  0\n"
                                                              "            dimension: 1\n"
                                                              "      outputs:\n"
                                                              "          - dimension: 4\n");
    ASSERT_NE(nullptr, def);
    ASSERT_TRUE(def->build());

    EXPECT_EQ("fragParams", def->name());

    ASSERT_EQ(1, def->parameters().size());
    EXPECT_EQ("a", def->parameters()[0].name());
    EXPECT_EQ(0.5, def->parameters()[0].defaultValue());
    EXPECT_EQ(0, def->indexForParameterName("a"));

    EXPECT_EQ(1, def->instances().size());
    EXPECT_EQ(0, def->computeFixedImages().size());
    EXPECT_EQ(0, def->drawFixedImages().size());
    EXPECT_EQ(0, def->computeParameterizedImages().size());
    EXPECT_EQ(0, def->drawParameterizedImages().size());
    EXPECT_FALSE(def->hasComputeStage());

    // Fragment and uniform manifests should be empty.
    EXPECT_EQ(0, def->fragmentManifest().numberOfElements());
    EXPECT_EQ(0, def->drawUniformManifest().numberOfElements());
    // Vertex manifest should contain a single element, the position.
    ASSERT_EQ(1, def->vertexManifest().numberOfElements());
    EXPECT_EQ(Intrinsic::kPosition, def->vertexManifest().intrinsicForElement(0));
}

} // namespace
} // namespace base
} // namesapce base

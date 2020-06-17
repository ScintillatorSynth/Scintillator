#include "gtest/gtest.h"

#include "base/AbstractScinthDef.hpp"
#include "base/Archetypes.hpp"
#include "base/VGen.hpp"

#include <memory>
#include <vector>

namespace scin { namespace base {

namespace {

class AbstractScinthDefTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_EQ(2, m_arch.parseAbstractVGensFromString(
            "---\n"
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
        ));
    }

    Archetypes m_arch;
};

TEST_F(AbstractScinthDefTest, InvalidRatesFailBuild) {
    std::vector<std::shared_ptr<const AbstractScinthDef>> defs = m_arch.parseFromString(
        "name: a\n"
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
        "          - dimension: 4\n"
    );
    ASSERT_EQ(1, defs.size());
}

} // namespace
} // namespace base
} // namesapce base

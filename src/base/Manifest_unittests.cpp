#include "base/GTestIncludes.hpp"

#include "base/Manifest.hpp"

#include "glm/glm.hpp"

#include <string>
#include <unordered_set>

namespace scin { namespace base {

TEST(ManifestTest, EmptyManifest) {
    Manifest unpacked;
    EXPECT_EQ(0, unpacked.numberOfElements());
    EXPECT_EQ(0, unpacked.sizeInBytes());

    Manifest packed;
    packed.pack();
    EXPECT_EQ(0, packed.numberOfElements());
    EXPECT_EQ(0, packed.sizeInBytes());
}

TEST(ManifestTest, HomogeneousTypes) {
    Manifest allFloats;
    EXPECT_TRUE(allFloats.addElement("a", Manifest::ElementType::kFloat));
    EXPECT_TRUE(allFloats.addElement("b", Manifest::ElementType::kFloat));
    // Duplicate insert should fail
    EXPECT_FALSE(allFloats.addElement("b", Manifest::ElementType::kVec2));
    EXPECT_TRUE(allFloats.addElement("c", Manifest::ElementType::kFloat));
    allFloats.pack();
    ASSERT_EQ(3, allFloats.numberOfElements());
    EXPECT_EQ(3 * sizeof(float), allFloats.sizeInBytes());
    // All floats should be aligned on their size and represented in the manifest.
    std::unordered_set<std::string> allFloatNames;
    for (size_t i = 0; i < allFloats.numberOfElements(); ++i) {
        EXPECT_EQ("float", allFloats.typeNameForElement(i));
        EXPECT_EQ(sizeof(float), allFloats.strideForElement(i));
        EXPECT_EQ(i * sizeof(float), allFloats.offsetForElement(i));
        EXPECT_EQ(Manifest::ElementType::kFloat, allFloats.typeForElement(i));
        EXPECT_EQ(Intrinsic::kNotFound, allFloats.intrinsicForElement(i));
        allFloatNames.insert(allFloats.nameForElement(i));
    }
    EXPECT_EQ(3, allFloatNames.size());
    EXPECT_EQ(1, allFloatNames.count("a"));
    EXPECT_EQ(1, allFloatNames.count("b"));
    EXPECT_EQ(1, allFloatNames.count("c"));
}

TEST(ManifestTest, HeterogeneousTypes) {
    Manifest oneOfEach;
    EXPECT_TRUE(oneOfEach.addElement("theFloat", Manifest::ElementType::kFloat, Intrinsic::kTime));
    EXPECT_TRUE(oneOfEach.addElement("theVec3", Manifest::ElementType::kVec3));
    EXPECT_TRUE(oneOfEach.addElement("theVec4", Manifest::ElementType::kVec4));
    EXPECT_TRUE(oneOfEach.addElement("theVec2", Manifest::ElementType::kVec2, Intrinsic::kNormPos));
    oneOfEach.pack();

    ASSERT_EQ(4, oneOfEach.numberOfElements());
    EXPECT_EQ(10 * sizeof(float), oneOfEach.sizeInBytes());

    // vec4 should be first.
    EXPECT_EQ("vec4", oneOfEach.typeNameForElement(0));
    EXPECT_EQ(sizeof(glm::vec4), oneOfEach.strideForElement(0));
    EXPECT_EQ(0, oneOfEach.offsetForElement(0));
    EXPECT_EQ("theVec4", oneOfEach.nameForElement(0));
    EXPECT_EQ(Manifest::ElementType::kVec4, oneOfEach.typeForElement(0));
    EXPECT_EQ(Intrinsic::kNotFound, oneOfEach.intrinsicForElement(0));

    // vec3 would be next but is not 3-float aligned, but there's room for a vec2, so that should be next.
    EXPECT_EQ("vec2", oneOfEach.typeNameForElement(1));
    EXPECT_EQ(sizeof(glm::vec2), oneOfEach.strideForElement(1));
    EXPECT_EQ(4 * sizeof(float), oneOfEach.offsetForElement(1));
    EXPECT_EQ("theVec2", oneOfEach.nameForElement(1));
    EXPECT_EQ(Manifest::ElementType::kVec2, oneOfEach.typeForElement(1));
    EXPECT_EQ(Intrinsic::kNormPos, oneOfEach.intrinsicForElement(1));

    // now we can fit the vec3, so we expect it next.
    EXPECT_EQ("vec3", oneOfEach.typeNameForElement(2));
    EXPECT_EQ(sizeof(glm::vec3), oneOfEach.strideForElement(2));
    EXPECT_EQ(6 * sizeof(float), oneOfEach.offsetForElement(2));
    EXPECT_EQ("theVec3", oneOfEach.nameForElement(2));
    EXPECT_EQ(Manifest::ElementType::kVec3, oneOfEach.typeForElement(2));
    EXPECT_EQ(Intrinsic::kNotFound, oneOfEach.intrinsicForElement(2));

    // Lastly the float fits in.
    EXPECT_EQ("float", oneOfEach.typeNameForElement(3));
    EXPECT_EQ(sizeof(float), oneOfEach.strideForElement(3));
    EXPECT_EQ(9 * sizeof(float), oneOfEach.offsetForElement(3));
    EXPECT_EQ("theFloat", oneOfEach.nameForElement(3));
    EXPECT_EQ(Manifest::ElementType::kFloat, oneOfEach.typeForElement(3));
    EXPECT_EQ(Intrinsic::kTime, oneOfEach.intrinsicForElement(3));
}

} // namespace base

} // namespace scin

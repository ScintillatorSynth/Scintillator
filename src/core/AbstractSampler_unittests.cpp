#include "gtest/gtest.h"

#include "core/AbstractSampler.hpp"

namespace scin { namespace core {

TEST(AbstractSamplerTest, FilterMode) {
    AbstractSampler a, b;
    a.setMinFilterMode(AbstractSampler::FilterMode::kLinear);
    b.setMinFilterMode(AbstractSampler::FilterMode::kNearest);
    EXPECT_NE(a.key(), b.key());
    EXPECT_EQ(a.minFilterMode(), AbstractSampler::FilterMode::kLinear);
    EXPECT_EQ(b.minFilterMode(), AbstractSampler::FilterMode::kNearest);
    EXPECT_EQ(a.magFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.isAnisotropicFilteringEnabled(), b.isAnisotropicFilteringEnabled());
    EXPECT_EQ(a.addressModeU(), b.addressModeU());
    EXPECT_EQ(a.addressModeV(), b.addressModeV());
    EXPECT_EQ(a.clampBorderColor(), b.clampBorderColor());

    a.setMinFilterMode(AbstractSampler::FilterMode::kNearest);
    EXPECT_EQ(a.key(), b.key());

    a.setMagFilterMode(AbstractSampler::FilterMode::kNearest);
    EXPECT_NE(a.key(), b.key());
    EXPECT_EQ(a.magFilterMode(), AbstractSampler::FilterMode::kNearest);
    EXPECT_EQ(b.magFilterMode(), AbstractSampler::FilterMode::kLinear);
    EXPECT_EQ(a.minFilterMode(), b.minFilterMode());
    EXPECT_EQ(a.isAnisotropicFilteringEnabled(), b.isAnisotropicFilteringEnabled());
    EXPECT_EQ(a.addressModeU(), b.addressModeU());
    EXPECT_EQ(a.addressModeV(), b.addressModeV());
    EXPECT_EQ(a.clampBorderColor(), b.clampBorderColor());
}

TEST(AbstractSamplerTest, AnistropicFiltering) {
    AbstractSampler a, b;
    a.enableAnisotropicFiltering(false);
    EXPECT_NE(a.key(), b.key());
    EXPECT_EQ(a.isAnisotropicFilteringEnabled(), false);
    EXPECT_EQ(b.isAnisotropicFilteringEnabled(), true);
    EXPECT_EQ(a.minFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.magFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.addressModeU(), b.addressModeU());
    EXPECT_EQ(a.addressModeV(), b.addressModeV());
    EXPECT_EQ(a.clampBorderColor(), b.clampBorderColor());
}

TEST(AbstractSamplerTest, AddressMode) {
    AbstractSampler a, b;
    a.setAddressModeU(AbstractSampler::AddressMode::kClampToEdge);
    EXPECT_NE(a.key(), b.key());
    EXPECT_EQ(a.addressModeU(), AbstractSampler::AddressMode::kClampToEdge);
    EXPECT_EQ(b.addressModeU(), AbstractSampler::AddressMode::kClampToBorder);
    EXPECT_EQ(a.minFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.magFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.isAnisotropicFilteringEnabled(), b.isAnisotropicFilteringEnabled());
    EXPECT_EQ(a.addressModeV(), b.addressModeV());
    EXPECT_EQ(a.clampBorderColor(), b.clampBorderColor());

    b.setAddressModeU(AbstractSampler::AddressMode::kRepeat);
    EXPECT_NE(a.key(), b.key());
    EXPECT_EQ(a.addressModeU(), AbstractSampler::AddressMode::kClampToEdge);
    EXPECT_EQ(b.addressModeU(), AbstractSampler::AddressMode::kRepeat);
    EXPECT_EQ(a.minFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.magFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.isAnisotropicFilteringEnabled(), b.isAnisotropicFilteringEnabled());
    EXPECT_EQ(a.addressModeV(), b.addressModeV());
    EXPECT_EQ(a.clampBorderColor(), b.clampBorderColor());

    a.setAddressModeU(AbstractSampler::AddressMode::kMirroredRepeat);
    EXPECT_NE(a.key(), b.key());
    EXPECT_EQ(a.addressModeU(), AbstractSampler::AddressMode::kMirroredRepeat);
    EXPECT_EQ(b.addressModeU(), AbstractSampler::AddressMode::kRepeat);
    EXPECT_EQ(a.minFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.magFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.isAnisotropicFilteringEnabled(), b.isAnisotropicFilteringEnabled());
    EXPECT_EQ(a.addressModeV(), b.addressModeV());
    EXPECT_EQ(a.clampBorderColor(), b.clampBorderColor());

    b.setAddressModeU(AbstractSampler::AddressMode::kMirroredRepeat);
    b.setAddressModeV(AbstractSampler::AddressMode::kClampToEdge);
    EXPECT_NE(a.key(), b.key());
    EXPECT_EQ(a.addressModeV(), AbstractSampler::AddressMode::kClampToBorder);
    EXPECT_EQ(b.addressModeV(), AbstractSampler::AddressMode::kClampToEdge);
    EXPECT_EQ(a.minFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.magFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.isAnisotropicFilteringEnabled(), b.isAnisotropicFilteringEnabled());
    EXPECT_EQ(a.addressModeU(), b.addressModeU());
    EXPECT_EQ(a.clampBorderColor(), b.clampBorderColor());

    a.setAddressModeV(AbstractSampler::AddressMode::kRepeat);
    EXPECT_NE(a.key(), b.key());
    EXPECT_EQ(a.addressModeV(), AbstractSampler::AddressMode::kRepeat);
    EXPECT_EQ(b.addressModeV(), AbstractSampler::AddressMode::kClampToEdge);
    EXPECT_EQ(a.minFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.magFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.isAnisotropicFilteringEnabled(), b.isAnisotropicFilteringEnabled());
    EXPECT_EQ(a.addressModeU(), b.addressModeU());
    EXPECT_EQ(a.clampBorderColor(), b.clampBorderColor());

    b.setAddressModeV(AbstractSampler::AddressMode::kMirroredRepeat);
    EXPECT_NE(a.key(), b.key());
    EXPECT_EQ(a.addressModeV(), AbstractSampler::AddressMode::kRepeat);
    EXPECT_EQ(b.addressModeV(), AbstractSampler::AddressMode::kMirroredRepeat);
    EXPECT_EQ(a.minFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.magFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.isAnisotropicFilteringEnabled(), b.isAnisotropicFilteringEnabled());
    EXPECT_EQ(a.addressModeU(), b.addressModeU());
    EXPECT_EQ(a.clampBorderColor(), b.clampBorderColor());
}

TEST(AbstractSamplerTest, ClampBorderColor) {
    AbstractSampler a, b;

    a.setClampBorderColor(AbstractSampler::ClampBorderColor::kBlack);
    EXPECT_NE(a.key(), b.key());
    EXPECT_EQ(a.clampBorderColor(), AbstractSampler::ClampBorderColor::kBlack);
    EXPECT_EQ(b.clampBorderColor(), AbstractSampler::ClampBorderColor::kTransparentBlack);
    EXPECT_EQ(a.minFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.magFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.isAnisotropicFilteringEnabled(), b.isAnisotropicFilteringEnabled());
    EXPECT_EQ(a.addressModeU(), b.addressModeU());
    EXPECT_EQ(a.addressModeV(), b.addressModeV());

    b.setClampBorderColor(AbstractSampler::ClampBorderColor::kWhite);
    EXPECT_EQ(a.clampBorderColor(), AbstractSampler::ClampBorderColor::kBlack);
    EXPECT_EQ(b.clampBorderColor(), AbstractSampler::ClampBorderColor::kWhite);
    EXPECT_EQ(a.minFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.magFilterMode(), b.magFilterMode());
    EXPECT_EQ(a.isAnisotropicFilteringEnabled(), b.isAnisotropicFilteringEnabled());
    EXPECT_EQ(a.addressModeU(), b.addressModeU());
    EXPECT_EQ(a.addressModeV(), b.addressModeV());
}

} // namespace core

} // namespace scin

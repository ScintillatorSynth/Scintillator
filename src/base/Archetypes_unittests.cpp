#include "base/GTestIncludes.hpp"

#include "base/AbstractScinthDef.hpp"
#include "base/AbstractVGen.hpp"
#include "base/FileSystem.hpp"
#include "base/Intrinsic.hpp"
#include "base/Archetypes.hpp"
#include "base/VGen.hpp"

#include <fstream>

namespace {

void populateAbstractVGens(std::shared_ptr<scin::base::Archetypes> parser) {
    parser->parseAbstractVGensFromString("---\n"
                                         "name: NoInput\n"
                                         "rates: [ pixel ]\n"
                                         "outputs: [ out ]\n"
                                         "dimensions:\n"
                                         "  - outputs: 1\n"
                                         "shader: \"@out = 1.0f;\"\n"
                                         "---\n"
                                         "name: OneInput\n"
                                         "rates: [ pixel ]\n"
                                         "inputs:\n"
                                         "  - a\n"
                                         "outputs:\n"
                                         "  - out\n"
                                         "dimensions:\n"
                                         "  - inputs: 1\n"
                                         "    outputs: 1\n"
                                         "shader: \"@out = @a;\"\n"
                                         "---\n"
                                         "name: TwoInput\n"
                                         "rates: [ pixel ]\n"
                                         "inputs:\n"
                                         "  - a\n"
                                         "  - b\n"
                                         "outputs:\n"
                                         "  - out\n"
                                         "dimensions:\n"
                                         "  - inputs: [1, 1]\n"
                                         "    outputs: 1\n"
                                         "shader: \"@out = @a * @b;\"\n"
                                         "---\n"
                                         "name: ThreeInput\n"
                                         "rates: [ pixel ]\n"
                                         "inputs: [a, b, c]\n"
                                         "outputs: [ out ]\n"
                                         "dimensions:\n"
                                         "  - inputs: 1\n"
                                         "    outputs: 1\n"
                                         "shader: \"@out = @a + @b + @c;\"\n");
}

void clobberFileWithString(const fs::path& filePath, const std::string& contents) {
    std::ofstream outFile(filePath, std::ofstream::out | std::ofstream::trunc);
    ASSERT_TRUE(outFile.good());
    outFile << contents;
    ASSERT_TRUE(outFile.good());
}

}

namespace scin { namespace base {

TEST(ArchetypesTest, InvalidAbstractVGenYamlStrings) {
    Archetypes parser;
    EXPECT_EQ(0, parser.parseAbstractVGensFromString(""));
    EXPECT_EQ(0, parser.parseAbstractVGensFromString("xxzz"));
    EXPECT_EQ(0, parser.parseAbstractVGensFromString("[ 1 2 3 4 ]"));
    EXPECT_EQ(0, parser.parseAbstractVGensFromString("\""));
    EXPECT_EQ(0, parser.parseAbstractVGensFromString("\a\b\f\v\\x08\x16"));
    EXPECT_EQ(0, parser.parseAbstractVGensFromString("'f#oo: '%>^'|"));

    // The tags "name", "outputs", "dimensions", and "shader" are currently required in all VGen specs.
    EXPECT_EQ(0,
              parser.parseAbstractVGensFromString("---\n"
                                                  "name: TestBadVGen\n"
                                                  "rates: [ pixel ]\n"
                                                  "inputs:\n"
                                                  "  - a\n"
                                                  "  - b\n"));
    EXPECT_EQ(0,
              parser.parseAbstractVGensFromString("---\n"
                                                  "rates: [ pixel ]\n"
                                                  "outputs:\n"
                                                  "  - out\n"
                                                  "shader: \"@out = 1.0;\"\n"));
    EXPECT_EQ(0, parser.numberOfAbstractVGens());
}

TEST(ArchetypesTest, ValidAbstractVGenYamlStrings) {
    Archetypes parser;
    EXPECT_EQ(3,
              parser.parseAbstractVGensFromString("---\n"
                                                  "name: JustNameAndFragment\n"
                                                  "rates: [ pixel ]\n"
                                                  "outputs: [ out ]\n"
                                                  "dimensions:\n"
                                                  "  - outputs: 1\n"
                                                  "shader: \"@out = 1.0;\"\n"
                                                  "---\n"
                                                  "name: AddInput\n"
                                                  "rates: [ pixel ]\n"
                                                  "inputs: [ a ]\n"
                                                  "outputs: [ out ]\n"
                                                  "dimensions:\n"
                                                  "  - inputs: [ 1 ]\n"
                                                  "    outputs: [ 1 ]\n"
                                                  "shader: \"@out = @a;\"\n"
                                                  "---\n"
                                                  "name: Overwrite\n"
                                                  "rates: [ pixel ]\n"
                                                  "outputs: [ out ]\n"
                                                  "dimensions:\n"
                                                  "  - outputs: 1\n"
                                                  "shader: \"@out = 0.0;\"\n"));
    EXPECT_EQ(3, parser.numberOfAbstractVGens());
    std::shared_ptr<const AbstractVGen> justNameAndFragment = parser.getAbstractVGenNamed("JustNameAndFragment");
    ASSERT_TRUE(justNameAndFragment);
    EXPECT_EQ("JustNameAndFragment", justNameAndFragment->name());
    EXPECT_EQ("@out = 1.0;", justNameAndFragment->shader());
    std::shared_ptr<const AbstractVGen> addInput = parser.getAbstractVGenNamed("AddInput");
    ASSERT_TRUE(addInput);
    EXPECT_EQ("AddInput", addInput->name());
    ASSERT_EQ(1, addInput->inputs().size());
    EXPECT_EQ("a", addInput->inputs()[0]);
    EXPECT_EQ("@out = @a;", addInput->shader());
    std::shared_ptr<const AbstractVGen> overwrite = parser.getAbstractVGenNamed("Overwrite");
    ASSERT_TRUE(overwrite);
    EXPECT_EQ("Overwrite", overwrite->name());
    EXPECT_EQ("@out = 0.0;", overwrite->shader());

    EXPECT_FALSE(parser.getAbstractVGenNamed("Nonexistent"));

    EXPECT_EQ(1,
              parser.parseAbstractVGensFromString("---\n"
                                                  "name: Overwrite\n"
                                                  "rates: [ pixel ]\n"
                                                  "outputs:\n"
                                                  "  - out\n"
                                                  "dimensions:\n"
                                                  "  - outputs: 1\n"
                                                  "shader: \"@out = @time;\"\n"));
    // Should have overwritten the first Overwrite VGen.
    EXPECT_EQ(3, parser.numberOfAbstractVGens());
    overwrite = parser.getAbstractVGenNamed("Overwrite");
    ASSERT_TRUE(overwrite);
    EXPECT_EQ("Overwrite", overwrite->name());
    ASSERT_EQ(1, overwrite->intrinsics().size());
    EXPECT_EQ(1, overwrite->intrinsics().count(Intrinsic::kTime));
    EXPECT_EQ("@out = @time;", overwrite->shader());
    EXPECT_EQ(1,
              parser.parseAbstractVGensFromString("---\n"
                                                  "name: Overwrite\n"
                                                  "rates: [ frame, shape, pixel ]\n"
                                                  "outputs: [ out ]\n"
                                                  "dimensions:\n"
                                                  "  - outputs: [ 1 ]\n"
                                                  "shader: \"float fl = 2.0; @out = fl;\"\n"));
    EXPECT_EQ(3, parser.numberOfAbstractVGens());
    overwrite = parser.getAbstractVGenNamed("Overwrite");
    ASSERT_TRUE(overwrite);
    EXPECT_EQ("Overwrite", overwrite->name());
    EXPECT_EQ(AbstractVGen::Rates::kFrame | AbstractVGen::Rates::kShape | AbstractVGen::Rates::kPixel,
              overwrite->supportedRates());
    EXPECT_EQ(0, overwrite->intrinsics().size());
    EXPECT_EQ("float fl = 2.0; @out = fl;", overwrite->shader());
}

TEST(ArchetypesTest, ParseAbstractVGenFromFile) {
    fs::path tempFile = fs::temp_directory_path() / "Archetypes_FileTest.yaml";
    if (fs::exists(tempFile)) {
        ASSERT_TRUE(fs::remove(tempFile));
    }

    Archetypes parser;
    // Nonexistent file should not parse.
    EXPECT_EQ(0, parser.loadAbstractVGensFromFile(tempFile.string()));

    clobberFileWithString(tempFile, "'f#oo: '%>^'|");
    EXPECT_EQ(0, parser.loadAbstractVGensFromFile(tempFile.string()));

    clobberFileWithString(tempFile,
                          "---\n"
                          "name: FileVGen\n"
                          "rates: [ pixel ]\n"
                          "outputs: [ out ]\n"
                          "dimensions:\n"
                          "  - outputs: [ 1 ]\n"
                          "shader: \"@out = 0.0f\"\n");
    EXPECT_EQ(1, parser.loadAbstractVGensFromFile(tempFile.string()));

    // Remove temp file.
    ASSERT_TRUE(fs::remove(tempFile));
}

TEST(ArchetypesTest, InvalidYAMLStrings) {
    std::shared_ptr<Archetypes> parser(new Archetypes());
    populateAbstractVGens(parser);
    EXPECT_EQ(0, parser->parseFromString("").size());
    EXPECT_EQ(0, parser->parseFromString("abcd").size());
    EXPECT_EQ(0, parser->parseFromString("[ a b c d ]").size());
    EXPECT_EQ(0, parser->parseFromString("'f#oo: '%>^'|").size());

    // Both name and vgens are required root keys.
    EXPECT_EQ(0, parser->parseFromString("name: NoVGens\n").size());
    EXPECT_EQ(0,
              parser
                  ->parseFromString("vgens:\n"
                                    "  - className: NoInput\n"
                                    "    rate: pixel\n")
                  .size());
    // The vgens key has to be a sequence.
    EXPECT_EQ(0,
              parser
                  ->parseFromString("name: VGensNotASequence\n"
                                    "vgens: NopeNopeNope\n")
                  .size());

    // The vgen must exist in the dictionary.
    EXPECT_EQ(0,
              parser
                  ->parseFromString("---\n"
                                    "name: missingVGen\n"
                                    "vgens:\n"
                                    "  - className: NotFound\n"
                                    "    rate: pixel\n")
                  .size());

    // The vgen must have the correct number of inputs.
    EXPECT_EQ(0,
              parser
                  ->parseFromString("name: wrongInputsVGen\n"
                                    "vgens:\n"
                                    "  - className: NoInput\n"
                                    "    rate: pixel\n"
                                    "    inputs:\n"
                                    "      - type: constant\n"
                                    "        value: 200.0\n")
                  .size());

    // Bad index on vgen inputs.
    EXPECT_EQ(0,
              parser
                  ->parseFromString("name: badVGenInputsOrder\n"
                                    "vgens:\n"
                                    "  - className: OneInput\n"
                                    "    rate: pixel\n"
                                    "    inputs:\n"
                                    "      - type: vgen\n"
                                    "        vgenIndex: 1\n")
                  .size());

    EXPECT_EQ(0, parser->numberOfAbstractScinthDefs());
}

TEST(ArchetypesTest, ValidYAMLStrings) {
    std::shared_ptr<Archetypes> parser(new Archetypes());
    populateAbstractVGens(parser);

    EXPECT_EQ(1,
              parser
                  ->parseFromString("---\n"
                                    "name: firstScinth\n"
                                    "shape:\n"
                                    "    name: Quad\n"
                                    "    widthEdges: 1\n"
                                    "    heightEdges: 1\n"
                                    "vgens:\n"
                                    "  - className: NoInput\n"
                                    "    rate: pixel\n"
                                    "    outputs:\n"
                                    "      - dimension: 1\n")
                  .size());
    ASSERT_EQ(1, parser->numberOfAbstractScinthDefs());
    std::shared_ptr<const AbstractScinthDef> scinthDef = parser->getAbstractScinthDefNamed("firstScinth");
    ASSERT_EQ(1, scinthDef->instances().size());
    EXPECT_EQ("NoInput", scinthDef->instances()[0].abstractVGen()->name());
    EXPECT_EQ(0, scinthDef->instances()[0].numberOfInputs());

    EXPECT_EQ(2,
              parser
                  ->parseFromString("---\n"
                                    "name: firstScinth\n"
                                    "shape:\n"
                                    "    name: Quad\n"
                                    "    widthEdges: 1\n"
                                    "    heightEdges: 1\n"
                                    "vgens:\n"
                                    "  - className: OneInput\n"
                                    "    rate: pixel\n"
                                    "    inputs:\n"
                                    "      - type: constant\n"
                                    "        dimension: 1\n"
                                    "        value: -123.0\n"
                                    "    outputs:\n"
                                    "      - dimension: 1\n"
                                    "  - className: TwoInput\n"
                                    "    rate: pixel\n"
                                    "    inputs:\n"
                                    "      - type: vgen\n"
                                    "        dimension: 1\n"
                                    "        vgenIndex: 0\n"
                                    "        outputIndex: 0\n"
                                    "      - type: constant\n"
                                    "        dimension: 1\n"
                                    "        value: 0.0\n"
                                    "    outputs:\n"
                                    "      - dimension: 1\n"
                                    "---\n"
                                    "name: secondScinth\n"
                                    "shape:\n"
                                    "    name: Quad\n"
                                    "    widthEdges: 1\n"
                                    "    heightEdges: 1\n"
                                    "vgens: \n"
                                    "  - className: ThreeInput\n"
                                    "    rate: pixel\n"
                                    "    inputs: \n"
                                    "      - type: constant\n"
                                    "        dimension: 1\n"
                                    "        value: 1\n"
                                    "      - type: constant\n"
                                    "        dimension: 1\n"
                                    "        value: 2\n"
                                    "      - type: constant\n"
                                    "        dimension: 1\n"
                                    "        value: 3\n"
                                    "    outputs:\n"
                                    "      - dimension: 1\n")
                  .size());
    ASSERT_EQ(2, parser->numberOfAbstractScinthDefs());

    scinthDef = parser->getAbstractScinthDefNamed("firstScinth");
    ASSERT_EQ(2, scinthDef->instances().size());
    EXPECT_EQ("OneInput", scinthDef->instances()[0].abstractVGen()->name());
    ASSERT_EQ(1, scinthDef->instances()[0].numberOfInputs());
    float inputValue = 0.0f;
    size_t vgenIndex = 0xfffff;
    size_t outputIndex = 0xffffe;
    EXPECT_TRUE(scinthDef->instances()[0].getInputConstantValue(0, inputValue));
    EXPECT_FALSE(scinthDef->instances()[0].getInputVGenIndex(0, vgenIndex, outputIndex));
    EXPECT_EQ(-123.0f, inputValue);
    EXPECT_EQ(0xfffff, vgenIndex); // vgenIndex should remain unmodified as the input is a constant.
    EXPECT_EQ(0xffffe, outputIndex);
    EXPECT_EQ("TwoInput", scinthDef->instances()[1].abstractVGen()->name());
    ASSERT_EQ(2, scinthDef->instances()[1].numberOfInputs());
    EXPECT_FALSE(scinthDef->instances()[1].getInputConstantValue(0, inputValue));
    EXPECT_TRUE(scinthDef->instances()[1].getInputVGenIndex(0, vgenIndex, outputIndex));
    EXPECT_EQ(-123.0f, inputValue); // inputValue should remain unmodified as the input is a VGen.
    EXPECT_EQ(0, vgenIndex);

    scinthDef = parser->getAbstractScinthDefNamed("secondScinth");
    ASSERT_EQ(1, scinthDef->instances().size());
    EXPECT_EQ("ThreeInput", scinthDef->instances()[0].abstractVGen()->name());
    EXPECT_TRUE(scinthDef->instances()[0].getInputConstantValue(0, inputValue));
    EXPECT_EQ(1.0f, inputValue);
    EXPECT_TRUE(scinthDef->instances()[0].getInputConstantValue(1, inputValue));
    EXPECT_EQ(2.0f, inputValue);
    EXPECT_TRUE(scinthDef->instances()[0].getInputConstantValue(2, inputValue));
    EXPECT_EQ(3.0f, inputValue);
}

} // namespace base

} // namespace scin

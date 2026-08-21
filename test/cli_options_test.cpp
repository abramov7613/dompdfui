#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
// We intentionally include the implementation here.
// It gives the tests direct access to parse_cli_args() and ApplicationOptions
// without changing production code architecture.
#define main dompdfui_embedded_main
#include "dompdfcli_main.cpp"
#undef main

namespace fs = std::filesystem;

namespace {

class CliOptionsTest : public ::testing::Test {
protected:
    fs::path root;
    fs::path input;
    fs::path output;

    void SetUp() override
    {
        root = fs::temp_directory_path() /
               ("dompdfui_gtest_" +
                std::to_string(
                    std::hash<std::string>{}(
                        std::to_string(
                            reinterpret_cast<std::uintptr_t>(this)))));

        fs::remove_all(root);
        fs::create_directories(root);

        input = root / "input.html";
        output = root / "output";

        std::ofstream file(input);
        ASSERT_TRUE(file.is_open());
        file << "<html><body>Hello</body></html>";
        file.close();

        fs::create_directories(output);
    }

    void TearDown() override
    {
        fs::remove_all(root);
    }

    ApplicationOptions parse(std::vector<std::string> args)
    {
        std::vector<std::vector<char>> storage;
        storage.reserve(args.size());

        std::vector<char*> argv;
        argv.reserve(args.size());

        for (auto& arg : args) {
            storage.emplace_back(arg.begin(), arg.end());
            storage.back().push_back('\0');
            argv.push_back(storage.back().data());
        }

        return parse_cli_args(
            static_cast<int>(argv.size()),
            argv.data());
    }

    std::vector<std::string> base_args()
    {
        return {
            "dompdfui",
            input.string(),
            output.string()
        };
    }
};

TEST_F(CliOptionsTest, UsesDefaultValues)
{
    auto opts = parse(base_args());

    EXPECT_EQ(opts.php_memory_limit(), 268435456ULL);
    EXPECT_FALSE(opts.force_out());

    EXPECT_FALSE(opts.isRemoteEnabled());
    EXPECT_TRUE(opts.isJavascriptEnabled());
    EXPECT_TRUE(opts.isFontSubsettingEnabled());
    EXPECT_FALSE(opts.sslAllowSelfSigned());

    EXPECT_EQ(opts.dpi(), "96");
    EXPECT_EQ(opts.fontHeightRatio(), "1.1");
    EXPECT_EQ(opts.defaultMediaType(), "screen");
    EXPECT_EQ(opts.defaultPaperSize(), "a4");
    EXPECT_EQ(opts.defaultPaperOrientation(), "portrait");
    EXPECT_EQ(opts.defaultFont(), "dejavu serif");

    EXPECT_TRUE(opts.allowedRemoteHosts().empty());
}

TEST_F(CliOptionsTest, ParsesApplicationOptions)
{
    auto args = base_args();

    args.insert(args.begin() + 1, "--php-memory-limit=67108864");
    args.insert(args.begin() + 2, "--force-out");

    auto opts = parse(args);

    EXPECT_EQ(opts.php_memory_limit(), 67108864ULL);
    EXPECT_TRUE(opts.force_out());
}

TEST_F(CliOptionsTest, ParsesShortApplicationOptions)
{
    auto args = base_args();

    args.insert(args.begin() + 1, "-m");
    args.insert(args.begin() + 2, "12345678");
    args.insert(args.begin() + 3, "-f");

    auto opts = parse(args);

    EXPECT_EQ(opts.php_memory_limit(), 12345678ULL);
    EXPECT_TRUE(opts.force_out());
}

TEST_F(CliOptionsTest, ParsesBooleanDompdfOptions)
{
    auto args = base_args();

    args.insert(args.begin() + 1, "--isRemoteEnabled=1");
    args.insert(args.begin() + 2, "--isJavascriptEnabled=0");
    args.insert(args.begin() + 3, "--isFontSubsettingEnabled=0");
    args.insert(args.begin() + 4, "--sslAllowSelfSigned=1");

    auto opts = parse(args);

    EXPECT_TRUE(opts.isRemoteEnabled());
    EXPECT_FALSE(opts.isJavascriptEnabled());
    EXPECT_FALSE(opts.isFontSubsettingEnabled());
    EXPECT_TRUE(opts.sslAllowSelfSigned());
}

TEST_F(CliOptionsTest, ParsesStringDompdfOptions)
{
    auto args = base_args();

    args.insert(args.begin() + 1, "--dpi=144");
    args.insert(args.begin() + 2, "--fontHeightRatio=1.25");
    args.insert(args.begin() + 3, "--defaultMediaType=print");
    args.insert(args.begin() + 4, "--defaultPaperSize=letter");
    args.insert(args.begin() + 5, "--defaultPaperOrientation=landscape");
    args.insert(args.begin() + 6, "--defaultFont=DejaVu Sans");

    auto opts = parse(args);

    EXPECT_EQ(opts.dpi(), "144");
    EXPECT_EQ(opts.fontHeightRatio(), "1.25");
    EXPECT_EQ(opts.defaultMediaType(), "print");
    EXPECT_EQ(opts.defaultPaperSize(), "letter");
    EXPECT_EQ(opts.defaultPaperOrientation(), "landscape");
    EXPECT_EQ(opts.defaultFont(), "DejaVu Sans");
}

TEST_F(CliOptionsTest, ParsesMultipleAllowedRemoteHosts)
{
    auto args = base_args();

    args.insert(args.begin() + 1, "--allowedRemoteHosts=example.com");
    args.insert(args.begin() + 2, "--allowedRemoteHosts=cdn.example.com");
    args.insert(args.begin() + 3, "--allowedRemoteHosts=images.example.org");

    auto opts = parse(args);

    ASSERT_EQ(opts.allowedRemoteHosts().size(), 3u);

    EXPECT_EQ(opts.allowedRemoteHosts()[0], "example.com");
    EXPECT_EQ(opts.allowedRemoteHosts()[1], "cdn.example.com");
    EXPECT_EQ(opts.allowedRemoteHosts()[2], "images.example.org");
}

TEST_F(CliOptionsTest, PositionalArgumentsBecomeInputAndOutputFiles)
{
    auto second_input = root / "second.html";

    {
        std::ofstream file(second_input);
        ASSERT_TRUE(file.is_open());
        file << "<html><body>Second</body></html>";
    }

    auto args = {
        std::string("dompdfui"),
        input.string(),
        second_input.string(),
        output.string()
    };

    auto opts = parse(std::vector<std::string>(args));

    ASSERT_EQ(opts.in_files().size(), 2u);
    ASSERT_EQ(opts.out_files().size(), 2u);

    EXPECT_EQ(opts.in_files()[0], fs::canonical(input));
    EXPECT_EQ(opts.in_files()[1], fs::canonical(second_input));

    EXPECT_EQ(
        opts.out_files()[0],
        output / "input.pdf");

    EXPECT_EQ(
        opts.out_files()[1],
        output / "second.pdf");
}

TEST_F(CliOptionsTest, CreatesMissingOutputDirectory)
{
    const auto missing_output = root / "generated-output";

    ASSERT_FALSE(fs::exists(missing_output));

    auto args = std::vector<std::string>{
        "dompdfui",
        input.string(),
        missing_output.string()
    };

    EXPECT_THROW(
        parse(args),
        std::runtime_error);
}

TEST_F(CliOptionsTest, RejectsMissingInputFile)
{
    const auto missing = root / "missing.html";

    auto args = std::vector<std::string>{
        "dompdfui",
        missing.string(),
        output.string()
    };

    EXPECT_THROW(
        parse(args),
        std::runtime_error);
}

TEST_F(CliOptionsTest, RejectsMissingOutputArgument)
{
    auto args = std::vector<std::string>{
        "dompdfui",
        input.string()
    };

    EXPECT_THROW(
        parse(args),
        std::runtime_error);
}

TEST_F(CliOptionsTest, RejectsNoPositionalArguments)
{
    auto args = std::vector<std::string>{
        "dompdfui"
    };

    EXPECT_THROW(
        parse(args),
        std::runtime_error);
}

TEST_F(CliOptionsTest, RejectsUnknownOption)
{
    auto args = base_args();
    args.insert(args.begin() + 1, "--this-option-does-not-exist");

    EXPECT_THROW(
        parse(args),
        std::runtime_error);
}

TEST_F(CliOptionsTest, RejectsExistingOutputWithoutForce)
{
    const auto existing = output / "input.pdf";

    {
        std::ofstream file(existing);
        ASSERT_TRUE(file.is_open());
        file << "existing";
    }

    auto args = base_args();

    EXPECT_THROW(
        parse(args),
        std::runtime_error);
}

TEST_F(CliOptionsTest, AllowsExistingOutputWithForce)
{
    const auto existing = output / "input.pdf";

    {
        std::ofstream file(existing);
        ASSERT_TRUE(file.is_open());
        file << "existing";
    }

    auto args = base_args();
    args.insert(args.begin() + 1, "--force-out");

    EXPECT_NO_THROW({
        auto opts = parse(args);
        EXPECT_TRUE(opts.force_out());
    });
}

TEST_F(CliOptionsTest, AcceptsCombinedDompdfConfiguration)
{
    auto args = base_args();

    args.insert(args.begin() + 1, "--isRemoteEnabled=true");
    args.insert(args.begin() + 2, "--isJavascriptEnabled=false");
    args.insert(args.begin() + 3, "--isFontSubsettingEnabled=false");
    args.insert(args.begin() + 4, "--sslAllowSelfSigned=true");
    args.insert(args.begin() + 5, "--dpi=192");
    args.insert(args.begin() + 6, "--fontHeightRatio=1.3");
    args.insert(args.begin() + 7, "--defaultMediaType=print");
    args.insert(args.begin() + 8, "--defaultPaperSize=A3");
    args.insert(args.begin() + 9, "--defaultPaperOrientation=landscape");
    args.insert(args.begin() + 10, "--defaultFont=DejaVu Sans");
    args.insert(args.begin() + 11, "--allowedRemoteHosts=example.com");
    args.insert(args.begin() + 12, "--allowedRemoteHosts=cdn.example.com");

    auto opts = parse(args);

    EXPECT_TRUE(opts.isRemoteEnabled());
    EXPECT_FALSE(opts.isJavascriptEnabled());
    EXPECT_FALSE(opts.isFontSubsettingEnabled());
    EXPECT_TRUE(opts.sslAllowSelfSigned());

    EXPECT_EQ(opts.dpi(), "192");
    EXPECT_EQ(opts.fontHeightRatio(), "1.3");
    EXPECT_EQ(opts.defaultMediaType(), "print");
    EXPECT_EQ(opts.defaultPaperSize(), "A3");
    EXPECT_EQ(opts.defaultPaperOrientation(), "landscape");
    EXPECT_EQ(opts.defaultFont(), "DejaVu Sans");

    ASSERT_EQ(opts.allowedRemoteHosts().size(), 2u);
    EXPECT_EQ(opts.allowedRemoteHosts()[0], "example.com");
    EXPECT_EQ(opts.allowedRemoteHosts()[1], "cdn.example.com");
}

} // namespace

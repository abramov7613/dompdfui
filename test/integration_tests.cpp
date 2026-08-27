#include <gtest/gtest.h>
#include <reproc++/reproc.hpp>
#include <reproc++/drain.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <boost/nowide/fstream.hpp>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>


namespace fs = std::filesystem;
namespace nw = boost::nowide;


namespace {


class TempDirectory {
public:
    TempDirectory()
    {
        const auto stamp =
            std::chrono::steady_clock::now().time_since_epoch().count();

        path_ = fs::temp_directory_path() /
                ("dompdfui_integration_tests" + std::to_string(stamp));

        fs::create_directories(path_);
    }

    ~TempDirectory()
    {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }

    const fs::path& path() const
    {
        return path_;
    }

private:
    fs::path path_;
};


class DompdfUiIntegrationTest : public ::testing::Test {
protected:

    static void SetUpTestSuite()
    {
        ASSERT_FALSE(std::string(PDFINFO_EXECUTABLE).empty());
        ASSERT_FALSE(std::string(PDFTOTEXT_EXECUTABLE).empty());
        ASSERT_FALSE(std::string(DOMPDFUI_EXECUTABLE).empty());
        ASSERT_FALSE(std::string(DOMPDFUI_TEST_SOURCE_DIR).empty());
        ASSERT_FALSE(std::string(DOMPDFUI_TEST_OUTPUT_DIR).empty());
        executable_ = fs::path(DOMPDFUI_EXECUTABLE);
        ASSERT_TRUE(fs::exists(executable_))
            << "dompdfui executable does not exist: "
            << executable_.string();
        pdfoutdir_ = fs::path(DOMPDFUI_TEST_OUTPUT_DIR);
        if (!fs::exists(pdfoutdir_)) fs::create_directories(pdfoutdir_);
    }


    void SetUp() override
    {
        temp_ = std::make_unique<TempDirectory>();
    }


    std::string read_file(const fs::path& path)
    {
        nw::ifstream file(path, std::ios::binary);

        EXPECT_TRUE(file.is_open())
            << "Cannot open " << path;

        std::ostringstream contents;
        contents << file.rdbuf();

        return contents.str();
    }


    std::tuple<int, std::string, std::string>
        run_program(const std::string& program, const std::vector<std::string>& args)
    {// return tuple of exit_code, stdout_output, stderr_output
      std::vector<std::string> arguments;
      reproc::process process;
      std::string out, err;

      arguments.push_back(program);
      for (const auto& arg : args) arguments.push_back(arg);

      reproc::options options;
      options.redirect.out = {reproc::redirect::pipe};
      options.redirect.err = {reproc::redirect::pipe};

      auto ec = process.start(arguments, options);
      if (ec) {
          ADD_FAILURE()
              << "Failed to start process '"
              << arguments.front()
              << "': "
              << ec.message();
          return {-1, {}, {}};
      }

      ec = reproc::drain(
          process,
          reproc::sink::string(out),
          reproc::sink::string(err)
      );
      if (ec) {
          ADD_FAILURE()
              << "Failed to wait for process '"
              << arguments.front()
              << "': "
              << ec.message();
          return {-1, {}, {}};
      }

      auto [exit_code, stop_ec] = process.stop(options.stop);
      if (stop_ec) {
          ADD_FAILURE()
              << "Failed to stop for process '"
              << arguments.front()
              << "': "
              << ec.message();
          return {-1, {}, {}};
      }

      return {exit_code, std::move(out), std::move(err)};
    }


    std::optional<int> is_pdf(const fs::path& path) // if success return count of pdf pages
    {
      static const fs::path pdfinfo { PDFINFO_EXECUTABLE };
      int result = 0;
      const std::string data = read_file(path);
      const bool valid_magic = data.size() >= 8 &&
                              data.compare(0, 5, "%PDF-") == 0 &&
                              data.rfind("%%EOF") != std::string::npos;
      if (valid_magic) {
        auto [exit_code, stdout_output, stderr_output] = run_program(
          pdfinfo.string(),
          {fs::canonical(path).string()}
        );
        if (exit_code == 0) {
          if (stdout_output.find("PDF version") != std::string::npos) {
            if (auto x = stdout_output.find("Pages:"); x != std::string::npos) {
              stdout_output.erase(0, x+6);
              try { result = std::stoi(stdout_output); }
              catch(...) { return 0; }
            }
            return result;
          }
        }
      }
      return std::nullopt;
    }


    fs::path fixture(const std::string& name) const
    {
      return fs::path(DOMPDFUI_TEST_SOURCE_DIR) / "fixture" / name;
    }


    fs::path create_test_html(const std::string& filename, const std::string& content)
    {
      fs::path file_path = temp_->path() / filename;
      nw::ofstream file(file_path);
      EXPECT_TRUE(file.is_open())
          << "Cannot open " << file_path;
      file << content;
      file.close();
      return file_path;
    }

/*
    fs::path create_large_html(const std::string& filename, size_t size_kb)
    {
      fs::path file_path = temp_->path() / filename;
      nw::ofstream file(file_path);
      EXPECT_TRUE(file.is_open())
          << "Cannot open " << file_path;

      file << "<!DOCTYPE html><html><body>";
      file << "<h1>Large HTML Document</h1>";

      // Generate content to reach desired size
      std::string line = "<p>This is line number ";
      size_t current_size = 0;
      size_t line_num = 0;

      while (current_size < size_kb * 1024) {
          std::string s = line + std::to_string(line_num) + " with some content to fill space.</p>\n";
          file << s ;
          current_size += s.length();
          line_num++;
      }

      file << "</body></html>";
      file.close();
      return file_path;
    }
*/

    fs::path convert(const fs::path& html,
                     const bool isRemoteEnabled=false,
                     const bool sslAllowSelfSigned=false)
    {
      std::vector<std::string> arguments{
          "--force-out",
          html.string(),
          pdfoutdir_.string()
      };
      if (isRemoteEnabled) arguments.insert(arguments.begin(), "--isRemoteEnabled=1");
      if (sslAllowSelfSigned) arguments.insert(arguments.begin(), "--sslAllowSelfSigned=1");

      auto [result_, stdout_, stderr_] = run_program(executable_.string(), arguments);

      EXPECT_EQ(result_, 0)
          << "dompdfui failed with exit code "
          << result_
          << "\n"
          << stdout_
          << "\n"
          << stderr_ ;

      const fs::path pdf =
          pdfoutdir_ /
          html.filename().replace_extension(".pdf");

      EXPECT_TRUE(fs::exists(pdf))
          << "Expected PDF was not created: "
          << pdf.string();

      return pdf;
    }


    std::string extract_text(const fs::path& pdf)
    {
      static const fs::path pdftotext { PDFTOTEXT_EXECUTABLE };

      const fs::path text =
          temp_->path() /
          (pdf.stem().string() + ".txt");

      const std::vector<std::string> arguments{
          pdf.string(),
          text.string()
      };

      auto [result_, stdout_, stderr_] = run_program(pdftotext.string(), arguments);

      EXPECT_EQ(result_, 0)
          << "pdftotext failed for "
          << pdf.string()
          << '\n'
          << stdout_
          << '\n'
          << stderr_ ;

      if (result_ != 0 || !fs::exists(text)) {
          return {};
      }

      return read_file(text);
    }


    static fs::path executable_;
    static fs::path pdfoutdir_;
    std::unique_ptr<TempDirectory> temp_;
};
fs::path DompdfUiIntegrationTest::executable_;
fs::path DompdfUiIntegrationTest::pdfoutdir_;


TEST_F(DompdfUiIntegrationTest, ConvertsBasicHtmlToValidPdf)
{
    const auto html = fixture("simple.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));
}


TEST_F(DompdfUiIntegrationTest, PreservesExpectedText)
{
    const auto html = fixture("text.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("Hello integration"),
        std::string::npos);

    EXPECT_NE(
        text.find("The converter should produce"),
        std::string::npos);

    EXPECT_NE(
        text.find("< > & \" '"),
        std::string::npos);
}


TEST_F(DompdfUiIntegrationTest, ProducesMultiplePages)
{
    const auto html = fixture("multipage.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));

    const auto valid_pdf = is_pdf(pdf);

    ASSERT_TRUE(valid_pdf);
    EXPECT_GE(valid_pdf.value(), 2);
}


TEST_F(DompdfUiIntegrationTest, RendersTableContent1)
{
    const auto html = fixture("table.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("Name"),
        std::string::npos);

    EXPECT_NE(
        text.find("Age"),
        std::string::npos);

    EXPECT_NE(
        text.find("City"),
        std::string::npos);

    EXPECT_NE(
        text.find("John Doe"),
        std::string::npos);

    EXPECT_NE(
        text.find("Chicago"),
        std::string::npos);
}


TEST_F(DompdfUiIntegrationTest, RendersTableContent2)
{
    const auto html = fixture("tables.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("TABLES_INTEGRATION_TEST_MARKER"),
        std::string::npos);

    EXPECT_NE(
        text.find("TABLES_TEST_END_MARKER"),
        std::string::npos);
}


TEST_F(DompdfUiIntegrationTest, RendersUnicodeText)
{
    const auto html = fixture("unicode.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("ЮНИКОД_ТЕСТ_КИРИЛЛИЦА"),
        std::string::npos);

    EXPECT_NE(
        text.find("Příliš žluťoučký kůň"),
        std::string::npos);

    EXPECT_NE(
        text.find("你好世界"),
        std::string::npos);
}

/*
TEST_F(DompdfUiIntegrationTest, TenMegabyteFile)
{
    const auto html = create_large_html("10mb.html", 10 * 1024);
    const auto pdf = convert(html);
    size_t size = fs::file_size(pdf);

    ASSERT_TRUE(fs::exists(pdf));
    EXPECT_GT(size, 1024);
}
*/

TEST_F(DompdfUiIntegrationTest, InvalidHtmlSyntax)
{
    const auto html = create_test_html("invalid.html",
        "This is not valid HTML!!!"
        "<html>"
        "<body>"
        "Missing closing tags"
    );
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));
}


TEST_F(DompdfUiIntegrationTest, UnclosedTags)
{
    const auto html = create_test_html("unclosed.html",
        "<!DOCTYPE html>"
        "<html>"
        "<body>"
        "<div>"
        "<p>Content without closing tags"
    );
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));
}


TEST_F(DompdfUiIntegrationTest, MalformedAttributes)
{
    const auto html = create_test_html("malformed_attr.html",
        "<!DOCTYPE html>"
        "<html>"
        "<body>"
        "<div class=invalid title=test name=value>"
        "Content"
        "</div>"
        "</body>"
        "</html>"
    );
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));
}


TEST_F(DompdfUiIntegrationTest, BasicCSS)
{
    const auto html = fixture("css.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("CSS_HIGHLIGHT_MARKER"),
        std::string::npos);

    EXPECT_NE(
        text.find("CSS_BOX_MARKER"),
        std::string::npos);

    EXPECT_NE(
        text.find("CSS_IMPORTANT_MARKER"),
        std::string::npos);
}


TEST_F(DompdfUiIntegrationTest, ExternalCSS)
{
    const auto html = fixture("external_css.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("EXTERNAL_CSS_MARKER"),
        std::string::npos);

    EXPECT_NE(
        text.find("EXTERNAL_CSS_IMPORTANT_MARKER"),
        std::string::npos);
}


TEST_F(DompdfUiIntegrationTest, AttributeCSS)
{
    const auto html = fixture("atribut_css.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));
}


TEST_F(DompdfUiIntegrationTest, LocalFontfaceTTF)
{
    const auto html = fixture("font-face-ttf.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("FONT_FACE_TTF_MARKER"),
        std::string::npos);
    EXPECT_NE(
        text.find("LOCAL_TTF_FONT_MARKER"),
        std::string::npos);
    EXPECT_NE(
        text.find("FONT_FACE_TTF_BOLD_MARKER"),
        std::string::npos);
    EXPECT_NE(
        text.find("Кириллица"),
        std::string::npos);
}


TEST_F(DompdfUiIntegrationTest, LocalImage)
{
    const auto html = fixture("images.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("IMAGE_RELATIVE_PATH_TEST"),
        std::string::npos);
}


TEST_F(DompdfUiIntegrationTest, NestedLocalImage)
{
    const auto html = fixture("nested_dir_img.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("NESTED_RESOURCE_MARKER"),
        std::string::npos);
}


TEST_F(DompdfUiIntegrationTest, EmbeddedBase64JpegImage)
{
    const auto html = fixture("images-base64-jpg.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("BASE64_JPEG_IMAGE_MARKER"),
        std::string::npos);
}


TEST_F(DompdfUiIntegrationTest, EmbeddedBase64PngImage)
{
    const auto html = fixture("images-base64-png.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("BASE64_PNG_IMAGE_MARKER"),
        std::string::npos);
}


TEST_F(DompdfUiIntegrationTest, EmbeddedBase64SvgImage)
{
    const auto html = fixture("images-base64-svg.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("BASE64_IMAGE_MARKER"),
        std::string::npos);
}


TEST_F(DompdfUiIntegrationTest, Links)
{
    const auto html = fixture("links.html");
    const auto pdf = convert(html);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("INTERNAL_LINK_TARGET_MARKER"),
        std::string::npos);
}


TEST_F(DompdfUiIntegrationTest, RemoteResources)
{
    const auto html = fixture("remote_resources.html");
    const bool isRemoteEnabled = true;
    const bool sslAllowSelfSigned = true;
    const auto pdf = convert(html, isRemoteEnabled, sslAllowSelfSigned);

    ASSERT_TRUE(fs::exists(pdf));
    ASSERT_TRUE(is_pdf(pdf));

    const auto text = extract_text(pdf);

    EXPECT_NE(
        text.find("Tangerine"),
        std::string::npos);
}


} // namespace

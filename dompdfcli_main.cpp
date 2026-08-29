#include <string>
#include <vector>
#include <array>
#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <random>
#include <utility>
#include <boost/nowide/args.hpp>
#include <boost/nowide/cstdlib.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/predef.h>
#include <boost/version.hpp>
#include <reproc++/run.hpp>
#include "embed_resources.h"
#include "timestamp.h"

namespace po = boost::program_options;
namespace fs = std::filesystem ;
namespace nw = boost::nowide;
namespace hr = std::chrono;

class ApplicationOptions {
  unsigned long long php_memory_limit_;
  bool force_out_;
  bool isRemoteEnabled_;
  bool isJavascriptEnabled_;
  bool isFontSubsettingEnabled_;
  bool sslAllowSelfSigned_;
  std::string dpi_;
  std::string fontHeightRatio_;
  std::string defaultMediaType_;
  std::string defaultPaperSize_;
  std::string defaultPaperOrientation_;
  std::string defaultFont_;
  std::vector<std::string> allowedRemoteHosts_;
  std::vector<fs::path> in_files_;
  std::vector<fs::path> out_files_;
public:
  ApplicationOptions(const po::variables_map& );
  unsigned long long php_memory_limit() const { return php_memory_limit_; }
  bool force_out() const { return force_out_; }
  bool isRemoteEnabled() const { return isRemoteEnabled_; }
  bool isJavascriptEnabled() const { return isJavascriptEnabled_; }
  bool isFontSubsettingEnabled() const { return isFontSubsettingEnabled_; }
  bool sslAllowSelfSigned() const { return sslAllowSelfSigned_; }
  const std::string& dpi() const { return dpi_; }
  const std::string& fontHeightRatio() const { return fontHeightRatio_; }
  const std::string& defaultMediaType() const { return defaultMediaType_; }
  const std::string& defaultPaperSize() const { return defaultPaperSize_; }
  const std::string& defaultPaperOrientation() const { return defaultPaperOrientation_; }
  const std::string& defaultFont() const { return defaultFont_; }
  const std::vector<std::string>& allowedRemoteHosts() const { return allowedRemoteHosts_; }
  const std::vector<fs::path>& in_files() const { return in_files_; }
  const std::vector<fs::path>& out_files() const { return out_files_; }
};


class CommandLineHelp {};
void extract_embedded_resources() ;
ApplicationOptions parse_cli_args(int argc, char** argv);
std::pair<std::string, std::string> generate_php_script(const ApplicationOptions&);
void run_php_script(const std::string& name, const std::string& body, const ApplicationOptions& opts);

fs::path temp_path() ;


int main(int argc, char** argv)
try
{
  nw::args utf8_args (argc, argv);
  auto opts = parse_cli_args(argc, argv) ;
  extract_embedded_resources();
  auto[script_name, script_body] = generate_php_script(opts);
  run_php_script(script_name, script_body, opts);
  fs::remove_all(temp_path());
  return 0;
}
catch (const CommandLineHelp&)
{
  return 0;
}
catch (const std::exception& e)
{
  nw::cout << "Error: " << e.what() << '\n';
  return -1;
}
catch (...)
{
  nw::cout << "Error: Unknown exception\n" ;
  return -1;
}


ApplicationOptions parse_cli_args(int argc, char** argv)
{
  static const char* short_descr = "HTML to PDF Converter";
  po::variables_map vm;
  try {
    po::options_description hopts("Hidden options");
    hopts.add_options()
        ("iofiles", po::value<std::vector<std::string>>(), "input output files")
        ;
    po::options_description popts("Program Options");
    popts.add_options()
        ("php-memory-limit,m", po::value<unsigned long long>()->default_value(268435456), "Limits the amount of memory (in bytes) a php-cli can use.")
        ("version,v", "print version")
        ("help,h", "view this help message")
        ("force-out,f", po::bool_switch(), "replace output file if exists")
        ;
    po::options_description dopts("DomPdf Options");
    dopts.add_options()
        ("isRemoteEnabled", po::value<bool>()->default_value(false))
        ("isJavascriptEnabled", po::value<bool>()->default_value(true))
        ("isFontSubsettingEnabled", po::value<bool>()->default_value(true))
        ("sslAllowSelfSigned", po::value<bool>()->default_value(false))
        ("dpi", po::value<std::string>()->default_value("96"))
        ("fontHeightRatio", po::value<std::string>()->default_value("1.1"))
        ("defaultMediaType", po::value<std::string>()->default_value("screen"))
        ("defaultPaperSize", po::value<std::string>()->default_value("a4"))
        ("defaultPaperOrientation", po::value<std::string>()->default_value("portrait"))
        ("defaultFont", po::value<std::string>()->default_value("dejavu serif"))
        ("allowedRemoteHosts", po::value<std::vector<std::string>>())
        ;
    po::options_description allopts("");
    allopts.add(hopts).add(popts).add(dopts);
    po::positional_options_description p;
    p.add("iofiles", -1);
    po::store(po::command_line_parser(argc, argv).options(allopts).positional(p).run(), vm);
    po::notify(vm);
    if (vm.count("help")) {
        nw::cout <<  short_descr << ' ' << git_tag_str << "\nUsage:\t" << argv[0]
                            << " [OPTIONS] INPUT-FILE1 [INPUT-FILE2] [INPUT-FILE3] [...] OUTPUT-DIR \n"
                            << (po::options_description("").add(popts).add(dopts)) << '\n';
        throw CommandLineHelp{};
    }
    if (vm.count("version")) {
        nw::cout << short_descr << ' ' << git_tag_str << "\nbuilt with: GCC " << __VERSION__
                  << "; boost " << BOOST_VERSION / 100000 << '.' << BOOST_VERSION / 100 % 1000
                  << '.' << BOOST_VERSION % 100 << "; php-cli " << PHPCLI_VERSION
                  << "; dompdf " << DOMPDF_VERSION << "\nbuilt time: " << build_time_str << "\ngit hash: "
                  << git_hash_str << '\n' ;
        throw CommandLineHelp{};
    }
  }
  catch(const po::error& e) {
    throw std::runtime_error(std::string(e.what()) + "\nTry:\t" + std::string(argv[0]) + " --help");
  }
  return ApplicationOptions{vm};
}


std::pair<std::string, std::string> generate_php_script(const ApplicationOptions& opts)
{
  auto make_php_array = [](const std::vector<std::string>& v)->std::string{
    std::string r;
    if(v.empty()) return r;
    if(std::all_of(v.begin(), v.end(), [](const auto& e){ return e.empty(); })) return r;
    std::vector<std::string> vv;
    using Separator = boost::char_separator<char>;
    using Tokenizer = boost::tokenizer<Separator>;
    Separator sep(";,");
    for(const auto& e: v){
      Tokenizer tokens(e, sep);
      for (Tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter){
        vv.push_back(*tok_iter);
      }
    }
    r += '[';
    std::for_each(vv.begin(), vv.end(), [&r](const auto& e){ r += '"' + e + "\","; });
    r.pop_back();
    r += ']';
    return r;
  };
  auto now = hr::system_clock::now();
  auto now_c = hr::system_clock::to_time_t(now);
  auto milliseconds = hr::duration_cast<hr::milliseconds>(now.time_since_epoch()) % 1000;
  std::tm *local_time = std::localtime(&now_c);
  std::stringstream script_name, script_body;
  script_name << std::put_time(local_time, "html2pdf_%d%B%Y_%Hh%Mm%Ss") << std::setfill('0')
              << std::setw(3) << milliseconds.count() << "ms.php" ;
  script_body <<
    "<?php\n"
    "if ($argc<3) exit(-1);\n\n"
    "require_once 'dompdf/autoload.inc.php';\n\n"
    "use Dompdf\\Dompdf;\n"
    "use Dompdf\\Options;\n\n"
    "$options = new Options();\n"
    "$options->setIsPhpEnabled(FALSE);\n"
    "$options->setIsPdfAEnabled(FALSE);\n"
    "$options->setDebugPng(FALSE);\n"
    "$options->setDebugKeepTemp(FALSE);\n"
    "$options->setDebugCss(FALSE);\n"
    "$options->setDebugLayout(FALSE);\n"
    "$options->setDebugLayoutLines(TRUE);\n"
    "$options->setDebugLayoutBlocks(TRUE);\n"
    "$options->setDebugLayoutInline(TRUE);\n"
    "$options->setDebugLayoutPaddingBox(TRUE);\n"
    "$options->setIsRemoteEnabled("                 << opts.isRemoteEnabled() << ");\n"
    "$options->setIsJavascriptEnabled("             << opts.isJavascriptEnabled() << ");\n"
    "$options->setIsFontSubsettingEnabled("         << opts.isFontSubsettingEnabled() << ");\n"
    "$options->setDpi("                             << opts.dpi() << ");\n"
    "$options->setFontHeightRatio("                 << opts.fontHeightRatio() << ");\n"
    "$options->setDefaultMediaType('"               << opts.defaultMediaType() << "');\n"
    "$options->setDefaultPaperSize('"               << opts.defaultPaperSize() << "');\n"
    "$options->setDefaultPaperOrientation('"        << opts.defaultPaperOrientation() << "');\n"
    "$options->setDefaultFont('"                    << opts.defaultFont() << "');\n" ;
  if (!opts.isRemoteEnabled()) {
    std::string s;
    for (const auto& e: opts.in_files()) s += e.parent_path().string() + ',' ;
    script_body << "$options->setChroot('" << s << "');\n" ;
  }
  if (!opts.allowedRemoteHosts().empty())
    script_body << "$options->setAllowedRemoteHosts(" << make_php_array(opts.allowedRemoteHosts()) << ");\n" ;
  script_body << "\n$dompdf = new Dompdf($options);\n" ;
  if( opts.isRemoteEnabled() && opts.sslAllowSelfSigned() ) {
    script_body <<
      "$context = stream_context_create([\n"
      "  'ssl' => [\n"
      "    'verify_peer' => FALSE,\n"
      "    'verify_peer_name' => FALSE,\n"
      "    'allow_self_signed'=> TRUE\n"
      "  ]\n"
      "]);\n"
      "$dompdf->setHttpContext($context);\n" ;
  }
  script_body <<
    "$html_content = file_get_contents(\"$argv[1]\");\n"
    "$dompdf->loadHtml($html_content);\n"
    "$dompdf->render();\n"
    "$output = $dompdf->output();\n"
    "if(file_put_contents(\"$argv[2]\", $output) === FALSE){\n"
    "  echo \"Error: can't write to file: $argv[2]\" , PHP_EOL;\n"
    "  exit(-1);\n"
    "}\n" ;
  return {script_name.str(), script_body.str()};
}


void run_php_script(const std::string& name, const std::string& body, const ApplicationOptions& opts)
{
  auto script_path = temp_path() / name ;
  nw::ofstream script( script_path ) ;
  if(!script.is_open()) throw std::runtime_error("Can't open file: " + script_path.string()) ;
  script << body ;
  script.close();
  std::string memlimit = "memory_limit=" + std::to_string( opts.php_memory_limit() );
  const auto& in_files = opts.in_files();
  const auto& out_files = opts.out_files();
  for(size_t i=0; i<in_files.size(); ++i) {
    std::array cmd = {
      (temp_path() / "php.exe").string(),
      std::string{"-d"},
      memlimit,
      script_path.string(),
      in_files[i].string(),
      out_files[i].string(),
    };
    std::string working_directory = in_files[i].parent_path().string() ;
    reproc::options options;
    options.redirect.parent = true;
    options.working_directory = working_directory.data();
    auto[status, error_code] = reproc::run(cmd, options);
    if( status || error_code )
      throw std::runtime_error("Can't execute '" + script_path.filename().string() + "' with files:\n\t"
              + in_files[i].string() + "\n\t" + out_files[i].string() + "\n\t" + error_code.message() + '\n');
  }
}


fs::path temp_path()
{
  static auto randomInt = [](int min, int max) {
      if (min > max) std::swap(min, max);
      static std::random_device rd;
      static std::mt19937 gen(rd());
      return std::uniform_int_distribution<int>(min, max)(gen);
  };
  static std::string x;
  if (x.empty()) {
    for (int i{}; i<24; ++i) {
      switch(randomInt(1,5)) {
        case 1:
        case 2:
          x += static_cast<char>(randomInt(97,122)) ;
          break;
        case 3:
        case 4:
          x += static_cast<char>(randomInt(65,90)) ;
          break;
        default:
          x += static_cast<char>(randomInt(48,57)) ;
      }
    }
    x = "dompdfui_"  + x ;
  }
  return fs::temp_directory_path() / x ;
}


void extract_embedded_resources()
{
  auto php_rsc_p = embedded::resource<"php.exe">().data();
  auto php_rsc_sz = embedded::resource<"php.exe">().size();
  auto dompdf_rsc_p = embedded::resource<"dompdf.zip">().data();
  auto dompdf_rsc_sz = embedded::resource<"dompdf.zip">().size();
  auto extract = [](const char* p, size_t sz, const fs::path& path){
    nw::ofstream os ( path, std::ios::binary );
    if(!os.is_open()) throw std::runtime_error("Can't open file: " + path.string()) ;
    os.exceptions(std::ios_base::badbit);
    try { os.write(p, sz); }
    catch(std::exception&) { throw std::runtime_error("Can't write to file: " + path.string()) ; }
    os.close();
  };
  auto php_exe_target_path = temp_path() ;
  if(!fs::exists(php_exe_target_path)) fs::create_directory(php_exe_target_path);
  php_exe_target_path /= "php.exe" ;
  if(!fs::exists(php_exe_target_path)) {
    extract(php_rsc_p, php_rsc_sz, php_exe_target_path) ;
  #if BOOST_OS_UNIX
    fs::permissions(php_exe_target_path, fs::perms::owner_all | fs::perms::group_all, fs::perm_options::add);
  #endif
  }
  auto dompdf_target_path = temp_path() / "dompdf.zip" ;
  if(!fs::exists(dompdf_target_path)){
    extract(dompdf_rsc_p, dompdf_rsc_sz, dompdf_target_path) ;
    auto dompdf_dir = temp_path() / "dompdf" ;
    if(fs::exists(dompdf_dir) && !fs::is_directory(dompdf_dir)) fs::remove(dompdf_dir) ;
    if(!fs::exists(dompdf_dir)){
      std::string unzipscript = "$zip = new ZipArchive; "
                                "if ($zip->open('dompdf.zip') === TRUE) { "
                                "   $zip->extractTo('.'); "
                                "   $zip->close(); "
                                "} else { "
                                "   exit(-1); "
                                "}" ;
      std::array cmd = {
        (temp_path() / "php.exe").string(),
        std::string{"-r"},
        unzipscript,
      };
      std::string working_directory = temp_path().string() ;
      reproc::options options;
      options.redirect.parent = true;
      options.working_directory = working_directory.data();
      auto[status, error_code] = reproc::run(cmd, options);
      if( status || error_code ) throw std::runtime_error("Can't unzip 'dompdf.zip' file");
    }
  }
}


ApplicationOptions::ApplicationOptions(const po::variables_map& vm)
    :
    php_memory_limit_{268435456u},
    force_out_{false},
    isRemoteEnabled_{false},
    isJavascriptEnabled_{true},
    isFontSubsettingEnabled_{true},
    sslAllowSelfSigned_{false},
    dpi_{"96"},
    fontHeightRatio_{"1.1"},
    defaultMediaType_{"screen"},
    defaultPaperSize_{"a4"},
    defaultPaperOrientation_{"portrait"},
    defaultFont_{"dejavu serif"},
    allowedRemoteHosts_{},
    in_files_{},
    out_files_{}
{
  fs::path out_dir;
  if (!vm.count("iofiles") || vm["iofiles"].as<std::vector<std::string>>().size()<2) {
      throw std::runtime_error("the options 'INPUT-FILE1' and 'OUTPUT-DIR' is required but missing");
  }
  for(const auto& e: vm["iofiles"].as<std::vector<std::string>>()) {
    if(!fs::exists(e)) throw std::runtime_error('"' + e + "\" not found") ;
    in_files_.emplace_back(e);
  }
  std::transform(in_files_.begin(), in_files_.end(), in_files_.begin(), [](auto& e){
    return fs::canonical(e);
  });
  out_dir = in_files_.back();
  in_files_.pop_back();
  if(fs::is_regular_file(out_dir)) out_dir = out_dir.parent_path();
  if(!fs::exists(out_dir)) fs::create_directory(out_dir);
  if(!fs::is_directory(out_dir)) {
      throw std::runtime_error("can't open output directory " + out_dir.string() ) ;
  }
  std::transform(in_files_.begin(), in_files_.end(), std::back_inserter(out_files_), [&out_dir](const auto& e){
    return out_dir / e.filename().replace_extension("pdf");
  });
  if (vm.count("php-memory-limit")) php_memory_limit_ = vm["php-memory-limit"].as<unsigned long long>();
  if (vm.count("force-out")) force_out_ = vm["force-out"].as<bool>();
  if(!force_out_){
    auto already_exists = std::find_if(out_files_.begin(), out_files_.end(), [](const auto& e){
      return fs::exists(e);
    });
    if( already_exists!=out_files_.end() )
      throw std::runtime_error("file '" + already_exists->string() + "' already exists");
  }
  if (vm.count("isRemoteEnabled")) isRemoteEnabled_ = vm["isRemoteEnabled"].as<bool>() ;
  if (vm.count("isJavascriptEnabled")) isJavascriptEnabled_ = vm["isJavascriptEnabled"].as<bool>();
  if (vm.count("isFontSubsettingEnabled")) isFontSubsettingEnabled_ = vm["isFontSubsettingEnabled"].as<bool>();
  if (vm.count("sslAllowSelfSigned")) sslAllowSelfSigned_ = vm["sslAllowSelfSigned"].as<bool>();
  if (vm.count("dpi")) dpi_ = vm["dpi"].as<std::string>();
  if (vm.count("fontHeightRatio")) fontHeightRatio_ = vm["fontHeightRatio"].as<std::string>();
  if (vm.count("defaultMediaType")) defaultMediaType_ = vm["defaultMediaType"].as<std::string>();
  if (vm.count("defaultPaperSize")) defaultPaperSize_ = vm["defaultPaperSize"].as<std::string>();
  if (vm.count("defaultPaperOrientation")) defaultPaperOrientation_ = vm["defaultPaperOrientation"].as<std::string>();
  if (vm.count("defaultFont")) defaultFont_ = vm["defaultFont"].as<std::string>();
  if (vm.count("allowedRemoteHosts")) allowedRemoteHosts_ = vm["allowedRemoteHosts"].as<std::vector<std::string>>() ;
}

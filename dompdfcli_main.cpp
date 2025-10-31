#include <string>
#include <vector>
#include <tuple>
#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <boost/nowide/args.hpp>
#include <boost/nowide/cstdlib.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/predef.h>
#include <boost/version.hpp>
#include "embed_resources.h"
#include "timestamp.h"

namespace po = boost::program_options;
namespace fs = std::filesystem ;
namespace nw = boost::nowide;


std::tuple<int, std::vector<fs::path>, std::vector<fs::path>, po::variables_map> parse_cli_args(int argc, char** argv) ;
void extract_embedded_resources(const po::variables_map&) ;
void html2pdf(const std::vector<fs::path>&, const std::vector<fs::path>&, const po::variables_map&) ;
fs::path temp_path() ;


bool cleanup_on_exit {};
int return_code {};


int main(int argc, char** argv)
{
  try {
    if(!nw::system(nullptr))
      throw std::runtime_error("the command processor is not exists");
    nw::args utf8_args (argc, argv);
    auto [parse_result, in_files, out_files, opts] = parse_cli_args(argc, argv) ;
    if( parse_result!=1 ) {
      return_code = parse_result ;
    } else {
      extract_embedded_resources(opts);
      html2pdf(in_files, out_files, opts);
    }
  }
  catch (const std::exception& e) {
    nw::cout << "Error: " << e.what() << '\n';
    return_code = -1;
  }
  catch (...) {
    nw::cout << "Error: Unknown exception\n" ;
    return_code = -1;
  }
  if (cleanup_on_exit) fs::remove_all(temp_path());
  return return_code;
}


// function return 4 values:
//     first is a cli parser result: 1 = OK; 0 = Help; -1 = parser error
//     second  - array of input files
//     third   - array of output files
//     fourth  - boost::program_options::variables_map container with all parameters
std::tuple<int, std::vector<fs::path>, std::vector<fs::path>, po::variables_map> parse_cli_args(int argc, char** argv)
{
  static const char* short_descr = "HTML to PDF Converter";
  po::variables_map vm;
  std::vector<fs::path> in_files, out_files;
  fs::path out_dir;
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
        ("no-clean,n", po::bool_switch(), "don't clean temp files on exit; use when running multiple instances")
        ("keep-php-scripts,k", po::bool_switch(), "don't remove generated php scripts in temp directory; ignore if --no-clean is not set")
        ;

    po::options_description dopts("DomPdf Options");
    dopts.add_options()
        ("isPhpEnabled", po::value<bool>()->default_value(false))
        ("isRemoteEnabled", po::value<bool>()->default_value(false))
        ("isPdfAEnabled", po::value<bool>()->default_value(false))
        ("isJavascriptEnabled", po::value<bool>()->default_value(true))
        ("isHtml5ParserEnabled", po::value<bool>()->default_value(true))
        ("isFontSubsettingEnabled", po::value<bool>()->default_value(true))
        ("sslAllowSelfSigned", po::value<bool>()->default_value(true))
        ("debugPng", po::value<bool>()->default_value(false))
        ("debugKeepTemp", po::value<bool>()->default_value(false))
        ("debugCss", po::value<bool>()->default_value(false))
        ("debugLayout", po::value<bool>()->default_value(false))
        ("debugLayoutLines", po::value<bool>()->default_value(true))
        ("debugLayoutBlocks", po::value<bool>()->default_value(true))
        ("debugLayoutInline", po::value<bool>()->default_value(true))
        ("debugLayoutPaddingBox", po::value<bool>()->default_value(true))
        ("dpi", po::value<std::string>()->default_value("96"))
        ("fontHeightRatio", po::value<std::string>()->default_value("1.1"))
        ("rootDir", po::value<std::string>())
        ("tempDir", po::value<std::string>())
        ("fontDir", po::value<std::string>())
        ("fontCache", po::value<std::string>())
        ("logOutputFile", po::value<std::string>())
        ("defaultMediaType", po::value<std::string>()->default_value("screen"))
        ("defaultPaperSize", po::value<std::string>()->default_value("a4"))
        ("defaultPaperOrientation", po::value<std::string>()->default_value("portrait"))
        ("defaultFont", po::value<std::string>()->default_value("dejavu serif"))
        ("pdfBackend", po::value<std::string>()->default_value("CPDF"))
        ("pdflibLicense", po::value<std::string>())
        ("chroot", po::value<std::vector<std::string>>())
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
        return {0, {}, {}, {}};
    }

    if (vm.count("version")) {
        nw::cout << short_descr << ' ' << git_tag_str << "\nbuilt with: GCC " << __VERSION__
                  << "; boost " << BOOST_VERSION / 100000 << '.' << BOOST_VERSION / 100 % 1000
                  << '.' << BOOST_VERSION % 100 << "; php-cli " << PHPCLI_VERSION
                  << "; dompdf " << DOMPDF_VERSION << "\nbuilt time: " << build_time_str << "\ngit hash: "
                  << git_hash_str << '\n' ;
        return {0, {}, {}, {}};
    }

    if (!vm.count("iofiles") || vm["iofiles"].as<std::vector<std::string>>().size()<2) {
        nw::cout << "Error: the options 'INPUT-FILE1' and 'OUTPUT-DIR' is required but missing\n";
        return {-1, {}, {}, {}};
    }

    for(const auto& e: vm["iofiles"].as<std::vector<std::string>>()) in_files.emplace_back(e);
    std::transform(in_files.begin(), in_files.end(), in_files.begin(), [](auto& e){
      return fs::absolute(e);
    });
    out_dir = in_files.back();
    in_files.pop_back();
    if(fs::is_regular_file(out_dir)) out_dir = out_dir.parent_path();
    if(!fs::exists(out_dir)) fs::create_directory(out_dir);
    if(!fs::is_directory(out_dir)) {
        nw::cout << "Error: can't open output directory " << out_dir.string() << '\n' ;
        return {-1, {}, {}, {}};
    }
    std::erase_if(in_files, [](const auto& e){
      bool remove_element = !fs::exists(e);
      if(remove_element) nw::cout << "Warning: file '" << e.string() << "' not found\n";
      return remove_element;
    });
    if(in_files.empty()) return {-1, {}, {}, {}};

    std::transform(in_files.begin(), in_files.end(), std::back_inserter(out_files), [&out_dir](const auto& e){
      return out_dir / e.filename().replace_extension("pdf");
    });

    if(!vm["force-out"].as<bool>()){
      bool already_exists = std::any_of(out_files.begin(), out_files.end(), [](const auto& e){
        bool result = fs::exists(e);
        if(result) nw::cout << "Warning: file '" << e.string() << "' already exists\n";
        return result;
      });
      if( already_exists ) return {-1, {}, {}, {}};
    }

    cleanup_on_exit = !vm["no-clean"].as<bool>() ;
  }
  catch(const po::error& e) {
    nw::cout << "Error: " << e.what() << "\nTry:\t" << argv[0] << " --help\n";
    return {-1, {}, {}, {}};
  }
  return {1, in_files, out_files, vm};
}


// function generate php script from Options and run it
void html2pdf(const std::vector<fs::path>& in_files, const std::vector<fs::path>& out_files, const po::variables_map& opts)
{
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  std::tm *local_time = std::localtime(&now_c);
  std::stringstream ss;
  ss << std::put_time(local_time, "html2pdf_%d%B%Y_%Hh%Mm%Ss") << std::setfill('0')
     << std::setw(3) << milliseconds.count() << "ms.php" ;
  auto script_path = temp_path() / ss.str() ;
  nw::ofstream script( script_path ) ;
  if(!script.is_open()) throw std::runtime_error("Can't open file: " + script_path.string()) ;
  auto php_array = [](const std::vector<std::string>& v){
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
  script <<
    "<?php\n"
    "if ($argc<3) exit(-1);\n\n"
    "require_once 'dompdf/autoload.inc.php';\n\n"
    "use Dompdf\\Dompdf;\n"
    "use Dompdf\\Options;\n\n"
    "$options = new Options();\n"
    "$options->setIsPhpEnabled("                    << opts["isPhpEnabled"].as<bool>() << ");\n"
    "$options->setIsRemoteEnabled("                 << opts["isRemoteEnabled"].as<bool>() << ");\n"
    "$options->setIsPdfAEnabled("                   << opts["isPdfAEnabled"].as<bool>() << ");\n"
    "$options->setIsJavascriptEnabled("             << opts["isJavascriptEnabled"].as<bool>() << ");\n"
    "$options->setIsHtml5ParserEnabled("            << opts["isHtml5ParserEnabled"].as<bool>() << ");\n"
    "$options->setIsFontSubsettingEnabled("         << opts["isFontSubsettingEnabled"].as<bool>() << ");\n"
    "$options->setDebugPng("                        << opts["debugPng"].as<bool>() << ");\n"
    "$options->setDebugKeepTemp("                   << opts["debugKeepTemp"].as<bool>() << ");\n"
    "$options->setDebugCss("                        << opts["debugCss"].as<bool>() << ");\n"
    "$options->setDebugLayout("                     << opts["debugLayout"].as<bool>() << ");\n"
    "$options->setDebugLayoutLines("                << opts["debugLayoutLines"].as<bool>() << ");\n"
    "$options->setDebugLayoutBlocks("               << opts["debugLayoutBlocks"].as<bool>() << ");\n"
    "$options->setDebugLayoutInline("               << opts["debugLayoutInline"].as<bool>() << ");\n"
    "$options->setDebugLayoutPaddingBox("           << opts["debugLayoutPaddingBox"].as<bool>() << ");\n"
    "$options->setDpi("                             << opts["dpi"].as<std::string>() << ");\n"
    "$options->setFontHeightRatio("                 << opts["fontHeightRatio"].as<std::string>() << ");\n" ;

  if(opts.count("rootDir")) script <<
    "$options->setRootDir('"                        << opts["rootDir"].as<std::string>() << "');\n" ;

  if(opts.count("tempDir")) script <<
    "$options->setTempDir('"                        << opts["tempDir"].as<std::string>() << "');\n" ;

  if(opts.count("fontDir")) script <<
    "$options->setFontDir('"                        << opts["fontDir"].as<std::string>() << "');\n" ;

  if(opts.count("fontCache")) script <<
    "$options->setFontCache('"                      << opts["fontCache"].as<std::string>() << "');\n" ;

  if(opts.count("logOutputFile")) script <<
    "$options->setLogOutputFile('"                  << opts["logOutputFile"].as<std::string>() << "');\n" ;

  script <<
    "$options->setDefaultMediaType('"               << opts["defaultMediaType"].as<std::string>() << "');\n"
    "$options->setDefaultPaperSize('"               << opts["defaultPaperSize"].as<std::string>() << "');\n"
    "$options->setDefaultPaperOrientation('"        << opts["defaultPaperOrientation"].as<std::string>() << "');\n"
    "$options->setDefaultFont('"                    << opts["defaultFont"].as<std::string>() << "');\n"
    "$options->setPdfBackend('"                     << opts["pdfBackend"].as<std::string>() << "');\n" ;

  if(opts.count("pdflibLicense")) script <<
    "$options->setPdflibLicense('"                  << opts["pdflibLicense"].as<std::string>() << "');\n" ;

  if(opts.count("chroot")) script <<
    "$options->setChroot(" << php_array(opts["chroot"].as<std::vector<std::string>>()) << ");\n" ;

  if(opts.count("allowedRemoteHosts")) script <<
    "$options->setAllowedRemoteHosts(" << php_array(opts["allowedRemoteHosts"].as<std::vector<std::string>>()) <<
    ");\n" ;

  script << "\n$dompdf = new Dompdf($options);\n" ;

  if( opts["isRemoteEnabled"].as<bool>() && opts["sslAllowSelfSigned"].as<bool>() ) {
    script <<
      "$context = stream_context_create([\n"
      "  'ssl' => [\n"
      "    'verify_peer' => FALSE,\n"
      "    'verify_peer_name' => FALSE,\n"
      "    'allow_self_signed'=> TRUE\n"
      "  ]\n"
      "]);\n"
      "$dompdf->setHttpContext($context);\n" ;
  }

  script <<
    "$html_content = file_get_contents(\"$argv[1]\");\n"
    "$dompdf->loadHtml($html_content);\n"
    "$dompdf->render();\n"
    "$output = $dompdf->output();\n"
    "if(file_put_contents(\"$argv[2]\", $output) === FALSE){\n"
    "  echo \"Error: can't write to file: $argv[2]\" , PHP_EOL;\n"
    "  exit(-1);\n"
    "}\n" ;
  script.close();
  nw::cout.flush();

  for(size_t i=0; i<in_files.size(); ++i) {
    std::string ifile = in_files[i].string();
    std::string ofile = out_files[i].string();
    std::string memlimit = std::to_string( opts["php-memory-limit"].as<unsigned long long>() );
    std::string cmd = "php.exe -d memory_limit=" + memlimit + " \"" + script_path.filename().string()
                      + "\" \"" + ifile + "\" \"" + ofile + "\"" ;
#if BOOST_OS_WINDOWS
    cmd = temp_path().root_name().string() + " && cd \"" + temp_path().string() + "\" && " + cmd ;
#else
    cmd = "cd \"" + temp_path().string() + "\" && ./" + cmd ;
#endif
    if( nw::system(cmd.c_str()) )
      throw std::runtime_error("Can't execute '" + script_path.filename().string() + "' with files:\n\t"
                                + ifile + "\n\t" + ofile + '\n');
  }

  if( !cleanup_on_exit && !opts["keep-php-scripts"].as<bool>()  ) fs::remove(script_path);
}


// function return application specific temp path
fs::path temp_path()
{
  std::string dir_name = "dompdfui_" + std::string(git_hash_str);
  return fs::temp_directory_path() / dir_name ;
}


// function extract embedded resources
void extract_embedded_resources(const po::variables_map& opts)
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
      nw::cout.flush();
#if BOOST_OS_WINDOWS
      std::string unzipscript = "$zip = new ZipArchive; "
                                "if ($zip->open('dompdf.zip') === TRUE) { "
                                "   $zip->extractTo('.'); "
                                "   $zip->close(); "
                                "} else { "
                                "   exit(-1); "
                                "}" ;
      std::string cmd = temp_path().root_name().string() + " && cd \""
                        + temp_path().string() + "\" && php.exe -r \"" + unzipscript + "\"";
#else
      std::string unzipscript = "\\$zip = new ZipArchive; "
                                "if (\\$zip->open('dompdf.zip') === TRUE) { "
                                "   \\$zip->extractTo('.'); "
                                "   \\$zip->close(); "
                                "} else { "
                                "   exit(-1); "
                                "}" ;
      std::string cmd = "cd \"" + temp_path().string() + "\" && ./php.exe -r \"" + unzipscript + "\"";
#endif
      if( nw::system(cmd.c_str()) )
        throw std::runtime_error("Can't unzip 'dompdf.zip' file");
    }
  }
}

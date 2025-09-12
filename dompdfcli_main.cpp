#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <algorithm>
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

int parse_cli_args(int argc, char** argv, po::variables_map& opts) ;
void html2pdf(const po::variables_map& opts) ;
void extract_embedded_resources(const po::variables_map& opts) ;
fs::path temp_path() ;

int main(int argc, char** argv)
{
  try {
    if(!nw::system(nullptr))
      throw std::runtime_error("the command processor not exists");
    nw::args utf8_args (argc, argv);
    po::variables_map opts;
    auto parse_cli_args_result = parse_cli_args(argc, argv, opts) ;
    if( parse_cli_args_result!=1 ) return parse_cli_args_result ;
    extract_embedded_resources(opts);
    html2pdf(opts);
    if(!opts["no-clean"].as<bool>()) fs::remove_all(temp_path());
  }
  catch (const std::exception& e) {
    nw::cout << "Error: " << e.what() << '\n';
    return -1;
  }
  catch (...) {
    nw::cout << "Error: Unknown exception\n" ;
    return -1;
  }
  return 0;
}

// function parse_cli_args returns:
//      1  - OK
//      0  - Help
//     -1  - Parse error
int parse_cli_args(int argc, char** argv, po::variables_map& vm)
{
  static const char* short_descr = "HTML to PDF Converter";
  try {
    po::options_description hopts("Hidden options");
    hopts.add_options()
        ("input-file", po::value<std::string>(), "input file")
        ("output-file", po::value<std::string>(), "output file")
        ;

    po::options_description popts("Program Options");
    popts.add_options()
        ("no-clean,n", po::bool_switch(), "don't clean temp files on exit")
        ("php-memory-limit,m", po::value<unsigned long long>()->default_value(268435456), "Limits the amount of memory (in bytes) a php-cli can use.")
        ("help,h", "view this help message")
        ("version,v", "print version")
        ;

    po::options_description dopts("DomPdf Options");
    dopts.add_options()
        ("isPhpEnabled", po::value<bool>()->default_value(false))
        ("isRemoteEnabled", po::value<bool>()->default_value(false))
        ("isPdfAEnabled", po::value<bool>()->default_value(false))
        ("isJavascriptEnabled", po::value<bool>()->default_value(true))
        ("isHtml5ParserEnabled", po::value<bool>()->default_value(true))
        ("isFontSubsettingEnabled", po::value<bool>()->default_value(true))
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
    p.add("input-file", 1);
    p.add("output-file", 1);
    po::store(po::command_line_parser(argc, argv).options(allopts).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        nw::cout <<  short_descr << "\nUsage:\t" << argv[0]
                            << " [OPTIONS] INPUT-FILE OUTPUT-FILE \n"
                            << (po::options_description("").add(popts).add(dopts)) << '\n';
        return 0;
    }
    if (vm.count("version")) {
        nw::cout << short_descr << ' ' << git_tag_str << '\n'
                 << "build time: " << build_time_str << '\n'
                 << "git hash: " << git_hash_str << "\nusing:\n"
                 << "GCC v" << __VERSION__ << '\n'
                 << "boost v" << BOOST_VERSION / 100000 << '.'
                              << BOOST_VERSION / 100 % 1000 << '.'
                              << BOOST_VERSION % 100 << '\n'
                 << "php-cli v" << PHPCLI_VERSION << '\n'
                 << "dompdf v" << DOMPDF_VERSION << '\n' ;
        return 0;
    }
    if (!vm.count("input-file") || !vm.count("output-file")) {
        nw::cout << "Error: the options 'INPUT-FILE' and 'OUTPUT-FILE' is required but missing\n";
        return -1;
    }
    if( fs::path p { vm["input-file"].as<std::string>() }; !fs::exists(p) ){
        nw::cout << "Error: file '" << p.string() << "' not found\n" ;
        return -1;
    }
  }
  catch(const po::error& e) {
    nw::cout << "Error: " << e.what() << "\nTry:\t" << argv[0] << " --help\n";
    return -1;
  }
  return 1;
}

// function generate php script from Options and run it
void html2pdf(const po::variables_map& opts)
{
  auto script_path = temp_path() / "html2pdf.php" ;
  auto from_path = fs::path( opts["input-file"].as<std::string>() );
  auto tmpfrom_path = temp_path() / from_path.filename() ;
  auto to_path = fs::path( opts["output-file"].as<std::string>() );
  auto tmpto_path = temp_path() / to_path.filename() ;
  if(!fs::copy_file(from_path, tmpfrom_path, fs::copy_options::overwrite_existing))
    throw std::runtime_error("Can't write to file: " + tmpfrom_path.string());
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
    "require_once 'dompdf/autoload.inc.php';\n\n"
    "use Dompdf\\Dompdf;\n"
    "use Dompdf\\Options;\n\n"
    "$options = new Options();\n"
    "$options->setIsPhpEnabled((bool)"              << opts["isPhpEnabled"].as<bool>() << ");\n"
    "$options->setIsRemoteEnabled((bool)"           << opts["isRemoteEnabled"].as<bool>() << ");\n"
    "$options->setIsPdfAEnabled((bool)"             << opts["isPdfAEnabled"].as<bool>() << ");\n"
    "$options->setIsJavascriptEnabled((bool)"       << opts["isJavascriptEnabled"].as<bool>() << ");\n"
    "$options->setIsHtml5ParserEnabled((bool)"      << opts["isHtml5ParserEnabled"].as<bool>() << ");\n"
    "$options->setIsFontSubsettingEnabled((bool)"   << opts["isFontSubsettingEnabled"].as<bool>() << ");\n"
    "$options->setDebugPng((bool)"                  << opts["debugPng"].as<bool>() << ");\n"
    "$options->setDebugKeepTemp((bool)"             << opts["debugKeepTemp"].as<bool>() << ");\n"
    "$options->setDebugCss((bool)"                  << opts["debugCss"].as<bool>() << ");\n"
    "$options->setDebugLayout((bool)"               << opts["debugLayout"].as<bool>() << ");\n"
    "$options->setDebugLayoutLines((bool)"          << opts["debugLayoutLines"].as<bool>() << ");\n"
    "$options->setDebugLayoutBlocks((bool)"         << opts["debugLayoutBlocks"].as<bool>() << ");\n"
    "$options->setDebugLayoutInline((bool)"         << opts["debugLayoutInline"].as<bool>() << ");\n"
    "$options->setDebugLayoutPaddingBox((bool)"     << opts["debugLayoutPaddingBox"].as<bool>() << ");\n"
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

  script <<
    "\n$dompdf = new Dompdf($options);\n"
    "$html_content = file_get_contents(\"" << tmpfrom_path.filename().string() << "\");\n"
    "$dompdf->loadHtml($html_content);\n"
    "$dompdf->render();\n"
    "$output = $dompdf->output();\n"
    "file_put_contents(\"" << tmpto_path.filename().string() << "\", $output);\n" ;
  script.close();

#if BOOST_OS_WINDOWS
  std::string cmd = temp_path().root_name().string() + " && cd \"" + temp_path().string()
                    + "\" && php.exe -c . \"" + script_path.filename().string() + "\"";
#else
  std::string cmd = "cd \"" + temp_path().string() + "\" && php.exe -c . \""
                    + script_path.filename().string() + "\"";
#endif
  if( nw::system(cmd.c_str()) )
    throw std::runtime_error("Can't execute script file: " + script_path.string());

  if(!fs::copy_file(tmpto_path, to_path, fs::copy_options::overwrite_existing))
    throw std::runtime_error("Can't write to file: " + to_path.string());
}

// function return temp path
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
    auto php_ini_path = temp_path() / "php.ini";
    nw::ofstream phpini ( php_ini_path );
    if(!phpini.is_open()) throw std::runtime_error("Can't open file: " + php_ini_path.string()) ;
    phpini << "memory_limit=" << opts["php-memory-limit"].as<unsigned long long>() << '\n';
    phpini.close();
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
      auto path = temp_path() / "unzip.php" ;
      nw::ofstream unzipscript( path ) ;
      if(!unzipscript.is_open()) throw std::runtime_error("Can't open file: " + path.string()) ;
      unzipscript <<
        "<?php\n"
        "$zip = new ZipArchive;\n"
        "if ($zip->open('dompdf.zip') === TRUE) {\n"
        "   $zip->extractTo('.');\n"
        "   $zip->close();\n"
        "} else {\n"
        "   exit(-1);\n"
        "}\n" ;
      unzipscript.close();
#if BOOST_OS_WINDOWS
      std::string cmd = temp_path().root_name().string() + " && cd \""
                        + temp_path().string() + "\" && php.exe unzip.php";
#else
      std::string cmd = "cd \"" + temp_path().string() + "\" && php.exe unzip.php";
#endif
      if( nw::system(cmd.c_str()) )
        throw std::runtime_error("Can't unzip 'dompdf.zip' file");
    }
  }
}

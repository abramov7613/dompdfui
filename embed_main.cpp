#include <boost/nowide/args.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/program_options.hpp>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace fs = std::filesystem;
namespace po = boost::program_options;
namespace nw = boost::nowide;

std::string_view symbols { "0123456789_"
                           "qwertyuiopasdfghjklzxcvbnm"
                           "QWERTYUIOPASDFGHJKLZXCVBNM" };

int main(int argc, char** argv)
{
  try {
    nw::args utf8_args (argc, argv);
    po::options_description opts("This program generate cpp-source files from set of "
                                 "binary files, for embedding it to executable. \n\nOptions");
    opts.add_options()
        ("help,h", "produce help message")
        ("output-dir,o", po::value<std::string>(), "output directory full path")
    ;
    po::options_description hidden_opts("");
    std::vector<std::string> infiles_;
    hidden_opts.add_options()("input-file", po::value<std::vector<std::string>>(&infiles_));
    po::options_description all_opts("");
    all_opts.add(opts).add(hidden_opts);
    po::positional_options_description p;
    p.add("input-file", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(all_opts).positional(p).run(), vm);
    po::notify(vm);
    if (vm.count("help")) {
      nw::cout << "Usage: " << argv[0]
               << " -o OUTDIR INFILE1 [INFILE2] [INFILE3] [...]\n"
               << opts << "\n";
      return 0;
    }
    if (!vm.count("input-file") || !vm.count("output-dir"))
      throw std::runtime_error("the options 'OUTDIR' and 'INFILE1' is required but missing");
    fs::path outdir (vm["output-dir"].as<std::string>());
    if(!fs::exists(outdir) || !fs::is_directory(outdir))
      throw std::runtime_error("Can't open directory: " + outdir.string());
    nw::ofstream embed_resources_h ( outdir / "embed_resources.h" );
    if(!embed_resources_h.is_open())
      throw std::runtime_error("Can't open file: embed_resources.h");
    nw::ofstream embed_resources_cpp ( outdir / "embed_resources.cpp" );
    if(!embed_resources_cpp.is_open())
      throw std::runtime_error("Can't open file: embed_resources.cpp");
    auto prepare_name = [](std::string_view name){
      std::string r {name};
      if(r.front()>47 && r.front()<58) r = '_' + r;
      for(;;){
        auto pos = r.find_first_not_of(symbols);
        if(pos==r.npos) break;
        r.replace(pos, 1, "_");
      }
      return r;
    };
    std::vector<fs::path> infiles;
    for(const auto& e: infiles_) infiles.push_back(fs::path(e));
    std::stable_sort(infiles.begin(), infiles.end(), [](const auto& e1, const auto& e2){
      return e1.filename().string() < e2.filename().string() ;
    });
    auto duplicate = std::adjacent_find(infiles.begin(), infiles.end(), [](const auto& e1, const auto& e2){
      return e1.filename().string() == e2.filename().string() ;
    });
    if(duplicate != infiles.end()){
      nw::cout << "Found files with identical names. Only the first one will be processed\n";
      std::unique(infiles.begin(), infiles.end(), [](const auto& e1, const auto& e2){
        return e1.filename().string() == e2.filename().string() ;
      });
    }
    embed_resources_h <<
      "#include <string>\n"
      "#include <algorithm>\n"
      "\n"
      "namespace embedded {\n"
      "\n"
      "  template<size_t N> struct compile_time_str {\n"
      "    constexpr compile_time_str(const char (&str)[N]) {\n"
      "        std::copy_n(str, N, value);\n"
      "    }\n"
      "    constexpr bool operator==(std::string_view other) const {\n"
      "        return other.compare(value) == 0;\n"
      "    }\n"
      "    [[nodiscard]] constexpr bool _false() const {\n"
      "        return false;\n"
      "    }\n"
      "    char value[N];\n"
      "  }; // struct compile_time_str\n"
      "\n"
      "  struct EmbeddedCollection {\n"
      "    struct EmbeddedResource {\n"
      "      constexpr EmbeddedResource(std::string_view data) : data_(data) {}\n"
      "      constexpr auto data() { return data_.data(); }\n"
      "      constexpr auto size() { return data_.size(); }\n"
      "    private:\n"
      "      std::string_view data_;\n"
      "    }; // struct EmbeddedResource\n" ;
    for(const auto& f: infiles){
      embed_resources_h << "    static EmbeddedResource resource_"
                        << prepare_name(f.filename().string()) << ";\n";
    }
    embed_resources_h <<
      "  }; // struct EmbeddedCollection\n"
      "\n"
      "  template<compile_time_str filename> constexpr EmbeddedCollection::EmbeddedResource resource()\n"
      "  {\n";
    for(size_t i=0; i<infiles.size(); ++i){
      embed_resources_h << "    ";
      if(i) embed_resources_h << "else ";
      const auto& f = infiles[i];
      embed_resources_h << "if constexpr(filename==\"" << f.filename().string()
                        << "\") return EmbeddedCollection::resource_"
                        << prepare_name(f.filename().string()) << " ;\n";
    }
    embed_resources_h <<
      "    else static_assert(filename._false(), \"Embedded resource filename not found\");\n"
      "    return {{}};\n"
      "  }\n"
      "\n"
      "} // namespace embedded\n\n";
    embed_resources_h.close();
    embed_resources_cpp <<
      "#include \"embed_resources.h\"\n"
      "\n"
      "namespace embedded {\n\n" ;
    for(const auto& f: infiles){
      embed_resources_cpp << "  EmbeddedCollection::EmbeddedResource EmbeddedCollection::resource_"
                          << prepare_name(f.filename().string()) << " = {\n    std::string_view(\n";
      nw::ifstream inf(f, std::ios::binary);
      if(!inf.is_open()) throw std::runtime_error("Can't open file: "+f.string());
      auto fsize = fs::file_size(f);
      std::vector<unsigned char> data(fsize);
      inf.read(reinterpret_cast<char*>(data.data()), fsize);
      inf.close();
      embed_resources_cpp << std::setfill('0') << std::setw(3) << std::oct ;
      unsigned ccount = 0;
      for(auto c: data){
        if(!ccount) embed_resources_cpp << "      \"";
        embed_resources_cpp << '\\' << (unsigned)c ;
        if(++ccount > 20) {
          ccount = 0;
          embed_resources_cpp << "\"\n" ;
        }
      }
      if(ccount) embed_resources_cpp << "\"\n" ;
      embed_resources_cpp << "      , " << std::dec << fsize << "\n    )\n  };\n\n";
    }
    embed_resources_cpp << "} // namespace embedded\n";
    embed_resources_cpp.close();
  }
  catch(const std::exception& e) {
    nw::cout << "Error: " << e.what() << '\n';
    return -1;
  }
  catch(...) {
    nw::cout << "Unknown exception\n";
    return -1;
  }
  return 0;
}

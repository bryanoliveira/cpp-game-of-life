#include <boost/program_options.hpp>

#include "config.hpp"

namespace po = boost::program_options;

namespace config {

std::string programName = "Automata";
unsigned long maxIterations = 0;
bool cpuOnly = false;
GLint width = 600;
GLint height = 600;
unsigned int renderDelayMs = 200;
// 12000 x 12000 uses up to 2GB RAM and 8.5GB VRAM
unsigned int rows = 100;
unsigned int cols = 100;
float fillProb = 0.08;
float virtualFillProb = 0; //.0001;

void load_file() {}
void load_cmd(int argc, char **argv) {
    po::options_description description("Usage");

    description.add_options()("help,h", "Display this help message") //
        ("width,w", po::value<GLint>(), "Window width")              //
        ("height,h", po::value<GLint>(), "Window height")            //
        ("rows,r", po::value<unsigned int>(), "Grid rows")           //
        ("cols,c", po::value<unsigned int>(), "Grid cols")           //
        ("render-delay,rd", po::value<unsigned int>(),
         "Render delay between frames (in milliseconds)") //
        ("fill-probability,fp", po::value<float>(),
         "Cell probability to start alive")                     //
        ("max,m", po::value<unsigned long>(), "Max iterations") //
        ("cpu", "CPU-only mode");                               //

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(description).run(),
              vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << description << std::endl;
        exit(0);
    }
    if (vm.count("width"))
        width = vm["width"].as<GLint>();
    if (vm.count("height"))
        height = vm["height"].as<GLint>();
    if (vm.count("rows"))
        rows = vm["rows"].as<unsigned int>();
    if (vm.count("cols"))
        cols = vm["cols"].as<unsigned int>();
    if (vm.count("render-delay"))
        renderDelayMs = vm["render-delay"].as<unsigned int>();
    if (vm.count("fill-probability"))
        fillProb = vm["fill-probability"].as<float>();
    if (vm.count("max"))
        maxIterations = vm["max"].as<unsigned long>();
    if (vm.count("cpu"))
        cpuOnly = true;
}

} // namespace config

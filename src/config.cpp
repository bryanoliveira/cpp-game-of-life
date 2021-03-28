#include "config.hpp"

namespace config {

std::string program_name = "Automata";
unsigned long max_iterations = 0;
GLint width = 640;
GLint height = 480;
GLfloat top = 0.0;
GLfloat bottom = 1.0;
GLfloat left = 0.0;
GLfloat right = 1.0;
unsigned int render_delay_ms = 0;
unsigned int rows = 1000;
unsigned int cols = 1000;
float fill_prob = 0.08;
float virtual_fill_prob = 0.0001;

}
// Let's make a scrolling pager for real this time.
// Made by anson.

// STL libraries
#include <iostream>
#include <fstream>
#include <format>
#include <vector>
#include <regex>

#include <cstring>
#include <cerrno>

// POSIX libraries
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////

#define TAB_DEF 	4
#define MARGIN_DEF	1
#define SPACE_DEF	2
#define NLCHARS		"\r\n"

// These variables below are designed to be user modified,
// and modified at run time, if need be. The preprocesors
// above are simply their respective default values.
static int TAB_COUNT = TAB_DEF;
static int NUM_MARGIN = MARGIN_DEF;
static int LINE_SPACE = SPACE_DEF;

static std::string NEWLINE(NLCHARS);

////////////////////////////////////////////////////////
// Utility functions
////////////////////////////////////////////////////////

using namespace std::literals::string_literals;
// A function for creating regex objects out of a string literal.
static inline std::regex operator""_r(const char *in, std::size_t len)	{ return std::regex(in, len);}
// Just something to have for familiarity sake, since std::format
// takes "{}" instead of printf format specifiers.
static inline std::string operator ""_p(const char *in, std::size_t len)
{
        return std::regex_replace(std::string(in, len), "%[a-z]"_r, "{}");
}
// String arithmetic functions for lazier operations, though at the cost of some readability (maybe?)
static inline std::string operator+(std::string &in, std::string &what) { return in.append(what); }
static inline std::string operator*(const std::string &in, int how)
{
	std::string ret;
	while(how--) { ret += in; }
	return ret;
}

// We're making our own printf() functions, so let's put them in a seperate namespace
// so that the linker doesn't get confused.
namespace
{
        template<typename... Args>
        inline std::string sprintf(const std::string_view &format, Args&&... args)
        {
                return std::vformat(format, std::make_format_args(args...));
        }

        template<typename... Args>
        inline void fprintf(std::ostream &out, const std::string_view &format, Args&&... args)
        {
                out << std::vformat(format, std::make_format_args(args...));
        }
};

// (maybe) temporary stream insertion overloader, for displaying contents of a vector.
template<typename T>
std::ostream &operator<<(std::ostream &stream, const std::vector<T> &what)
{
	for(const auto &index: what) { stream << index << std::endl; }
	return stream;
}

// I'd rather you not call this function directly, instead
// I would rather you call the macro below
void _error(const char *file, int line, const char *what)
{
        write(STDOUT_FILENO, "\033[2J", 4);
        write(STDOUT_FILENO, "\033[H", 3);

        fprintf(std::cerr, "(%d:%d) %s: %s\n"_p, file, line, what, strerror(errno));
        exit(EXIT_FAILURE);
}

#define error(x)        _error(__FILE__, __LINE__, x)

////////////////////////////////////////////////////////
// Pager class
////////////////////////////////////////////////////////

static termios cooked;
class terminal
{
	private:
		struct termios raw;
		bool rawMode = false;

		std::string scrBuf;

		void setRaw(void);
		void revert(void);

	public:
		 terminal();
		~terminal();

		void print(void);
};

void terminal::revert(void) { if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked) == -1) error("tcsetattr"); return; }
void terminal::setRaw(void)
{
	if(this->rawMode == true) return;
        if(tcgetattr(STDIN_FILENO, &cooked) == -1) error("tcgetattr");
        this->raw = cooked;

        // Disable output processing
        this->raw.c_oflag &= ~(OPOST);
        // We also ensure that each character being sent is 8 bits.
        this->raw.c_cflag |=  (CS8);
        this->raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        // So that read() doesn't halt the program to wait for input.
        this->raw.c_cc[VMIN] = 0;
        this->raw.c_cc[VTIME] = 1;

        if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &this->raw) == -1) error("tcsetattr");
        this->rawMode = true;
	return;
}

terminal::terminal() { this->setRaw(); }
terminal::~terminal(){ this->revert(); }

class pager: public terminal
{
	private:
		std::string_view fileName;
		std::ifstream fObj;

		std::vector<std::string> lines;
		int linecount = 0;

	public:
		pager();
		~pager();

		bool slurp();
		bool slurp(const std::string_view &filename);
		bool refresh(void);
};

pager::pager() : fileName("(null)") 			{}


////////////////////////////////////////////////////////
// main()
////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
	std::ifstream fObj(argv[1]);
	if(!fObj) return -1;

	int linecount = 0;
	std::vector<std::string> lines;
	// Iterating through file, line by line...
	for(std::string index; std::getline(fObj, index); linecount++)
	{
		// Replacing a tab character with spaces, for easier displaying on a
		// terminal set with no output processing
		index = std::regex_replace(index, "\t"_r, std::string(TAB_COUNT, ' '));
		// Line spacing can be user determined, so we need to make sure that the
		// user gives a valid value. If any funny business is detected, just
		// keep the line as is.
		index = (LINE_SPACE > 1) ? index + (NEWLINE * LINE_SPACE) : index;
		// The pager can also choose to display line numbers, which adds a bit
		// of complexity; since a tab character pushes text into the next "field"
                // characters can still be inserted into the first field, until
                // they are overflown into the next field to preserve the spaces
                // that the tabs make.
		// Since output processing is disabled, then we need to do
                // something similar on our own.
		std::string linenum(std::to_string(linecount));
		if((std::size_t)TAB_COUNT < linenum.length())
		{
			// If the user has set a low tab count, then we need to make
			// sure that the numbers don't intrude on the data being displayed.
			TAB_COUNT += TAB_COUNT;
			NUM_MARGIN++;
		}
		// And make sure to deduct the width of the line number from a tab section.
		std::string spacer((TAB_COUNT - linenum.length()) + NUM_MARGIN, ' ');
		// Lines are in the vector now.
		lines.push_back(std::to_string(linecount) + spacer + index);
	}

	std::cout << lines << std::endl;
	return 0;
}

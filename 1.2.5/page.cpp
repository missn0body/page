// Let's make a scrolling pager for real this time.
// Made by anson.

// STL libraries
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>

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
// String arithmetic functions for lazier operations, though at the cost of some readability (maybe?)
static inline std::string operator+(std::string &in, std::string &what) { return in.append(what); }
static inline std::string operator*(const std::string &in, int how)
{
	std::string ret;
	while(how--) { ret += in; }
	return ret;
}

// (maybe) temporary stream insertion overloader, for displaying contents of a vector.
template<typename T>
std::ostream &operator<<(std::ostream &stream, const std::vector<T> &what)
{
	for(const auto &index: what) { stream << index << std::endl; }
	return stream;
}

////////////////////////////////////////////////////////
// Pager class
////////////////////////////////////////////////////////

class terminal
{
	private:
		struct termios *cooked, raw;
		bool isRaw = false;

		std::string scrBuf;

		bool setRaw(void);
		bool revert(void);
	public:
		 terminal();
		~terminal();

		std::string data(void) const { return this->scrBuf; }
		void print(void);
};

class pager: public terminal
{
	private:
		std::string_view fileName;
		std::ifstream fObj;

		std::vector<std::string> lines;
		int linecount = 0;

	public:
		pager();
		pager(const std::string_view &in) : fileName(in) {};
		~pager();

		bool slurp(const std::string_view &filename);
		bool refresh(void);
};

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

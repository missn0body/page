// Let's make a scrolling pager!
// Written by anson

// STL libraries
#include <iostream>
#include <format>
#include <regex>
#include <vector>
#include <utility>

// C libraries
#include <cstdlib>
#include <cerrno> // for errno
#include <cstring> // for strerror()

// POSIX libraries
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

//////////////////////////////////////
// Definitions
//////////////////////////////////////

const char *VERSION = "1.2.0";

//////////////////////////////////////
// Utilites
//////////////////////////////////////

#define CTRL_KEY(x)	((x) & 0x1f)

static inline std::regex operator  ""_r(const char *in, std::size_t len) { return std::regex(in, len); }
static inline std::string operator ""_p(const char *in, std::size_t len)
{
	return std::regex_replace(std::string(in, len), "%[a-z]"_r, "{}");
}

// We're making our own printf() functions, so let's put them in a seperate namespace
// so that the linker doesn't get confused.
namespace
{
	template<typename... Args>
	inline void printf(const std::string_view &format, Args&&... args)
	{
		std::cout << std::vformat(format, std::make_format_args(args...));
	}

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

// I'd rather you not call this function directly, instead
// I would rather you call the macro below
void _error(const char *file, int line, const char *what)
{
	write(STDOUT_FILENO, "\033[2J", 4);
        write(STDOUT_FILENO, "\033[H", 3);

	fprintf(std::cerr, "(%d:%d) %s: %s\n"_p, file, line, what, strerror(errno));
	exit(EXIT_FAILURE);
}

#define error(x)	_error(__FILE__, __LINE__, x)

//////////////////////////////////////
// Terminal
//////////////////////////////////////

class pg
{
	private:
		std::pair<int, int> dim;
		std::pair<int, int> cursor;
		std::pair<int, int> offset;

		std::string filename;

		struct termios original, raw;
		std::string scrBuf;

		bool rawMode = false;

		bool getTermSize(int &row, int &col);
		bool bfTermSize(int &row, int &col);
	public:
		pg();
		~pg();

		int getRows() { return this->dim.first; }
		int getCols() { return this->dim.second; }

		void setRaw();
		void revert();

		void draw();
		void refresh();
};

pg::pg() : dim(0, 0), cursor(0, 0), offset(0, 0), filename("(null)")
{
	setRaw();
	getTermSize(this->dim.first, this->dim.second);
}

pg::~pg() { revert(); }

void pg::revert()
{
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &this->original) == -1) error("tcsetattr");
};

// As antirez says: "1960s magic shit" below.
void pg::setRaw()
{
	if(this->rawMode == true) return;
	if(tcgetattr(STDIN_FILENO, &this->original) == -1) error("tcgetattr");
	this->raw = this->original;

	// Disable most of all signal processing, parity checking,
	// output processing, terminal echo, canonical mode,
	// and most of what the CTRL key does.
	this->raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	this->raw.c_oflag &= ~(OPOST);
	// We also ensure that each character being sent is 8 bits.
	this->raw.c_cflag |=  (CS8);
	this->raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	// So that read() doesn't halt the program to wait for input.
	this->raw.c_cc[VMIN] = 0;
	this->raw.c_cc[VTIME] = 1;

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &this->raw) == -1) error("tcsetattr");
	this->rawMode = true;
};

bool pg::bfTermSize(int &row, int &col)
{
	std::string buf;
	buf.reserve((std::size_t)32);

	if(write(STDOUT_FILENO, "\033[6N", 4) == -1) return false;

	for(char &index : buf)
	{
		if(read(STDIN_FILENO, &index, 1) != -1) break;
		if(index == 'R') break;
	}

	if(buf.at(0) != '\033' || buf.at(1) != '[') return false;
	if(buf.find(";") != std::string::npos) return false;

	row = std::stoi(buf.substr(0, buf.find(";")));
	col = std::stoi(buf.substr(buf.find(";") + 1));

	return true;
}

// The easy way to get terminal size: just ask for it.
bool pg::getTermSize(int &row, int &col)
{
	struct winsize term;
	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &term) == -1 || term.ws_col == 0)
	{
		if(write(STDOUT_FILENO, "\033[999C\033[999B", 12) != 12) return false;
		return bfTermSize(row, col);
	}
	else
	{
		this->dim = std::make_pair(term.ws_col, term.ws_row);
		return true;
	}
}

void pg::draw()
{
	for(int y = 0; y < this->dim.first; y++)
	{
		write(STDOUT_FILENO, "`", 1);
		if(y < (this->dim.first - 1))
		{
			write(STDOUT_FILENO, "\r\n", 2);
		}
	}
}

void pg::refresh()
{
	write(STDOUT_FILENO, "\033[2J", 4);
	write(STDOUT_FILENO, "\033[H", 3);

	this->draw();

	write(STDOUT_FILENO, "\033[H", 3);
}

char readch()
{
	int result;
	char returnType;

	while((result = read(STDIN_FILENO, &returnType, 1)) == -1)
	{
		if(result == -1 && errno != EAGAIN) error("read");
	}
	return returnType;
}

// Returns a request for an exit, true for exit request and
// false for otherwise. The reason we're doing this is so
// that the deconstructor for 'pg' has the chance to actually
// run, rather than exiting here
bool parseKeys()
{
	char in = readch();
	switch(in)
	{
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\033[2J", 4);
        		write(STDOUT_FILENO, "\033[H", 3);
			return false;
	}

	return true;
}

//////////////////////////////////////
// main()
//////////////////////////////////////

int main()
{
	pg mainScreen;

	while(1 > 0)
	{
		mainScreen.refresh();
		if(parseKeys() == false) break;
	}

	return 0;
}

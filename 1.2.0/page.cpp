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

enum {  ARROW_UP = 1000,
	ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT,
	HOME_KEY, END_KEY, PAGE_UP, PAGE_DOWN };

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
		std::string row;
		int numRows;

		bool rawMode = false;

		static int readch(void);

		bool getTermSize(int &row, int &col);
		static bool bfTermSize(int &row, int &col);
	public:
		pg();
		~pg();

		int getRows() const { return this->dim.first; }
		int getCols() const { return this->dim.second; }

		void setRaw(void);
		void pgOpen(void);
		void revert(void);

		void draw(void);
		void refresh(void);

		void moveCursor(int key);
		bool parseKeys(void);
};

pg::pg() : dim(0, 0), cursor(0, 0), offset(0, 0), filename("(null)"), numRows(0);
{
	this->setRaw();
	this->pgOpen();

	if(getTermSize(this->dim.first, this->dim.second) == false) error("getTermSize");
}

pg::~pg() { revert(); }

void pg::revert(void)
{
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &this->original) == -1) error("tcsetattr");
};

// As antirez says: "1960s magic shit" below.
void pg::setRaw(void)
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

void pg::pgOpen(void)
{
	return;
}

// ...or brute force it using escape codes.
bool pg::bfTermSize(int &row, int &col)
{
	std::string buf(32, '0');
	if(write(STDOUT_FILENO, "\033[6N", 4) == -1) return false;

	for(unsigned i = 0; i < 31; i++)
	{
		if(read(STDIN_FILENO, &buf.at(i), 1) != -1) break;
		if(buf.at(i) == 'R') break;
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
		// If the system call doesn't work, just move the cursor as far
		// as we can put it, and call our secondary function...
		if(write(STDOUT_FILENO, "\033[999C\033[999B", 12) != 12) return false;
		return bfTermSize(row, col);
	}
	else
	{
		this->dim = std::make_pair(term.ws_row, term.ws_col);
		return true;
	}
}

// Draws the entire screen.
void pg::draw(void)
{
	for(int y = 0; y < this->getRows(); y++)
	{
		if(y == (this->getRows() / 3))
		{
			std::string message = sprintf("welcome to page. (version %s)"_p, VERSION);

			int padding = (this->getCols() - message.length()) / 2;
			if(padding) { while(padding--) this->scrBuf.append(" ", 1); }
			this->scrBuf.append(message);
		}
		else this->scrBuf.append(" ", 1);

		this->scrBuf.append("\033[K", 3);
		if(y < (this->getRows() - 1))
		{
			this->scrBuf.append("\r\n", 2);
		}
	}
}

// We use the private std::string as an append buffer,
// So we can avoid flickering by printing the entire
// screen all at once.
void pg::refresh(void)
{
	this->scrBuf.append("\033[?25l", 6);
	this->scrBuf.append("\033[H", 3);

	this->draw();

	this->scrBuf.append("\033[H", 3);
	this->scrBuf.append("\033[?25h", 6);

	write(STDOUT_FILENO, this->scrBuf.data(), this->scrBuf.length());
	this->scrBuf.clear();
}

// Reads a single unbuffered character from stdin.
int pg::readch(void)
{
	int result;
  	char c;

  	while((result = read(STDIN_FILENO, &c, 1)) != 1)
	{
    		if(result == -1 && errno != EAGAIN) error("read");
  	}

	if(c == '\033')
	{
		std::array<char, 3> sequence = { '0', '0', '0' };
		if(read(STDIN_FILENO, &sequence.at(0), 1) != -1) return '\033';
		if(read(STDIN_FILENO, &sequence.at(1), 1) != -1) return '\033';

		if(sequence.at(0) == '[')
		{
			if(sequence.at(1) >= '0' && sequence.at(1) <= '9')
			{
				if(read(STDIN_FILENO, &sequence.at(2), 1) != 1) return '\033';
        			if(sequence.at(2) == '~')
				{
          				switch(sequence.at(1))
					{
						case '1': return HOME_KEY;
            					case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
        			}
      			}
			else
			{
				switch(sequence.at(1))
				{
					case 'A': return ARROW_UP;
					case 'B': return ARROW_DOWN;
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'H': return HOME_KEY;
          				case 'F': return END_KEY;
				}
			}
		}
		else if(sequence.at(0) == 'O')
		{
      			switch(sequence.at(1))
			{
        			case 'H': return HOME_KEY;
        			case 'F': return END_KEY;
      			}
		}

		return '\033';
	}
	else return c;
}

void pg::moveCursor(int key)
{
        switch(key)
	{
		case ARROW_UP:
			if(this->cursor.second != 0) this->cursor.second--;
			break;
		case ARROW_LEFT:
			if(this->cursor.first != 0)  this->cursor.first--;
			break;
		case ARROW_DOWN:
			if(this->cursor.second != (this->dim.second - 1)) this->cursor.second++;
			break;
		case ARROW_RIGHT:
			if(this->cursor.first != (this->dim.first - 1)) this->cursor.first++;
			break;
	}
}

// Returns a request for an exit, true for exit request and
// false for otherwise. The reason we're doing this is so
// that the deconstructor for 'pg' has the chance to actually
// run, rather than exiting here
bool pg::parseKeys(void)
{
	int in = this->readch();
	switch(in)
	{
		case CTRL_KEY('q'):
		case '\n':
			write(STDOUT_FILENO, "\033[2J", 4);
        		write(STDOUT_FILENO, "\033[H", 3);
			return false;

		case HOME_KEY: 	this->cursor.first = 0; break;
    		case END_KEY: 	this->cursor.first = this->dim.first - 1; break;

    		case PAGE_UP:
    		case PAGE_DOWN:
      		{
        		int times = this->dim.second;
        		while(times--) this->moveCursor(in == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      		} break;

		case ARROW_UP:
		case ARROW_DOWN:
		case ARROW_LEFT:
		case ARROW_RIGHT:
			this->moveCursor(in);
			break;
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
		if(mainScreen.parseKeys() == false) break;
	}

	return 0;
}

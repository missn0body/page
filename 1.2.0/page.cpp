// Let's make a scrolling pager!
// Written by anson

// STL libraries
#include <iostream>
#include <format>
#include <regex>
#include <vector>

// C libraries
#include <cstdlib>
#include <cerrno> // for errno
#include <cstring> // for strerror()
#include <unistd.h>

// POSIX libraries
// If you don't recognize these headers, then get Linux
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
		struct termios original, raw;
		std::vector<std::string> scrBuf;

	public:
		pg();
		~pg();

		void setRaw();
		void revert();
};

pg::pg() { setRaw(); }
pg::~pg(){ revert(); }

void pg::setRaw() { printf("setRaw()"); return; };
void pg::revert() { printf("revert"); return; };

//////////////////////////////////////
// main()
//////////////////////////////////////

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		fprintf(std::cerr, "%s: too few arguments\n"_p, argv[0]);
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

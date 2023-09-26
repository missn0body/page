// Let's make a scrolling pager!

// STL libraries
#include <iostream>
#include <format>
#include <regex>

// C libraries
#include <cstdlib>
#include <cerrno> // for errno
#include <cstring> // for strerror()

//////////////////////////////////////
// Utilites
//////////////////////////////////////

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

void _error(const char *file, int line, const char *what)
{
	fprintf(std::cerr, "(%d:%d) %s: %s\n"_p, file, line, what, strerror(errno));
	exit(EXIT_FAILURE);
}

#define error(x)	_error(__FILE__, __LINE__, x)

//////////////////////////////////////
// Terminal
//////////////////////////////////////

void setRaw() { printf("setRaw()"); return; };
void revert() { printf("revert"); return; };

//////////////////////////////////////
// Structures
//////////////////////////////////////

class pg
{
	public:
		pg();
		~pg();
};

pg::pg() { setRaw(); }
pg::~pg(){ revert(); }

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

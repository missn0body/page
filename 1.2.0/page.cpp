#include <iostream>
#include <format>
#include <regex>

//////////////////////////////////////
// Utilites
//////////////////////////////////////

static inline std::regex operator  ""_r(const char *in, std::size_t len) { return std::regex(in, len); }
inline std::string operator ""_p(const char *in, std::size_t len)
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

//////////////////////////////////////
// main()
//////////////////////////////////////

int main(void)
{
	return 0;
}

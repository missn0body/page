// Let's make a scrolling pager for real this time.
// Made by anson.
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>

#define TAB_DEF 	4
#define MARGIN_DEF	1
#define SPACE_DEF	2
#define NLCHARS	"\r\n"

static int TAB_COUNT = TAB_DEF;
static int NUM_MARGIN = MARGIN_DEF;
static int LINE_SPACE = SPACE_DEF;

static std::string NEWLINE(NLCHARS);

using namespace std::literals::string_literals;
static inline std::regex operator""_r(const char *in, std::size_t len)	{ return std::regex(in, len);}
static inline std::string operator+(std::string &in, std::string &what) { return in.append(what); }
static inline std::string operator*(const std::string &in, int how)
{
	std::string ret;
	while(how--) { ret += in; }
	return ret;
}

template<typename T>
std::ostream &operator<<(std::ostream &stream, const std::vector<T> &what)
{
	for(const auto &index: what) { stream << index << std::endl; }
	return stream;
}

int main(int argc, char *argv[])
{
	std::ifstream fObj(argv[1]);
	if(!fObj) return -1;

	int linecount = 0;
	std::vector<std::string> lines;
	for(std::string index; std::getline(fObj, index); linecount++)
	{
		index = std::regex_replace(index, "\t"_r, std::string(TAB_COUNT, ' '));
		index = (LINE_SPACE > 1) ? index + (NEWLINE * LINE_SPACE) : index;

		std::string linenum(std::to_string(linecount));
		if((std::size_t)TAB_COUNT < linenum.length())
		{
			TAB_COUNT += TAB_COUNT;
			NUM_MARGIN++;
		}
		std::string spacer((TAB_COUNT - linenum.length()) + NUM_MARGIN, ' ');
		lines.push_back(std::to_string(linecount) + spacer + index);
	}

	std::cout << lines << std::endl;
	return 0;
}

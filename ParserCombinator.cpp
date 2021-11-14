#include "ParserCombinator.hpp"
#include <iostream>

using  std::cout;
using  std::endl;

Result<int> parseNumber(const string_view& sv)
{
	int num = 0;
	size_t i;
	for(i = 0; i < sv.size() && isdigit(sv[i]); ++i) num = num * 10 + sv[i] - '0';
	return i == 0? Result<int>{} : Result<int>(num, sv.substr(i));
}

Parser<string_view>	operator ""_T(const char* c, size_t len)
{
	return [=](const string_view& sv)
	{
		size_t i;
		for(i = 0; i < sv.size() && i < len && sv[i] == c[i]; ++i)
			;
		return i < sv.size() && i != 0? Result<string_view>(string_view(c, i), sv.substr(i)): Result<string_view>{};
	};
}


int main()
{
	Combinator<int> number{Parser<int>(parseNumber)};
	Combinator<string_view> op{Parser<string_view>("+"_T)};
	auto exp = number + op + number >> [](int i, string_view, int j){ return i + j; };
	auto res = exp("12+3");
	cout << res._val << endl;
}

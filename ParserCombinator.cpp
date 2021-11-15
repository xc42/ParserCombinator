#include "ParserCombinator.hpp"
#include <iostream>
#include <cassert>

Result<int> parseNumber(const string_view& sv)
{
	int num = 0;
	size_t i;
	for(i = 0; i < sv.size() && isdigit(sv[i]); ++i) num = num * 10 + sv[i] - '0';
	return i == 0? Result<int>{} : Result<int>(num, sv.substr(i));
}

Combinator<string_view>	operator ""_T(const char* c, size_t len)
{
	return Combinator<string_view>( [=](const string_view& sv)
	{
		size_t i;
		for(i = 0; i < sv.size() && i < len && sv[i] == c[i]; ++i)
			;
		return i < sv.size() && i != 0? Result<string_view>(string_view(c, i), sv.substr(i)): Result<string_view>{};
	});
}


void TEST_Product()
{
	static_assert(std::is_same<product_type<int, int>, Product<int,int>>::value, "");
	static_assert(std::is_same<product_type<int, std::string>, Product<int,std::string>>::value, "");
	static_assert(std::is_same<product_type<Product<int,char>, std::string>, Product<int,char,std::string>>::value, "");
	Combinator<int> number{Parser<int>(parseNumber)};
	Combinator<string_view> op = "+"_T;
	auto exp = number + op + number >> [](int i, string_view, int j){ return i + j; };
	assert(exp("12+3")._val == 15);
}

void TEST_Sum()
{
	static_assert(std::is_same<sum_type<int, std::string>, Sum<int,std::string>>::value, "");
	static_assert(std::is_same<sum_type<Sum<int,char>,int>, Sum<int,char>>::value, "");
	
	Combinator<int> number{Parser<int>(parseNumber)};
	Combinator<string_view> add{"+"_T}, minus{"-"_T}, mul{"*"_T}, div{"/"_T};
	auto exp = (number + (add|minus|mul|div) + number )>> [](int i, string_view sv, int j) {
		if(sv == "+") return i + j;
		else if(sv == "*") return i * j;
		else if(sv == "-") return i - j;
		else if(sv == "/") return i / j;
		return -10086;
	};
	assert(exp("10+15")._val == 25);
	assert(exp("11*11")._val == 121);
	assert(exp("23-12")._val == 11);
	assert(!exp("88^66"));
}

void TEST_Calculator()
{
	Combinator<int> number{Parser<int>{parseNumber}};
	Combinator<int> expr, multive, factor;
	expr = (multive + ("+"_T|"-"_T) + multive) >> [](int i, string_view op, int j) { 
		if(op == "+") return i+j;
		else if(op == "-") return i-j;
		return -10086;
	};
	multive = factor + ("*"_T|"/"_T) + factor >> [](int i, string_view op, int j) {
		if(op == "*") return i*j;
		else if(op == "/") return i/j;
		return 0;
	};
	factor = number | (("("_T + expr + ")"_T) >> [](string_view,int i,string_view)
	{
		return i;
	});
}

int main()
{
	TEST_Product();
	TEST_Sum();
	TEST_Calculator();
}

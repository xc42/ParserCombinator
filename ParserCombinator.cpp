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
		return i == len ? Result<string_view>(string_view(c, i), sv.substr(i)): Result<string_view>{};
	});
}


void TEST_Product()
{
	std::cout << "Run Test: " << __FUNCTION__ << std::endl;
	static_assert(std::is_same<product_type<int, int>, Product<int,int>>::value, "");
	static_assert(std::is_same<product_type<int, std::string>, Product<int,std::string>>::value, "");
	static_assert(std::is_same<product_type<Product<int,char>, std::string>, Product<int,char,std::string>>::value, "");
	Combinator<int> number{Parser<int>(parseNumber)};
	Combinator<string_view> op = "+"_T;
	auto exp = number + op + number >> [](int i, string_view, int j){ return i + j; };
	assert(exp("12+3")._val == 15);
	std::cout << "Run Test Complete: " << __FUNCTION__ << std::endl;
}

void TEST_Sum()
{
	std::cout << "Run Test: " << __FUNCTION__ << std::endl;
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
	std::cout << "Run Test Complete: " << __FUNCTION__ << std::endl;
}

void TEST_Calculator()
{
	std::cout << "Run Test: " << __FUNCTION__ << std::endl;
	Combinator<int> number{Parser<int>{parseNumber}};
	Combinator<int> expr, multive, factor;
	expr = (multive + ("+"_T|"-"_T) + expr) >> [](int i, string_view op, int j) { 
		if(op == "+") return i+j;
		else if(op == "-") return i-j;
		return -10086;
	}|multive;
	multive = factor + ("*"_T|"/"_T) + multive>> [](int i, string_view op, int j) {
		if(op == "*") return i*j;
		else if(op == "/") return i/j;
		return 0;
	}|factor;
	factor = ("("_T + expr + ")"_T) >> [](string_view,int i,string_view) { return i; }|number;

	assert(expr("1314")._val == 1314);
	assert(expr("(1314)")._val == 1314);
	assert(expr("1+2+3+4+10")._val == 20);
	assert(expr("1*2*3*4")._val == 24);
	assert(expr("2*3+24")._val == 30);
	assert(expr("1+3*3+4")._val == 14);
	assert(expr("10*3-2*5")._val == 20);
	assert(expr("10*(5-2)+3")._val == 33);
	std::cout << "Run Test Complete: " << __FUNCTION__ << std::endl;
}

int main()
{
	TEST_Product();
	TEST_Sum();
	TEST_Calculator();
}

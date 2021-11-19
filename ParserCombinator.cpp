#include "ParserCombinator.hpp"
#include <cctype>
#include <iostream>
#include <cassert>

using namespace std;

void TEST_Product()
{
	std::cout << "Run Test: " << __FUNCTION__ << std::endl;
	static_assert(std::is_same<product_type<int, int>, Product<int,int>>::value, "");
	static_assert(std::is_same<product_type<int, std::string>, Product<int,std::string>>::value, "");
	static_assert(std::is_same<product_type<Product<int,char>, std::string>, Product<int,char,std::string>>::value, "");
	Combinator<int> number = numberPC();
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
	
	Combinator<int> number =  numberPC();
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
	Combinator<int> number = numberPC();
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
	factor = ("("_T + expr + ")"_T) >> [](PlaceHolder, int i, PlaceHolder) { return i; }|number;

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

void TEST_TC_Config()
{

	struct TC_Config
	{
	public:
		struct Content {
			map<string_view, string_view>		_dict;
			vector<shared_ptr<TC_Config>>		_subdomain;
		};

		TC_Config(const string_view& tag, Content ctt): _tag(tag), _content(std::move(ctt)) {}

		void addKV(const pair<string,string>& kv) {
			_content._dict.insert(kv.first, kv.second);
		}

		Content								_content;
		string								_tag;
	};


	Combinator<TC_Config> 			doc;
	Combinator<TC_Config::Content>  content;
	Combinator<string_view>			opentag, closetag;
	Combinator<pair<string_view,string_view>>	kv;
	Combinator<string_view>				alphanum = makePC(::isalnum);

	doc = opentag + content + closetag >> [](string_view ot, TC_Config::Content ctt, string_view ct) { return TC_Config{ot, std::move(ctt)}; };

	content = kv + content >> [](pair<string_view,string_view> kv, TC_Config::Content& ctt) { ctt.addKV(kv); }
		| doc + content >> [](TC_Config subdo
	//kv = ignoreBlank(alphanum) + "="_T + ignoreBlank(alphanum) >> [](string_view k, PlaceHolder, string_view v) { return make_pair(k,v); };
	opentag = "<"_T + alphanum + ">"_T >> [](PlaceHolder, string_view tag,PlaceHolder) { return tag; };
	closetag = "</"_T + alphanum + ">"_T >> [](PlaceHolder, string_view tag,PlaceHolder) { return tag; };
}

int main()
{
	TEST_Product();
	TEST_Sum();
	TEST_Calculator();
}

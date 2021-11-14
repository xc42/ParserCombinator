#include <boost/variant.hpp>
#include <boost/utility/string_view.hpp>
#include <tuple>
#include <functional>

using boost::variant;
using boost::string_view;
using std::tuple;

template<typename T>
struct Result
{
	bool			_succ;
	string_view		_rest;
	T				_val;

	Result(): _succ(false) {}
	Result(const T& val, const string_view& sv):_succ(true), _rest(sv), _val(val) {}

	operator bool() const{ return _succ; }
	bool operator!() const { return !_succ; }

};

template<typename T>
using Parser = std::function<Result<T>(const string_view&)>;

template<typename ...Ts>
struct Product: public tuple<Ts...> 
{
	using tuple_type = tuple<Ts...>;
	Product()=default;
	template<typename ...Args>
	Product(Args&&...t):tuple<Ts...>(t...){}

	template<typename ...Args, typename T>
	Product(Product<Args...>&& prod, T&& t) {
		const auto size = std::tuple_size<tuple_type>::value;
		assign(std::move(prod), std::make_index_sequence<size-1>());
		std::get<size-1>(*this) = std::forward<T>(t);
	}

	template<typename ...Args, typename T>
	Product(Product<Args...>& prod, T&& t) {
		const auto size = std::tuple_size<tuple_type>::value;
		assign(std::move(prod), std::make_index_sequence<size-1>());
		std::get<size-1>(*this) = std::forward<T>(t);
	}

private:

	template<class Product, size_t ...I>
	constexpr void assign(Product&& prod, std::index_sequence<I...>)
	{
		std::tie(std::get<I>(*this)...) = prod;
	}

	template <class F, class Tuple, std::size_t... I>
	constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
	{
		return std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...);
	}
public:
	template<typename Fn>
	constexpr decltype(auto) operator>>(Fn&& fn)
	{
		return apply_impl(std::forward<Fn>(fn), *this, std::make_index_sequence<std::tuple_size<tuple_type>::value>{});
	}
};


template<typename T1, typename T2>
struct MakeProduct
{
	using type = Product<T1, T2>;
};

template<typename ...T1, typename T2>
struct MakeProduct<Product<T1...>, T2>
{
	using type = Product<T1..., T2>;
};

template<typename ...T1, typename ...T2>
struct MakeProduct<Product<T1...>, Product<T2...>>
{
	using type = Product<T1..., T2...>;
};

template<typename T1, typename T2>
using product_type = typename MakeProduct<T1, T2>::type;


template<typename ...Ts>
struct Sum:public variant<Ts...>
{
	Sum()=default;
	template<typename T>
	Sum(T&& t): variant<Ts...>(std::forward<T>(t)){}
};

template<typename T>
class Combinator
{
public:
	Combinator()=default;
	Combinator(Parser<T> p): _fn_ptr(std::make_shared<Parser<T>>(std::move(p))) {}

	using val_type = T;

	auto operator()(const string_view& sv) { return (*_fn_ptr)(sv); }

	template<typename C2>
	auto operator+(C2&& c2) 
	{
		using Prod = product_type<T, typename std::remove_reference_t<C2>::val_type>;
		return Combinator<Prod>( [p1=_fn_ptr, p2 = c2.getFn()](const string_view& sv) 
		{
			auto& f1 = *p1;
			auto& f2 = *p2;
			auto r1 = f1(sv);
			if(!r1) return Result<Prod>{};
			auto r2 = f2(r1._rest);
			return r2? Result<Prod>{Prod(r1._val, r2._val), r2._rest}: Result<Prod>{};
		});
	}

	template<typename Fn>
	auto operator>>(Fn&& fn) 
	{
		return [parser=_fn_ptr, fn = std::forward<Fn>(fn)](const string_view& sv)
		{
			auto r = (*parser)(sv);
			using NT = std::decay_t<decltype(r._val >> fn)>;
			return r? Result<NT>{r._val >> fn, r._rest}: Result<NT>{};
		};
	}

	const auto& getFn() { return _fn_ptr; }
private:
	std::shared_ptr<Parser<T>> _fn_ptr;
};

Result<int> parseNumber(const string_view& sv);
Parser<string_view>	operator ""_T(const char* c, size_t);

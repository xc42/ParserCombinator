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
	template<typename Fn>
	constexpr decltype(auto) operator>>(Fn&& fn)
	{
		return boost::apply_visitor(*this, fn);
	}

};

template<class ...Ts>
struct already_exist
{
	static constexpr bool value = false;
};

template<class T0, class T1, class ...Ts>
struct already_exist<Sum<T1,Ts...>, T0>
{
	static constexpr bool value = std::is_same<T1, T0>::value || already_exist<Sum<Ts...>, T0>::value;
};

template<class T1, class T2>
struct already_exist<Sum<T1>, T2>
{
	static constexpr bool value = std::is_same<T1, T2>::value;
};

template<class T1, class T2>
struct MakeSum
{
	using type = Sum<T1,T2>;
};

template<class T>
struct MakeSum<T, T>
{
	using type = T;
};

template< class ...Ts, class T0>
struct MakeSum<Sum<Ts...>, T0>
{
	using type = std::conditional_t<already_exist<Sum<Ts...>, T0>::value, Sum<Ts...>, Sum<T0, Ts...>>;
};

template<class T1, class T2>
struct MakeSum<Sum<T1>, T2>
{
	using type = std::conditional_t<std::is_same<T1, T2>::value, Sum<T1>, Sum<T1,T2>>;
};

template<class T1, class T2>
using sum_type = typename MakeSum<T1,T2>::type;

template<class T>
struct is_sum_type { static constexpr bool value = false; };

template<class ...Ts>
struct is_sum_type<Sum<Ts...>> { static constexpr bool value = true; };

template<class T>
struct is_product_type { static constexpr bool value = false; };

template<class ...Ts>
struct is_product_type<Product<Ts...>> { static constexpr bool value = true; };
static_assert(is_product_type<Product<int,string_view>>::value,"");

template<typename T>
class Combinator
{
public:
	Combinator():_fn_ptr(std::make_shared<Parser<T>>()){} ;
	Combinator(Parser<T> p):Combinator() { *_fn_ptr = std::move(p); }
	Combinator(Combinator& c):Combinator() { *_fn_ptr = *c._fn_ptr; }
	Combinator(Combinator&& c):Combinator() { *_fn_ptr = std::move(*c._fn_ptr); }
	Combinator& operator=(Combinator c) { *_fn_ptr = std::move(*c._fn_ptr); return *this; }

	using val_type = T;

	auto operator()(const string_view& sv) { return (*_fn_ptr)(sv); }

	template<typename C2>
	auto operator+(C2&& c2) 
	{
		using Prod = product_type<T, typename std::decay_t<C2>::val_type>;
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

	template<typename C2>
	auto operator|(C2&& c2)
	{
		using SumT = sum_type<val_type, typename std::decay_t<C2>::val_type>;
		
		return Combinator<SumT>( [p1=_fn_ptr, p2 = c2.getFn()](const string_view& sv) 
		{
			auto r1 = (*p1)(sv);
			if(r1) return Result<SumT>{r1._val, r1._rest};
			auto r2 = (*p2)(sv);
			return r2? Result<SumT>{r2._val, r2._rest}: Result<SumT>{};
		});
	}

	template<typename Fn>
	auto operator>>(Fn&& fn)
	{

		//using CT = std::conditional_t<is_product_type<T>::value || is_sum_type<T>::value,
		//	  std::decay_t<decltype(std::declval<T>() >> fn)>,
		//	  std::decay_t<decltype(fn(std::declval<T>()))>>;

		using CT = std::decay_t<decltype(T() >> fn)>;
		return Combinator<CT>( [parser=_fn_ptr, fn = std::forward<Fn>(fn)](const string_view& sv)
		{
			auto r = (*parser)(sv);
			return r? Result<CT>{r._val >> fn, r._rest}: Result<CT>{};
		} );
	}

	const auto& getFn() { return _fn_ptr; }
private:
	std::shared_ptr<Parser<T>> _fn_ptr;
};

Result<int> parseNumber(const string_view& sv);
Combinator<string_view>	operator ""_T(const char* c, size_t);

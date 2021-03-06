
#include "MLProcMultiply.h"

// definition of static constexpr data members at namespace scope is still required, until C++17
constexpr constStr ProcMultiply::paramNames[];
constexpr constStr ProcMultiply::textParamNames[];
constexpr constStr ProcMultiply::inputNames[];
constexpr constStr ProcMultiply::outputNames[];

constexpr constStrArray ProcMultiply::pn_;
constexpr constStrArray ProcMultiply::tn_;
constexpr constStrArray ProcMultiply::in_;
constexpr constStrArray ProcMultiply::on_;

/*
// output function that requires a compile-time constant, for testing
template<int n>
struct constTest
{
	constTest() { std::cout << n << '\n'; }
};
*/


void ProcMultiply::process() 
{
	output("baz") = multiply(input("foo"), input("bar"));
	
//	constTest< constFind(paramNames, "c") >();	
}


// instead: each proc as template?

// proc name as parameter.




#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include "../include/madronalib.h"
#include "../source/DSP/MLDSP.h"
#include "../tests/tests.h"

#include "../source/core/MLMemory.h"

#ifdef _WINDOWS
#include "Windows.h"
#endif

using namespace ml;

constexpr float mySinFillFn(int n){ return const_math::sin(n*kTwoPi/(kFloatsPerDSPVector));  }
	
int main()
{
#ifdef _WINDOWS
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
#endif


	std::cout << "DSP Ops:\n";
	
	// generate a vector using map() and columnIndex()
	std::cout << "index squared: " << map(([](float x){return x*x;}), columnIndex()) << "\n\n";		

	auto sinMadronaLib = sin(rangeClosed(0, kPi));
	std::cout << "madronalib sin: " << sinMadronaLib << "\n\n";
	 	
	// store a lambda on (DSPVector)->(DSPVector) defined using map(float)->(float)
	auto sinNative = [&](const DSPVector& x){ return map( [](float x){ return sinf(x*kPi/(kFloatsPerDSPVector - 1)); }, x); }(columnIndex());
	std::cout << "native sin: " << sinNative << "\n\n";	
	
	std::cout << "difference: " << sinNative - sinMadronaLib << "\n\n";	
	
	// constexpr fill. unfortunately this can not be made to work with a lambda in C++11.
	constexpr DSPVector kSinVec(mySinFillFn);
	std::cout << "constexpr sin table: " << kSinVec << "\n\n";
	
	/*
	 // store a lambda on ()->(DSPVector) defined using fill()->(float)
	auto randFill = [&](){ return map( [](){ return ml::rand(); }); };
	std::cout << randFill();
	
	DSPVector q = randFill();
	std::cout << "\n\n" << q << "\n";
	std::cout << "max: " << max(q) << "\n";
	
//	DSPVector q = sin(rangeOpen(-kMLPi, kMLPi));
//	std::cout << "\n\n" << q << "\n";
	std::cout << "min: " << min(q) << "\n";
	 */
	
//	DSPVectorArray<4> f;
	
//	auto f = repeat<4>(columnIndex());
//	f.setRowVector<1>(columnIndex()*2);
//	f.setRowVector(3, columnIndex()*2);
					  
//	f = map( [](float x){ return x + ml::rand()*0.01; }, f );
//	f = map( [](DSPVector v, int row){ return v*row; }, f );
// 	f = row( DSPVectorArray<4>() ) * repeat<4>(columnIndex()) ;
	
//	constexpr int iters = 100;
	
//	auto doFDNVector = [&](){ return map([](DSPVector v, int row){ return sin(rangeClosed(0 + row*kPi/2, kPi + row*kPi/2)); }, DSPVectorArray<ROWS>() ); } ;	
//	auto doFDNVector = [&](){ return rowIndex( DSPVectorArray<ROWS>() ); } ;
//	auto fdnTimeVector = timeIterations< DSPVectorArray<ROWS> >(doFDNVector, iters);
	
	/*
	auto rr = rowIndex<3>();
	auto qq = repeat<3>(rr);
	std::cout << qq << "\n";
*/
	
	/*
	// ----------------------------------------------------------------
	// time FDN: scalars
	int iters = 10;
	float sr = 44100.f;
	
	MLSignal freqs({10000, 11000, 12000, 14000});
	freqs.scale(kTwoPi/sr);

	static FDN fdn
	{ 
		{"delays", {69, 70, 71, 72} },
		{"cutoffs", freqs } , // TODO more functional rewrite of MLSignal so we can create freqs inline
		{"gains", {0.99, 0.99, 0.99, 0.99} }
	};

	DSPVector input(0);
	input[0] = 1;
	
	fdn(input);
	input = 0;
	auto doFDNVector = [&](){return fdn(input);};

	// note: fdn returns a DSPVectorArray<2>, so we need to pass this template parameter to timeIterations().
	auto fdnTimeVector = timeIterations< DSPVectorArray<2> >(doFDNVector, iters);
	std::cout << "VECTOR time: " << fdnTimeVector.elapsedTime << "\n";
	std::cout << fdnTimeVector.result << "\n";
	 */
	
	
	
	RandomSource r;
	DSPVector a(2.f);
	DSPVector m(3.f);
	DSPVector n(r());
	
	a = m + n;
	
	std::cout << "sum: " << a << "\n";
		
	std::cout << "aligned? " << EIGEN_MALLOC_ALREADY_ALIGNED << "\n";



#ifdef _WINDOWS
	system("pause");
#endif

}


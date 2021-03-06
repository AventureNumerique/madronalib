
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// TODO this will be the only clamp.
// first we need to make it efficient for constant signals
// and take parameter inputs for convenience.

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition


class MLProcClampSignal : public MLProc
{
public:
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcClampSignal> mInfo;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcClampSignal> classReg("clamp_signal");
//	ML_UNUSED MLProcParam<MLProcClampSignal> params[2] = { "min", "max" };
	ML_UNUSED MLProcInput<MLProcClampSignal> inputs[] = {"in", "min", "max"};	
	ML_UNUSED MLProcOutput<MLProcClampSignal> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation

void MLProcClampSignal::process()
{
	const MLSignal& x = getInput(1);
	const MLSignal& min = getInput(2);
	const MLSignal& max = getInput(3);
	MLSignal& y = getOutput();

	for (int n=0; n < kFloatsPerDSPVector; ++n)
	{
		y[n] = ml::clamp(x[n], min[n], max[n]);
	}
}




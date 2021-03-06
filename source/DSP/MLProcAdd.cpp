
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcAdd : public MLProc
{
public:
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcAdd> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcAdd> classReg("add");
	ML_UNUSED MLProcInput<MLProcAdd> inputs[] = {"in1", "in2"};
	ML_UNUSED MLProcOutput<MLProcAdd> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation

using namespace ml;

void MLProcAdd::process()
{
	const DSPVector* pvm1 = reinterpret_cast<const DSPVector*>(getInput(1).getConstBuffer());
	const DSPVector* pvm2 = reinterpret_cast<const DSPVector*>(getInput(2).getConstBuffer());
	DSPVector* pvout = reinterpret_cast<DSPVector*>(getOutput().getBuffer());
	(*pvout) = (*pvm1) + (*pvm2);
}


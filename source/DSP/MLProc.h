
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// MLProcs are DSP objects for dynamic dispatch.
// MLProcs:
// - are made by the MLProcFactory 
// - must have a MLProcRegistryEntry<class> to register the class with the factory
// - probably have processing methods that are not inlineable
// - have process() methods that are dynamically dispatched
// - have names
// - operate on named signals set with setInput() and setOutput()
// - use named parameters set with setParam()
// - can only be made by and run within MLProcContainers
//
// for smaller, statically dispatched DSP objects, see DSPUtils.
//
// TODO by changing Container to do oversampling with multiple calls to the same process vector size, 
// instead of one larger call as currently, we can make all MLProcs have chunk sizes that are fixed
// at compile time. 


#ifndef _ML_PROC_H
#define _ML_PROC_H

#include <cassert>
#include <math.h>
#include <map>
#include <vector>
#include <iostream>

#include "MLDebug.h"
#include "MLDSPContext.h"
#include "MLSymbol.h"
#include "MLSymbolMap.h"
#include "MLProperty.h"
#include "MLTextUtils.h"
#include "MLClock.h"

#include "MLDSPDeprecated.h"

#include "MLMemory.h"

#define CHECK_IO    0

#ifdef	__GNUC__
#define	ML_UNUSED	__attribute__ (( __unused__ )) 
#else
#define	ML_UNUSED 
#endif

class ProcParamMap
{
public:
	ProcParamMap() {};
	
	MLProperty& operator[] (ml::Symbol key)
	{
		int v = keys.size();
		
		for(int i=0; i<v; ++i)
		{
			if(keys[i] == key)
			{
				return values[i];
			}
		}

		keys.emplace_back(key);
		values.emplace_back(MLProperty());
		return values[v];
	}
	
	inline float getFloatParam(ml::Symbol key) const
	{
		int v = keys.size();
		for(int i=0; i<v; ++i)
		{
			if(keys[i] == key)
			{
				return values[i].getFloatValue();
			}
		}
		return 0.f;
	}
	
	std::vector< ml::Symbol > keys;
	std::vector< MLProperty > values;
};
// ----------------------------------------------------------------
#pragma mark important constants

// keep this small.
const int kMLProcLocalParams = 16; // TODO band-aid!  this should be 4 or something.  crashing evil threading(?) bug.

const std::string kMLProcAliasUndefinedStr = "undefined";

// ----------------------------------------------------------------
#pragma mark templates

// virtual base class allows templated functions to be called from an 
// object of unknown derived type.
class MLProcInfoBase
{    
	
public:
	MLProcInfoBase() {};	
	virtual ~MLProcInfoBase() {};
	
	virtual const MLProperty& getParamProperty(const ml::Symbol paramName) = 0;
	virtual void setParamProperty(const ml::Symbol paramName, const MLProperty& value) = 0;
	
	virtual MLSymbolMap& getParamMap() const = 0;
	virtual MLSymbolMap& getInputMap() const = 0;
	virtual MLSymbolMap& getOutputMap() const = 0;
	virtual bool hasVariableParams() const = 0;
	virtual bool hasVariableInputs() const = 0;
	virtual bool hasVariableOutputs() const = 0;
	virtual ml::Symbol getClassName() = 0;
		
private:
	// make uncopyable
	MLProcInfoBase (const MLProcInfoBase&) = delete;
	const MLProcInfoBase& operator= (const MLProcInfoBase&) = delete;
};

// Static member functions can't be virtual, so to provide access to 
// the class data from derived classes we create virtual wrapper functions.
// each must own an MLProcInfo<MLProcSubclass>.
//
// static part (per class): the name maps. 
// dynamic part (per object): the parameter data.
//
template <class MLProcSubclass>
class MLProcInfo : public MLProcInfoBase
{
public:
	friend class MLProcFactory;
	
	MLProcInfo()
	{ 
		// set up SymbolMappedArray for parameters
		mParams.setMap(getClassParamMap()); 
	}
	~MLProcInfo() {};
	
	const MLProperty& getParamProperty(const ml::Symbol paramName)
	{
		if (hasVariableParams())
		{
			// if entry is not present, make it and initialize value to 0
			MLSymbolMap& map = getParamMap();
			if (!map.getIndex(paramName))
			{
				map.addEntry(paramName);
				setParamProperty(paramName, MLProperty(0.f));
			}
		}
		return *(mParams[paramName]);
	}
	
	void setParamProperty(const ml::Symbol paramName, const MLProperty& value)
	{
		if (hasVariableParams())
		{
			// if entry is not present, make it
			MLSymbolMap& map = getParamMap();
			if (!map.getIndex(paramName))
			{
				map.addEntry(paramName);
			}
		}
#if CHECK_IO
		float* paramToSet = mParams[paramName];
		if (paramToSet != mParams.getNullElement())
		{
			*paramToSet = value;
		}
		else
		{
			debug() << "setParam: " << getClassName() << " has no parameter " << paramName << "!\n";
		}
#else
		*(mParams[paramName]) = value;
#endif
	}
	
	inline MLSymbolMap& getParamMap() const { return getClassParamMap(); }
	inline MLSymbolMap& getInputMap() const { return getClassInputMap(); } 
	inline MLSymbolMap& getOutputMap() const { return getClassOutputMap(); } 
	inline bool hasVariableParams() const { return getVariableParamsFlag(); }
	inline bool hasVariableInputs() const { return getVariableInputsFlag(); }
	inline bool hasVariableOutputs() const { return getVariableOutputsFlag(); }
	
	ml::Symbol getClassName() { return getClassClassNameRef(); } 
	static void setClassName(const ml::Symbol n) { getClassClassNameRef() = n; }	
	
	// consider these private-- they are public so the syntax of access from templates isn't ridiculous
	static MLSymbolMap &getClassParamMap()  { static MLSymbolMap pMap; return pMap; } 
	static MLSymbolMap &getClassInputMap()  { static MLSymbolMap inMap; return inMap; } 
	static MLSymbolMap &getClassOutputMap()  { static MLSymbolMap outMap; return outMap; } 
	static ml::Symbol& getClassClassNameRef() { static ml::Symbol cName; return cName; } 
	
	// is there a variable number of inputs / outputs for this class?
	// if so, they can be accessed with names "1", "2"... instead of the map.	
	static bool& getVariableParamsFlag() { static bool mHasVariableParams; return mHasVariableParams; }
	static bool& getVariableInputsFlag() { static bool mHasVariableInputs; return mHasVariableInputs; }
	static bool& getVariableOutputsFlag() { static bool mHasVariableInputs; return mHasVariableInputs; }
	
private:
	
	// parameter storage per MLProc subclass instance, each must own an MLProcInfo<MLProcSubclass>.
	//
	SymbolMappedArray<MLProperty, kMLProcLocalParams> mParams;
	
};

// an MLProcParam creates a single indexed parameter that is shared by all
// instances of an MLProc subclass. This is written as a class so that
// parameters for each MLProc subclass can be set up at static initialization time.
template <class MLProcSubclass>
class MLProcParam
{
public:
	MLProcParam(const char * name)
	{
		if (!std::string(name).compare("*"))
		{
			MLProcInfo<MLProcSubclass>::getVariableParamsFlag() = true;
		}
		else
		{
			MLProcInfo<MLProcSubclass>::getVariableParamsFlag() = false;
			MLSymbolMap & pMap = MLProcInfo<MLProcSubclass>::getClassParamMap();
			pMap.addEntry(ml::Symbol(name));
		}
	}
};

// MLProcInput and MLProcOutput are similar to MLProcParam, except there is no
// limit on the number of entries. 
template <class MLProcSubclass>
class MLProcInput
{
public:
	MLProcInput(const char *name)
	{
		if (!std::string(name).compare("*"))
		{
			MLProcInfo<MLProcSubclass>::getVariableInputsFlag() = true;
			// debug() << "added input " << name << ", variable size \n";
		}
		else
		{
			MLProcInfo<MLProcSubclass>::getVariableInputsFlag() = false;
			MLSymbolMap & iMap = MLProcInfo<MLProcSubclass>::getClassInputMap();
			iMap.addEntry(ml::Symbol(name));
			// debug() << "added input " << name << ", size " << iMap.getSize() << "\n";
		}
	}
};

template <class MLProcSubclass>
class MLProcOutput
{
public:
	MLProcOutput(const char *name)
	{
		if (!std::string(name).compare("*"))
		{
			MLProcInfo<MLProcSubclass>::getVariableOutputsFlag() = true;
			// debug() << "added output " << name << ", variable size \n";
		}
		else
		{
			MLProcInfo<MLProcSubclass>::getVariableOutputsFlag() = false;
			MLSymbolMap & oMap = MLProcInfo<MLProcSubclass>::getClassOutputMap();
			oMap.addEntry(ml::Symbol(name));
			//		debug() << "added output " << name << ", size " << oMap.getSize() << "\n";
		}
	}
};

// an MLProc processes signals.  It contains Signals to receive its output.  
// If the block size is small enough, the buffers in these Signals are 
// internal to the MLProc object.  
//
// All inputs and outputs to an MLProc must have the same sampling rate
// and buffer size.  The one exception is MLProcResample, which is only
// created by MLProcContainer.  Possibly this means container resamplers 
// should not really be MLProcs, but part of containers. TODO

class MLProc
{
	friend class MLProcContainer;
	friend class MLProcMultiple;
	friend class MLMultProxy;
	friend class MLMultiProc;
	friend class MLMultiContainer;
	friend class MLProcFactory;
public:
	enum err
	{
		OK = 0,
		memErr,
		inputBoundsErr,
		inputOccupiedErr,
		inputRateErr,
		noInputErr,
		inputMismatchErr,
		fractionalBlockSizeErr,
		connectScopeErr,
		nameInUseErr,
		headNotContainerErr,
		nameNotFoundErr,
		fileOpenErr,
		newProcErr,
		docSyntaxErr,
		needsResampleErr, // ??
		ratioErr,
		scopeErr, 
		resizeErr,
		badIndexErr,
		SSE2RequiredErr,
		SSE3RequiredErr,
		unknownErr
	};
	
	MLProc() : mpContext(0), mParamsChanged(true), mCopyIndex(0) {}
	virtual ~MLProc() {}	
	
	// ----------------------------------------------------------------
	// Custom allocators for 32-bit Windows. This should be sufficient to allow use of 
	// DSPVectors within Procs on the heap. There may still be cases where DSPVectors 
	// are not aligned properly, such as STL containers. Test and tread carefully.
	
	#if(_WIN32 && (!_WIN64))
		ML_MAKE_ALIGNED_OPERATOR_NEW;
	#endif

	// ----------------------------------------------------------------
	// wrapper for class static info
	virtual MLProcInfoBase& procInfo() = 0; 	
	//
	virtual bool isContainer() { return false; } 
	inline bool isEnabled() { return getContext()->isProcEnabled(this); }
	
	// for subclasses to make changes based on startup parameters, before prepareToProcess() is called.
	// currently being used for resamplers.
	virtual void setup() {}
	
	// for subclasses to set up buffers, etc.  Can be left at this if there is nothing to resize.
	virtual err resize() { return OK; }		
	virtual err prepareToProcess();
	
	// the process method. One is needed!
	virtual void process() = 0;	
	
	// clearProc() is called by engine, procs override clear() to clear histories. 
	void clearProc();	 
	virtual void clear() {}
	virtual void clearInputs();	 
	virtual void clearInput(const int i);
	
	// getInput and getOutput get references to I/O signals.
	// used commonly in process(), so non-virtual and inline.
	// They do no checking in a release build. 
	// In a debug build they will complain and return a bogus Signal
	// if a signal or signal ptr has not been made at
	// the given index.  Signals and room for signal ptrs are created when 
	// connections are made in the DSP graph.   
	
	// TODO making all inputs and outputs 0-indexed would save a little time. 
	
	inline const MLSignal& getInput(const int idx)
	{ 	
#if CHECK_IO
		if (idx > (int)mInputs.size())
		{
			debug() << "MLProc::getInput: no input " << idx << " of proc " << getName() << "!\n";
			return (getContext()->getNullInput());
		}
#endif
		return (*mInputs[idx-1]); 
	}
	
	inline MLSignal& getOutput(const int idx) 
	{	
#if CHECK_IO
		if (idx > (int)mOutputs.size())
		{
			debug() << "MLProc::getOutput: no output " << idx << " of proc " << getName() << "!\n";
			return (getContext()->getNullOutput());
		}
#endif
		return (*mOutputs[idx-1]); 
	}
	
	inline MLSignal& getOutput() 
	{	
		return (*mOutputs[0]); 
	}
	
	virtual err setInput(const int idx, const MLSignal& srcSig);	 
	void setOutput(const int idx, MLSignal& srcSig);	 
	
	// params
	/*
	bool paramExists(const ml::Symbol p);
	*/
	
	// get and set parameters
	virtual void setParam(const ml::Symbol p, const MLProperty& val);
	
	inline float getParam(const ml::Symbol pname)
	{
		return procInfo().getParamProperty(pname).getFloatValue();
	}

	virtual const ml::Text getTextParam(const ml::Symbol p);
	virtual const MLSignal& getSignalParam(const ml::Symbol p);

	// MLProc returns the index to an entry in its proc map.
	// MLProcContainer returns an index to a published input or output.
	virtual int getInputIndex(const ml::Symbol name);	
	virtual int getOutputIndex(const ml::Symbol name);	
	
	// MLProcContainer overrides this to return published output names
	// virtual ml::Symbol getOutputName(int index);
	
	// getNumInputs() and getNumOutputs() are not virtual.  
	// Proxy classes, for example, override procInfo()
	// and resize mInputs and mOutputs so that these methods
	// work for all classes.
	inline int getNumInputs() 
	{ 
		return (int)mInputs.size(); 
	}
	
	inline int getNumRequiredInputs() 
	{
		if (procInfo().hasVariableInputs())
		{
			return 0; 
		}
		else
		{
			return procInfo().getInputMap().getSize();
		}
	}
	
	inline int getNumOutputs() 
	{ 
		return (int)mOutputs.size(); 
	}
	
	inline int getNumRequiredOutputs() 
	{ 
		if (procInfo().hasVariableOutputs())
		{
			return 0; 
		}
		else
		{
			return procInfo().getOutputMap().getSize();	
		}
	}
	
	virtual void resizeInputs(int n);
	virtual void resizeOutputs(int n);
	
	bool inputIsValid(int idx);
	bool outputIsValid(int idx);
	
	ml::Symbol getClassName() { return procInfo().getClassName(); }
	inline const ml::Symbol getName() const { return mName; }
	inline int getCopyIndex() const { return mCopyIndex; }
	ml::Symbol getNameWithCopyIndex();
	void dumpParams();
	virtual void dumpProc(int indent);
	
	// set enclosing DSP context. This should be protected, but some procs are using others directly in a
	// functional style now, and they need to share context. TODO step back and look at this
	void setContext(MLDSPContext* pc) { mpContext = pc; }

	// get enclosing DSP context
	MLDSPContext* getContext() const { return mpContext; }
	
	// TODO how could there be no context? correct any possibility of that. 
	inline int getContextVectorSize() { return mpContext ? mpContext->getVectorSize() : 0; }	
	inline float getContextSampleRate() { return mpContext ? mpContext->getSampleRate() : kMLTimeless; }
	inline float getContextInvSampleRate() { return mpContext ? mpContext->getInvSampleRate() : kMLTimeless; }
	inline ml::Time getContextTime() { return mpContext ? mpContext->getTime() : 0; }
	
protected:	
	
	void dumpNode(int indent);
	void printErr(MLProc::err err);	
	
	void setName(const ml::Symbol name) { mName = name; }
	void setCopyIndex(int c)  { mCopyIndex = c; }
	
	virtual void createInput(const int idx);
	
	// most signals have frame size 1. By overriding this a proc can tell the compiler that
	// it needs output signals of a larger size.
	virtual int getOutputFrameSize(const int outputIdx) { return 1; }
	
	// ----------------------------------------------------------------
	// data
	
protected:
	MLDSPContext* mpContext;		// set by our enclosing context to itself on creation
	bool mParamsChanged;			// set by setParam() // TODO Context stores list of Parameter changes
	
	// pointers to input signals.  A subclass of MLProc will get data from these
	// signals directly in its process() method.  A subclass of MLProcContainer
	// will pass the pointers to the inputs of its subprocs.
	std::vector<const MLSignal*> mInputs;
	std::vector<MLSignal*> mOutputs;
	
private:	
	int mCopyIndex;		// copy index if in multicontainer, 0 otherwise
	ml::Symbol mName;
};


typedef std::unique_ptr<MLProc> MLProcOwner;

typedef MLProc* MLProcPtr;
typedef std::list<MLProcPtr> MLProcList;
typedef MLProcList::iterator MLProcListIterator;

// ----------------------------------------------------------------
#pragma mark factory

class MLProcFactory
{
private:
	MLProcFactory();
	~MLProcFactory();

	// delete copy and move constructors and assign operators
	MLProcFactory(MLProcFactory const&) = delete;             // Copy construct
	MLProcFactory(MLProcFactory&&) = delete;                  // Move construct
	MLProcFactory& operator=(MLProcFactory const&) = delete;  // Copy assign
	MLProcFactory& operator=(MLProcFactory &&) = delete;      // Move assign
	
public:
	// singleton: we only want one MLProcFactory, even for multiple MLDSPEngines. 
	static MLProcFactory &theFactory()  { static MLProcFactory f; return f; }
	
	typedef MLProcPtr (*MLProcCreateFnT)(void);
	typedef std::map<ml::Symbol, MLProcCreateFnT> FnRegistryT;
	FnRegistryT procRegistry;
 
	// register an object creation function by the name of the class.
	void registerFn(const ml::Symbol className, MLProcCreateFnT fn);
	
	// create a new object of the named class.  
	MLProcPtr createProc(const ml::Symbol className, MLDSPContext* context);
	
	// debug. 
	void printRegistry(void);
};

// Subclasses of MLProc make an MLProcRegistryEntry object.
// This class is passed a className and links a creation function 
// for the subclass to the className in the registry.  This way the MLProcFactory
// knows how to make them.
template <class MLProcSubclass>
class MLProcRegistryEntry
{
public:
	MLProcRegistryEntry(const char* className)
	{
		ml::Symbol classSym(className);
		MLProcFactory::theFactory().registerFn(classSym, createProcInstance);	
		MLProcInfo<MLProcSubclass>::setClassName(classSym);
	}
	
	// return shared_ptr to a new MLProc instance. 
	static MLProcPtr createProcInstance()
	{
		// NOTE sizes are quite large! 
// 		debug() << " size = " << sizeof(MLProcSubclass) << "\n";
		
//		void* newMem = malloc(sizeof(MLProcSubclass));		
//		MLProcPtr pNew(new(newMem) MLProcSubclass);		
//		return pNew;
		
		return new MLProcSubclass;
	}
};

#endif // _ML_PROC_H

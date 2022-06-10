/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "CPlusPlusCHOPExample.h"

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <assert.h>

#include "StringUtils.h"

vector<ofxAAValue> availableValues {
    RMS,
    POWER,
    ZERO_CROSSING_RATE,
    LOUDNESS,
    
    PITCH_YIN_FREQUENCY,
    PITCH_YIN_CONFIDENCE,
    
    DISSONANCE,
    HFC,
    PITCH_SALIENCE,
    
    INHARMONICITY,
    ODD_TO_EVEN,
    STRONG_PEAK,

    SILENCE_RATE_20dB,
    SILENCE_RATE_30dB,
    SILENCE_RATE_60dB,
    
    ENERGY_BAND_LOW,
    ENERGY_BAND_MID_LOW,
    ENERGY_BAND_MID_HI,
    ENERGY_BAND_HI,
    
    SPECTRAL_ROLLOFF,
    SPECTRAL_ENERGY,
    SPECTRAL_ENTROPY,
    SPECTRAL_CENTROID,
    SPECTRAL_COMPLEXITY,
    SPECTRAL_FLUX
};

// These functions are basic C function, which the DLL loader can find
// much easier than finding a C++ Class.
// The DLLEXPORT prefix is needed so the compile exports these functions from the .dll
// you are creating
extern "C"
{

DLLEXPORT
void
FillCHOPPluginInfo(CHOP_PluginInfo *info)
{
	// Always set this to CHOPCPlusPlusAPIVersion.
	info->apiVersion = CHOPCPlusPlusAPIVersion;

	// The opType is the unique name for this CHOP. It must start with a 
	// capital A-Z character, and all the following characters must lower case
	// or numbers (a-z, 0-9)
	info->customOPInfo.opType->setString("Audioanalyzer");

	// The opLabel is the text that will show up in the OP Create Dialog
	info->customOPInfo.opLabel->setString("Audio Analyzer");

	// Information about the author of this OP
	info->customOPInfo.authorName->setString("Leo Zimmerman");
	info->customOPInfo.authorEmail->setString("leozimmerman@gmail.com");

	// This CHOP can work with 0 inputs
	info->customOPInfo.minInputs = 1;

	// It can accept up to 1 input though, which changes it's behavior
	info->customOPInfo.maxInputs = 1;
}

DLLEXPORT
CHOP_CPlusPlusBase*
CreateCHOPInstance(const OP_NodeInfo* info)
{
	// Return a new instance of your class every time this is called.
	// It will be called once per CHOP that is using the .dll
	return new CPlusPlusCHOPExample(info);
}

DLLEXPORT
void
DestroyCHOPInstance(CHOP_CPlusPlusBase* instance)
{
	// Delete the instance here, this will be called when
	// Touch is shutting down, when the CHOP using that instance is deleted, or
	// if the CHOP loads a different DLL
	delete (CPlusPlusCHOPExample*)instance;
}

};


CPlusPlusCHOPExample::CPlusPlusCHOPExample(const OP_NodeInfo* info) : myNodeInfo(info)
{
	myExecuteCount = 0;
	myOffset = 0.0;
}

CPlusPlusCHOPExample::~CPlusPlusCHOPExample()
{

}

void
CPlusPlusCHOPExample::getGeneralInfo(CHOP_GeneralInfo* ginfo, const OP_Inputs* inputs, void* reserved1)
{
	// This will cause the node to cook every frame
	ginfo->cookEveryFrameIfAsked = true;

	// Note: To disable timeslicing you'll need to turn this off, as well as ensure that
	// getOutputInfo() returns true, and likely also set the info->numSamples to how many
	// samples you want to generate for this CHOP. Otherwise it'll take on length of the
	// input CHOP, which may be timesliced.
	ginfo->timeslice = true;

	ginfo->inputMatchIndex = 0;
}

bool
CPlusPlusCHOPExample::getOutputInfo(CHOP_OutputInfo* info, const OP_Inputs* inputs, void* reserved1)
{
	// If there is an input connected, we are going to match it's channel names etc
	// otherwise we'll specify our own.
	
    info->numChannels = availableValues.size();

    // Since we are outputting a timeslice, the system will dictate
    // the numSamples and startIndex of the CHOP data
    //info->numSamples = 1;
    //info->startIndex = 0
    
    // For illustration we are going to output 120hz data
    info->sampleRate = 60;
    return true;
	
}

void
CPlusPlusCHOPExample::getChannelName(int32_t index, OP_String *name, const OP_Inputs* inputs, void* reserved1)
{
    if (index >= availableValues.size()) { return ;}
	name->setString(utils::valueTypeToString(availableValues[index]));
}

void
CPlusPlusCHOPExample::execute(CHOP_Output* output,
                              const OP_Inputs* inputs,
                              void* reserved)
{
    myExecuteCount++;
    
    double smoothing = inputs->getParDouble("Smoothing");
    double alpha = inputs->getParDouble("Alpha");
    double timeThresh = inputs->getParDouble("Timethreshold");
    double silenceThresh = inputs->getParDouble("Silencethreshold");
    
    // One and only one input needed
    if (inputs->getNumInputs() != 1) {
        return;
    }
    
    const OP_CHOPInput	*cinput = inputs->getInputCHOP(0);
    
    auto sampleRate = cinput->sampleRate;
    auto bufferSize = cinput->numSamples;
    auto channels = cinput->numChannels;
    
    bool isValidBuffer = (bufferSize % 2 == 0);
    if (!isValidBuffer || availableValues.size() != output->numChannels) {
        return;
    }
    audioAnalyzer.setup(sampleRate, bufferSize, channels);
    audioAnalyzer.setOnsetsParameters(0, alpha, silenceThresh, timeThresh);
    audioAnalyzer.analyze(*cinput);
    
    for (int i = 0 ; i < output->numChannels; i++)
    {
        for (int j = 0; j < output->numSamples; j++)
        {
            auto valueType = availableValues[i];
            output->channels[i][j] = audioAnalyzer.getAverageValue(valueType, smoothing, false);
        }
    }
}

int32_t
CPlusPlusCHOPExample::getNumInfoCHOPChans(void * reserved1)
{
	// We return the number of channel we want to output to any Info CHOP
	// connected to the CHOP. In this example we are just going to send one channel.
	return 2;
}

void
CPlusPlusCHOPExample::getInfoCHOPChan(int32_t index,
										OP_InfoCHOPChan* chan,
										void* reserved1)
{
	// This function will be called once for each channel we said we'd want to return
	// In this example it'll only be called once.

	if (index == 0)
	{
		chan->name->setString("executeCount");
		chan->value = (float)myExecuteCount;
	}

	if (index == 1)
	{
		chan->name->setString("offset");
		chan->value = (float)myOffset;
	}
}

bool		
CPlusPlusCHOPExample::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1)
{
	infoSize->rows = 2;
	infoSize->cols = 2;
	// Setting this to false means we'll be assigning values to the table
	// one row at a time. True means we'll do it one column at a time.
	infoSize->byColumn = false;
	return true;
}

void
CPlusPlusCHOPExample::getInfoDATEntries(int32_t index,
										int32_t nEntries,
										OP_InfoDATEntries* entries, 
										void* reserved1)
{
	char tempBuffer[4096];

	if (index == 0)
	{
		// Set the value for the first column
		entries->values[0]->setString("executeCount");

		// Set the value for the second column
#ifdef _WIN32
		sprintf_s(tempBuffer, "%d", myExecuteCount);
#else // macOS
		snprintf(tempBuffer, sizeof(tempBuffer), "%d", myExecuteCount);
#endif
		entries->values[1]->setString(tempBuffer);
	}

	if (index == 1)
	{
		// Set the value for the first column
		entries->values[0]->setString("offset");

		// Set the value for the second column
#ifdef _WIN32
		sprintf_s(tempBuffer, "%g", myOffset);
#else // macOS
		snprintf(tempBuffer, sizeof(tempBuffer), "%g", myOffset);
#endif
		entries->values[1]->setString( tempBuffer);
	}
}

void
CPlusPlusCHOPExample::setupParameters(OP_ParameterManager* manager, void *reserved1)
{
	// Smoothing
	{
		OP_NumericParameter	np;

		np.name = "Smoothing";
		np.label = "Smoothing";
		np.defaultValues[0] = 0.0;
		np.minSliders[0] = 0.0;
		np.maxSliders[0] =  1.0;
		
		OP_ParAppendResult res = manager->appendFloat(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// Alpha
	{
		OP_NumericParameter	np;

		np.name = "Alpha";
		np.label = "Alpha";
		np.defaultValues[0] = 0.0;
		np.minSliders[0] = 0.0;
		np.maxSliders[0] =  1.0;
		
		OP_ParAppendResult res = manager->appendFloat(np);
		assert(res == OP_ParAppendResult::Success);
	}

    // Alpha
    {
        OP_NumericParameter    np;

        np.name = "Timethreshold";
        np.label = "Time Threshold";
        np.defaultValues[0] = 0.0;
        np.minSliders[0] = 0.0;
        np.maxSliders[0] =  1000.0;
        
        OP_ParAppendResult res = manager->appendFloat(np);
        assert(res == OP_ParAppendResult::Success);
    }
    
    // Alpha
    {
        OP_NumericParameter    np;

        np.name = "Silencethreshold";
        np.label = "Silence Threshold";
        np.defaultValues[0] = 0.0;
        np.minSliders[0] = 0.0;
        np.maxSliders[0] =  1;
        
        OP_ParAppendResult res = manager->appendFloat(np);
        assert(res == OP_ParAppendResult::Success);
    }

	// pulse
	{
		OP_NumericParameter	np;

		np.name = "Resetonsets";
		np.label = "Reset Onsets";
		
		OP_ParAppendResult res = manager->appendPulse(np);
		assert(res == OP_ParAppendResult::Success);
	}

}

void 
CPlusPlusCHOPExample::pulsePressed(const char* name, void* reserved1)
{
	if (!strcmp(name, "Resetonsets"))
	{
        audioAnalyzer.resetOnsets(0);
	}
}


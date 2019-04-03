#include "PluginProcessor.h"
#include "PluginEditor.h"

// Constructor
ChorusFlangerAudioProcessor::ChorusFlangerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
	// Construct and add parameters
	addParameter(mDryWetParameter = new AudioParameterFloat("dryWet", "Dry Wet", 0.0f, 1.0f, 0.0f));
	addParameter(mDepthParameter = new AudioParameterFloat("depth", "Depth", 0.0f, 1.0f, 0.5f));
	addParameter(mRateParameter = new AudioParameterFloat("rate", "Rate", 0.1f, 20.0f, 10.0f));
	addParameter(mPhaseOffsetParameter = new AudioParameterFloat("phaseOffset", "Phase Offset", 0.0f, 1.0f, 0.0f));
	addParameter(mFeedbackParameter = new AudioParameterFloat("feedback", "Feedback", 0.0f, 0.98f, 0.0f));
	addParameter(mTypeParameter = new AudioParameterFloat("type", "Type", 0, 1, 0));

	// Initialize variables
	mCircularBufferLeft = nullptr;
	mCircularBufferRight = nullptr;
	mCircularBufferWriteHead = 0;
	mCircularBufferLength = 0;
	mDelayReadHead = 0;
	mFeedbackLeft = 0;
	mFeedbackRight = 0;
	mLFOPhase = 0;
}

// Destructor
ChorusFlangerAudioProcessor::~ChorusFlangerAudioProcessor()
{
	// Delete left and right delay buffers
	if (mCircularBufferLeft != nullptr)
	{
		delete[] mCircularBufferLeft;
		mCircularBufferLeft == nullptr;
	}

	if (mCircularBufferLeft != nullptr)
	{
		delete[] mCircularBufferRight;
		mCircularBufferRight == nullptr;
	}
}

// Plugin instantiation function
void ChorusFlangerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	// Initialize phase
	mLFOPhase = 0;

	// Calculate circular buffer length
	mCircularBufferLength = sampleRate * MAX_DELAY_TIME;

	// Delete left delay buffers
	if (mCircularBufferLeft != nullptr)
	{
		delete[] mCircularBufferLeft;
		mCircularBufferLeft == nullptr;
	}

	// Initialize left buffer
	if (mCircularBufferLeft == nullptr)
		mCircularBufferLeft = new float[(int)mCircularBufferLength];

	// Clear any junk data
	zeromem(mCircularBufferLeft, mCircularBufferLength * sizeof(float));

	// Delete right buffer
	if (mCircularBufferLeft != nullptr)
	{
		delete[] mCircularBufferRight;
		mCircularBufferRight == nullptr;
	}

	// Initialize right buffer
	if (mCircularBufferRight == nullptr)
		mCircularBufferRight = new float[(int)mCircularBufferLength];

	// Clear any junk data
	zeromem(mCircularBufferRight, mCircularBufferLength * sizeof(float));

	// Initialize write data
	mCircularBufferWriteHead = 0;
}

// Main audio processing algorithm
void ChorusFlangerAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any garbage data from output buffers
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
	
	// Get left and right channel buffers
	float* leftChannel = buffer.getWritePointer(0);
	float* rightChannel = buffer.getWritePointer(1);

	// Iterate through all samples in input buffer
	for (int i = 0; i < buffer.getNumSamples(); i++)
	{
		// Write sample and any feedback into delay buffers
		mCircularBufferLeft[mCircularBufferWriteHead] = leftChannel[i] + mFeedbackLeft;
		mCircularBufferRight[mCircularBufferWriteHead] = rightChannel[i] + mFeedbackRight;

		mCircularBufferWriteHead = updateWriteHead();

		// Configure LFO for effect processing
		auto lfo = generateLFO();
		mLFOPhase = updateLFOPhase();

		// Apply Depth setting to LFO
		lfo.left *= *mDepthParameter;
		lfo.right *= *mDepthParameter;

		// Map LFO range to millisecond range according to Chorus or Flanger effect
		auto lfoMapped = (*mTypeParameter == 0)
			? processChorus(lfo.left, lfo.right)
			: processFlanger(lfo.left, lfo.right);

		// Determine where to read from delay buffer
		auto delayReadHead = getDelayReadHead(lfoMapped);
		auto interpHeads = getInterpHeads(delayReadHead);

		// Interpolate and read from delay buffer
		auto delay_sample_left = lin_interp(mCircularBufferLeft[interpHeads.currentLeft], mCircularBufferLeft[interpHeads.nextRight], interpHeads.fractionLeft);
		auto delay_sample_right = lin_interp(mCircularBufferRight[interpHeads.currentRight], mCircularBufferRight[interpHeads.nextRight], interpHeads.fractionRight);

		// Store delayed samples as feedback
		mFeedbackLeft = delay_sample_left * *mFeedbackParameter;
		mFeedbackRight = delay_sample_right * *mFeedbackParameter;

		// Send Dry/Wet signal to audio buffer output
		buffer.setSample(0, i, buffer.getSample(0, i) * (1 - *mDryWetParameter) + delay_sample_left * (*mDryWetParameter));
		buffer.setSample(1, i, buffer.getSample(1, i) * (1 - *mDryWetParameter) + delay_sample_right * (*mDryWetParameter));
	}
}

// Retrieves plugin state information when being loaded by the host
void ChorusFlangerAudioProcessor::getStateInformation (MemoryBlock& destData)
{
	std::unique_ptr<XmlElement> xml (new XmlElement ("FlangerChorus"));

	xml->setAttribute ("DryWet", *mDryWetParameter);
	xml->setAttribute ("Depth", *mDepthParameter);
	xml->setAttribute ("Rate", *mRateParameter);
	xml->setAttribute ("Phase Offset", *mPhaseOffsetParameter);
	xml->setAttribute ("Feedback", *mFeedbackParameter);
	xml->setAttribute ("Type", *mTypeParameter);

	copyXmlToBinary(*xml, destData);
}

// Saves plugin state information whenever the host performs a "Save" operation
void ChorusFlangerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	std::unique_ptr<XmlElement> xml(getXmlFromBinary (data, sizeInBytes));

	if (xml.get() != nullptr && xml->hasTagName ("FlangerChorus"))
	{
		*mDryWetParameter = xml->getDoubleAttribute("DryWet");
		*mDepthParameter = xml->getDoubleAttribute("Depth");
		*mRateParameter = xml->getDoubleAttribute("Rate");
		*mPhaseOffsetParameter = xml->getDoubleAttribute("Phase Offset");
		*mFeedbackParameter = xml->getDoubleAttribute("Feedback");
		*mTypeParameter = xml->getIntAttribute("Type");
	}
}

//======================== Self-created functions ==============================
// Interpolate between two sample values
float ChorusFlangerAudioProcessor::lin_interp(float inSampleX, float inSampleY, float inFloatPhase)
{
	// Linear interpolation equation
	return (1 - inFloatPhase) * inSampleX + inFloatPhase * inSampleY;
}

// Generate an LFO for creating a chorus or flanger effect
ChorusFlangerAudioProcessor::LFO ChorusFlangerAudioProcessor::generateLFO()
{
	float lfoOutLeft = std::sin(2 * double_Pi * mLFOPhase);

	// Since our plugin supports phase offset, we need an out-of-phase LFO
	float lfoPhaseRight = mLFOPhase + *mPhaseOffsetParameter;

	if (lfoPhaseRight >= 1)
		lfoPhaseRight -= 1;

	float lfoOutRight = std::sin(2 * double_Pi * lfoPhaseRight);

	return { lfoOutLeft, lfoOutRight };
}

// Perform processing for chorus effect
ChorusFlangerAudioProcessor::LFO ChorusFlangerAudioProcessor::processChorus(float lfoLeft, float lfoRight)
{
	// Map LFO values to range in milliseconds for chorus
	float lfoOutMappedLeft = jmap(lfoLeft, -1.f, 1.f, 0.005f, 0.03f);
	float lfoOutMappedRight = jmap(lfoRight, -1.f, 1.f, 0.005f, 0.03f);

	return { lfoOutMappedLeft, lfoOutMappedRight };
}

// Perform processing for flanger effect
ChorusFlangerAudioProcessor::LFO ChorusFlangerAudioProcessor::processFlanger(float lfoLeft, float lfoRight)
{
	// Map LFO values to range in milliseconds for chorus
	float lfoOutMappedLeft = jmap(lfoLeft, -1.f, 1.f, 0.001f, 0.005f);
	float lfoOutMappedRight = jmap(lfoRight, -1.f, 1.f, 0.001f, 0.005f);

	return { lfoOutMappedLeft, lfoOutMappedRight };
}

// Determine read head for delay buffer based on parameter setting
ChorusFlangerAudioProcessor::delayReadHead ChorusFlangerAudioProcessor::getDelayReadHead(LFO lfo)
{
	// Calculate delay lengths in samples
	float delayTimeSamplesLeft = getSampleRate() * lfo.left;
	float delayTimeSamplesRight = getSampleRate() * lfo.right;

	// Calculate read head positions
	float delayReadHeadLeft = mCircularBufferWriteHead - delayTimeSamplesLeft;
	float delayReadHeadRight = mCircularBufferWriteHead - delayTimeSamplesRight;

	if (delayReadHeadLeft < 0)
		delayReadHeadLeft += mCircularBufferLength;

	if (delayReadHeadRight < 0)
		delayReadHeadRight += mCircularBufferLength;

	return { delayReadHeadLeft, delayReadHeadRight };
}

// Get samples from delay buffer in order to perform linear interpolation
ChorusFlangerAudioProcessor::interpHeads ChorusFlangerAudioProcessor::getInterpHeads(delayReadHead drh)
{
	// Calculate linear interpolation point for left channel
	auto currentLeft = (int)drh.left;
	auto nextLeft = currentLeft + 1;
	auto fractionLeft = drh.left - currentLeft;

	if (nextLeft >= mCircularBufferLength)
		nextLeft -= mCircularBufferLength;

	// Calculate linear interpolation point for right channel
	auto currentRight = (int)drh.right;
	auto nextRight = currentRight + 1;
	auto fractionRight = drh.right - currentRight;

	if (nextRight >= mCircularBufferLength)
		nextRight -= mCircularBufferLength;

	return { currentLeft, currentRight, nextLeft, nextRight, fractionLeft, fractionRight};
}

// Increment LFO phase for next block of samples
float ChorusFlangerAudioProcessor::updateLFOPhase()
{
	mLFOPhase += *mRateParameter / getSampleRate();

	if (mLFOPhase >= 1)
		mLFOPhase -= 1;

	return mLFOPhase;
}

// Increment write head for next block of samples
float ChorusFlangerAudioProcessor::updateWriteHead()
{
	mCircularBufferWriteHead++;

	if (mCircularBufferWriteHead >= mCircularBufferLength)
		mCircularBufferWriteHead = 0;

	return mCircularBufferWriteHead;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new ChorusFlangerAudioProcessor();
}

const String ChorusFlangerAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool ChorusFlangerAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool ChorusFlangerAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool ChorusFlangerAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double ChorusFlangerAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int ChorusFlangerAudioProcessor::getNumPrograms()
{
	return 1;
}

int ChorusFlangerAudioProcessor::getCurrentProgram()
{
	return 0;
}

void ChorusFlangerAudioProcessor::setCurrentProgram(int index)
{
}

const String ChorusFlangerAudioProcessor::getProgramName(int index)
{
	return {};
}

void ChorusFlangerAudioProcessor::changeProgramName(int index, const String& newName)
{
}

bool ChorusFlangerAudioProcessor::hasEditor() const
{
	return true;
}

AudioProcessorEditor* ChorusFlangerAudioProcessor::createEditor()
{
	return new ChorusFlangerAudioProcessorEditor(*this);
}

void ChorusFlangerAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ChorusFlangerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	ignoreUnused(layouts);
	return true;
#else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
		&& layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
		return false;

	// This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
#endif

	return true;
#endif
}
#endif
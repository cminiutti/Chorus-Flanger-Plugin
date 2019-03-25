#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#define MAX_DELAY_TIME 2

//==============================================================================
class ChorusFlangerAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    ChorusFlangerAudioProcessor();
    ~ChorusFlangerAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

	//========================= Self-created functions =============================
	float lin_interp(float inSampleX, float inSampleY, float inFloatPhase);
	float updateLFOPhase();
	float updateWriteHead();

	struct LFO
	{
		float left, right;
	};
	LFO generateLFO();
	LFO processChorus(float lfoLeft, float lfoRight);
	LFO processFlanger(float lfoLeft, float lfoRight);

	struct delayReadHead
	{
		float left, right;
	};
	delayReadHead getDelayReadHead(LFO lfo);

	struct interpHeads
	{
		int currentLeft, currentRight, nextLeft, nextRight;
		float fractionLeft, fractionRight;
	};
	interpHeads getInterpHeads(delayReadHead drh);

private:
	// Circular buffers used for delay
	float* mCircularBufferLeft;
	float* mCircularBufferRight;

	// Delay buffer size
	int mCircularBufferLength;

	// Write head for delay buffer
	int mCircularBufferWriteHead;

	// Read head for delay buffer
	float mDelayReadHead;

	// Feedback variables
	float mFeedbackLeft, mFeedbackRight;

	// Phase of LFO
	float mLFOPhase;

	// Plugin parameters
	AudioParameterFloat* mRateParameter;
	AudioParameterFloat* mDepthParameter;
	AudioParameterFloat* mPhaseOffsetParameter;
	AudioParameterFloat* mDryWetParameter;
	AudioParameterFloat* mFeedbackParameter;
	AudioParameterFloat* mTypeParameter;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusFlangerAudioProcessor)
};

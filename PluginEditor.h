#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
class ChorusFlangerAudioProcessorEditor : public AudioProcessorEditor,
										  public Timer
{
public:
    ChorusFlangerAudioProcessorEditor (ChorusFlangerAudioProcessor&);
    ~ChorusFlangerAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
	void timerCallback() override;

	//=========================Self-Created Functions===============================
	void setSlider(Slider& slider, Label& label, AudioParameterFloat* parameter, const String &name, Rectangle<int> bounds);
	void setComboBox(ComboBox& comboBox, AudioParameterFloat* parameter, const String &item1, const String &item2, Rectangle<int> bounds);

	enum Type {normal, left, right, feedback1, feedback2};

	struct Ellipse
	{
		float x, y, width, height, opacity, thickness;
		Type type;
		bool boundsFlag;
		int offset, counter;
	};

	void updateEllipseLocations(Ellipse *ellipseArray);
	void paintEllipses(Ellipse *ellipseArray, Graphics& g);
	void resetEllipses(Ellipse *ellipseArray);
	void updateCounters(Ellipse *ellipseArray);

	void drawGUI(Graphics &g);
	void animateGUI(Ellipse *ellipseArray, Graphics& g);

private:
    // Reference to processor object that created the editor
    ChorusFlangerAudioProcessor& processor;

	// Plugin sliders
	Slider mDryWetSlider, mDepthSlider, mRateSlider, mPhaseOffsetSlider, mFeedbackSlider;

	// Labels for sliders
	Label mFeedbackLabel, mDryWetLabel, mDepthLabel, mRateLabel, mPhaseOffsetLabel;

	// Plugin combo box
	ComboBox mType;

	// Ellipses for GUI animation
	Ellipse  mEllipse1, mEllipse2, mEllipse3, mEllipse4, mEllipseF1, mEllipseF2, mEllipseLeft, mEllipseRight;

	// Array of ellipses to be painted to GUI
	Ellipse* ellipses = nullptr;

	// Rate for timer callback function
	int mTimerRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusFlangerAudioProcessorEditor)
};

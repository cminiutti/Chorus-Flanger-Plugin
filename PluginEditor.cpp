#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ChorusFlangerAudioProcessorEditor::ChorusFlangerAudioProcessorEditor(ChorusFlangerAudioProcessor& p)
	: AudioProcessorEditor(&p), processor(p)
{
	// Set size of plugin window
	setSize(500, 400);

	// Initialize timer rate
	mTimerRate = 0;

	// Retrieve reference to plugin parameters
	auto& params = processor.getParameters();


	// Set pointer to Dry/Wet parameter
	AudioParameterFloat* dryWetParameter = (AudioParameterFloat*)params.getUnchecked(0);

	// Set Dry/Wet slider
	setSlider(mDryWetSlider, mDryWetLabel, dryWetParameter, "Dry/Wet", Rectangle<int>(0, 250, 100, 100));

	// Define slider functionality
	mDryWetSlider.onValueChange = [this, dryWetParameter]{*dryWetParameter = mDryWetSlider.getValue(); };
	mDryWetSlider.onDragStart = [dryWetParameter] {dryWetParameter->beginChangeGesture(); };
	mDryWetSlider.onDragEnd = [dryWetParameter] {dryWetParameter->endChangeGesture(); };


	// Set pointer to Depth parameter
	AudioParameterFloat* depthParameter = (AudioParameterFloat*)params.getUnchecked(1);

	// Set Depth slider
	setSlider(mDepthSlider, mDepthLabel, depthParameter, "Depth", Rectangle<int>(100, 250, 100, 100));

	// Define slider functionality
	mDepthSlider.onValueChange = [this, depthParameter]{*depthParameter = mDepthSlider.getValue(); };
	mDepthSlider.onDragStart = [depthParameter] {depthParameter->beginChangeGesture(); };
	mDepthSlider.onDragEnd = [depthParameter] {depthParameter->endChangeGesture(); };


	// Set pointer to Rate parameter
	AudioParameterFloat* rateParameter = (AudioParameterFloat*)params.getUnchecked(2);

	// Set Rate slider
	setSlider(mRateSlider, mRateLabel, rateParameter, "Rate", Rectangle<int>(200, 250, 100, 100));

	// Define slider functionality
	mRateSlider.onValueChange = [this, rateParameter]
	{
		*rateParameter = mRateSlider.getValue();

		// Set timer rate and start new timer for repaint callback function
		mTimerRate = (int)((mRateSlider.getValue()) * 5);

		if (mTimerRate < 5)
			mTimerRate = 5;

		startTimerHz(mTimerRate);
	};
	mRateSlider.onDragStart = [rateParameter] {rateParameter->beginChangeGesture(); };
	mRateSlider.onDragEnd = [rateParameter] {rateParameter->endChangeGesture(); };


	// Set pointer to Phase Offset parameter
	AudioParameterFloat* phaseOffsetParameter = (AudioParameterFloat*)params.getUnchecked(3);

	// Set Phase Offset slider
	setSlider(mPhaseOffsetSlider, mPhaseOffsetLabel, phaseOffsetParameter, "Phase Offset", Rectangle<int>(300, 250, 100, 100));

	// Define slider functionality
	mPhaseOffsetSlider.onValueChange = [this, phaseOffsetParameter]{*phaseOffsetParameter = mPhaseOffsetSlider.getValue(); };
	mPhaseOffsetSlider.onDragStart = [phaseOffsetParameter] {phaseOffsetParameter->beginChangeGesture(); };
	mPhaseOffsetSlider.onDragEnd = [phaseOffsetParameter] {phaseOffsetParameter->endChangeGesture(); };


	// Set pointer to Feedback parameter
	AudioParameterFloat* feedbackParameter = (AudioParameterFloat*)params.getUnchecked(4);

	// Set Feedback slider
	setSlider(mFeedbackSlider, mFeedbackLabel, feedbackParameter, "Feedback", Rectangle<int>(400, 250, 100, 100));

	// Define slider functionality
	mFeedbackSlider.onValueChange = [this, feedbackParameter]{*feedbackParameter = mFeedbackSlider.getValue(); };
	mFeedbackSlider.onDragStart = [feedbackParameter] {feedbackParameter->beginChangeGesture(); };
	mFeedbackSlider.onDragEnd = [feedbackParameter] {feedbackParameter->endChangeGesture(); };


	// Set pointer to Type parameter
	AudioParameterFloat* typeParameter = (AudioParameterFloat*)params.getUnchecked(5);

	// Set Type combo box
	setComboBox(mType, typeParameter, "Chorus", "Flanger", Rectangle<int>(400, 10, 100, 20));

	// Define combo box functionality
	mType.onChange = [this, typeParameter]
	{
		typeParameter->beginChangeGesture();
		*typeParameter = mType.getSelectedItemIndex();
		typeParameter->endChangeGesture();
	};


	// Initialize set of Ellipses
	ellipses = new Ellipse[8];

	Ellipse ellipseAssignment[8] = { mEllipse1, mEllipse2, mEllipse3, mEllipse4, mEllipseLeft, mEllipseRight, mEllipseF1, mEllipseF2 };
	Type ellipseTypes[8] = { normal, normal, normal, normal, left, right, feedback1, feedback2 };
	int ellipseCounters[8] = { 0, 20, 40, 60, 0, 0, 10, 30 };

	for (int i = 0; i < 8; i++)
	{
		ellipses[i] = ellipseAssignment[i];
		ellipses[i].type = ellipseTypes[i];
		ellipses[i].counter = ellipseCounters[i];
		ellipses[i].offset = 0;
		ellipses[i].boundsFlag = true;
	}

	resetEllipses(ellipses);
}

ChorusFlangerAudioProcessorEditor::~ChorusFlangerAudioProcessorEditor()
{
}

//==============================================================================
void ChorusFlangerAudioProcessorEditor::paint (Graphics& g)
{
	drawGUI(g);
	animateGUI(ellipses, g);
}

void ChorusFlangerAudioProcessorEditor::timerCallback()
{
	repaint();
}

void ChorusFlangerAudioProcessorEditor::resized()
{
}

//=========================Self-Created Functions===============================
void ChorusFlangerAudioProcessorEditor::setSlider(Slider& slider, Label& label, AudioParameterFloat* parameter, const String &name, Rectangle<int> bounds)
{
	// Configure slider
	getLookAndFeel().setColour(Slider::thumbColourId, Colour(0xfffff800));
	slider.setBounds(bounds);
	slider.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
	slider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
	slider.setRange(parameter->range.start, parameter->range.end);
	slider.setValue(*parameter);
	addAndMakeVisible(slider);

	// Adjust bounds for label
	bounds.setY(bounds.getY() + 100);
	bounds.setHeight(bounds.getHeight() - 70);

	// Configure label
	label.setColour(Label::textColourId, Colour(0xff99f7f0));
	label.setFont(Font("Ravie", 20.0f, Font::plain));
	label.setBounds(bounds);
	label.setText(name, dontSendNotification);
	addAndMakeVisible(label);
}

void ChorusFlangerAudioProcessorEditor::setComboBox(ComboBox& comboBox, AudioParameterFloat* parameter, const String &item1, const String &item2, Rectangle<int> bounds) 
{
	// Configure combo box object
	comboBox.setBounds(bounds);
	comboBox.addItem(item1, 1);
	comboBox.addItem(item2, 2);
	comboBox.setSelectedItemIndex(*parameter);
	addAndMakeVisible(comboBox);
}

void ChorusFlangerAudioProcessorEditor::paintEllipses(Ellipse *ellipseArray, Graphics& g)
{
	// Setup color for ellipses
	g.setColour(Colour(0xff22f07f));

	for (int i = 0; i < 8; i++)
	{
		if (ellipseArray[i].offset >= ellipseArray[i].counter)
		{
			// Draw ellipse
			g.setOpacity(ellipseArray[i].opacity);
			g.drawEllipse(ellipseArray[i].x, ellipseArray[i].y, ellipseArray[i].width, ellipseArray[i].height, ellipseArray[i].thickness);
		}
	}
}

void ChorusFlangerAudioProcessorEditor::updateEllipseLocations(Ellipse *ellipseArray)
{
	for (int i = 0; i < 8; i++)
	{
		// Enforce ellipse offset
		if (ellipseArray[i].offset >= ellipseArray[i].counter)
		{
			ellipseArray[i].x -= 5;
			ellipseArray[i].y -= 5;
			ellipseArray[i].width += 10;
			ellipseArray[i].height += 10;
			ellipseArray[i].opacity -= 0.010;
			if (ellipseArray[i].opacity <= 0)
				ellipseArray[i].opacity = 0;
			ellipseArray[i].thickness -= 0.025;
	
			switch (ellipseArray[i].type)
			{
			// Normal and feedback ellipse types
			case 0:
			case 3:
			case 4:
				if (ellipseArray[i].x <= -100)
					ellipseArray[i].boundsFlag = true;
				break;

			// Left type
			case 1:
				if (ellipseArray[i].x <= -250)
					ellipseArray[i].boundsFlag = true;
				break;

			// Right type
			case 2:
				if (ellipseArray[i].x <= 50)
					ellipseArray[i].boundsFlag = true;
				break;
			}
		}
	}
}

void ChorusFlangerAudioProcessorEditor::resetEllipses(Ellipse *ellipseArray)
{
	for (int i = 0; i < 8; i++)
	{
		// Once ellipse has gone off screen
		if (ellipseArray[i].boundsFlag == true)
		{
			// Set ellipse dimensions
			ellipseArray[i].y = 150;
			ellipseArray[i].width = 100;
			ellipseArray[i].height = 100;
			ellipseArray[i].thickness = mDepthSlider.getValue() * 30.0f;

			switch (ellipseArray[i].type)
			{
			// Normal type
			case 0:
				ellipseArray[i].x = 200;
				ellipseArray[i].opacity = mDryWetSlider.getValue();
				break;

			// Left type
			case 1:
				ellipseArray[i].x = 50;
				ellipseArray[i].opacity = (mDryWetSlider.getValue() <= mPhaseOffsetSlider.getValue()) ?
					mDryWetSlider.getValue() : mPhaseOffsetSlider.getValue();
				break;

			// Right type
			case 2:
				ellipseArray[i].x = 350;
				ellipseArray[i].opacity = (mDryWetSlider.getValue() <= mPhaseOffsetSlider.getValue()) ?
					mDryWetSlider.getValue() : mPhaseOffsetSlider.getValue();
				break;

			// Feedback 1 type
			case 3:
				ellipseArray[i].x = 200;
				ellipseArray[i].opacity = (mDryWetSlider.getValue() <= mFeedbackSlider.getValue()) ?
					mDryWetSlider.getValue() : mFeedbackSlider.getValue();
				break;
			// Feedback 2 type
			case 4:
				ellipseArray[i].x = 200;
				ellipseArray[i].opacity = (mDryWetSlider.getValue() <= mFeedbackSlider.getValue()) ?
					mDryWetSlider.getValue() : mFeedbackSlider.getValue() / 2;
				break;
			}

			// Reset bounds flag
			ellipseArray[i].boundsFlag = false;
		}
	}
}

void ChorusFlangerAudioProcessorEditor::updateCounters(Ellipse *ellipseArray)
{
	for (int i = 0; i < 8; i++)
	{
		if (ellipseArray[i].offset <= ellipseArray[i].counter)
		{
			ellipseArray[i].offset++;
		}
	}
}

void ChorusFlangerAudioProcessorEditor::drawGUI(Graphics& g)
{
	// Fill background color
	g.fillAll(Colours::black);

	// Draw title text
	if (mType.getSelectedItemIndex() == 0)
	{
		g.setColour(Colour(0xff99f7f0));
		g.setOpacity(0.8);
		g.setFont(Font("Bauhaus 93", 90.0f, Font::plain));
		g.drawText("Chorus", 50, 130, 400, 100, Justification::centred, true);
	}
	else
	{
		g.setColour(Colour(0xff99f7f0));
		g.setOpacity(0.8);
		g.setFont(Font("Magneto", 90.0f, Font::plain));
		g.drawText("Flanger", 50, 130, 400, 100, Justification::centred, true);
	}
}

void ChorusFlangerAudioProcessorEditor::animateGUI(Ellipse *ellipseArray, Graphics& g)
{
	updateCounters(ellipses);
	paintEllipses(ellipses, g);
	updateEllipseLocations(ellipses);
	resetEllipses(ellipses);
}
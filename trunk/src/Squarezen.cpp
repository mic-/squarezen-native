/*
 * Copyright 2013 Mic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "Squarezen.h"
#include "SquarezenFrame.h"

using namespace Tizen::App;
using namespace Tizen::Base;
using namespace Tizen::Media;
using namespace Tizen::System;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;


SquarezenApp::SquarezenApp(void) :
		mPlayer(NULL)
{
}

SquarezenApp::~SquarezenApp(void)
{
}

UiApp*
SquarezenApp::CreateInstance(void)
{
	// Create the instance through the constructor.
	return new SquarezenApp();
}

bool
SquarezenApp::OnAppInitializing(AppRegistry& appRegistry)
{
	// TODO:
	// Initialize Frame and App specific data.
	// The App's permanent data and context can be obtained from the appRegistry.
	//
	// If this method is successful, return true; otherwise, return false.
	// If this method returns false, the App will be terminated.

	// Uncomment the following statement to listen to the screen on/off events.
	//PowerManager::SetScreenEventListener(*this);

	result r;

    r = mAudioOut.Construct(*this);
    if (IsFailed(r)) {
        return false;
    }
    mAudioOut.Prepare(AUDIO_TYPE_PCM_S16_LE, AUDIO_CHANNEL_TYPE_STEREO, 44100);
    mMinBufferSize = mAudioOut.GetMinBufferSize() * 4;

    mBuffers[0].Construct(mMinBufferSize);
    mBuffers[1].Construct(mMinBufferSize);

    Tizen::Base::String extStoragePath = Tizen::System::Environment::GetExternalStoragePath();
    extStoragePath += L"Stork2.vgm";

    mPlayer = new VgmPlayer();

    AppLog("Preparing file %S", extStoragePath.GetPointer());
    if (IsFailed(mPlayer->Prepare(extStoragePath))) {
    	AppLog("Prepare failed");
    	return false;
    }

    // Fill up both buffers with audio data and send the first one to the audio hw
	mPlayer->Run(mMinBufferSize >> 2, (int16_t*)mBuffers[0].GetPointer());
	mPlayer->Run(mMinBufferSize >> 2, (int16_t*)mBuffers[1].GetPointer());
	mAudioOut.WriteBuffer(mBuffers[0]);
	mCurPlayingBuffer = 0;

	if (IsFailed(mAudioOut.Start())) {
		return false;
    }

	return true;
}


void SquarezenApp::OnAudioOutBufferEndReached(Tizen::Media::AudioOut& src)
{
	// Switch buffers and fill up with new audio data
    mAudioOut.WriteBuffer(mBuffers[mCurPlayingBuffer ^ 1]);
    mPlayer->Run(mMinBufferSize >> 2, (int16_t*)mBuffers[mCurPlayingBuffer].GetPointer());
    mCurPlayingBuffer ^= 1;
}


bool
SquarezenApp::OnAppInitialized(void)
{
	// TODO:
	// Comment.

	// Create a Frame
	SquarezenFrame* pSquarezenFrame = new SquarezenFrame();
	pSquarezenFrame->Construct();
	pSquarezenFrame->SetName(L"Squarezen");
	AddFrame(*pSquarezenFrame);

	return true;
}

bool
SquarezenApp::OnAppWillTerminate(void)
{
	// TODO:
	// Comment.
	return true;
}

bool
SquarezenApp::OnAppTerminating(AppRegistry& appRegistry, bool forcedTermination)
{
	// TODO:
	// Deallocate resources allocated by this App for termination.
	// The App's permanent data and context can be saved via appRegistry.

	mAudioOut.Stop();
	mAudioOut.Unprepare();
	mPlayer->Reset();
	delete mPlayer;
	mPlayer = NULL;

	return true;
}

void
SquarezenApp::OnForeground(void)
{
	// TODO:
	// Start or resume drawing when the application is moved to the foreground.
}

void
SquarezenApp::OnBackground(void)
{
	// TODO:
	// Stop drawing when the application is moved to the background.
}

void
SquarezenApp::OnLowMemory(void)
{
	// TODO:
	// Free unused resources or close the application.
}

void
SquarezenApp::OnBatteryLevelChanged(BatteryLevel batteryLevel)
{
	// TODO:
	// Handle any changes in battery level here.
	// Stop using multimedia features(camera, mp3 etc.) if the battery level is CRITICAL.
}

void
SquarezenApp::OnScreenOn(void)
{
	// TODO:
	// Get the released resources or resume the operations that were paused or stopped in OnScreenOff().
}

void
SquarezenApp::OnScreenOff(void)
{
	// TODO:
	// Unless there is a strong reason to do otherwise, release resources (such as 3D, media, and sensors) to allow the device
	// to enter the sleep mode to save the battery.
	// Invoking a lengthy asynchronous method within this listener method can be risky, because it is not guaranteed to invoke a
	// callback before the device enters the sleep mode.
	// Similarly, do not perform lengthy operations in this listener method. Any operation must be a quick one.
}

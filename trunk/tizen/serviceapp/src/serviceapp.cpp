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

#include "serviceapp.h"
#include "../../../emu-players/GbsPlayer.h"
#include "../../../emu-players/VgmPlayer.h"
#include "../../../emu-players/YmPlayer.h"

using namespace Tizen::App;
using namespace Tizen::Base;
using namespace Tizen::Base::Collection;
using namespace Tizen::Media;
using namespace Tizen::System;

static const wchar_t* LOCAL_MESSAGE_PORT_NAME = L"SQZSERVICE_PORT";

serviceappApp::serviceappApp(void)
	: mPlayer(NULL)
	, mAudioPlaybackSession(0)
	, mAudioCallbackSession(1)
{
}

serviceappApp::~serviceappApp(void)
{
}

ServiceApp*
serviceappApp::CreateInstance(void)
{
	// Create the instance through the constructor.
	return new serviceappApp();
}


bool
serviceappApp::OnAppInitializing(AppRegistry& appRegistry)
{
	result r = E_SUCCESS;

	// Initialize ServerChannel
	mMessagePort = new (std::nothrow) SquarezenMessagePort();
	TryReturn(mMessagePort != null, false, "SquarezenService: Failed to create MessagePort.");
	AppLog("SquarezenService: MessagePort has been created.");

	r = mMessagePort->Construct(LOCAL_MESSAGE_PORT_NAME);
	TryReturn(IsFailed(r) != true, r, "SquarezenService: [%s] Failed to construct MessagePort", GetErrorMessage(r));
	AppLog("SquarezenService: MessagePort has been constructed.");

	mPlayerMutex.Create();

    r = mAudioOut.Construct(*this);
    if (IsFailed(r)) {
        return r;
    }
    mAudioOut.Prepare(AUDIO_TYPE_PCM_S16_LE, AUDIO_CHANNEL_TYPE_STEREO, 44100);
    mMinBufferSize = mAudioOut.GetMinBufferSize() * 4;

    AppLog("Buffer size: %d bytes", mMinBufferSize);

    mBuffers[0].Construct(mMinBufferSize);
    mBuffers[1].Construct(mMinBufferSize);

	return true;
}


bool
serviceappApp::OnAppInitialized(void)
{
	// TODO:
	// Comment.

	return true;
}

bool
serviceappApp::OnAppWillTerminate(void)
{
	// TODO:
	// Comment.

	return true;
}

bool
serviceappApp::OnAppTerminating(AppRegistry& appRegistry, bool forcedTermination)
{
	mAudioOut.Stop();
	mAudioOut.Unprepare();
	if (mPlayer) {
		mPlayer->Reset();
		delete mPlayer;
		mPlayer = NULL;
	}

	return true;
}

void
serviceappApp::OnLowMemory(void)
{
	// TODO:
	// Free unused resources or close the App.
}

void
serviceappApp::OnBatteryLevelChanged(BatteryLevel batteryLevel)
{
	// TODO:
	// Handle any changes in battery level here.
	// Stop using multimedia features(camera, mp3 etc.) if the battery level is CRITICAL.
}


void serviceappApp::OnAudioOutBufferEndReached(Tizen::Media::AudioOut& src)
{
	mPlayerMutex.Acquire();

	if (mAudioCallbackSession == mAudioPlaybackSession) {
		// Switch buffers and fill up with new audio data
		mAudioOut.WriteBuffer(mBuffers[mCurPlayingBuffer ^ 1]);
		mPlayer->Run(mMinBufferSize >> 2, (int16_t*)mBuffers[mCurPlayingBuffer].GetPointer());
		mCurPlayingBuffer ^= 1;
	}

	mAudioCallbackSession = mAudioPlaybackSession;

    mPlayerMutex.Release();
}


result serviceappApp::PlayFile(String *filePath) {
	mPlayerMutex.Acquire();

	if (AUDIOOUT_STATE_PLAYING == mAudioOut.GetState()) {
		if (IsFailed(mAudioOut.Stop())) {
			AppLog("AudioOut::Stop failed");
		}
		mAudioOut.Reset();
	}

	if (mPlayer) {
		mPlayer->Reset();
		delete mPlayer;
	}

	if (filePath->EndsWith(".VGM") || filePath->EndsWith(".vgm")) {
		mPlayer = new VgmPlayer;
	} else if (filePath->EndsWith(".YM") || filePath->EndsWith(".ym")) {
		mPlayer = new YmPlayer;
	} else if (filePath->EndsWith(".GBS") || filePath->EndsWith(".gbs")) {
		mPlayer = new GbsPlayer;
	} else {
		AppLog("Unrecognized file type: %S", filePath->GetPointer());
		return E_FAILURE;
	}

	std::wstring ws = std::wstring(filePath->GetPointer());
	if (IsFailed(mPlayer->Prepare(ws))) {
    	AppLog("Prepare failed");
    	return E_FAILURE;
    }

    // Fill up both buffers with audio data and send the first one to the audio hw
	mPlayer->Run(mMinBufferSize >> 2, (int16_t*)mBuffers[0].GetPointer());
	mPlayer->Run(mMinBufferSize >> 2, (int16_t*)mBuffers[1].GetPointer());
	mAudioOut.WriteBuffer(mBuffers[0]);
	mCurPlayingBuffer = 0;

	if (IsFailed(mAudioOut.Start())) {
		AppLog("AudioOut::Start failed");
		return E_FAILURE;
    }

	mAudioPlaybackSession++;

	mPlayerMutex.Release();

	return E_SUCCESS;
}


void
serviceappApp::OnUserEventReceivedN(RequestId requestId, IList* pArgs)
{
	AppLog("SquarezenService: OnUserEventReceivedN called. requestId is %d", requestId);

	switch (requestId) {
	case PLAYBACK_REQUEST:
		if (pArgs && pArgs->GetCount() >= 1) {
			PlayFile(static_cast<String *>(pArgs->GetAt(0)));
		}
		break;

	case PAUSE_UNPAUSE_REQUEST:
		break;

	case STOP_REQUEST:
		mPlayerMutex.Acquire();
		mAudioOut.Stop();
		mPlayerMutex.Release();
		break;

	/*case EXIT :
		Terminate();
		break;*/
	default:
		break;
	}
}

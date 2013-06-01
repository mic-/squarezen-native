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

#ifndef _SERVICEAPP_H_
#define _SERVICEAPP_H_

#include <FApp.h>
#include <FBase.h>
#include <FMedia.h>
#include <FSystem.h>
#include <FUi.h>
#include <FUiIme.h>
#include <FGraphics.h>
#include <gl.h>
#include "SquarezenMessagePort.h"
#include "../../../emu-players/MusicPlayer.h"


/**
 * [serviceappApp] ServiceApp must inherit from ServiceApp class
 * which provides basic features necessary to define an ServiceApp.
 */
class serviceappApp
	: public Tizen::App::ServiceApp
	, public Tizen::Media::IAudioOutEventListener
{
public:

	/**
	 * [serviceappApp] ServiceApp must have a factory method that creates an instance of itself.
	 */
	static Tizen::App::ServiceApp* CreateInstance(void);

public:

	serviceappApp(void);
	~serviceappApp(void);

public:

	// Called when the ServiceApp is initializing.
	bool OnAppInitializing(Tizen::App::AppRegistry& appRegistry);

	// Called when the ServiceApp initializing is finished.
	bool OnAppInitialized(void); 

	// Called when the ServiceApp is requested to terminate.
	bool OnAppWillTerminate(void);

	// Called when the ServiceApp is terminating.
	bool OnAppTerminating(Tizen::App::AppRegistry& appRegistry, bool forcedTermination = false);

	// Called when the system memory is not sufficient to run the ServiceApp any further.
	void OnLowMemory(void);

	// Called when the battery level changes.
	void OnBatteryLevelChanged(Tizen::System::BatteryLevel batteryLevel);

	virtual void OnUserEventReceivedN(RequestId requestId, Tizen::Base::Collection::IList* pArgs);

protected:
    virtual void OnAudioOutBufferEndReached(Tizen::Media::AudioOut& src);
    virtual void OnAudioOutErrorOccurred(Tizen::Media::AudioOut& src, result r) {}
    virtual void OnAudioOutInterrupted(Tizen::Media::AudioOut& src) {}
    virtual void OnAudioOutReleased(Tizen::Media::AudioOut& src) {}
    virtual void OnAudioOutAudioFocusChanged(Tizen::Media::AudioOut& src) {}

    result PlayFile(Tizen::Base::String *fileName);

private:
    Tizen::Base::Runtime::Mutex mPlayerMutex;
    Tizen::Media::AudioOut mAudioOut;
    MusicPlayer *mPlayer;
    Tizen::Base::ByteBuffer mBuffers[2];
    int mCurPlayingBuffer;
    int mMinBufferSize;
    int mAudioPlaybackSession, mAudioCallbackSession;

private:
	SquarezenMessagePort *mMessagePort;
};

#endif // _SERVICEAPP_H_

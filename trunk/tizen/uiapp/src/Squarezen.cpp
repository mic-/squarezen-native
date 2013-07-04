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
#include "AppResourceId.h"

using namespace Tizen::App;
using namespace Tizen::Base;
using namespace Tizen::Base::Collection;
using namespace Tizen::Io;
using namespace Tizen::Media;
using namespace Tizen::System;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;


static const wchar_t* REMOTE_PORT_NAME = L"SQZSERVICE_PORT";


SquarezenApp::SquarezenApp(void)
	: mForm(NULL), mServiceProxy(NULL), mServiceReady(false)
{
}

SquarezenApp::~SquarezenApp(void)
{
	delete mServiceProxy;
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

	return true;
}


bool
SquarezenApp::OnAppInitialized(void)
{
	// Create a Frame
	SquarezenFrame* squarezenFrame = new SquarezenFrame();
	squarezenFrame->Construct();
	squarezenFrame->SetName(L"Squarezen");
	AddFrame(*squarezenFrame);

	mForm = squarezenFrame->GetCurrentForm();
	AppAssert(mForm != null);
	mForm->SendUserEvent(STATE_READY, null);

	String serviceName(L".SquarezenService");
	String repAppId(15);

	GetAppId().SubString(0, 10, repAppId);
	AppId serviceId(repAppId+serviceName);
	AppLog("Squarezen : Service Id is %S", serviceId.GetPointer());

	result r = E_SUCCESS;
	mServiceProxy = new (std::nothrow) SquarezenServiceProxy();
	TryReturn(mServiceProxy != null, false, "Squarezen: [%s] SeviceProxy creation failed.", GetErrorMessage(r));
	r = mServiceProxy->Construct(serviceId, REMOTE_PORT_NAME);
	if (IsFailed(r)) {
		AppLog("Squarezen: [%s] SeviceProxy creation failed.", GetErrorMessage(r));
		mForm->SendUserEvent(STATE_FAIL, null);
	} else {
		mServiceReady = true;
	}

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

	return true;
}


void
SquarezenApp::OnUserEventReceivedN(RequestId requestId, IList* pArgs)
{
	AppLog("SquarezenApp: OnUserEventReceivedN is called. requestId is %d", requestId);

	result r = E_SUCCESS;

	switch (requestId) {
	case STATE_CONNECT_REQUEST :
		if (mServiceReady) {
			HashMap *map =	new HashMap(SingleObjectDeleter);
			map->Construct();
			map->Add(new String(L"Squarezen"), new String(L"connect"));

			r = mServiceProxy->SendMessage(map);

			delete map;

			TryReturnVoid(!IsFailed(r), "SquarezenApp: [%s] MessagePort Operation Failed", GetErrorMessage(r));
		}
		break;
	case STATE_CONNECTED :
		if (mServiceReady) {
			mForm->SendUserEvent(STATE_CONNECTED, null);
		}
		break;

	case STATE_PLAYBACK_REQUEST :
		if (mServiceReady) {
			HashMap *map =	new HashMap(SingleObjectDeleter);
			map->Construct();
			map->Add(new String(L"Squarezen"), new String(L"play"));
			if (pArgs != null) {
				AppLog("pArgs length = %d", pArgs->GetCount());
				map->Add(new String(L"SqzFilename"), (String*)(pArgs->GetAt(0)));
			}

			r = mServiceProxy->SendMessage(map);

			delete map;

			TryReturnVoid(!IsFailed(r), "SquarezenApp: [%s] MessagePort Operation Failed", GetErrorMessage(r));
		}
		break;

	case STATE_PLAYBACK_STARTED :
		if (mServiceReady) {
			mForm->SendUserEvent(STATE_PLAYBACK_STARTED, null);
		}
		break;

	case STATE_PLAYBACK_FINISHED :
		if (mServiceReady) {
			mForm->SendUserEvent(STATE_PLAYBACK_FINISHED, null);
		}
		break;

	case STATE_SET_SUBSONG_REQUEST :
		if (mServiceReady) {
			HashMap *map =	new HashMap(SingleObjectDeleter);
			map->Construct();
			map->Add(new String(L"Squarezen"), new String(L"set_subsong"));
			if (pArgs != null) {
				map->Add(new String(L"SqzSubSong"), (String*)(pArgs->GetAt(0)));
			}
			r = mServiceProxy->SendMessage(map);
			delete map;
			TryReturnVoid(!IsFailed(r), "SquarezenApp: [%s] MessagePort Operation Failed", GetErrorMessage(r));
		}
		break;

	case STATE_STOP_REQUEST :
		if (mServiceReady) {
			HashMap *map =	new HashMap(SingleObjectDeleter);
			map->Construct();
			map->Add(new String(L"Squarezen"), new String(L"stop"));
			r = mServiceProxy->SendMessage(map);
			delete map;
			TryReturnVoid(!IsFailed(r), "SquarezenApp: [%s] MessagePort Operation Failed", GetErrorMessage(r));
		}
		break;

	case STATE_STOPPED:
		if (mServiceReady) {
			mForm->SendUserEvent(STATE_STOPPED, null);
		}
		break;

	case STATE_SONG_METADATA_REQUEST :
		if (mServiceReady) {
			HashMap *map =	new HashMap(SingleObjectDeleter);
			map->Construct();
			map->Add(new String(L"Squarezen"), new String(L"get_song_metadata"));
			r = mServiceProxy->SendMessage(map);
			delete map;
			TryReturnVoid(!IsFailed(r), "SquarezenApp: [%s] MessagePort Operation Failed", GetErrorMessage(r));
		}
		break;

	case STATE_SONG_METADATA_RECEIVED :
		if (mServiceReady) {
			mForm->SendUserEvent(STATE_SONG_METADATA_RECEIVED, pArgs);
		}
		break;

	case STATE_EXIT :
		Terminate();
		break;
	default:
		break;
	}
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

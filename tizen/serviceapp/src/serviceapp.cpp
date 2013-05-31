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

using namespace Tizen::App;
using namespace Tizen::Base;
using namespace Tizen::Base::Collection;
using namespace Tizen::System;

static const wchar_t* LOCAL_MESSAGE_PORT_NAME = L"SQZSERVICE_PORT";

serviceappApp::serviceappApp(void)
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
	// TODO:
	// Deallocate resources allocated by this App for termination.
	// The App's permanent data and context can be saved via appRegistry.

	// TODO: Add your termination code here

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

void
serviceappApp::OnUserEventReceivedN(RequestId requestId, IList* pArgs)
{
	AppLog("SquarezenService: OnUserEventReceivedN called. requestId is %d", requestId);

	switch (requestId) {
	case PLAYBACK_REQUEST:
		break;

	case PAUSE_UNPAUSE_REQUEST:
		break;

	/*case EXIT :
		Terminate();
		break;*/
	default:
		break;
	}
}

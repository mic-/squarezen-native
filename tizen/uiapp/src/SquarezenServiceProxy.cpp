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

#include "SquarezenServiceProxy.h"

using namespace Tizen::App;
using namespace Tizen::Base;
using namespace Tizen::Base::Runtime;
using namespace Tizen::Base::Collection;
using namespace Tizen::Io;

static const int CHECK_INTERVAL = 1000;  // milliseconds

static const wchar_t* LOCAL_MESSAGE_PORT_NAME = L"UI_PORT";

SquarezenServiceProxy::SquarezenServiceProxy()
	: mLocalMessagePort(null), mRemoteMessagePort(null)
{
}

SquarezenServiceProxy::~SquarezenServiceProxy()
{
}

result SquarezenServiceProxy::Construct(const AppId& appId, const String& remotePortName)
{
	result r = E_FAILURE;

	mAppId = appId;

	AppManager* appManager = AppManager::GetInstance();
	TryReturn(appManager != null, E_FAILURE, "Squarezen: Failed to get AppManager instance.");

	AppLog("Squarezen: Checking service status for time out 5 sec...");

	for (int i = 0; i < 5; i++) {
		if (appManager->IsRunning(mAppId)) {
			AppLog("Squarezen: Service is ready.");
			break;
		} else {
			AppLog("Squarezen: Service is not ready; attempting to launch..");
			r = appManager->LaunchApplication(mAppId, null);
			TryReturn(!IsFailed(r), r, "Squarezen: [%s]", GetErrorMessage(r));
			Thread::Sleep(CHECK_INTERVAL);
		}
	}

	TryReturn(appManager->IsRunning(mAppId), E_FAILURE, "Squarezen: Unable to start service");

	mLocalMessagePort = MessagePortManager::RequestLocalMessagePort(LOCAL_MESSAGE_PORT_NAME);
	TryReturn(mLocalMessagePort != null, E_FAILURE, "[E_FAILURE] Failed to get LocalMessagePort instance.");

	mLocalMessagePort->AddMessagePortListener(*this);

	mRemoteMessagePort = MessagePortManager::RequestRemoteMessagePort(appId, remotePortName);
	TryReturn(mRemoteMessagePort != null, E_FAILURE, "[E_FAILURE] Failed to get LocalMessagePort instance.");

	mMessageArgList = new ArrayList(NoOpDeleter);
	mMessageArgList->Construct();

	AppLog("LocalMessagePort(\"%ls\") is ready", mLocalMessagePort->GetName().GetPointer());

	return E_SUCCESS;
}

result SquarezenServiceProxy::SendMessage(const IMap* message)
{
	result r = E_SUCCESS;

	if (mRemoteMessagePort != null) {
		r = mRemoteMessagePort->SendMessage(mLocalMessagePort, message);
	} else {
		r = E_FAILURE;
	}

	return r;
}

void SquarezenServiceProxy::OnMessageReceivedN(RemoteMessagePort* remoteMessagePort, IMap* message)
{
	AppLog("Squarezen: Received a message from the service");

	String key(L"SquarezenService");
	String* data = static_cast<String*>(message->GetValue(key));

	App* app = App::GetInstance();

	if (data != null && app != null)
	{
		AppLog("Squarezen: Received data : %S", data->GetPointer());

		if (data->CompareTo(L"ready") == 0) {
			app->SendUserEvent(STATE_CONNECTED, null);

		} else if (data->CompareTo(L"play_started") == 0) {
			app->SendUserEvent(STATE_PLAYBACK_STARTED, null);

		} else if (data->CompareTo(L"play_finished") == 0) {
			app->SendUserEvent(STATE_PLAYBACK_FINISHED, null);

		} else if (data->CompareTo(L"play_stopped") == 0) {
			app->SendUserEvent(STATE_STOPPED, null);

		} else if (data->CompareTo(L"song_metadata") == 0) {
			AppLog("Squarezen serviceproxy got metadata");
			if (mMessageArgList->GetCount()) {
				mMessageArgList->RemoveAll();
			}
			mMessageArgList->Add( new String(*static_cast<String*>(message->GetValue(String(L"SongTitle"))))  );
			mMessageArgList->Add( new String(*static_cast<String*>(message->GetValue(String(L"SongAuthor")))) );
			mMessageArgList->Add( new String(*static_cast<String*>(message->GetValue(String(L"SongComment")))) );
			mMessageArgList->Add( new String(*static_cast<String*>(message->GetValue(String(L"SongLength")))) );
			mMessageArgList->Add( new String(*static_cast<String*>(message->GetValue(String(L"SubSongs"))))   );
			mMessageArgList->Add( new String(*static_cast<String*>(message->GetValue(String(L"DefaultSong"))))   );
			app->SendUserEvent(STATE_SONG_METADATA_RECEIVED, mMessageArgList);

		} else if (data->CompareTo(L"exit") == 0) {
			app->SendUserEvent(STATE_EXIT, null);
		}
	}

	delete message;
}

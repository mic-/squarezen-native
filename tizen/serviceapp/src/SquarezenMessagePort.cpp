#include "SquarezenMessagePort.h"

using namespace Tizen::App;
using namespace Tizen::Base;
using namespace Tizen::Base::Collection;
using namespace Tizen::Io;


SquarezenMessagePort::SquarezenMessagePort(void)
	: mLocalMessagePort(null)
	, mRemoteMessagePort(null)
	, mPlayer(null)
{
}

SquarezenMessagePort::~SquarezenMessagePort(void)
{
}

result SquarezenMessagePort::Construct(const String& localPortName)
{
	result r = E_SUCCESS;

	mLocalMessagePort = MessagePortManager::RequestLocalMessagePort(localPortName);
	TryReturn(mLocalMessagePort != null, E_FAILURE, "SquarezenService: Failed to get LocalMessagePort instance.");

	mLocalMessagePort->AddMessagePortListener(*this);

	mMessageArgList = new ArrayList(SingleObjectDeleter);
	mMessageArgList->Construct();

	AppLog("SquarezenService: LocalMessagePort is ready.");

	return r;
}


void SquarezenMessagePort::OnMessageReceivedN(RemoteMessagePort* remoteMessagePort, IMap* message)
{
	String *data = static_cast<String *>(message->GetValue(String(L"Squarezen")));

	AppLog("SquarezenService: Received data: %S", data->GetPointer());

	if (mRemoteMessagePort == null && remoteMessagePort != null) {
		mRemoteMessagePort = remoteMessagePort;
	}

	HashMap *map =	new HashMap(SingleObjectDeleter);
	map->Construct();

	if (data->CompareTo(L"connect") == 0) {
		mRemoteMessagePort = remoteMessagePort;
		map->Add(new String(L"SquarezenService"), new String(L"ready"));

	} else if (data->CompareTo(L"play") == 0) {
		App* app = App::GetInstance();
		String *arg = static_cast<String *>(message->GetValue(String(L"SqzFilename")));
		AppLog("SquarezenService: With argument: %S", arg->GetPointer());
		if (mMessageArgList->GetCount()) mMessageArgList->RemoveAll();
		mMessageArgList->Add(new String(*arg));
		AppLog("SquarezenMessagePort: Add");
		//mMessageArgList->Add(remoteMessagePort);
		//AppLog("SquarezenMessagePort: SendUserEvent");

		app->SendUserEvent(PLAYBACK_REQUEST, mMessageArgList);
		delete map;
		delete message;
		// The reply will be sent from serviceappApp::OnUserEventReceivedN
		return;

	} else if (data->CompareTo(L"set_subsong") == 0) {
		App* app = App::GetInstance();
		String *arg = static_cast<String *>(message->GetValue(String(L"SqzSubSong")));
		AppLog("SquarezenService: With argument: %S", arg->GetPointer());
		if (mMessageArgList->GetCount()) mMessageArgList->RemoveAll();
		AppLog("SquarezenService: Add");
		mMessageArgList->Add(new String(*arg));
		//mMessageArgList->Add(remoteMessagePort);
		AppLog("SquarezenService: SendUserEvent");
		app->SendUserEvent(SET_SUBSONG_REQUEST, mMessageArgList);
		delete map;
		delete message;
		AppLog("SquarezenService: UserEvent sent");
		// The reply will be sent from serviceappApp::OnUserEventReceivedN
		return;


	} else if (data->CompareTo(L"get_song_metadata") == 0) {
		App* app = App::GetInstance();
		app->SendUserEvent(SONG_METADATA_REQUEST, null);
		delete map;
		delete message;
		// The reply will be sent from serviceappApp::OnUserEventReceivedN
		return;

	} else if (data->CompareTo(L"stop") == 0) {
		App* app = App::GetInstance();
		app->SendUserEvent(STOP_REQUEST, null);
		delete map;
		delete message;
		// The reply will be sent from serviceappApp::OnUserEventReceivedN
		return;

	} else if (data->CompareTo(L"exit") == 0) {
		App* app = App::GetInstance();
		//app->SendUserEvent(EXIT, null);
		map->Add(new String(L"SquarezenService"), new String(L"exit"));

	} else {
		map->Add(new String(L"SquarezenService"), new String(L"unsupported"));
	}

	if (remoteMessagePort != null) {
		remoteMessagePort->SendMessage(mLocalMessagePort, map);
	}

	delete map;
	delete message;
}


result SquarezenMessagePort::SendMessage(const IMap* message)
{
	result r = E_SUCCESS;

	if (mRemoteMessagePort != null) { // && mLocalMessagePort != null && message != null) {
		r = mRemoteMessagePort->SendMessage(mLocalMessagePort, message);
	} else {
		AppLog("Squarezen no remote message port set");
		r = E_FAILURE;
	}

	return r;
}

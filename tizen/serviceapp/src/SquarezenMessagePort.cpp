#include "SquarezenMessagePort.h"

using namespace Tizen::App;
using namespace Tizen::Base;
using namespace Tizen::Base::Collection;
using namespace Tizen::Io;


SquarezenMessagePort::SquarezenMessagePort(void)
	: mLocalMessagePort(null), mRemoteMessagePort(null)
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

	AppLog("SquarezenService: LocalMessagePort is ready.");

	return r;
}


void SquarezenMessagePort::OnMessageReceivedN(RemoteMessagePort* remoteMessagePort, IMap* message)
{
	String *data = static_cast<String *>(message->GetValue(String(L"Squarezen")));

	AppLog("SquarezenService: Received data: %S", data->GetPointer());

	HashMap *map =	new HashMap(SingleObjectDeleter);
	map->Construct();

	if (data->CompareTo(L"connect") == 0) {
		mRemoteMessagePort = remoteMessagePort;
		map->Add(new String(L"SquarezenService"), new String(L"ready"));

	} else if (data->CompareTo(L"start") == 0) {
		App* app = App::GetInstance();
		//app->SendUserEvent(TIMER_START, null);
		map->Add(new String(L"SquarezenService"), new String(L"started"));

	} else if (data->CompareTo(L"stop") == 0) {
		App* app = App::GetInstance();
		//app->SendUserEvent(TIMER_STOP, null);
		map->Add(new String(L"SquarezenService"), new String(L"stopped"));

	} else if (data->CompareTo(L"exit") == 0) {
		App* app = App::GetInstance();
		//app->SendUserEvent(TIMER_EXIT, null);
		map->Add(new String(L"SquarezenService"), new String(L"exit"));

	} else {
		map->Add(new String(L"SquarezenService"), new String(L"unsupported"));
	}

	remoteMessagePort->SendMessage(mLocalMessagePort, map);

	delete map;

	delete message;
}

result SquarezenMessagePort::SendMessage(const IMap* message)
{
	result r = E_SUCCESS;

	if (mRemoteMessagePort != null) {
		r = mRemoteMessagePort->SendMessage(mLocalMessagePort, message);
	} else {
		r = E_FAILURE;
	}

	return r;
}

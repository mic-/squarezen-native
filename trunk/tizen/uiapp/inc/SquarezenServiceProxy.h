/*
 * SquarezenServiceProxy.h
 *
 *  Created on: May 30, 2013
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

#ifndef SQUAREZENSERVICEPROXY_H_
#define SQUAREZENSERVICEPROXY_H_

#include <FApp.h>
#include <FBase.h>
#include <FIo.h>

static const RequestId STATE_INIT = 0;
static const RequestId STATE_READY = 1;
static const RequestId STATE_CONNECT_REQUEST = 2;
static const RequestId STATE_CONNECTED = 3;
static const RequestId STATE_PLAYBACK_REQUEST = 4;
static const RequestId STATE_PLAYBACK_STARTED = 5;
static const RequestId STATE_PLAYBACK_FINISHED = 6;
static const RequestId STATE_PAUSE_UNPAUSE_REQUEST = 7;
static const RequestId STATE_PAUSED_UNPAUSED = 8;
static const RequestId STATE_EXIT = 9;
static const RequestId STATE_FAIL = 10;

class SquarezenServiceProxy
	: public Tizen::Io::IMessagePortListener
{
public:
	SquarezenServiceProxy(void);

	~SquarezenServiceProxy(void);

	result Construct(const Tizen::App::AppId& appId, const Tizen::Base::String& remotePortName);

	result SendMessage(const Tizen::Base::Collection::IMap* message);

	virtual void OnMessageReceivedN(Tizen::Io::RemoteMessagePort* remoteMessagePort, Tizen::Base::Collection::IMap* message);

private:
	Tizen::Io::LocalMessagePort* mLocalMessagePort;
	Tizen::Io::RemoteMessagePort* mRemoteMessagePort;
	Tizen::App::AppId mAppId;
};

#endif /* SQUAREZENSERVICEPROXY_H_ */

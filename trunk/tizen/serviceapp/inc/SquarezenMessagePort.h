/*
 * SquarezenMessagePort.h
 *
 *  Created on: May 30, 2013
 *
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

#ifndef SQUAREZENMESSAGEPORT_H_
#define SQUAREZENMESSAGEPORT_H_

#include <FApp.h>
#include <FBase.h>
#include <FIo.h>

static const RequestId PLAYBACK_REQUEST = 4;
static const RequestId PLAYBACK_STARTED = 5;
static const RequestId PLAYBACK_FINISHED = 6;
static const RequestId PAUSE_UNPAUSE_REQUEST = 7;
static const RequestId PAUSED_UNPAUSED = 8;

class SquarezenMessagePort
	: public Tizen::Io::IMessagePortListener
{
public :
	SquarezenMessagePort(void);

	virtual ~SquarezenMessagePort(void);

	result Construct(const Tizen::Base::String& localPortName);

	virtual void OnMessageReceivedN(Tizen::Io::RemoteMessagePort* remoteMessagePort, Tizen::Base::Collection::IMap* message);

	result SendMessage(const Tizen::Base::Collection::IMap* message);

private :
	Tizen::Io::LocalMessagePort *mLocalMessagePort;
	Tizen::Io::RemoteMessagePort *mRemoteMessagePort;
	Tizen::Base::Collection::ArrayList *mMessageArgList;
};


#endif /* SQUAREZENMESSAGEPORT_H_ */

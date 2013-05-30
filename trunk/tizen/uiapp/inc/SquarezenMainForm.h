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

#ifndef _SQUAREZEN_MAIN_FORM_H_
#define _SQUAREZEN_MAIN_FORM_H_

#include <FApp.h>
#include <FBase.h>
#include <FMedia.h>
#include <FSystem.h>
#include <FUi.h>
#include <FUiIme.h>
#include <FGraphics.h>
#include <gl.h>
#include "../../../emu-players/MusicPlayer.h"


class SquarezenMainForm
	: public Tizen::Ui::Controls::Form
	, public Tizen::Ui::IActionEventListener
	, public Tizen::Ui::Controls::IFormBackEventListener
 	, public Tizen::Ui::Scenes::ISceneEventListener
    , public Tizen::Ui::Controls::IListViewItemEventListener
    , public Tizen::Ui::Controls::IListViewItemProvider
	, public Tizen::Media::IAudioOutEventListener
{
public:
	SquarezenMainForm(void);
	virtual ~SquarezenMainForm(void);
	bool Initialize(void);

public:
	virtual result OnInitializing(void);
	virtual result OnTerminating(void);
	virtual void OnActionPerformed(const Tizen::Ui::Control& source, int actionId);
	virtual void OnFormBackRequested(Tizen::Ui::Controls::Form& source);
	virtual void OnSceneActivatedN(const Tizen::Ui::Scenes::SceneId& previousSceneId,
								   const Tizen::Ui::Scenes::SceneId& currentSceneId, Tizen::Base::Collection::IList* pArgs);
	virtual void OnSceneDeactivated(const Tizen::Ui::Scenes::SceneId& currentSceneId,
									const Tizen::Ui::Scenes::SceneId& nextSceneId);

    // IListViewItemEventListener
    virtual void OnListViewContextItemStateChanged(Tizen::Ui::Controls::ListView &listView, int index, int elementId, Tizen::Ui::Controls::ListContextItemStatus state);
    virtual void OnListViewItemStateChanged(Tizen::Ui::Controls::ListView &listView, int index, int elementId, Tizen::Ui::Controls::ListItemStatus status);
    virtual void OnListViewItemSwept(Tizen::Ui::Controls::ListView &listView, int index, Tizen::Ui::Controls::SweepDirection direction);

    // IListViewItemProvider
    virtual Tizen::Ui::Controls::ListItemBase* CreateItem(int index, int itemWidth);
    virtual bool DeleteItem(int index, Tizen::Ui::Controls::ListItemBase* pItem, int itemWidth);
    virtual int GetItemCount(void);

protected:
    result PlayFile(Tizen::Base::String *fileName);

    Tizen::Ui::Controls::ListContextItem *mItemContext;
    Tizen::Base::Collection::ArrayList *mFileList;
    Tizen::Base::String mExtStoragePath;
    Tizen::Base::Runtime::Mutex mPlayerMutex;
    int mNumFiles;

    static const int ID_FORMAT_STRING = 100;
	static const int ID_BUTTON_OK = 101;

protected:
    virtual void OnAudioOutBufferEndReached(Tizen::Media::AudioOut& src);
    virtual void OnAudioOutErrorOccurred(Tizen::Media::AudioOut& src, result r) {}
    virtual void OnAudioOutInterrupted(Tizen::Media::AudioOut& src) {}
    virtual void OnAudioOutReleased(Tizen::Media::AudioOut& src) {}
    virtual void OnAudioOutAudioFocusChanged(Tizen::Media::AudioOut& src) {}

private:
    Tizen::Media::AudioOut mAudioOut;
    MusicPlayer *mPlayer;
    Tizen::Base::ByteBuffer mBuffers[2];
    int mCurPlayingBuffer;
    int mMinBufferSize;
};

#endif	//_SQUAREZEN_MAIN_FORM_H_

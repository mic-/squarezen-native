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
#include <FSystem.h>
#include <FUi.h>
#include <FUiIme.h>
#include <FGraphics.h>
#include <gl.h>

class SquarezenMainForm;


class FileScannerThread : public Tizen::Base::Runtime::Thread
{
public:
	FileScannerThread()
		: mForm(NULL)
	{
	}

	void SetParent(SquarezenMainForm *form) { mForm = form; }

	virtual ~FileScannerThread(void)
	{
	}

	/*result FileScannerThread(void)
	{
		return Thread::Construct();
	}*/


	Object* Run(void);

   private:
	SquarezenMainForm *mForm;
 };


class SquarezenMainForm
	: public Tizen::Ui::Controls::Form
	, public Tizen::Ui::IActionEventListener
    , public Tizen::Ui::IAdjustmentEventListener
	, public Tizen::Ui::Controls::IFormBackEventListener
 	, public Tizen::Ui::Scenes::ISceneEventListener
    , public Tizen::Ui::Controls::IListViewItemEventListener
    , public Tizen::Ui::Controls::IListViewItemProvider
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

	virtual void OnUserEventReceivedN(RequestId requestId, Tizen::Base::Collection::IList* args);

    // IListViewItemEventListener
    virtual void OnListViewContextItemStateChanged(Tizen::Ui::Controls::ListView &listView, int index, int elementId, Tizen::Ui::Controls::ListContextItemStatus state);
    virtual void OnListViewItemStateChanged(Tizen::Ui::Controls::ListView &listView, int index, int elementId, Tizen::Ui::Controls::ListItemStatus status);
    virtual void OnListViewItemSwept(Tizen::Ui::Controls::ListView &listView, int index, Tizen::Ui::Controls::SweepDirection direction);

    // IListViewItemProvider
    virtual Tizen::Ui::Controls::ListItemBase* CreateItem(int index, int itemWidth);
    virtual bool DeleteItem(int index, Tizen::Ui::Controls::ListItemBase* pItem, int itemWidth);
    virtual int GetItemCount(void);

    // IAdjustmentEventListener
    virtual void OnAdjustmentValueChanged(const Tizen::Ui::Control& source, int adjustment);

    Tizen::Ui::Controls::ListContextItem *mItemContext;
    Tizen::Base::Collection::ArrayList *mFileList;
    Tizen::Base::Collection::ArrayList *mMessageArgList;
    Tizen::Base::String mExtStoragePath;
    Tizen::Base::Runtime::Mutex mFileListMutex;
    int mNumFiles;
    int mSongLengthMs, mNumSubSongs, mCurrSubSong;

protected:
    FileScannerThread mFileScanner;

    static const int ID_FORMAT_STRING = 100;
	static const int ID_BUTTON_OK = 101;
};

#endif	//_SQUAREZEN_MAIN_FORM_H_

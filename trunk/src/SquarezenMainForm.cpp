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

#include "SquarezenMainForm.h"
#include "AppResourceId.h"
#include <FIo.h>
#include "VgmPlayer.h"
#include "YmPlayer.h"

using namespace Tizen::Base;
using namespace Tizen::App;
using namespace Tizen::Io;
using namespace Tizen::Media;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;


SquarezenMainForm::SquarezenMainForm(void) :
		mItemContext(NULL), mFileList(NULL), mNumFiles(0), mPlayer(NULL)
{
}

SquarezenMainForm::~SquarezenMainForm(void)
{
}

bool
SquarezenMainForm::Initialize(void)
{
	Construct(IDF_FORM);

	return true;
}

result
SquarezenMainForm::OnInitializing(void)
{
	result r = E_SUCCESS;

	// Setup back event listener
	SetFormBackEventListener(this);

	mPlayerMutex.Create();

	Directory* dir;
	DirEnumerator* dirEnum;
	String dirName;

	mExtStoragePath = Tizen::System::Environment::GetExternalStoragePath();
	dir = new Directory;

	// Open the directory
	r = dir->Construct(mExtStoragePath);
	if (IsFailed(r)) {
		// TODO: handle error
	    //goto CATCH;
	}

	// Reads all the directory entries
	dirEnum = dir->ReadN();
	if (!dirEnum) {
		// TODO: handle error
		//goto CATCH;
	}

    mFileList = new Tizen::Base::Collection::ArrayList(Tizen::Base::Collection::SingleObjectDeleter);
    mFileList->Construct();

	// Loops through all the directory entries
	while (dirEnum->MoveNext() == E_SUCCESS) {
		DirEntry entry = dirEnum->GetCurrentDirEntry();
		String s;
		entry.GetName().ToUpper(s);
		if (s.EndsWith(".YM") || s.EndsWith(".VGM")) {
			String *temp = new String(entry.GetName());
			mFileList->Add(temp);
			//AppLog("Found file %S", temp->GetPointer());
		}
    }

	ListView *listView = static_cast< ListView* >(GetControl(IDC_LISTVIEW1));
    listView->SetItemProvider(*this);
    listView->AddListViewItemEventListener(*this);

    mItemContext = new ListContextItem();
	mItemContext->Construct();

	delete dir;
	dir = null;

	// Get a button via resource ID
	Tizen::Ui::Controls::Button *pButtonOk = static_cast<Button*>(GetControl(IDC_BUTTON_OK));
	if (pButtonOk != null)
	{
		pButtonOk->SetActionId(ID_BUTTON_OK);
		pButtonOk->AddActionEventListener(*this);
	}

    r = mAudioOut.Construct(*this);
    if (IsFailed(r)) {
        return r;
    }
    mAudioOut.Prepare(AUDIO_TYPE_PCM_S16_LE, AUDIO_CHANNEL_TYPE_STEREO, 44100);
    mMinBufferSize = mAudioOut.GetMinBufferSize() * 4;

    mBuffers[0].Construct(mMinBufferSize);
    mBuffers[1].Construct(mMinBufferSize);

    AppLog("Form initialized");

	return r;
}


void SquarezenMainForm::OnAudioOutBufferEndReached(Tizen::Media::AudioOut& src)
{
	mPlayerMutex.Acquire();

	// Switch buffers and fill up with new audio data
    mAudioOut.WriteBuffer(mBuffers[mCurPlayingBuffer ^ 1]);
    mPlayer->Run(mMinBufferSize >> 2, (int16_t*)mBuffers[mCurPlayingBuffer].GetPointer());
    mCurPlayingBuffer ^= 1;

    mPlayerMutex.Release();
}


result SquarezenMainForm::PlayFile(String *fileName) {
	String path = String(mExtStoragePath) + *fileName;

	mPlayerMutex.Acquire();

	if (AUDIOOUT_STATE_PLAYING == mAudioOut.GetState()) {
		if (IsFailed(mAudioOut.Stop())) {
			AppLog("AudioOut::Stop failed");
		}
	}
	mAudioOut.Reset();

	if (mPlayer) {
		mPlayer->Reset();
		delete mPlayer;
	}

	if (fileName->EndsWith(".VGM") || fileName->EndsWith(".vgm")) {
		mPlayer = new VgmPlayer;
	} else if (fileName->EndsWith(".YM") || fileName->EndsWith(".ym")) {
		mPlayer = new YmPlayer;
	} else {
		AppLog("Unrecognized file type: %S", fileName->GetPointer());
		return E_FAILURE;
	}

	std::wstring ws = std::wstring(path.GetPointer());
	if ( IsFailed(mPlayer->Prepare(ws)) ) {
    	AppLog("Prepare failed");
    	return E_FAILURE;
    }

    // Fill up both buffers with audio data and send the first one to the audio hw
	mPlayer->Run(mMinBufferSize >> 2, (int16_t*)mBuffers[0].GetPointer());
	mPlayer->Run(mMinBufferSize >> 2, (int16_t*)mBuffers[1].GetPointer());
	mAudioOut.WriteBuffer(mBuffers[0]);
	mCurPlayingBuffer = 0;

	if (IsFailed(mAudioOut.Start())) {
		AppLog("AudioOut::Start failed");
		return E_FAILURE;
    }

	mPlayerMutex.Release();

	return E_SUCCESS;
}


result
SquarezenMainForm::OnTerminating(void)
{
	result r = E_SUCCESS;

	if (mItemContext) {
		delete mItemContext;
		mItemContext = NULL;
	}

	mAudioOut.Stop();
	mAudioOut.Unprepare();
	if (mPlayer) {
		mPlayer->Reset();
		delete mPlayer;
		mPlayer = NULL;
	}

	return r;
}


// IListViewItemEventListener implementation
void
SquarezenMainForm::OnListViewItemStateChanged(ListView &listView, int index, int elementId, ListItemStatus status)
{
	if (mFileList && elementId >= 0 && elementId < mFileList->GetCount()) {
		AppLog("Clicked %S", ((String*)mFileList->GetAt(index))->GetPointer());
		PlayFile((String*)mFileList->GetAt(index));

	}
}

void
SquarezenMainForm::OnListViewContextItemStateChanged(ListView &listView, int index, int elementId, ListContextItemStatus state)
{

}

void
SquarezenMainForm::OnListViewItemSwept(ListView &listView, int index, SweepDirection direction)
{
    // ....
}

ListItemBase*
SquarezenMainForm::CreateItem(int index, int itemWidth)
{
    // Creates an instance of CustomItem
    CustomItem* pItem = new CustomItem();
    ListAnnexStyle style = LIST_ANNEX_STYLE_NORMAL;

    if (mFileList && index >= 0 && index < mFileList->GetCount()) {
    	style = LIST_ANNEX_STYLE_NORMAL;
        pItem->Construct(Tizen::Graphics::Dimension(itemWidth, 112), style);
        pItem->AddElement(Tizen::Graphics::Rectangle(0, 25, itemWidth, 50), index, ((String*)mFileList->GetAt(index))->GetPointer(), true);
    }

    pItem->SetContextItem(mItemContext);

    return pItem;
}

bool
SquarezenMainForm::DeleteItem(int index, ListItemBase* pItem, int itemWidth)
{
    delete pItem;
    pItem = null;
    return true;
}

int
SquarezenMainForm::GetItemCount(void)
{
	if (mFileList) {
		return mFileList->GetCount();
	}
    return 0;
}


void
SquarezenMainForm::OnActionPerformed(const Tizen::Ui::Control& source, int actionId)
{
	SceneManager* pSceneManager = SceneManager::GetInstance();
	AppAssert(pSceneManager);

	switch(actionId)
	{
	case ID_BUTTON_OK: {
		UiApp* pApp = UiApp::GetInstance();
		AppAssert(pApp);
		pApp->Terminate();
		break;
	}
	default: {
		break;
	}
	}
}

void
SquarezenMainForm::OnFormBackRequested(Tizen::Ui::Controls::Form& source)
{
	UiApp* pApp = UiApp::GetInstance();
	AppAssert(pApp);
	pApp->Terminate();
}

void
SquarezenMainForm::OnSceneActivatedN(const Tizen::Ui::Scenes::SceneId& previousSceneId,
										  const Tizen::Ui::Scenes::SceneId& currentSceneId, Tizen::Base::Collection::IList* pArgs)
{
	// TODO:
	// Add your scene activate code here
	AppLog("OnSceneActivatedN");
}

void
SquarezenMainForm::OnSceneDeactivated(const Tizen::Ui::Scenes::SceneId& currentSceneId,
										   const Tizen::Ui::Scenes::SceneId& nextSceneId)
{
	// TODO:
	// Add your scene deactivate code here
	AppLog("OnSceneDeactivated");
}


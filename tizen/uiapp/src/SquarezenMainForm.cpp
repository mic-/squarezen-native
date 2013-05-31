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
#include "SquarezenServiceProxy.h"
#include "AppResourceId.h"
#include <FIo.h>

using namespace Tizen::Base;
using namespace Tizen::Base::Collection;
using namespace Tizen::App;
using namespace Tizen::Io;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;


SquarezenMainForm::SquarezenMainForm(void) :
		mItemContext(NULL),
		mFileList(NULL),
		mMessageArgList(NULL),
		mNumFiles(0)
{
}

SquarezenMainForm::~SquarezenMainForm(void)
{
	//delete mFileList;
	delete mMessageArgList;
}

bool
SquarezenMainForm::Initialize(void)
{
	Construct(IDF_FORM);

	return true;
}


Object *FileScannerThread::Run()
{
	return null;
 }


result
SquarezenMainForm::OnInitializing(void)
{
	result r = E_SUCCESS;

	// Setup back event listener
	SetFormBackEventListener(this);

	mFileListMutex.Create();

	mExtStoragePath = Tizen::System::Environment::GetExternalStoragePath();

	mFileScanner.Construct();
	mFileScanner.Start();

	Directory* dir;
	DirEnumerator* dirEnum;
	String dirName;

	AppLog("FileScannerThread started");

	dir = new Directory;

	// Open the directory
	r = dir->Construct(mExtStoragePath);
	if (IsFailed(r)) {
		// TODO: handle error
	}

	// Reads all the directory entries
	dirEnum = dir->ReadN();
	if (!dirEnum) {
		// TODO: handle error
	}

	mFileList = new Tizen::Base::Collection::ArrayList(Tizen::Base::Collection::SingleObjectDeleter);
	mFileList->Construct();

	// Loops through all the directory entries
	while (dirEnum->MoveNext() == E_SUCCESS) {
		DirEntry entry = dirEnum->GetCurrentDirEntry();
		String s;
		entry.GetName().ToUpper(s);
		if (s.EndsWith(".YM") ||
				s.EndsWith(".VGM") ||
				s.EndsWith(".GBS")) {
			String *temp = new String(entry.GetName());
			mFileList->Add(temp);
		}
	}

	delete dir;
	dir = null;

	ListView *listView = static_cast< ListView* >(GetControl(IDC_LISTVIEW1));
    listView->SetItemProvider(*this);
    listView->AddListViewItemEventListener(*this);

    mItemContext = new ListContextItem();
	mItemContext->Construct();

	mMessageArgList = new ArrayList(SingleObjectDeleter);
	mMessageArgList->Construct();

	// Get a button via resource ID
	Tizen::Ui::Controls::Button *pButtonOk = static_cast<Button*>(GetControl(IDC_BUTTON_OK));
	if (pButtonOk != null) {
		pButtonOk->SetActionId(ID_BUTTON_OK);
		pButtonOk->AddActionEventListener(*this);
	}

    AppLog("Form initialized");

	return r;
}

result
SquarezenMainForm::OnTerminating(void)
{
	result r = E_SUCCESS;

	if (mItemContext) {
		delete mItemContext;
		mItemContext = NULL;
	}

	return r;
}


// IListViewItemEventListener implementation
void
SquarezenMainForm::OnListViewItemStateChanged(ListView &listView, int index, int elementId, ListItemStatus status)
{
	mFileListMutex.Acquire();

	if (mFileList && elementId >= 0 && elementId < mFileList->GetCount()) {
		UiApp* app = UiApp::GetInstance();
		AppAssert(app);
		mMessageArgList->RemoveAll();
		String *path = new String(String(mExtStoragePath) + *(String*)(mFileList->GetAt(index)));
		mMessageArgList->Add(path);
		app->SendUserEvent(STATE_PLAYBACK_REQUEST, mMessageArgList);

		AppLog("Clicked %S", ((String*)mFileList->GetAt(index))->GetPointer());
		//PlayFile((String*)mFileList->GetAt(index));
	}

	mFileListMutex.Release();
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

    mFileListMutex.Acquire();

    if (mFileList && index >= 0 && index < mFileList->GetCount()) {
    	style = LIST_ANNEX_STYLE_NORMAL;
        pItem->Construct(Tizen::Graphics::Dimension(itemWidth, 112), style);
        pItem->AddElement(Tizen::Graphics::Rectangle(0, 25, itemWidth, 50), index, ((String*)mFileList->GetAt(index))->GetPointer(), true);
    }

    mFileListMutex.Release();

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
	int count = 0;

	mFileListMutex.Acquire();

	if (mFileList) {
		count = mFileList->GetCount();
	}

	mFileListMutex.Release();

    return count;
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
SquarezenMainForm::OnUserEventReceivedN(RequestId requestId, IList* args)
{
	AppLog("SquarezenMainForm: OnUserEventReceivedN is called. requestId is %d", requestId);

	switch (requestId) {
	case STATE_READY :
		break;

	case STATE_PLAYBACK_STARTED :
		break;

	default:
		break;
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


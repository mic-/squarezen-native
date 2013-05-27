#include "SquarezenFrame.h"
#include "SquarezenFormFactory.h"
#include "SquarezenPanelFactory.h"
#include "AppResourceId.h"

using namespace Tizen::Base;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;

SquarezenFrame::SquarezenFrame(void)
{
}

SquarezenFrame::~SquarezenFrame(void)
{
}

result
SquarezenFrame::OnInitializing(void)
{
	// Prepare Scene management.
	SceneManager* pSceneManager = SceneManager::GetInstance();
	static SquarezenFormFactory formFactory;
	static SquarezenPanelFactory panelFactory;
	pSceneManager->RegisterFormFactory(formFactory);
	pSceneManager->RegisterPanelFactory(panelFactory);
	pSceneManager->RegisterScene(L"workflow");

	// Go to the scene.
	result r = E_FAILURE;
	r = pSceneManager->GoForward(SceneTransitionId(ID_SCNT_MAINSCENE));

	// TODO: Add your initialization code here
	return r;
}

result
SquarezenFrame::OnTerminating(void)
{
	result r = E_SUCCESS;

	// TODO:
	// Add your termination code here
	return r;
}

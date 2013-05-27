#include "SquarezenPanelFactory.h"

using namespace Tizen::Ui::Scenes;


SquarezenPanelFactory::SquarezenPanelFactory(void)
{
}

SquarezenPanelFactory::~SquarezenPanelFactory(void)
{
}

Tizen::Ui::Controls::Panel*
SquarezenPanelFactory::CreatePanelN(const Tizen::Base::String& panelId, const Tizen::Ui::Scenes::SceneId& sceneId)
{
	SceneManager* pSceneManager = SceneManager::GetInstance();
	AppAssert(pSceneManager);
	Tizen::Ui::Controls::Panel* pNewPanel = null;

	// TODO:
	// Add your panel creation code here
	return pNewPanel;
}

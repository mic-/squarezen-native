#include "SquarezenFormFactory.h"
#include "SquarezenMainForm.h"
#include "AppResourceId.h"

using namespace Tizen::Ui::Scenes;


SquarezenFormFactory::SquarezenFormFactory(void)
{
}

SquarezenFormFactory::~SquarezenFormFactory(void)
{
}

Tizen::Ui::Controls::Form*
SquarezenFormFactory::CreateFormN(const Tizen::Base::String& formId, const Tizen::Ui::Scenes::SceneId& sceneId)
{
	SceneManager* pSceneManager = SceneManager::GetInstance();
	AppAssert(pSceneManager);
	Tizen::Ui::Controls::Form* pNewForm = null;

	if (formId == IDF_FORM)
	{
		SquarezenMainForm* pForm = new SquarezenMainForm();
		pForm->Initialize();
		pSceneManager->AddSceneEventListener(sceneId, *pForm);
		pNewForm = pForm;
	}
	// TODO:
	// Add your form creation code here

	return pNewForm;
}

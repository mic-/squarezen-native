#ifndef _SQUAREZEN_FORM_FACTORY_H_
#define _SQUAREZEN_FORM_FACTORY_H_

#include <FApp.h>
#include <FBase.h>
#include <FSystem.h>
#include <FUi.h>
#include <FUiIme.h>
#include <FGraphics.h>
#include <gl.h>

class SquarezenFormFactory
	: public Tizen::Ui::Scenes::IFormFactory
{
public:
	SquarezenFormFactory(void);
	virtual ~SquarezenFormFactory(void);

	virtual Tizen::Ui::Controls::Form* CreateFormN(const Tizen::Base::String& formId, const Tizen::Ui::Scenes::SceneId& sceneId);
};

#endif // _SQUAREZEN_FORM_FACTORY_H_

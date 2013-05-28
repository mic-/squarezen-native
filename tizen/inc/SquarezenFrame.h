#ifndef _SQUAREZEN_FRAME_H_
#define _SQUAREZEN_FRAME_H_

#include <FApp.h>
#include <FBase.h>
#include <FSystem.h>
#include <FUi.h>
#include <FUiIme.h>
#include <FGraphics.h>
#include <gl.h>

class SquarezenFrame
	: public Tizen::Ui::Controls::Frame
{
public:
	SquarezenFrame(void);
	virtual ~SquarezenFrame(void);

public:
	virtual result OnInitializing(void);
	virtual result OnTerminating(void);
};

#endif	//_SQUAREZEN_FRAME_H_

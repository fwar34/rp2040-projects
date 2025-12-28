#ifndef __EC11_H_
#define __EC11_H_

#include "qpc.h"

enum Ec11State
{
	EC11_KEY_INVALID = 0,
	EC11_KEY_PRESS, // 按下
	EC11_KEY_CONTINUE_CLICK,	 // 单击（按下后释放触发）
	EC11_KEY_DOUBLE_CLICK, // 双击
	EC11_KEY_LONG_CLICK, // 长按
	EC11_KEY_LEFT_ROTATE, // 向左旋转一下
	EC11_KEY_RIGHT_ROTATE, // 向右旋转一下
	EC11_KEY_PRESS_LEFT_ROTATE, // 按住向左旋转一下
	EC11_KEY_PRESS_RIGHT_ROTATE, // 按住向右旋转一下
	EC11_DEBUG
};

const char *GetKeyName(uint8_t keyIndex);
const char *GetKeyEventName(uint8_t keyEvent);
void Ec11Ctor(void);
extern QActive *g_Ec11;

#endif // !__EC11_H_
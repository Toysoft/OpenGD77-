#include <stdbool.h>

#include "common.h"
#if defined(PLATFORM_DM5R)

static bool torch_state = false;			 // Baofeng DM-5R torch

void toggle_torch(void)
{
	torch_state=!torch_state;
	GPIO_PinWrite(GPIO_Torch, Pin_Torch, torch_state);
}
#endif

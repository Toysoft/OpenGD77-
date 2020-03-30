#include <stdbool.h>

#include "common.h"


#if defined(PLATFORM_DM5R)
void toggle_torch(bool *torch_state)
{
	if (*torch_state == true)
		*torch_state = false;
	else
		*torch_state = true;

	GPIO_PinWrite(GPIO_Torch, Pin_Torch, *torch_state);
}
#endif

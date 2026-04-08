#include "app_ir/app_ir.h"
#include "drv_ir/drv_ir.h"

void app_ir_task(void)
{
	uint8_t ir_code = 0;
	if(!drv_ir_get_code(&ir_code))
	{
		return;
	}
	printf("IR code is 0x%X .",ir_code);
}
	

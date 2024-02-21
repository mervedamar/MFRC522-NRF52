
#include <zephyr/drivers/gpio.h>

#include <zephyr/drivers/SPi.h>
#include "MFRC522.h"



/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
uint8_t ID[5];
/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
	int ret;
SPI_Init();
TM_MFRC522_Init();
	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	while (1) {

    if ((TM_MFRC522_Check(ID) == MI_OK))
	{
		gpio_pin_toggle_dt(&led);
printk(" [%02x%02x%02x%02x%02x]\n\r\n", ID[0], ID[1], ID[2], ID[3], ID[4]);
	}

	
	
	}
	return 0;
}
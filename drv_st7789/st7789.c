#include "st7789.h"
#include <hardware/gpio.h>
#include <pico/time.h>
#include <pico/binary_info/code.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef USE_DMA
#include <string.h>
#include <hardware/dma.h>
uint DmaChann;
dma_channel_config DmaCfg;
#endif


void HAL_Delay(uint16_t ms);

static void ST7789_UnSelect() {
#ifndef CFG_NO_CS

#ifdef USE_DMA
	if (!dma_channel_is_busy(DmaChann)) {
#endif
		gpio_put(ST7789_CS_PIN, 1);
#ifdef USE_DMA
	}
#endif

#endif
}

static void ST7789_Select() {
#ifndef CFG_NO_CS
	gpio_put(ST7789_CS_PIN, 0);
#endif
}

/**
 * @brief Write command to ST7789 controller
 * @param cmd -> command to write
 * @return none
 */
static void ST7789_WriteCommand(uint8_t cmd)
{
	ST7789_Select();
	ST7789_DC_Clr();
	//HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
	spi_write_blocking(ST7789_SPI_PORT, &cmd, 1);
	ST7789_UnSelect();
}

/**
 * @brief Write data to ST7789 controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */
static void ST7789_WriteData(uint8_t *buff, size_t buff_size)
{
	ST7789_Select();
	ST7789_DC_Set();
#ifdef USE_DMA
	if (dma_channel_is_claimed(DmaChann)) {
		dma_channel_set_read_addr(DmaChann, buff, false); //设置读地址但是不启动传输
		dma_channel_set_trans_count(DmaChann, buff_size, true); //设置传输数据长度并且开始传输
	}
	else {	//DMA 通道还没有获得，即初始化时不用DMA SPI
		spi_write_blocking(ST7789_SPI_PORT, buff, buff_size);
	}
#else
	spi_write_blocking(ST7789_SPI_PORT, buff, buff_size);
#endif //USE_DMA
	ST7789_UnSelect();
}

static void ST7789_WriteDataNoDma(uint8_t *buff, size_t buff_size) {
	ST7789_Select();
	ST7789_DC_Set();

	spi_write_blocking(ST7789_SPI_PORT, buff, buff_size);

	ST7789_UnSelect();
}
/**
 * @brief Write data to ST7789 controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */
static void ST7789_WriteSmallData(uint8_t data)
{
	ST7789_Select();
	ST7789_DC_Set();
	//HAL_SPI_Transmit(&ST7789_SPI_PORT, &data, sizeof(data), HAL_MAX_DELAY);
	spi_write_blocking(ST7789_SPI_PORT, &data, 1);
	ST7789_UnSelect();
}

/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in st7789.h)
 * @return none
 */
void ST7789_SetRotation(uint8_t m)
{
	ST7789_WriteCommand(ST7789_MADCTL);	// MADCTL
	switch (m) {
	case 0:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
		break;
	case 1:
		ST7789_WriteSmallData(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	case 2:
		ST7789_WriteSmallData(ST7789_MADCTL_RGB);
		break;
	case 3:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	default:
		break;
	}
}

/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	ST7789_Select();
	uint16_t x_start = x0 + X_SHIFT, x_end = x1 + X_SHIFT;
	uint16_t y_start = y0 + Y_SHIFT, y_end = y1 + Y_SHIFT;
	
	/* Column Address set */
	ST7789_WriteCommand(ST7789_CASET); 
	{
		uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
		ST7789_WriteDataNoDma(data, sizeof(data));
	}

	/* Row Address set */
	ST7789_WriteCommand(ST7789_RASET);
	{
		uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
		ST7789_WriteDataNoDma(data, sizeof(data));
	}
	/* Write to RAM */
	ST7789_WriteCommand(ST7789_RAMWR);
	ST7789_UnSelect();
}

/**
 * @brief Initialize ST7789 controller
 * @param none
 * @return none
 */
void ST7789_Init(void)
{
	gpio_init(ST7789_RST_PIN);
	gpio_set_dir(ST7789_RST_PIN, GPIO_OUT);
	gpio_init(ST7789_DC_PIN);
	gpio_set_dir(ST7789_DC_PIN, GPIO_OUT);
#ifndef CFG_NO_CS
	gpio_init(ST7789_CS_PIN);
	gpio_set_dir(ST7789_CS_PIN, GPIO_OUT);
#endif
	gpio_init(BLK_PIN);
	gpio_set_dir(BLK_PIN, GPIO_OUT);
	ST7789_SetBacklight(0);

	spi_init(ST7789_SPI_PORT, 65000000);	//最大速度62.5MHz
	gpio_set_function(ST7789_SDA_PIN, GPIO_FUNC_SPI);
	gpio_set_function(ST7789_SCL_PIN, GPIO_FUNC_SPI);
	spi_set_format(ST7789_SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

#ifdef USE_DMA
	DmaCfg = dma_channel_get_default_config(DmaChann);
	channel_config_set_transfer_data_size(&DmaCfg, DMA_SIZE_8);
	channel_config_set_dreq(&DmaCfg, spi_get_dreq(ST7789_SPI_PORT, true));
	channel_config_set_read_increment(&DmaCfg, true);
	channel_config_set_bswap(&DmaCfg, true);
	dma_channel_cleanup(DmaChann);
	dma_channel_configure(DmaChann,
						  &DmaCfg,
						  &spi_get_hw(ST7789_SPI_PORT)->dr,
						  NULL,
						  0,
						  false);
#endif

	HAL_Delay(25);
    ST7789_RST_Clr();
    HAL_Delay(25);
    ST7789_RST_Set();
    HAL_Delay(50);
		
    ST7789_WriteCommand(ST7789_COLMOD);		//	Set color mode
    ST7789_WriteSmallData(ST7789_COLOR_MODE_16bit);
  	ST7789_WriteCommand(0xB2);				//	Porch control
	{
		uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
		ST7789_WriteData(data, sizeof(data));
	}
	ST7789_SetRotation(ST7789_ROTATION);	//	MADCTL (Display Rotation)
	
	/* Internal LCD Voltage generator settings */
    ST7789_WriteCommand(0XB7);				//	Gate Control
    ST7789_WriteSmallData(0x35);			//	Default value
    ST7789_WriteCommand(0xBB);				//	VCOM setting
    ST7789_WriteSmallData(0x19);			//	0.725v (default 0.75v for 0x20)
    ST7789_WriteCommand(0xC0);				//	LCMCTRL	
    ST7789_WriteSmallData (0x2C);			//	Default value
    ST7789_WriteCommand (0xC2);				//	VDV and VRH command Enable
    ST7789_WriteSmallData (0x01);			//	Default value
    ST7789_WriteCommand (0xC3);				//	VRH set
    ST7789_WriteSmallData (0x12);			//	+-4.45v (defalut +-4.1v for 0x0B)
    ST7789_WriteCommand (0xC4);				//	VDV set
    ST7789_WriteSmallData (0x20);			//	Default value
    ST7789_WriteCommand (0xC6);				//	Frame rate control in normal mode
    ST7789_WriteSmallData (0x0F);			//	Default value (60HZ)
    ST7789_WriteCommand (0xD0);				//	Power control
    ST7789_WriteSmallData (0xA4);			//	Default value
    ST7789_WriteSmallData (0xA1);			//	Default value
	/**************** Division line ****************/

	ST7789_WriteCommand(0xE0);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
		ST7789_WriteData(data, sizeof(data));
	}

    ST7789_WriteCommand(0xE1);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
		ST7789_WriteData(data, sizeof(data));
	}
    ST7789_WriteCommand (ST7789_INVON);		//	Inversion ON
	ST7789_WriteCommand (ST7789_SLPOUT);	//	Out of sleep mode
  	ST7789_WriteCommand (ST7789_NORON);		//	Normal Display on
  	ST7789_WriteCommand (ST7789_DISPON);	//	Main screen turned on	

	// HAL_Delay(50);
	// ST7789_Fill_Color(WHITE);				//	Fill with Black.

#ifdef USE_DMA
	//初始化完成之后，使能DMA完成中断，在中断里面通知LVGL屏幕刷新准备好了
	dma_channel_set_irq0_enabled(DmaChann, true);
	irq_set_exclusive_handler(DMA_IRQ_0, DMA_FINISH_CB);
	irq_set_enabled(DMA_IRQ_0, true);

	dma_channel_start(DmaChann);
	dma_channel_wait_for_finish_blocking(DmaChann);
#endif

}

/**
 * @brief Fill the DisplayWindow with single color
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill_Color(uint16_t color)
{
	uint16_t i = 0;
	ST7789_SetAddressWindow(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
	ST7789_Select();

	uint16_t j;
	for (i = 0; i < ST7789_WIDTH; i++)
			for (j = 0; j < ST7789_HEIGHT; j++) {
				uint8_t data[] = {color >> 8, color & 0xFF};
				ST7789_WriteData(data, sizeof(data));
			}
	ST7789_UnSelect();
}

/**
 * @brief Draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x < 0) || (x >= ST7789_WIDTH) ||
		 (y < 0) || (y >= ST7789_HEIGHT))	return;
	
	ST7789_SetAddressWindow(x, y, x, y);
	uint8_t data[] = {color >> 8, color & 0xFF};
	ST7789_Select();
	ST7789_WriteData(data, sizeof(data));
	ST7789_UnSelect();
}

/**
 * @brief Fill an Area with single color
 * @param xSta&ySta -> coordinate of the start point
 * @param xEnd&yEnd -> coordinate of the end point
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
	if ((xEnd < 0) || (xEnd >= ST7789_WIDTH) ||
		 (yEnd < 0) || (yEnd >= ST7789_HEIGHT))	return;
	ST7789_Select();
	uint16_t i, j;
	ST7789_SetAddressWindow(xSta, ySta, xEnd, yEnd);
	for (i = ySta; i <= yEnd; i++)
		for (j = xSta; j <= xEnd; j++) {
			uint8_t data[] = {color >> 8, color & 0xFF};
			ST7789_WriteData(data, sizeof(data));
		}
	ST7789_UnSelect();
}

/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */
void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
	if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT))
		return;
	if ((x + w - 1) >= ST7789_WIDTH)
		return;
	if ((y + h - 1) >= ST7789_HEIGHT)
		return;

	ST7789_Select();
	ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);
	ST7789_WriteData((uint8_t *)data, sizeof(uint16_t) * w * h);
	ST7789_UnSelect();
}

void HAL_Delay(const uint16_t ms) {
	sleep_ms(ms);
}

void ST7789_SetBacklight(uint8_t on) {
	gpio_put(BLK_PIN, on);
}

#ifdef __cplusplus
}
#endif

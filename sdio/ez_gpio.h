#define SET_GPIO_sdio0_pad_card_detect_n(gpio) do { \
	  uint32_t _ezchip_macro_read_value_=MA_INW(gpio_sdio0_pad_card_detect_n_REG_ADDR); \
	  _ezchip_macro_read_value_ &= ~(0xFF); \
	  _ezchip_macro_read_value_ |= (((gpio) + 2) & 0xFF); \
	  MA_OUTW(gpio_sdio0_pad_card_detect_n_REG_ADDR,_ezchip_macro_read_value_); \
} while(0)

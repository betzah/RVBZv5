/*
 * peripherals.c
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */ 


#include "peripherals.h"


static void resetCausePrint();
static void initPowersaver();
static void wdtConfig(bool enableWdt, WDT_PER_t wdtLength, bool enableWin, WDT_WPER_t winLength);
static void initClocks(const bool useExt32KHz, const uint8_t pllMultiplier, const CLK_SCLKSEL_t cpuClk, const CLK_PSADIV_t sysClkDivA);

static void GPIOSet();
static void initBoardID();


/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t USBSerialClass =
{
	.Config =
	{
		.ControlInterfaceNumber   = INTERFACE_ID_CDC_CCI,
		.DataINEndpoint           =
		{
			.Address          = CDC_TX_EPADDR,
			.Size             = CDC_TXRX_EPSIZE,
			.Banks            = 1,
		},
		.DataOUTEndpoint =
		{
			.Address          = CDC_RX_EPADDR,
			.Size             = CDC_TXRX_EPSIZE,
			.Banks            = 1,
		},
		.NotificationEndpoint =
		{
			.Address          = CDC_NOTIFICATION_EPADDR,
			.Size             = CDC_NOTIFICATION_EPSIZE,
			.Banks            = 1,
		},
	},
};


FILE USBStream;


hardware_t hardware = 
{
	.time =
	{
		.year = 16,
		.month = 12,
		.day = 8,
		.hour = 1,
		.minute = 0,
		.second = 0,
	},
	.alarm = 
	{
		.year = 16,
		.month = 12,
		.day = 8,
		.hour = 1,
		.minute = 0,
		.second = 0,
	},
	.device = 
	{
		.type = device_noone,
		.number = 0,
		.numberTotal = 12,
		.numberTotal_bm = 0xFFF,
	}
};


uint8_t EEMEM wdtCrashesEEPROM = 0;
static bool usbIsConnected = false;


void initHardware() 
{
	//Essentials
	wdtConfig(true, WDT_PER_8KCLK_gc, false, WDT_WPER_8CLK_gc);
	initClocks(true, 16, CLK_SCLKSEL_PLL_gc, CLK_PSADIV_1_gc);
	GPIOSet();
	RTCInit(&hardware.time, CLK_RTCSRC_RCOSC_gc, RTC_OVFINTLVL_LO_gc, RTC_PRESCALER_DIV256_gc, 3, 1);
	initPowersaver();
	JTAG_FORCE_DISABLE();
	
	//USB
	USB_Init();
	CDC_Device_CreateStream(&USBSerialClass, &USBStream);
	stdin = &USBStream;
	stdout = &USBStream;
	 
	//Interrupts
	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
	sei();
	
	_delay_ms(1000);
	
	//Peripherals
	initBoardID();
	roomInit();
	adcInit(&hardware.adc);
	//serialInit(&PORTC, &USARTC0, false, F_CPU, 38400);
	twiInit(&hardware.twi, &TWIC, &PORTC, true, 0x00, NULL, TWI_MASTER_INTLVL_LO_gc, F_CPU, 50000);
	eventInit(&hardware.cpu);
	
	//Finished
	appUIPrint("Hardware Initialized.");
	resetCausePrint(&USBStream);
}


static void wdtConfig(bool enableWdt, WDT_PER_t wdtLength, bool enableWin, WDT_WPER_t winLength)
{
	WDT_RESET();
	CCPWrite((void *)&WDT.CTRL, ((enableWdt << WDT_ENABLE_bp) | WDT_CEN_bm | wdtLength));
	while (WDT.STATUS & WDT_SYNCBUSY_bm);
	CCPWrite((void *)&WDT.WINCTRL, ((enableWin << WDT_WCEN_bp) | winLength));
	while (WDT.STATUS & WDT_SYNCBUSY_bm);
}


void softwareReset()
{
	appUIPrint("Rebooting...");
	writeTimeToEE();
	_delay_ms(20);		//Wait for printing
	USB_Detach();
	_delay_ms(200);		//Wait for USB detachment
	CCPWrite(&RST.CTRL, RST_SWRST_bm);
}


int16_t freeRam()
{
	static int16_t minFreeRam = INTERNAL_SRAM_SIZE;
	uint8_t v;
	int16_t curFreeRam = (int16_t) &v - (int16_t)__malloc_heap_start;
	if (curFreeRam < minFreeRam) minFreeRam = curFreeRam;
	return minFreeRam;
	//extern int __heap_start, *__brkval;
	//return (uint16_t) (&v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
}


static void GPIOSet(void)	
{
	// ADC Pins
	PORTA.OUTCLR = 0xFF;
	PORTB.OUTCLR = 0xF3;
	PORTA.DIRCLR = 0xFF;
	PORTB.DIRCLR = 0xF3;
	
	PORTB.DIRSET = 0x04; // turn off ESP
	PORTB.OUTCLR = 0x04;
	
	// Remote Power Switch
	PORTE.OUTSET = 0xFF;
	PORTF.OUTSET = 0xFF;
	PORTH.OUTSET = 0xFF;
	PORTE.DIRSET = 0xFF;
	PORTF.DIRSET = 0xFF;
	PORTH.DIRSET = 0xFF;
	
	// IR Data
	PORTJ.OUTSET = 0xFF;
	PORTK.OUTSET = 0x2F;
	PORTK.OUTCLR = 0x10;
	PORTJ.DIRCLR = 0xFF;
	PORTK.DIRCLR = 0x3F;
	
	PORTQ.OUTSET = 0x08;
	PORTQ.DIRSET = 0x08;
	PORTR.OUTSET = 0x03;
	PORTR.OUTSET = 0x03;
	
	memset((void *) &PORTA.PIN0CTRL, PORT_ISC_INPUT_DISABLE_gc | PORT_SRLEN_bm, 8);
	memset((void *) &PORTB.PIN4CTRL, PORT_ISC_INPUT_DISABLE_gc | PORT_SRLEN_bm, 4);
	
	memset((void *) &PORTE.PIN0CTRL, PORT_ISC_INPUT_DISABLE_gc | PORT_SRLEN_bm, 8);
	memset((void *) &PORTF.PIN0CTRL, PORT_ISC_INPUT_DISABLE_gc | PORT_SRLEN_bm, 8);
	memset((void *) &PORTH.PIN0CTRL, PORT_ISC_INPUT_DISABLE_gc | PORT_SRLEN_bm, 8);
	memset((void *) &PORTJ.PIN0CTRL, PORT_SRLEN_bm, 8);
	memset((void *) &PORTK.PIN0CTRL, PORT_SRLEN_bm, 8);
	
	PORTB.PIN0CTRL = PORT_SRLEN_bm | PORT_OPC_PULLDOWN_gc; // VREF: +2048mV 
	PORTB.PIN1CTRL = PORT_SRLEN_bm | PORT_OPC_PULLDOWN_gc; // ADC: +12V div 11
	PORTB.PIN2CTRL = PORT_SRLEN_bm | PORT_OPC_PULLDOWN_gc; // ESP-EN
	PORTB.PIN3CTRL = PORT_SRLEN_bm | PORT_OPC_PULLDOWN_gc; // ESP-IO??6
	
	PORTC.PIN0CTRL = PORT_SRLEN_bm | PORT_OPC_WIREDANDPULL_gc; // SDA
	PORTC.PIN1CTRL = PORT_SRLEN_bm | PORT_OPC_WIREDANDPULL_gc; // SCL
	PORTC.PIN2CTRL = PORT_SRLEN_bm | PORT_OPC_PULLDOWN_gc; // ATMEGA-TXD
	PORTC.PIN3CTRL = PORT_SRLEN_bm | PORT_OPC_PULLDOWN_gc; // ATMEGA-RXD
	PORTC.PIN4CTRL = PORT_SRLEN_bm | PORT_OPC_PULLDOWN_gc; // ATMEGA-RST
	PORTC.PIN5CTRL = PORT_SRLEN_bm | PORT_OPC_PULLDOWN_gc; // NRF-PWRSW
	PORTC.PIN6CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc; // ESP-TXD
	PORTC.PIN7CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc; // ESP-RXD
	
	PORTD.PIN0CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc; // NRF-INT
	PORTD.PIN1CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc; // NRF-ENABLE
	PORTD.PIN2CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc; // NRF-TXD
	PORTD.PIN3CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc; // NRF-RXD
	PORTD.PIN4CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc; // ESP-IO??1
	PORTD.PIN5CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc; // ESP-IO??2
	
	PORTK.PIN4CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc; // PWR-Select Multiplexer ADC
	PORTK.PIN5CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc; // PWR-ON switch
	PORTK.PIN6CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc; // PWR-OK input
	PORTK.PIN7CTRL = PORT_SRLEN_bm | PORT_OPC_TOTEM_gc; // Timer Fout
	
	PORTQ.PIN2CTRL = PORT_SRLEN_bm | PORT_OPC_PULLUP_gc; // Button
	PORTQ.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc | PORT_SRLEN_bm | PORT_OPC_WIREDAND_gc; // LED Blue
	PORTR.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc | PORT_SRLEN_bm | PORT_OPC_WIREDAND_gc; // LED Green
	PORTR.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc | PORT_SRLEN_bm | PORT_OPC_WIREDAND_gc; // LED Red
}


static void initBoardID()
{
	if (hardware.board.xmegaID == 0)
	{
		NVM.CMD = NVM_CMD_READ_CALIB_ROW_gc;
		hardware.board.xmegaID =	pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, COORDY1)) << 0;
		hardware.board.xmegaID |=	pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, COORDY0)) << 8;
		hardware.board.xmegaID |=	(uint32_t) pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, COORDX1)) << 16;
		hardware.board.xmegaID |=	(uint32_t) pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, COORDX0)) << 24;
		NVM.CMD = 0;
	}
	
	switch (hardware.board.xmegaID)
	{
		case 0x22001500:
			hardware.board.id = 1;
			//hardware.board.deviceNames[10] = { "MBC1", "Bein M2", "MBC Dra", "Bein M1", "MBC 2", "MBC P", "MBC 4", "MBC Max", };
		break;
			
		case 0x03001500:
			hardware.board.id = 2;
			//hardware.board.deviceNames = { "Bein 1", "Bein 2", "Bein 3", "Bein 8", "Bein 7", "Bein 4", "Bein 5", "Bein 6", };
		break;
			
		case 0x04001500:
			hardware.board.id = 3;
			//hardware.board.deviceNames = { "??", "??", "??", "??", "??", "??", "??", "??", };
		break;
		
		case 0x01000b00:
			hardware.board.id = 10;
			//hardware.board.deviceNames = { "??", "??", "??", "??", "??", "??", "??", "??", };
		break;
		
		default:
			hardware.board.id = BOARDID_UNKNOWN;
			//hardware.board.deviceNames = { "??", "??", "??", "??", "??", "??", "??", "??", };
			appUIPrintln("Warning: unknown microcontroller ID: 0x%08lx ", hardware.board.xmegaID);
		break;
	}
}


char * deviceNameGet(uint8_t deviceNumber)
{
	static char str[16] = "?";
	
	switch (hardware.board.id) 
	{
		case 1: // MLG
			switch(deviceNumber)
			{
				case 0: strcpy_P(str, PSTR("MBC 1")); break;
 				case 1: strcpy_P(str, PSTR("Bein M2")); break;
				case 2: strcpy_P(str, PSTR("MBC Dra")); break;
				case 3: strcpy_P(str, PSTR("Bein M1")); break;
				case 4: strcpy_P(str, PSTR("MBC 2")); break;
				case 5: strcpy_P(str, PSTR("MBC P")); break;
				case 6: strcpy_P(str, PSTR("MBC 4")); break;
				case 7: strcpy_P(str, PSTR("MBC Max")); break;
			}
		break;
		
		case 2: // DRN
			if (hardware.board.id == 2) 
			{
				switch(deviceNumber)
				{
					case 0: strcpy_P(str, PSTR("Bein 1")); break;
					case 1: strcpy_P(str, PSTR("Bein 2")); break;
					case 2: strcpy_P(str, PSTR("Bein 3")); break;
					case 3: strcpy_P(str, PSTR("Bein 8")); break;
					case 4: strcpy_P(str, PSTR("Bein 7")); break;
					case 5: strcpy_P(str, PSTR("Bein 4")); break;
					case 6: strcpy_P(str, PSTR("Bein 5")); break;
					case 7: strcpy_P(str, PSTR("Bein 6")); break;
				}
			}
		break;
		
		case 3: // MLG
		default:
			switch(deviceNumber)
			{
// 				case 0: strcpy_P(str, PSTR("?")); break;
// 				case 1: strcpy_P(str, PSTR("?")); break;
// 				case 2: strcpy_P(str, PSTR("?")); break;
// 				case 3: strcpy_P(str, PSTR("?")); break;
// 				case 4: strcpy_P(str, PSTR("?")); break;
// 				case 5: strcpy_P(str, PSTR("?")); break;
// 				case 6: strcpy_P(str, PSTR("?")); break;
// 				case 7: strcpy_P(str, PSTR("?")); break;
			}
		break;
	}
	return str;
}
//strcpy_P(str, ((const char **)({ PSTR("MBC1"), PSTR("Bein M2"); })[deviceNumber]));
//strcpy_P(str, ({ PSTR("MBC1"), PSTR("Bein M2"), PSTR("MBC Dra"), PSTR("Bein M1"), PSTR("MBC 2"), PSTR("MBC P"), PSTR("MBC 4"), PSTR("MBC Max") })[deviceNumber] );
//return ((char **) { "MBC1", "Bein M2", "MBC Dra", "Bein M1", "MBC 2", "MBC P", "MBC 4", "MBC Max", })[deviceNumber];
//strcpy_P(str, (PGM_P) (device[deviceNumber]));
// 	hardware.board.deviceNames[10] = { "MBC1", "Bein M2", "MBC Dra", "Bein M1", "MBC 2", "MBC P", "MBC 4", "MBC Max", };
// 	hardware.board.deviceNames = { "Bein 1", "Bein 2", "Bein 3", "Bein 8", "Bein 7", "Bein 4", "Bein 5", "Bein 6", };
// 	hardware.board.deviceNames = { "??", "??", "??", "??", "??", "??", "??", "??", };


static void initClocks(const bool useExt32KHz, const uint8_t pllMultiplier, const CLK_SCLKSEL_t cpuClk, const CLK_PSADIV_t sysClkDivA)
{
	OSC.CTRL = OSC_RC2MEN_bm | OSC_RC32MEN_bm;
	
	if (useExt32KHz)
	{
		OSC.XOSCCTRL = OSC_FRQRANGE_2TO9_gc | 0 | OSC_XOSCSEL_32KHz_gc; // XTAL has 32kHz instead of 0.4-16 MHz crytal
		OSC.CTRL |= OSC_XOSCEN_bm;
		OSC.DFLLCTRL = OSC_RC32MCREF_XOSC32K_gc | OSC_RC2MCREF_XOSC32K_gc; // Calibrate Internal 2MHz & 32MHz with External 32.768kHz crystal
	}
	else
	{
		OSC.CTRL |= OSC_RC32KEN_bm;
		OSC.DFLLCTRL = OSC_RC32MCREF_USBSOF_gc | OSC_RC2MCREF_RC32K_gc;	// Calibrate 32MHz RC with USB SOF & 2MHz with Internal 32.768 RC
	}
	
	if (pllMultiplier > 0 && pllMultiplier < 32)
	{
		CLKSYS_PLL_Config(OSC_PLLSRC_RC2M_gc, pllMultiplier);
		OSC.CTRL |= OSC_PLLEN_bm; // PLL must be configurated before enabled
	}
	
	/* Configure CPU prescaler, PLL and external crystal */
	CLKSYS_Prescalers_Config(sysClkDivA, 0);

	// Use DFLL to change 32MHz to 48MHz for USB
	DFLLRC32M.COMP1 = ((F_USB / 1024) & 0xFF);
	DFLLRC32M.COMP2 = ((F_USB / 1024) >> 8);
	
	// Read calibration value 32MHz DFLL for USB
	NVM.CMD        = NVM_CMD_READ_CALIB_ROW_gc;
	DFLLRC32M.CALA = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, USBRCOSCA));
	DFLLRC32M.CALB = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, USBRCOSC));
	NVM.CMD        = NVM_CMD_NO_OPERATION_gc;
	
	while (!((OSC.STATUS & 0x1F) == (OSC.CTRL & 0x1F)));
	
	// Select CPU clock source
	CLKSYS_Main_ClockSource_Select(cpuClk);
	
	// Start DFLL
	DFLLRC32M.CTRL = DFLL_ENABLE_bm;

	if (cpuClk == CLK_SCLKSEL_RC2M_gc || (cpuClk == CLK_SCLKSEL_PLL_gc && OSC.PLLCTRL == OSC_PLLSRC_RC2M_gc))
	{
		DFLLRC2M.CTRL = DFLL_ENABLE_bm;
	}
	else //Disable 2MHz if unused
	{
		OSC.CTRL &= ~OSC_RC2MEN_bm;
	}
}
	

static void resetCausePrint()
{
	uint8_t resetCause = RST.STATUS;
	RST.STATUS = RST_PORF_bm | RST_EXTRF_bm | RST_BORF_bm | RST_WDRF_bm | RST_PDIRF_bm | RST_SRF_bm | RST_SDRF_bm;

	switch(resetCause) {
		case RST_EXTRF_bm:	appUIPrint("External!");		break;
		case RST_BORF_bm:	appUIPrint("Undervoltage!");	break;
		case RST_PDIRF_bm:	appUIPrint("Debug System!");	break;
		case RST_PORF_bm:	appUIPrint("Power-On!");		break;
		case RST_SRF_bm:	appUIPrint("Software!");		break;
		case RST_SDRF_bm:	appUIPrint("Spike!");			break;
		case RST_WDRF_bm:	appUIPrint("Watchdog!");		break;
		case 0:				appUIPrint("Unknown!");			break;
		default:			appUIPrint("%u!", resetCause);	break;
	}
	
	hardware.board.wdtCrashes = eeprom_read_byte(&wdtCrashesEEPROM);
	if (resetCause & RST_WDRF_bm && hardware.board.wdtCrashes < 250)
	{
		hardware.board.wdtCrashes++;
		eeprom_update_byte(&wdtCrashesEEPROM, hardware.board.wdtCrashes);
	}
}


static void initPowersaver(void) 
{
	PR_PRGEN =	(!PR_USB_bm) | ( PR_AES_bm) | ( PR_EBI_bm) | (!PR_RTC_bm) | (!PR_EVSYS_bm) | (!PR_DMA_bm);
	PR_PRPA =	(!PR_ADC_bm) | ( PR_AC_bm) | ( PR_DAC_bm);
	PR_PRPB =	(!PR_ADC_bm) | ( PR_AC_bm) | ( PR_DAC_bm);
	PR_PRPC =	(!PR_USART0_bm) | (!PR_USART1_bm) | (!PR_TWI_bm) | ( PR_SPI_bm) | (!PR_HIRES_bm) | (!PR_TC0_bm) | (!PR_TC1_bm);
	PR_PRPD =	(!PR_USART0_bm) | ( PR_USART1_bm) | ( PR_TWI_bm) | ( PR_SPI_bm) | (!PR_HIRES_bm) | (!PR_TC0_bm) | (!PR_TC1_bm);
	PR_PRPE =	( PR_USART0_bm) | ( PR_USART1_bm) | ( PR_TWI_bm) | ( PR_SPI_bm) | (!PR_HIRES_bm) | (!PR_TC0_bm) | (!PR_TC1_bm);
	PR_PRPF =	( PR_USART0_bm) | ( PR_USART1_bm) | ( PR_TWI_bm) | ( PR_SPI_bm) | (!PR_HIRES_bm) | (!PR_TC0_bm) | (!PR_TC1_bm);
}


void TMP112Read() // ADDR High => 0b 1001 001RW
{
	static bool isInitialized = false;
	static const uint8_t addr = 0b1001001;
		
	if (hardware.twi.status != TWI_STATUS_READY) {
		return;
	}

	if (!isInitialized) 
	{
		hardware.twi.outputData[0] = 0x01;
		hardware.twi.outputData[1] = 0x60;
		hardware.twi.outputData[2] = 0xE0;
		twiMasterWriteRead(addr, 3, 0);
		_delay_ms(2);
		
		if (hardware.twi.status != TWI_STATUS_READY) {
			appUIPrintln("TMP112 failed!");
			return;
		}
		
		hardware.twi.outputData[0] = 0x00;
		twiMasterWriteRead(addr, 1, 0);
		_delay_ms(2);
		
		if (hardware.twi.status != TWI_STATUS_READY) {
			appUIPrintln("TMP112 failed 2!");
			return;
		}
		appUIPrintln("TMP112 init success!");
		isInitialized = true;
	}
	
	twiMasterWriteRead(addr, 0, 2);
	_delay_ms(2);
	if (hardware.twi.result == TWIM_RESULT_OK && hardware.twi.bytesRead == 2) 
	{
		uint16_t temp = ((uint16_t) hardware.twi.inputData[0] << 4) | (hardware.twi.inputData[1] >> 4); 
		if (temp < 0x800) 
		{
			hardware.temperature = temp * 0.0625f;
		}
		else
		{
			hardware.temperature = -1.0f; // todo: negative numbers
		}
	}
}


void RX8900Read() // ADDR => 0b 0110 010RW
{
	static bool isInitialized = false;
	static const uint8_t addr = 0b0110010;
	
	if (hardware.twi.status != TWI_STATUS_READY) {
		return;
	}
	
	if (!isInitialized)
	{
		hardware.twi.outputData[0] = 0x0D;
		hardware.twi.outputData[1] = 0x5A;
		hardware.twi.outputData[2] = 0x02;
		hardware.twi.outputData[3] = 0x0;
		twiMasterWriteRead(addr, 1, 0);
		_delay_ms(10);
		
		if (hardware.twi.status != TWI_STATUS_READY) {
			appUIPrintln("TMP112 failed!");
			return;
		}
		
		hardware.twi.outputData[0] = 0x00;
		twiMasterWriteRead(addr, 1, 0);
		_delay_ms(10);
		
		if (hardware.twi.status != TWI_STATUS_READY) {
			appUIPrintln("TMP112 failed 2!");
			return;
		}
		appUIPrintln("TMP112 init success!");
		isInitialized = true;
	}
	
	if (hardware.twi.result == TWIM_RESULT_OK && hardware.twi.bytesRead == 7)
	{
		appUIPrintln("RX8900CE read success!");
		time_t timeRead = 
		{
			.second = (hardware.twi.inputData[0] & 0x0F) | (10 * (hardware.twi.inputData[0] & 0x70)),
			.minute = (hardware.twi.inputData[1] & 0x0F) | (10 * (hardware.twi.inputData[1] & 0x70)),
			.hour	= (hardware.twi.inputData[2] & 0x0F) | (10 * (hardware.twi.inputData[2] & 0x30)),
			.day	= (hardware.twi.inputData[4] & 0x0F) | (10 * (hardware.twi.inputData[4] & 0x30)),
			.month	= (hardware.twi.inputData[5] & 0x0F) | (10 * (hardware.twi.inputData[5] & 0x10)),
			.year	= (hardware.twi.inputData[6] & 0x0F) | (10 * (hardware.twi.inputData[6] & 0xF0)),
		};
		
		if (RTCIsValid(&timeRead)) {
			appUIPrintln("time is valid!");
		}
		appUIPrintln("time NOT valid!");
		hardware.time = timeRead;
	}
	twiMasterWriteRead(addr, 0, 7);
}


void ledController()
{
	static bool isLedBlue = false;
	
	
	if (usbIsConnected)
	{
		if ((isLedBlue = !isLedBlue)) {
			ledBlueEnable();
			events_t * ev;
			if ((ev = eventFind(&ledController))) {
				ev->timeLeft = 10;
			}
		}
		else {
			ledBlueDisable();
		}
	}
}


void ledRedEnable()
{
	PORTR.DIRSET = 0x02;
	PORTR.OUTCLR = 0x02;
}

void ledRedDisable()
{
	PORTR.DIRSET = 0x02;
	PORTR.OUTSET = 0x02;
}

bool ledRedGet()
{
	return (~PORTR.OUT & 0x02) && (PORTR.DIR & 0x02);
}


void ledGreenEnable()
{
	PORTR.DIRSET = 0x01;
	PORTR.OUTCLR = 0x01;
}

void ledGreenDisable()
{
	PORTR.DIRSET = 0x01;
	PORTR.OUTSET = 0x01;
}

bool ledGreenGet()
{
	return (~PORTR.OUT & 0x01) && (PORTR.DIR & 0x01);
}


void ledBlueEnable()
{
	PORTQ.DIRSET = 0x08;
	PORTQ.OUTCLR = 0x08;
}

void ledBlueDisable()
{
	PORTQ.DIRSET = 0x08;
	PORTQ.OUTSET = 0x08;
}

bool ledBlueGet()
{
	return (~PORTQ.OUT & 0x08) && (PORTQ.DIR & 0x08);
}



/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect()
{
	usbIsConnected = true;
	//LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect()
{
	usbIsConnected = false;
	//LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

void EVENT_USB_Device_Suspend()
{
	
}

void EVENT_USB_Device_WakeUp()
{
	
}

void EVENT_USB_Device_Reset()
{
	
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged()
{
	CDC_Device_ConfigureEndpoints(&USBSerialClass);

	//LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest()
{
	CDC_Device_ProcessControlRequest(&USBSerialClass);
}






























#define PORT_PIN_CTRL(_PORT, _PIN_bm, _VALUE) do{ PORTCFG.MPCMASK = _PIN_bm; _PORT.PIN0CTRL = _VALUE; } while(0);

// void GPIOTester(void)
// {
// 	uint8_t len = 5;
// 	PORT_t ports[] = { PORTA, PORTB, PORTC, PORTD, PORTE };
// 	uint8_t ports_normal[len], ports_down[len], ports_up[len];
//
// 	for(uint8_t i = 0; i < len; i++)
// 	{
// 		ports[i].DIR = 0;
//
// // 		PORT_PIN_CTRL(ports[i], 0xFF, 0);
// 		PORTCFG.MPCMASK = 0;
// 		ports[i].PIN0CTRL = 0xFF;
// 		ports_normal[i] = ports[i].IN;
//
// // 		PORT_PIN_CTRL(ports[i], 0xFF, PORT_OPC_PULLDOWN_gc);
// 		PORTCFG.MPCMASK = PORT_OPC_PULLDOWN_gc;
// 		ports[i].PIN0CTRL = 0xFF;
// 		ports_down[i] = ports[i].IN;
//
// // 		PORT_PIN_CTRL(ports[i], 0xFF, PORT_OPC_PULLUP_gc);
// 		PORTCFG.MPCMASK = PORT_OPC_PULLUP_gc;
// 		ports[i].PIN0CTRL = 0xFF;
// 		ports_up[i] = ports[i].IN;
//
// 		appUIPrintln("PORT%c:  Normal:%2x Down:%2x Up:%2x ", 'a' + i, ports_normal[i], ports_down[i], ports_up[i]);
// 	}
// }

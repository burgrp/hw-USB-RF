const int LED_PIN = 5;
const int RF_DATA_PIN = 2;
const int RF_DATA_EXTIN = RF_DATA_PIN;
const int SAFE_BOOT_PIN = 8;

class App
{
public:
	class : public ookey::rx::Decoder
	{
	public:
		void setTimerInterrupt(int time)
		{
				target::TC1.COUNT32.COUNT = 0;
			if (time)
			{
				target::TC1.COUNT32.CC[0].setCC(time);
				target::TC1.COUNT32.INTENSET.setOVF(1);
			}
			else
			{
				target::TC1.COUNT32.INTENSET.setOVF(0);
				target::TC1.COUNT32.CC[0].setCC(0xFFFFFFFF);
			}
		}

		void setRfPinInterrupt(bool enabled)
		{
			if (enabled)
			{
				target::EIC.INTENSET.setEXTINT(RF_DATA_EXTIN, 1);
			}
			else
			{
				target::EIC.INTENCLR.setEXTINT(RF_DATA_EXTIN, 1);
			}
		}

		void dataReceived(unsigned char *data, int len)
		{
			target::PORT.OUTTGL.setOUTTGL(1 << LED_PIN);
		}

	} decoder;

	void init()
	{

		atsamd::safeboot::init(SAFE_BOOT_PIN, false, LED_PIN);

		// enable external crystal oscillator		
		target::SYSCTRL.XOSC.setGAIN(3);
		target::SYSCTRL.XOSC.setXTALEN(1);
		target::SYSCTRL.XOSC.setENABLE(1);

		// GEN1: divide output to 4MHz to be suitable for 32kHz division later
		target::GCLK.GENDIV = 3 << 8 | 1;

		// GEN1: generator enabled, sourced from XOSC
		target::GCLK.GENCTRL = 1 << 16 | 0 << 8 | 1;

		// wait for XOSC ready
		while(!target::SYSCTRL.PCLKSR.getXOSCRDY());

		// GEN3: divide output to 32kHz
		target::GCLK.GENDIV = 125 << 8 | 3;

		// GEN3: generator enabled, sourced from GEN1
		target::GCLK.GENCTRL = 1 << 16 | 2 << 8 | 3; 

		// FLL48: sourced from GEN3 (32kHz)
		target::GCLK.CLKCTRL = 1 << 14 | 3 << 8 | 0x00; 
	
		// FLL48: enable
		// does not work: target::SYSCTRL.DFLLCTRL.setENABLE(1);
		target::SYSCTRL.DFLLCTRL = 1 << 1; 
		while (!target::SYSCTRL.PCLKSR.getDFLLRDY());

		// FLL48: coarse step
		target::SYSCTRL.DFLLVAL.setCOARSE(100);
		while (!target::SYSCTRL.PCLKSR.getDFLLRDY());

		// FLL48: fine step
		target::SYSCTRL.DFLLVAL.setFINE(10);
		while (!target::SYSCTRL.PCLKSR.getDFLLRDY());

		// FLL48: multiplier
		target::SYSCTRL.DFLLMUL.setMUL(1500);
		while (!target::SYSCTRL.PCLKSR.getDFLLRDY());

		// FLL48: switch to closed loop mode
		// does not work: target::SYSCTRL.DFLLCTRL.setMODE(1);		
		target::SYSCTRL.DFLLCTRL = 3 << 1; 

		
		target::PORT.OUT.setOUT(1 << LED_PIN);

		// GEN 4: output enabled, generator enabled, sourced from DFLL48
		target::GCLK.GENCTRL = 1 << 19 | 1 << 16 | 7 << 8 | 4; 


//target::PORT.OUTTGL.setOUTTGL(1 << LED_PIN);

		// PA14 to alt. function H (GCLK_IO_4)
		target::PORT.PMUX[7].setPMUXE(7);
		target::PORT.PINCFG[14].setPMUXEN(1);


		// target::SYSCTRL.OSC8M.setPRESC(0);

		// target::PM.APBBMASK.setPORT(1);
		// target::PORT.DIRSET.setDIRSET(1 << LED_PIN);
		// target::PORT.DIRCLR.setDIRCLR(1 << RF_DATA_PIN);
		// target::PORT.PINCFG[RF_DATA_PIN].setINEN(1);
		// target::PORT.PINCFG[RF_DATA_PIN].setPMUXEN(1);

		// target::GCLK.CLKCTRL = 1 << 14 | 0 << 8 | 0x05; // EIC from generator 0
		// target::PM.APBAMASK.setEIC(1);
		// target::EIC.CONFIG.setSENSE(RF_DATA_EXTIN, 1);
		// target::EIC.CTRL.setENABLE(1);

		// target::GCLK.CLKCTRL = 1 << 14 | 0 << 8 | 0x12; // TC1 & TC2 from generator 0
		// target::PM.APBCMASK.setTC(1, 1);
		// target::TC1.COUNT32.CTRLA.setMODE(2);		 // 32bit
		// target::TC1.COUNT32.CTRLA.setPRESCALER(0x5); // Prescaler: GCLK_TC/256
		// target::TC1.COUNT32.CTRLA.setWAVEGEN(1);	 // top = CC0
		// target::TC1.COUNT32.CTRLA.setENABLE(1);

		// decoder.init(1);

		// target::NVIC.ISER.setSETENA(1 << target::interrupts::External::TC1);
		// target::NVIC.ISER.setSETENA(1 << target::interrupts::External::EIC);
	}

} app;

//target::PORT.OUTTGL.setOUTTGL(1 << LED_PIN);

void interruptHandlerTC1()
{
	app.decoder.handleTimerInterrupt((target::PORT.IN >> RF_DATA_PIN) & 1);
	target::TC1.COUNT32.INTFLAG.setOVF(1);
}

void interruptHandlerEIC()
{
	if (target::EIC.INTFLAG.getEXTINT(RF_DATA_EXTIN))
	{
		app.decoder.handleRfPinInterrupt(target::TC1.COUNT32.COUNT);
		target::EIC.INTFLAG.setEXTINT(RF_DATA_EXTIN, 1);
	}
}

void initApplication()
{
	genericTimer::clkHz = 2E6;
	app.init();
}

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

		target::SYSCTRL.OSC8M.setPRESC(0);

		target::PM.APBBMASK.setPORT(1);
		target::PORT.DIRSET.setDIRSET(1 << LED_PIN);
		target::PORT.DIRCLR.setDIRCLR(1 << RF_DATA_PIN);
		target::PORT.PINCFG[RF_DATA_PIN].setINEN(1);
		target::PORT.PINCFG[RF_DATA_PIN].setPMUXEN(1);

		target::GCLK.CLKCTRL = 1 << 14 | 0 << 8 | 0x05; // EIC from generator 0
		target::PM.APBAMASK.setEIC(1);
		target::EIC.CONFIG.setSENSE(RF_DATA_EXTIN, 1);
		target::EIC.CTRL.setENABLE(1);

		target::GCLK.CLKCTRL = 1 << 14 | 0 << 8 | 0x12; // TC1 & TC2 from generator 0
		target::PM.APBCMASK.setTC(1, 1);
		target::TC1.COUNT32.CTRLA.setMODE(2);		 // 32bit
		target::TC1.COUNT32.CTRLA.setPRESCALER(0x5); // Prescaler: GCLK_TC/256
		target::TC1.COUNT32.CTRLA.setWAVEGEN(1);	 // top = CC0
		target::TC1.COUNT32.CTRLA.setENABLE(1);

		decoder.init(1);

		target::NVIC.ISER.setSETENA(1 << target::interrupts::External::TC1);
		target::NVIC.ISER.setSETENA(1 << target::interrupts::External::EIC);

		target::NVIC.IPR7.set
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

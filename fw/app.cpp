const int LED_PIN = 5;
const int RF_DATA_PIN = 2;
const int RF_DATA_EXTIN = RF_DATA_PIN;
const int SAFE_BOOT_PIN = 8;

class App {
public:
  class : public ookey::rx::Decoder {
  public:
    void setTimerInterrupt(int time) {
      // 	target::TC1.COUNT32.COUNT = 0;
      // if (time)
      // {
      // 	target::TC1.COUNT32.CC[0].setCC(time);
      // 	target::TC1.COUNT32.INTENSET.setOVF(1);
      // }
      // else
      // {
      // 	target::TC1.COUNT32.INTENSET.setOVF(0);
      // 	target::TC1.COUNT32.CC[0].setCC(0xFFFFFFFF);
      // }
    }

    void setRfPinInterrupt(bool enabled) {
      // if (enabled)
      // {
      // 	target::EIC.INTENSET.setEXTINT(RF_DATA_EXTIN, 1);
      // }
      // else
      // {
      // 	target::EIC.INTENCLR.setEXTINT(RF_DATA_EXTIN, 1);
      // }
    }

    void dataReceived(unsigned char *data, int len) {
      target::PORT.OUTTGL.setOUTTGL(1 << LED_PIN);
    }

  } decoder;

  void init() {
    atsamd::safeboot::init(SAFE_BOOT_PIN, false, LED_PIN);

    //////////////////////////////////////////////////////////////////////////

    // enable external crystal oscillator
    target::SYSCTRL.XOSC.setGAIN(3);
    target::SYSCTRL.XOSC.setXTALEN(1);
    target::SYSCTRL.XOSC.setENABLE(1);

    //////////////////////////////////////////////////////////////////////////

    // GEN1: divide output to 4MHz to be suitable for 32kHz division later
    target::GCLK.GENDIV = 3 << 8 | 1;

    // GEN1: generator enabled, clocked from XOSC
    target::GCLK.GENCTRL = 1 << 16 | 0 << 8 | 1;

    // wait for XOSC ready
    while (!target::SYSCTRL.PCLKSR.getXOSCRDY())
      ;

    //////////////////////////////////////////////////////////////////////////

    // GEN3: divide output to 32kHz
    target::GCLK.GENDIV = 125 << 8 | 3;

    // GEN3: generator enabled, clocked from GEN1
    target::GCLK.GENCTRL = 1 << 16 | 2 << 8 | 3;

    //////////////////////////////////////////////////////////////////////////

    // // FLL48: clocked from GEN3 (32kHz)
    // target::GCLK.CLKCTRL = 1 << 14 | 3 << 8 | 0x00;

    // // FLL48: enable
    // // does not work: target::SYSCTRL.DFLLCTRL.setENABLE(1);
    // target::SYSCTRL.DFLLCTRL = 1 << 1;
    // while (!target::SYSCTRL.PCLKSR.getDFLLRDY())
    //   ;

    // // FLL48: coarse step
    // target::SYSCTRL.DFLLVAL.setCOARSE(100);
    // while (!target::SYSCTRL.PCLKSR.getDFLLRDY())
    //   ;

    // // FLL48: fine step
    // target::SYSCTRL.DFLLVAL.setFINE(10);
    // while (!target::SYSCTRL.PCLKSR.getDFLLRDY())
    //   ;

    // // FLL48: multiplier
    // target::SYSCTRL.DFLLMUL.setMUL(1500);
    // while (!target::SYSCTRL.PCLKSR.getDFLLRDY())
    //   ;

    // // FLL48: switch to closed loop mode
    // // does not work: target::SYSCTRL.DFLLCTRL.setMODE(1);
    // target::SYSCTRL.DFLLCTRL = 3 << 1;

    //////////////////////////////////////////////////////////////////////////

    // GEN0: clocked from GEN1 (4MHz)
    target::GCLK.GENCTRL = 1 << 16 | 2 << 8 | 0;

    //////////////////////////////////////////////////////////////////////////

    // PLL96: clocked from GEN3 (32kHz)
    target::GCLK.CLKCTRL = 1 << 14 | 3 << 8 | 0x01;

    // PLL96: reference clock is GCLK_DPLL
    target::SYSCTRL.DPLLCTRLB.setREFCLK(2);

    // PLL96: output to 2*27.1383MHz, divide by 2 later in GEN4
    target::SYSCTRL.DPLLRATIO.setLDR(1695);
    target::SYSCTRL.DPLLRATIO.setLDRFRAC(2);

    // PLL96: enable
    target::SYSCTRL.DPLLCTRLA.setENABLE(1);

    //////////////////////////////////////////////////////////////////////////

    // GEN 4: divide PLL96 by two
    target::GCLK.GENDIV = 2 << 8 | 4;

    // GEN 4: output enabled, generator enabled, clocked from PLL96
    target::GCLK.GENCTRL = 1 << 19 | 1 << 16 | 8 << 8 | 4;

    // PA14 to alt. function H (GCLK_IO_4)
    target::PORT.PMUX[7].setPMUXE(7);
    target::PORT.PINCFG[14].setPMUXEN(1);

    //////////////////////////////////////////////////////////////////////////

    target::PM.APBBMASK.setPORT(1);
    target::PORT.DIRSET.setDIRSET(1 << LED_PIN);
    target::PORT.DIRCLR.setDIRCLR(1 << RF_DATA_PIN);
    target::PORT.PINCFG[RF_DATA_PIN].setINEN(1);
    target::PORT.PINCFG[RF_DATA_PIN].setPMUXEN(1);

    //////////////////////////////////////////////////////////////////////////

    // EIC: clocked from generator 0
    target::GCLK.CLKCTRL = 1 << 14 | 0 << 8 | 0x05;

    // EIC: RF_DATA_EXTIN interrupt on rising edge
    target::EIC.CONFIG.setSENSE(RF_DATA_EXTIN, 1);


    // EIC: RF_DATA_EXTIN filter enabled
    target::EIC.CONFIG.setFILTEN(RF_DATA_EXTIN, 1);

    // EIC: enable
    target::EIC.CTRL.setENABLE(1);

    // NVIC: EIC interrupt enable
    target::NVIC.ISER.setSETENA(1 << target::interrupts::External::EIC);

    //////////////////////////////////////////////////////////////////////////

    // TC1 & TC2: clocked from generator 0
    target::GCLK.CLKCTRL =
        1 << 14 | 0 << 8 | 0x12; // TC1 & TC2 from generator 0

    // PM: power-on TC1
    target::PM.APBCMASK.setTC(1, 1);

    // TC1: 32bit mode
    target::TC1.COUNT32.CTRLA.setMODE(2);

    // TC1: prescale to tick on 1us
    target::TC1.COUNT32.CTRLA.setPRESCALER(0x2);

    target::TC1.COUNT32.CTRLBSET.setONESHOT(1);
    target::TC1.COUNT32.CTRLBSET.setCMD(2); // STOP
    target::TC1.COUNT32.CC[0] = 1000;
    target::TC1.COUNT32.INTENSET.setMC(0, 1);
    target::TC1.COUNT32.CC[1] = 2500;
    target::TC1.COUNT32.INTENSET.setMC(1, 1);

    // TC1: enable
    target::TC1.COUNT32.CTRLA.setENABLE(1);

    target::NVIC.ISER.setSETENA(1 << target::interrupts::External::TC1);

    //////////////////////////////////////////////////////////////////////////

    decoder.init(1);

    // EIC: RF_DATA_EXTIN interrupt enabled
    target::EIC.INTENSET.setEXTINT(RF_DATA_EXTIN, 1);    
  }

} app;

void interruptHandlerTC1() {
  if (target::TC1.COUNT32.INTFLAG.getMC(0)) {
    target::TC1.COUNT32.INTFLAG.setMC(0, 1);
    app.decoder.pushBit((target::PORT.IN.getIN() >> RF_DATA_PIN) & 1);
  }

  if (target::TC1.COUNT32.INTFLAG.getMC(1)) {
    target::TC1.COUNT32.INTFLAG.setMC(1, 1);    
    app.decoder.restart();
  }  
    
}

void interruptHandlerEIC() {
  if (target::EIC.INTFLAG.getEXTINT(RF_DATA_EXTIN)) {
    target::TC1.COUNT32.CTRLBSET = 2 << 6; // STOP
    target::TC1.COUNT32.CTRLBSET = 1 << 6; // RESTART
    target::EIC.INTFLAG.setEXTINT(RF_DATA_EXTIN, 1);
  }
}

void initApplication() {
  genericTimer::clkHz = 1E6; // FIXME
  mark::init(4);
  app.init();
}

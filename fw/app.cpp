const int LED_PIN = 5;
const int RF_DATA_PIN = 2;
const int RF_DATA_EXTIN = RF_DATA_PIN;
const int SAFE_BOOT_PIN = 8;

class App {
public:
  class : public ookey::rx::Decoder {
  public:
    void dataReceived(unsigned char *data, int len) {
      target::PORT.OUTTGL.setOUTTGL(1 << LED_PIN);
    }

  } decoder;

  void init() {
    atsamd::safeboot::init(SAFE_BOOT_PIN, false, LED_PIN);

    target::gclk::GENDIV::Register workGendiv;
    target::gclk::GENCTRL::Register workGenctrl;
    target::gclk::CLKCTRL::Register workClkctrl;

    //////////////////////////////////////////////////////////////////////////

    // enable external crystal oscillator
    target::SYSCTRL.XOSC.setGAIN(target::sysctrl::XOSC::GAIN::_16MHZ);
    target::SYSCTRL.XOSC.setXTALEN(true);
    target::SYSCTRL.XOSC.setENABLE(true);

    //////////////////////////////////////////////////////////////////////////

    // GEN1: divide output to 4MHz to be suitable for 32kHz division later
    target::GCLK.GENDIV = workGendiv.zero().setID(1).setDIV(3);

    // GEN1: generator enabled, clocked from XOSC
    target::GCLK.GENCTRL = workGenctrl.zero()
                               .setID(1)
                               .setSRC(target::gclk::GENCTRL::SRC::XOSC)
                               .setGENEN(true);

    // wait for XOSC ready
    while (!target::SYSCTRL.PCLKSR.getXOSCRDY())
      ;

    //////////////////////////////////////////////////////////////////////////

    // GEN3: divide output to 32kHz
    target::GCLK.GENDIV = workGendiv.zero().setID(3).setDIV(125);

    // GEN3: generator enabled, clocked from GEN1
    target::GCLK.GENCTRL = workGenctrl.zero()
                               .setID(3)
                               .setSRC(target::gclk::GENCTRL::SRC::GCLKGEN1)
                               .setGENEN(true);

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
    target::GCLK.GENCTRL = workGenctrl.zero()
                               .setID(0)
                               .setSRC(target::gclk::GENCTRL::SRC::GCLKGEN1)
                               .setGENEN(true);

    //////////////////////////////////////////////////////////////////////////

    // PLL96: clocked from GEN3 (32kHz)
    target::GCLK.CLKCTRL = workClkctrl.zero()
                               .setID(target::gclk::CLKCTRL::ID::FDPLL)
                               .setGEN(target::gclk::CLKCTRL::GEN::GCLK3)
                               .setCLKEN(true);

    // PLL96: reference clock is GCLK_DPLL
    target::SYSCTRL.DPLLCTRLB.setREFCLK(
        target::sysctrl::DPLLCTRLB::REFCLK::GCLK);

    // PLL96: output to 2*27.1383MHz, divide by 2 later in GEN4
    target::SYSCTRL.DPLLRATIO.setLDR(1695);
    target::SYSCTRL.DPLLRATIO.setLDRFRAC(2);

    // PLL96: enable
    target::SYSCTRL.DPLLCTRLA.setENABLE(true);

    //////////////////////////////////////////////////////////////////////////

    // GEN 4: divide PLL96 by two
    target::GCLK.GENDIV = workGendiv.zero().setID(4).setDIV(2);

    // GEN 4: output enabled, generator enabled, clocked from PLL96
    target::GCLK.GENCTRL = workGenctrl.zero()
                               .setID(4)
                               .setSRC(target::gclk::GENCTRL::SRC::DPLL96M)
                               .setOE(true)
                               .setGENEN(true);

    // PA14 to alt. function H (GCLK_IO_4)
    target::PORT.PMUX[7].setPMUXE(target::port::PMUX::PMUXE::H);
    target::PORT.PINCFG[14].setPMUXEN(true);

    //////////////////////////////////////////////////////////////////////////

    target::PM.APBBMASK.setPORT(1);
    target::PORT.DIRSET.setDIRSET(1 << LED_PIN);
    target::PORT.DIRCLR.setDIRCLR(1 << RF_DATA_PIN);
    target::PORT.PINCFG[RF_DATA_PIN].setINEN(true);
    target::PORT.PINCFG[RF_DATA_PIN].setPMUXEN(true);

    //////////////////////////////////////////////////////////////////////////

    // EIC: clocked from generator 0
    target::GCLK.CLKCTRL = 1 << 14 | 0 << 8 | 0x05;
    target::GCLK.CLKCTRL = workClkctrl.zero()
                               .setID(target::gclk::CLKCTRL::ID::EIC)
                               .setGEN(target::gclk::CLKCTRL::GEN::GCLK0)
                               .setCLKEN(true);

    // EIC: RF_DATA_EXTIN interrupt on rising edge
    target::EIC.CONFIG.setSENSE(RF_DATA_EXTIN,
                                target::eic::CONFIG::SENSE::RISE);

    // EIC: RF_DATA_EXTIN filter enabled
    target::EIC.CONFIG.setFILTEN(RF_DATA_EXTIN, true);

    // EIC: enable
    target::EIC.CTRL.setENABLE(true);

    // NVIC: EIC interrupt enable
    target::NVIC.ISER.setSETENA(1 << target::interrupts::External::EIC);

    //////////////////////////////////////////////////////////////////////////

    // TC1 & TC2: clocked from generator 0
      target::GCLK.CLKCTRL = workClkctrl.zero()
                                 .setID(target::gclk::CLKCTRL::ID::TC1_TC2)
                                 .setGEN(target::gclk::CLKCTRL::GEN::GCLK0)
                                 .setCLKEN(true);

    // PM: power-on TC1
    target::PM.APBCMASK.setTC(1, true);

    // TC1: 32bit mode
    target::TC1.COUNT32.CTRLA.setMODE(
        target::tc::COUNT32::CTRLA::MODE::COUNT32);

    // TC1: prescale to tick on 1us
    target::TC1.COUNT32.CTRLA.setPRESCALER(
        target::tc::COUNT32::CTRLA::PRESCALER::DIV4);

    target::TC1.COUNT32.CTRLBSET.setONESHOT(true);
    target::TC1.COUNT32.CTRLBSET.setCMD(
        target::tc::COUNT32::CTRLBSET::CMD ::STOP); // STOP
    target::TC1.COUNT32.CC[0] = 1000;
    target::TC1.COUNT32.INTENSET.setMC(0, true);
    target::TC1.COUNT32.CC[1] = 2500;
    target::TC1.COUNT32.INTENSET.setMC(1, true);

    // TC1: enable
    target::TC1.COUNT32.CTRLA.setENABLE(true);

    target::NVIC.ISER.setSETENA(1 << target::interrupts::External::TC1);

    //////////////////////////////////////////////////////////////////////////

    decoder.init(1);

    // EIC: RF_DATA_EXTIN interrupt enabled
    target::EIC.INTENSET.setEXTINT(RF_DATA_EXTIN, true);
  }

} app;

void interruptHandlerTC1() {
  if (target::TC1.COUNT32.INTFLAG.getMC(0)) {
    target::TC1.COUNT32.INTFLAG.setMC(0, true);
    app.decoder.pushBit((target::PORT.IN.getIN() >> RF_DATA_PIN) & 1);
  }

  if (target::TC1.COUNT32.INTFLAG.getMC(1)) {
    target::TC1.COUNT32.INTFLAG.setMC(1, true);
    app.decoder.restart();
  }
}

void interruptHandlerEIC() {
  if (target::EIC.INTFLAG.getEXTINT(RF_DATA_EXTIN)) {
    target::tc::COUNT32::CTRLBSET::Register workCTRLBSET;
    target::TC1.COUNT32.CTRLBSET = workCTRLBSET.zero().setCMD(target::tc::COUNT32::CTRLBSET::CMD::STOP);
    target::TC1.COUNT32.CTRLBSET = workCTRLBSET.zero().setCMD(target::tc::COUNT32::CTRLBSET::CMD::RETRIGGER);
    target::EIC.INTFLAG.setEXTINT(RF_DATA_EXTIN, true);
  }
}

void initApplication() {
  genericTimer::clkHz = 1E6; // FIXME
  app.init();
}

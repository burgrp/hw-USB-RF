namespace usb {

typedef struct {
  unsigned char *ADDR;
  unsigned long PCKSIZE;
  unsigned short EXTREG;
  unsigned char STATUS_BK;
  unsigned char Reserved[2];
} EPBank;

typedef struct {
  EPBank banks[2];
} EPConfig;

class Device {
public:
  unsigned char ep0dataOut[64];
  unsigned char ep0dataIn[64];

  EPConfig epConfig[1] = {
      {{{ep0dataOut,
         (unsigned long)(3 << 28 | 1 << 31), // 64bytes, auto ZLP
         0, 0, 0

        },
        {ep0dataIn,
         (unsigned long)(3 << 28 | 1 << 31 | 18 << 0), // 64bytes, auto ZLP
         0, 0, 0}}}};

  void init() {

    target::PM.APBBMASK.setUSB(true);
    target::PM.AHBMASK.setUSB(true);

    target::PORT.PMUX[12].setPMUXE(target::port::PMUX::PMUXE::G);
    target::PORT.PMUX[12].setPMUXO(target::port::PMUX::PMUXO::G);
    target::PORT.PINCFG[24].setPMUXEN(true);
    target::PORT.PINCFG[25].setPMUXEN(true);

    // target::USB.DEVICE.CTRLA.setSWRST(true);
    // while (!target::USB.DEVICE.SYNCBUSY.getSWRST())
    //   ;


    target::USB.DEVICE.DESCADD = (unsigned int)(&epConfig);

    // target::USB.DEVICE.CTRLA.setSWRST(true);
    // while (!target::USB.DEVICE.SYNCBUSY.getSWRST())
    //   ;

    unsigned long long NVM_CALIBRATION =
        *((unsigned long long *)(void *)0x806020);

    target::USB.DEVICE.PADCAL.setTRANSN((NVM_CALIBRATION >> 45) & 0x1F);
    target::USB.DEVICE.PADCAL.setTRANSP((NVM_CALIBRATION >> 50) & 0x1F);

    target::USB.DEVICE.CTRLA.setENABLE(true);
    while (!target::USB.DEVICE.SYNCBUSY.getENABLE())
      ;

    target::USB.DEVICE.DESCADD = (unsigned int)(&epConfig);


    target::USB.DEVICE.CTRLB.setSPDCONF(target::usb::DEVICE::CTRLB::SPDCONF::FS);

    target::USB.DEVICE.EPCFG[0].reg.setEPTYPE(0, 1).setEPTYPE(1, 1);

    //target::USB.DEVICE.EPSTATUSSET[0].reg.setBK_RDY(0, true);

    target::USB.DEVICE.EPINTENSET[0]
        .reg.setRXSTP(true)
        .setTRCPT(0, true)
        .setTRCPT(1, true);
    target::USB.DEVICE.INTENSET.setEORST(true);//.setWAKEUP(true);

    target::USB.DEVICE.CTRLB.setDETACH(false);
  }

  void handleInterrupt() {
    if (target::USB.DEVICE.INTFLAG.getWAKEUP()) {
      target::USB.DEVICE.INTENCLR.setWAKEUP(true);
      target::USB.DEVICE.CTRLB.setUPRSM(true);
    } else {
      volatile int x;
      x++;
    }
  }
};

} // namespace usb
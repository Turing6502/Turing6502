/*
 * (C) 2022 Matt Regan
 */
#define REPEATS     5
#define DATA0       0x01
#define DATA1       0x02
#define ClockRAW    0x10
#define Reset       0x20
#define RightBAR    0x01
#define DataClk     0x02
#define SerialData  0x04
#define AddrClk     0x08
#define MRead       0x10
#define MWrite      0x20

#define DATA            141
#define MARLO           150
#define MARHI           159


//  PortB0 = DATA0
//  PortB1 = DATA1
//  PortB4 = ClockRAW
//  PortB5 = Reset
//  PortC0 = RightBAR
//  PortC1 = DataClk
//  PortC2 = SerialData
//  PortC3 = AddrClk
//  PortC4 = MRead
//  PortC5 = MWrite

void setup() {
  Serial.begin (1000000);
  while (!Serial) {
    //  Wait for serial port
  }
  DDRB = ClockRAW|Reset;
  DDRC = 0x00;

  PORTB = Reset;
  delay (100);
  PORTB = 0x00;
  delay (100);
}
unsigned char notePad [256];
unsigned long notePadPointer = 0;
void loop() {
  for (int j=0; j<10000; j++) {
    for (int i=0; i<REPEATS; i++) PORTB = 0x00;
    for (int i=0; i<REPEATS; i++) PORTB = ClockRAW;
    if (!(notePadPointer & 0x7FF00)) {
      notePad [notePadPointer&0xFF] = PINB & 0x03;
    }
    unsigned char CPort;
    CPort = PINC;
    if (CPort&MWrite) {
      unsigned char DBus = 0, marLo = 0, marHi = 0;
  
      //  Extract write address and data from notepad
      for (int i = 0; i < 8; i++) {
          if (notePad[DATA + i] == 0x02)      DBus |= (1 << (7 - i));
          if (notePad[MARLO + i] == 0x02)     marLo |= (1 << (7 - i));
          if (notePad[MARHI + i] == 0x02)     marHi |= (1 << (7 - i));
      }
      unsigned int ABus = (marHi << 8) + marLo;
      Serial.write ( (ABus>>10)&0x3f);
      Serial.write (((ABus>>4)&0x3f)|0x40);
      Serial.write (((ABus<<2)&0x3c)|((DBus>>6)&0x03)|0x80);
      Serial.write ( (DBus&0x3f)|0xc0); 
    }
    notePadPointer += (CPort&0x01) ? -1:1;
  }
}









void loop_UPDN() {
  PORTB = 0x00;
  DDRB = ClockRAW|Reset|DATA0|DATA1;
  if (Serial.available()) {
    unsigned char symbol = Serial.read();
    for (int i=0; i<REPEATS; i++) PORTB = symbol&(DATA0|DATA1);
    PORTB = ClockRAW | (symbol&(DATA0|DATA1));
    DDRB = ClockRAW|Reset;
    for (int i=0; i<REPEATS; i++) PORTB = ClockRAW;
    symbol = ((PINB&(DATA0|DATA1))<<6) | (PINC&0x3F);
    Serial.write (symbol);
  }
}


unsigned int ABus = 0;
unsigned char DBus = 0;
void loop_x() {
  for (int j=0; j<10000; j++) {
    for (int i=0; i<REPEATS; i++) PORTB = 0x00;
    for (int i=0; i<REPEATS; i++) PORTB = ClockRAW;
    unsigned char CPort = PINC;
    if (CPort&AddrClk) ABus = ABus/2 | ((CPort&SerialData)?0x8000:0x00);
    if (CPort&DataClk) DBus = DBus/2 | ((CPort&SerialData)?0x80:0x00);
    if ((CPort&MWrite)||(CPort&MRead)) {
      Serial.write ( (ABus>>10)&0x3f);
      Serial.write (((ABus>>4)&0x3f)|0x40);
      Serial.write (((ABus<<2)&0x3c)|((DBus>>6)&0x03)|0x80);
      Serial.write ( (DBus&0x3f)|0xc0);
    }
  }
}

/*
 *   for (long j=0; j<0x80000; j++) {
    for (int i=0; i<10; i++) PORTB = ClockRAW;
    for (int i=0; i<10; i++) PORTB = 0x00;
  }
*/

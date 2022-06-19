//
#include "framework.h"
#include <iostream>
#include <fstream>
using namespace std;
extern HDC g_hdc;

#define     LEFT            -1
#define     RIGHT           1
#define     RULES           0x4000
#define     SYMBOLS         4
#define     NOTEPADSIZE     0x100000
#define     NOTEPADMASK     0x07ffff
#define     WRITEMEMORY     7477
#define     DATA            141
#define     MARLO           150
#define     MARHI           159
#define     WHITE           0x00ffffff
#define     BLACK           0x00000000
#define     MRead           0x10
#define     MWrite          0x20

struct rule {
    int next;
    int direction;
    int writeSymbol;
    unsigned int control;
} ruleBook[RULES][SYMBOLS];

char notePad[NOTEPADSIZE];

void    LoadMachine()
{
    int i = 0;
    unsigned char buffer[4];

    //  Read the Rulebook from a file
    ifstream file("D://AppleFT//Turing6502Rom.bin", ios::in | ios::binary);
    for (int j = 0; j < SYMBOLS; j++) {
        i = 0;
        while (file.is_open() && (i < RULES)) {
            file.read((char *) & buffer, 3);
            unsigned int behaviour = buffer[0] + (buffer[1] << 8) + (buffer[2] << 16);
            ruleBook[i][j].next = behaviour & 0x3fff;   
            ruleBook[i][j].writeSymbol = (behaviour >> 14) & 0x03;
            ruleBook[i][j].direction = (behaviour & 0x10000) ? LEFT : RIGHT;
            ruleBook[i][j].control = buffer[2];
            i++;
        }
    }
    file.close();

    //  Read the Notepad from a file
    file.open("D://AppleFT//PacmanTuringTape6502.bin", ios::in | ios::binary);
    i = 0;
    while (file.is_open() && (i < NOTEPADSIZE)) {
        file.read((char*)&notePad[i++], 1);
    }
    file.close();
}

DWORD   btsIO;
HANDLE  hDevice;
bool    arduinoWorking = false;

void InitArduino()
{
    arduinoWorking = false;
    hDevice = CreateFileA("COM3", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (hDevice != INVALID_HANDLE_VALUE) {
        DCB lpTest;
        GetCommState(hDevice, &lpTest);
        lpTest.BaudRate = 1000000;
        lpTest.ByteSize = 8;
        lpTest.Parity = NOPARITY;
        lpTest.StopBits = ONESTOPBIT;
        SetCommState(hDevice, &lpTest);
        arduinoWorking = true;
    }
}

bool    WriteArduino(unsigned char value)
{
    char temp[2];
    if (hDevice == INVALID_HANDLE_VALUE) return false;
    temp[0] = value;
    WriteFile(hDevice, temp, 1, &btsIO, NULL);
    return true;
}

unsigned char   ReadArduino()
{
    char temp[2];
    if (hDevice == INVALID_HANDLE_VALUE) return 0;
    temp[0] = 0;
    ReadFile(hDevice, temp, 1, &btsIO, NULL);
    return temp[0];
}

bool    CloseArduino()
{
    if (hDevice == INVALID_HANDLE_VALUE) return false;
    CloseHandle(hDevice);
    return true;
}

int     DeWozAddress (unsigned int address) 
{
    int line;
    line = ((address & 0x7f) / 40) << 6;
    line += (address & 0x380) >> 4;
    line += (address & 0x1c00) >> 10;
    return line;
}

void    SetPix(int column, int line, unsigned int colour)
{
    int scale = 4;
    for (int u = 0; u < scale; u++) {
        for (int v = 0; v < scale; v++) {
            SetPixel(g_hdc, column * scale + u, line * scale + v, colour);
        }
    }
}

void    WriteMemory(unsigned int address, unsigned char data) {
    unsigned int column = 0, line = 0;
    if ((address & 0xe000) == 0x2000) {
        column = (address & 0x7f) % 40;
        if ((address & 0x7f) > 119) return;
        line = DeWozAddress(address);
        for (int i = 0; i < 7; i++) {
            SetPix(column * 7 + i, line, (data & (1 << i)) ? WHITE : BLACK);
        }
    }
}

void    WriteMemory()
{
    unsigned char data = 0, marLo = 0, marHi = 0;

    //  Extract write address and data from notepad
    for (int i = 0; i < 8; i++) {
        if (notePad[DATA + i] == 0x02)      data |= (1 << (7 - i));
        if (notePad[MARLO + i] == 0x02)     marLo |= (1 << (7 - i));
        if (notePad[MARHI + i] == 0x02)     marHi |= (1 << (7 - i));
    }
    unsigned int address = (marHi << 8) + marLo;
    WriteMemory(address, data);
}

unsigned int ABus = 0;
unsigned char DBus = 0;
int ComputeTuring()
{
    if (!arduinoWorking)    return 0;
    unsigned int value = ReadArduino();
    switch (value >> 6) {
    case 0:     ABus = (value & 0x3f) << 10;        break;
    case 1:     ABus |= (value & 0x3f) << 4;        break;
    case 2:     
        ABus |= (value & 0x3c) >> 2;
        DBus = (value & 0x03) << 6;
        break;
    case 3:
        DBus |= (value & 0x3f);
        if ((ABus>=0x0000)&&(ABus<0x10000)) WriteMemory(ABus, DBus);
        ABus &= 0xfff0;
        DBus = 0;
        break;
    }
    return 1;
}

// Turing Machine.cpp : This file contains the 'main' function. Program execution begins and ends there.
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
#define     WHITE           0xffffff
#define     BLACK           0x000000

struct rule {
    int next;
    int direction;
    int writeSymbol;
} ruleBook[RULES][SYMBOLS];
char    notePad[NOTEPADSIZE];

//  Encode the rules for dividing by 2
void ProgramRuleBook()
{
    //  Load the rulebook and the notepad from external files
    int i = 0;
    unsigned char buffer[4];
 
    ifstream file ("D:\\AppleFT\\Turing6502Rom.bin", ios::in | ios::binary);
    for (int j = 0; j < SYMBOLS; j++) {
        i = 0;
        while (file.is_open() && (i < RULES)) {
            file.read((char*)&buffer, 3);
            unsigned int behaviour = buffer[0] + (buffer[1] << 8) + (buffer[2] << 16);
            ruleBook[i][j].next = behaviour & (RULES - 1);
            ruleBook[i][j].writeSymbol = (behaviour >> 14) & 0x03;
            ruleBook[i][j].direction = (behaviour & 0x10000) ? LEFT : RIGHT;
            i++;
        }
    }
    file.close();

    //  Read notePad from file
    file.open("D:\\AppleFT\\PacmanTuringTape6502.bin", ios::in | ios::binary);
    i = 0;
    while (file.is_open() && (i < NOTEPADSIZE)) {
        file.read((char*)&notePad[i++], 1);
    }
    file.close();
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
            SetPixel (g_hdc, column * scale + u, line * scale + v, colour);
        }
    }
}

void    WriteMemoryByte()
{
    unsigned int data = 0, marLo = 0, marHi = 0;

    //  Extract the write address and data from the notepad
    for (int i = 0; i < 8; i++) {
        if (notePad[DATA + i] == 0x02)      data |= (1 << (7 - i));
        if (notePad[MARLO + i] == 0x02)     marLo |= (1 << (7 - i));
        if (notePad[MARHI + i] == 0x02)     marHi |= (1 << (7 - i));
    }
    unsigned int address = (marHi << 8) + marLo;
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

int rule = 0;
int notePadPointer = 0;
int ComputeTuring ()
{
    while (true) {
        char symbol = notePad[notePadPointer & NOTEPADMASK];
        int nextRule = ruleBook[rule][symbol].next;
        notePad[notePadPointer & NOTEPADMASK] = ruleBook[rule][symbol].writeSymbol;
        notePadPointer += ruleBook[rule][symbol].direction;
        rule = nextRule;
        if (rule == WRITEMEMORY) {
            WriteMemoryByte();
        }
    }
    return 1;
}


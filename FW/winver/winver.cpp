#include "../main/Alpaca.h"
#include <stdio.h>
#include <Windows.h>


class CMyFocusser : public CFocuser
{public:
    CMyFocusser(int id): CFocuser(id, "CdB Focusser Driver", "1", "CdB Alpaca Focusser", "Focusser for RC600") { }
    bool get_absolute() override { return true; }
    bool get_ismoving() override { return false; }
    int32_t get_maxincrement() override { return 100000; }
    int32_t get_maxstep() override { return 100000; }
    int32_t pos= 0;
    int32_t get_position() override { return pos; }
    int32_t get_stepsize() override { return 6; }
    TAlpacaErr put_halt() override { return ALPACA_OK; };
    TAlpacaErr put_move(int32_t position) override { pos= position; return ALPACA_OK; };
};

class CMyFilterWheel : public CFilterWheel { public: 
    CMyFilterWheel(int id): CFilterWheel(id, "CdB Filter Driver", "1", "CdB Alpaca Filters", "Filters for RC600") { }
    int get_position() { return pos; };
    TAlpacaErr put_position(int32_t position) { requestedPos= position; return ALPACA_OK; }
    int pos= 0;              // if 0, all motors are in off state, or getting there...
    // else, motor "n" is activated, or getting there...
    uint32_t posOpenedAt= 0; // time at which destination will be reached. pos can not be changed until this is 0
    int requestedPos= 0;     // next requested position. will be transfered to pos once posOpenedAt= 0.
    void next(uint32_t nowms)
    {
        if (posOpenedAt!=0)
        {
            if (int(posOpenedAt-nowms)<0) posOpenedAt= 0;
            else return;
        }
        if (pos==requestedPos) return;
        posOpenedAt= nowms+1000; pos= requestedPos;
        if (pos==0) { analogWrite(filterpin1, 0); analogWrite(filterpin2, 0); analogWrite(filterpin3, 0); }
        if (pos==1) { analogWrite(filterpin1, 255); analogWrite(filterpin2, 0); analogWrite(filterpin3, 0); }
        if (pos==2) { analogWrite(filterpin1, 0); analogWrite(filterpin2, 255); analogWrite(filterpin3, 0); }
        if (pos==3) { analogWrite(filterpin1, 0); analogWrite(filterpin2, 0); analogWrite(filterpin3, 255); }
    }
    static int const filterpin1= 0, filterpin2= 1, filterpin3= 2;
    void analogWrite(int pin, int val)
    {

    }
};

int now= 0;
int main()
{
    CAlpaca *alpaca= new CAlpaca("CdB", "1", "FocusServer", "Mars");
    alpaca->addDevice(new CMyFocusser(0)); // Add as many as you want. But the device ID (one counter per type), has to be correct...
    CMyFilterWheel *fw;
    alpaca->addDevice(fw= new CMyFilterWheel(0)); // Add as many as you want. But the device ID (one counter per type), has to be correct...
    alpaca->start();
    while (true) { now+=100; fw->next(now); Sleep(100); }
}

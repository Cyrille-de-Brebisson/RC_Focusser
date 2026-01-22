#include "sdkconfig.h"

#include "Alpaca.h"

#include <esp_app_desc.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_ota_ops.h>

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "rom/ets_sys.h"
#define NOATOMIC
//#include "components\esp_driver_gptimer\src\gptimer_priv.h" // Must add idf_build_set_property(COMPILE_OPTIONS "-I$ENV{IDF_PATH}" APPEND) in cmakefile and remove _Atomic in .h file
#include "soc\gpio_reg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/usb_serial_jtag.h"
#include "esp_adc/adc_continuous.h"
#include "driver/ledc.h"
#include "driver/uart.h"
//#include "soc/uart_periph.h"

static const char *TAG = "main"; // For debug on ESP32...

///////////////////////////////////////////////////////////////////
// Time management. gives millisecond and microsecond time...
static void inline delayMicroseconds(int i) { ets_delay_us(i); }

#include "driver/gptimer.h"
#include "freertos/semphr.h"
#include "register\soc\timer_group_struct.h"

gptimer_handle_t ustimer = NULL;
void millisBegin() 
{ 
    gptimer_config_t timer_config; memset(&timer_config, 0, sizeof(timer_config));
    timer_config.clk_src = GPTIMER_CLK_SRC_DEFAULT, timer_config.direction = GPTIMER_COUNT_UP, timer_config.resolution_hz = 1000000; // 1MHz, 1 tick=1us
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &ustimer));
    ESP_ERROR_CHECK(gptimer_enable(ustimer));
    ESP_ERROR_CHECK(gptimer_start(ustimer));
}
uint32_t IRAM_ATTR micros()
{ 
    TIMERG0.hw_timer[0].update.tx_update = 1; while (TIMERG0.hw_timer[0].update.tx_update) { }
    return TIMERG0.hw_timer[0].lo.tx_lo;
}
uint32_t IRAM_ATTR millis()
{
    TIMERG0.hw_timer[0].update.tx_update = 1; while (TIMERG0.hw_timer[0].update.tx_update) { }
    return ((uint64_t)TIMERG0.hw_timer[0].hi.tx_hi << 22) | ((TIMERG0.hw_timer[0].lo.tx_lo)>>10);
}


//////////////////////////////////////////////////////
// HW Pins, GPIO

static const int8_t KeyOKCancel= 0;
static const int8_t KeyUpDown=   1;
static const int8_t KeyButee=    3;

static const int8_t FocStep   = 5; 
static const int8_t FocSer    = 6; 

static const gpio_num_t LCDSDA    = GPIO_NUM_8;  // Data and LED
static const gpio_num_t LCDSLC    = GPIO_NUM_9;  // clock
// static const int8_t LED    = 8;     // LED
static const int8_t filter1PWM= 10;
static const int8_t filter2PWM= 20;
static const int8_t filter3PWM= 7;


static uint32_t const keyOK= 1<<0;
static uint32_t const keyCancel=    1<<1;
static uint32_t const keyUp= 1<<2;
static uint32_t const keyDown=  1<<3;
static uint32_t const ButeeUp= 1<<4;
static uint32_t const ButeeDown=  1<<5;


class CGPIO { public:
    static void output(uint64_t mask)
    {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = mask;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        gpio_config(&io_conf);
    }
    static void input(uint64_t mask, bool poolup)
    {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.pin_bit_mask = mask;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = poolup?GPIO_PULLUP_ENABLE:GPIO_PULLUP_DISABLE;
        gpio_config(&io_conf);
    }

    static int inline read(int pin) { return gpio_get_level(gpio_num_t(pin)); }
    static void inline set(int pin, int value) { gpio_set_level(gpio_num_t(pin), value); }
    #define GPIO_OUT_W1TS_REG          (0x60004008)
    #define GPIO_OUT_W1TC_REG          (0x6000400C)
    static void inline clear(int pin) { *((uint32_t*)GPIO_OUT_W1TC_REG)=1<<pin; }
    static void inline set(int pin) { *((uint32_t*)GPIO_OUT_W1TS_REG)=1<<pin; }
};


//////////////////////////////////////////////
// LCD
class CDisplay { public:
    static int const W= 128, H= 32;
    uint8_t *fb= FB+1;             // 512 bytes of RAM for the frame buffer. by packs of 8 lines wih 1 byte = 8 vertical pixels!!!! crapy formatting!!!!
    void clear(uint8_t v=0) { memset(FB, v, sizeof(FB)); } 

    i2c_master_dev_handle_t i2c_dev;      /*!< I2C device handle */
    i2c_master_bus_handle_t bus_handle;
    void begin() 
    {
        i2c_master_bus_config_t i2c_bus_config = {
            .i2c_port = -1, // auto select
            .sda_io_num = LCDSDA,
            .scl_io_num = LCDSLC,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
        };

        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

        i2c_device_config_t i2c_dev_conf = {
            .device_address = 0x3c,
            .scl_speed_hz = 200000,
        };

        ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &i2c_dev_conf, &i2c_dev));

        uint8_t const lcdInit[30] = { 0,
            0xAE, 0xD5,0x80 , 0xA8,0x1f, // SSD1306_DISPLAYOFF, SSD1306_SETDISPLAYCLOCKDIV, 0x80 (ratio), SSD1306_SETMULTIPLEX, 0x1F (HEIGHT - 1)
            0xD3,0x0, 0x40, 0x8D,0x14, // SSD1306_SETDISPLAYOFFSET, 0x0, SSD1306_SETSTARTLINE | 0x0, SSD1306_CHARGEPUMP, 0x14,
            0x20,0x00, 0xA0, 0xC0, // SSD1306_MEMORYMODE, 0x00, SSD1306_SEGREMAP | 0x1, SSD1306_COMSCANDEC; // memory mode 0x0 act like ks0108
            // w*h=128*32: comPins = 0x02
            // w*h=128*64: comPins = 0x12;
            // w*h=96*12: comPins = 0x2;
            // else: comPins = 0x02;
            0xDA, 0x02, // SSD1306_SETCOMPINS comPins
            0x81, 0x00, // SSD1306_SETCONTRAST contrast
            0xD9, 0xf1, // SSD1306_SETPRECHARGE 0xF1h
            0xDB,0x40, 0xA4, 0xA6, 0x2E, 0xAF, // SSD1306_SETVCOMDETECT, 0x40, SSD1306_DISPLAYALLON_RESUME, SSD1306_NORMALDISPLAY, SSD1306_DEACTIVATE_SCROLL, SSD1306_DISPLAYON // Main screen turn on
            0x21,0x0,0x7F, // Set columns 0 to 127
        }; send(lcdInit, 30);
    }

    void disp() // send the whole screen to the driver and wait until sent to continue...
    { 
        uint8_t const c[4]= { 0x00, 0x10, 0x00, 0xB0}; send(c, 4); // send write location
        FB[0]= 0x40; send(FB, W*H/8+1);
    }
    void disp2(uint8_t const *v, int size) // v MUST start with a 0x40!!!! and that 40 has to be included in size...
    {
        uint8_t const c[4]= { 0x00, 0x10, 0x00, 0xB0}; send(c, 4); // send write location
        send(v, size);
    }

    void screenOn() { static uint8_t const cmd[2]= { 0, 0xAF }; send(cmd, 2); }
    void screenOff() { static uint8_t const cmd[2]= { 0, 0xAE }; send(cmd, 2); }
    // Framebuffer manipulations
    // The organization in the framebuffer is non-trivial and non-intuitive it's by set of 8 rows, but with 1 byte = 1 colum of 8 pixels...
    // Note that none of these function do any cliping!
    bool pixel(int8_t x, int8_t y) { return (fb[x+(y>>3)*W] & (1<<(y&7)))!=0; }
    void pixon(int8_t x, int8_t y) { fb[x+(y>>3)*W]|= 1<<(y&7); }
    void pixoff(int8_t x, int8_t y) { fb[x+(y>>3)*W]&= ~(1<<(y&7)); }
    void hline(int8_t x, uint8_t w, int8_t y, bool on=true) // Horizontal line
    { 
        uint8_t m= 1<<(y&7);
        uint8_t *d= fb+x+(y>>3)*W;
        if (on) while (w!=0) { w--; *d++|= m; } else { m= ~m; while (w!=0) { w--; *d++&=m; }  }
    }
    void rect(int8_t x, int8_t y, uint8_t w, int8_t h, bool on=true) { while (--h>=0) hline(x, w, y++, on); }
    void vline(int8_t x, int8_t y, int8_t h, bool on=true) // Vertical lune // could be speed on... 
    { if (on) while (h!=0) { h--; pixon(x, y++); } else while (h!=0) { h--; pixoff(x, y++); } }

    uint8_t text(char const *s, int8_t x, int8_t y, uint8_t eraseLastCol= 1, int8_t nb=127) // display in 8*6 pixel font. if eraseLastCol=0 then the last character only displays 5 columns
    { 
        while (*s!=0 && --nb>=0)
        {
            char c= *s++;
            if (eraseLastCol==0 && *s==0) eraseLastCol= 3;
            x= dspChar(c, x, y, eraseLastCol);
        }
        return x;
    }
    uint8_t text2(char const *s, int8_t x, int8_t y, uint8_t eraseLastCol= 1, int8_t nb=127, bool invert= false)  // display in 16*10 pixel font. if eraseLastCol=0 then the last character only displays 10 columns
    { 
        while (*s!=0 && --nb>=0)
        {
            char c= *s++;
            if (eraseLastCol==0 && (nb==0 || *s==0)) eraseLastCol= 3;
            x= dspChar2(c, x, y, eraseLastCol, invert);
        }
        return x;
    }
    // b is width, height and then the bitmap bits, vertically, with the next column starting as soon as the previous is done (no empty space)
    void blit(uint8_t const *b, uint8_t x, uint8_t y)
    {
        int8_t w= *b++; int8_t h= *b++; uint8_t B= 0; int8_t bs= 1;
        uint8_t *P= fb+x+(y>>3)*W; y= 1<<(y&7);
        while (--w>=0)
        {
            uint8_t *p= P; uint8_t v= *p; uint8_t m= y; int8_t r= h;
            while (true)
            {
                if (--bs==0) { bs= 8; B= *b++; }
                if ((B&1)==0) v&= ~m; else v|= m;
                B>>=1; 
                if (--r==0) break;
                m<<=1; if (m==0) { *p= v; p+=W; m= 1; v= *p; }
            }
            *p= v;
            P++;
        }
    }
  private:
      static uint8_t const font8[];
    uint8_t dspChar(char s, uint8_t x, uint8_t y, uint8_t eraseLastCol= 1) // disp 1 8*6 pixel character
    {
        uint8_t const *f= font8+(s-32)*8;
        uint8_t h= 8; if (y+8>H) h= H-y;
        for (int i=0; i<h; i++) 
        { 
            uint8_t m= 1<<((y+i)&7);
            uint8_t *p= fb+x+((y+i)>>3)*W;
            uint8_t v= *(f+i);
            if ((v&1)!=0)  *p++|=m; else *p++&=~m; 
            if ((v&2)!=0)  *p++|=m; else *p++&=~m; 
            if ((v&4)!=0)  *p++|=m; else *p++&=~m; 
            if ((v&8)!=0)  *p++|=m; else *p++&=~m; 
            if ((v&16)!=0) *p++|=m; else *p++&=~m; 
            if (eraseLastCol!=3) { if ((v&32)!=0) *p++|=m; else *p++&=~m; }
        }
        return x+6;
    }
    uint8_t dspChar2(char s, uint8_t x, uint8_t y, uint8_t eraseLastCol= 1, bool invert=false) // displ 1 16*12 pixels character
    {
        uint8_t const *f= font8+(s-32)*8;
        uint8_t h= 16; if (y+16>H) h= H-y;
        for (int8_t i=0; i<h; i++) 
        { 
            uint8_t m= 1<<((y+i)&7);
            uint8_t *p= fb+x+((y+i)>>3)*W;
            uint8_t v= *(f+(i>>1));
            if (invert) v= ~v;
            if ((v&1)!=0) { *p++|=m; *p++|=m; } else { *p++&=~m; *p++&=~m; }
            if ((v&2)!=0) { *p++|=m; *p++|=m; } else { *p++&=~m; *p++&=~m; }
            if ((v&4)!=0) { *p++|=m; *p++|=m; } else { *p++&=~m; *p++&=~m; }
            if ((v&8)!=0) { *p++|=m; *p++|=m; } else { *p++&=~m; *p++&=~m; }
            if ((v&16)!=0) { *p++|=m; *p++|=m; } else { *p++&=~m; *p++&=~m; }
            if (eraseLastCol!=3) { if ((v&32)!=0) { *p++|=m; *p++|=m; } else { *p++&=~m; *p++&=~m; } }
        }
        return x+12;
    }
    uint8_t FB[W*H/8+10]; // ok, so this is here, and fb is actually 5 byte after to allow for I2C data to be placed JUST before the framebuffer!
                          // When sending I2C data, the system will use the bytes just before the start of each pack of 8 rows to add some data that needs to go to the LCD driver
    public:
    void inline send(uint8_t const *i, int16_t nb) // Send an I2C packet...
    {
        i2c_master_transmit(i2c_dev, i, nb, -1); // no timeout!
    }
} display;

        uint8_t const CDisplay::font8[] ={ // 6*8 pixel font with only chr 32 to 127
            /* */ 0,0,0,0,0,0,0,0, /*!*/ 4,4,4,4,4,0,4,0, /*"*/ 10,10,10,0,0,0,0,0, /*#*/ 10,10,31,10,31,10,10,0, /*$*/ 4,30,5,14,20,15,4,0, /*%*/ 3,19,8,4,2,25,24,0, /*&*/ 2,5,5,2,21,9,22,0, /*'*/ 4,4,4,0,0,0,0,0, /*(*/ 8,4,2,2,2,4,8,0, /*)*/ 2,4,8,8,8,4,2,0, /***/ 0,10,4,31,4,10,0,0, /*+*/ 0,4,4,31,4,4,0,0, /*,*/ 0,0,0,0,6,6,4,2, /*-*/ 0,0,0,31,0,0,0,0, /*.*/ 0,0,0,0,0,6,6,0, /*/*/ 0,16,8,4,2,1,0,0,
            /*0*/ 14,17,25,21,19,17,14,0, /*1*/ 4,6,4,4,4,4,14,0, /*2*/ 14,17,16,12,2,1,31,0, /*3*/ 14,17,16,14,16,17,14,0, /*4*/ 8,12,10,9,31,8,8,0, /*5*/ 31,1,15,16,16,17,14,0, /*6*/ 12,2,1,15,17,17,14,0, /*7*/ 31,16,8,4,2,2,2,0, /*8*/ 14,17,17,14,17,17,14,0, /*9*/ 14,17,17,30,16,8,6,0, /*:*/ 0,6,6,0,6,6,0,0, /*;*/ 0,6,6,0,6,6,4,2, /*<*/ 8,4,2,1,2,4,8,0, /*=*/ 0,0,31,0,31,0,0,0, /*>*/ 1,2,4,8,4,2,1,0, /*?*/ 14,17,16,8,4,0,4,0,
            /*@*/ 14,17,21,29,5,1,30,0, /*A*/ 14,17,17,31,17,17,17,0, /*B*/ 15,17,17,15,17,17,15,0, /*C*/ 14,17,1,1,1,17,14,0, /*D*/ 7,9,17,17,17,9,7,0, /*E*/ 31,1,1,15,1,1,31,0, /*F*/ 31,1,1,15,1,1,1,0, /*G*/ 14,17,1,1,25,17,30,0, /*H*/ 17,17,17,31,17,17,17,0, /*I*/ 14,4,4,4,4,4,14,0, /*J*/ 16,16,16,16,17,17,14,0, /*K*/ 17,9,5,3,5,9,17,0, /*L*/ 1,1,1,1,1,1,31,0, /*M*/ 17,27,21,21,17,17,17,0, /*N*/ 17,17,19,21,25,17,17,0, /*O*/ 14,17,17,17,17,17,14,0,
            /*P*/ 15,17,17,15,1,1,1,0, /*Q*/ 14,17,17,17,21,9,22,0, /*R*/ 15,17,17,15,5,9,17,0, /*S*/ 14,17,1,14,16,17,14,0, /*T*/ 31,4,4,4,4,4,4,0, /*U*/ 17,17,17,17,17,17,14,0, /*V*/ 17,17,17,10,10,4,4,0, /*W*/ 17,17,17,21,21,27,17,0, /*X*/ 17,17,10,4,10,17,17,0, /*Y*/ 17,17,10,4,4,4,4,0, /*Z*/ 31,16,8,4,2,1,31,0, /*[*/ 14,2,2,2,2,2,14,0, /*\*/ 0,1,2,4,8,16,0,0, /*]*/ 14,8,8,8,8,8,14,0, /*^*/ 4,10,17,0,0,0,0,0, /*_*/ 0,0,0,0,0,0,31,0,
            /*`*/ 2,2,4,0,0,0,0,0, /*a*/ 0,0,14,16,30,17,30,0, /*b*/ 1,1,15,17,17,17,15,0, /*c*/ 0,0,30,1,1,1,30,0, /*d*/ 16,16,30,17,17,17,30,0, /*e*/ 0,0,14,17,31,1,14,0, /*f*/ 4,10,2,7,2,2,2,0, /*g*/ 0,0,14,17,17,30,16,14, /*h*/ 1,1,15,17,17,17,17,0, /*i*/ 4,0,6,4,4,4,14,0, /*j*/ 8,0,12,8,8,8,9,6, /*k*/ 1,1,9,5,3,5,9,0, /*l*/ 6,4,4,4,4,4,14,0, /*m*/ 0,0,11,21,21,21,17,0, /*n*/ 0,0,15,17,17,17,17,0, /*o*/ 0,0,14,17,17,17,14,0,
            /*p*/ 0,0,15,17,17,15,1,1, /*q*/ 0,0,30,17,17,30,16,16, /*r*/ 0,0,29,3,1,1,1,0, /*s*/ 0,0,30,1,14,16,15,0, /*t*/ 2,2,7,2,2,10,4,0, /*u*/ 0,0,17,17,17,17,30,0, /*v*/ 0,0,17,17,17,10,4,0, /*w*/ 0,0,17,17,21,21,10,0, /*x*/ 0,0,17,10,4,10,17,0, /*y*/ 0,0,17,17,17,30,16,14, /*z*/ 0,0,31,8,4,2,31,0, /*{*/ 12,2,2,1,2,2,12,0, /*|*/ 4,4,4,4,4,4,4,0, /*}*/ 6,8,8,16,8,8,6,0, /*~*/ 0,0,2,21,8,0,0,0, /*⌂*/ 7,5,7,0,0,0,0,0,
        };




//////////////////////////////////////////////////////////////
// Driver low level (rs232)
class Ctmc2209 { public:
    static uint16_t const tmc2209Delay= 8; // 8micro s = 125Kb/s = 12.5KB/s = 1.6millis for a read/modify/write of a register... Trying 500kb/s did not work...
    static void inline udelay(int i) { delayMicroseconds(i); }
    uint8_t serialPin;
    uint8_t const validUARTAddrs= 7; // Bitmasks of all valid addresses...
    static uint8_t const GCONFAdr= 0;
    static uint8_t const CHOPCONFAdr= 0x6C;
    static uint8_t const IHOLD_IRUNAdr= 0x10;
    uint8_t GCONF[8], CHOPCONF[8];
    static bool const soft= false;
    static uart_port_t const DEFAULT_UART_CHANNEL= UART_NUM_0;
    Ctmc2209(uint8_t serialPin, uint32_t addrs= 1): serialPin(serialPin), validUARTAddrs(addrs)
    {
        // Read register to make sure motor is OK... But don't care what it is or does...
        // uint8_t v[4]; while (readRegister(serialPin, 0, v)!=0) { delay(10); }
        static uint8_t const IGCONF[8]=    { 5, 0, GCONFAdr|0x80, 0, 0, 1, 0b11000001 };       memcpy(GCONF, IGCONF, 8);       // GCONF.pdn_disable=1, GCONF.mstep_reg_select = 1 (use MRES for step count)
        static uint8_t const ICHOPCONF[8]= { 5, 0, CHOPCONFAdr|0x80, 0x33, 0x00, 0x00, 0x53 }; memcpy(CHOPCONF, ICHOPCONF, 8); // TOFF=3, microsteps=8(0), INTPOL=1, HSTRT= 5, DBL=0, double edge off (because we can do up/down in 1 function)
        // sendPacket(serialPin, GCONF, 8); sendPacket(serialPin, CHOPCONF, 8);
        if (soft) { 
            CGPIO::output((1<<serialPin)); CGPIO::set(serialPin,1); 
        } else {
            uart_config_t uart_config = {
                .baud_rate = 115200,
                .data_bits = UART_DATA_8_BITS,
                .parity    = UART_PARITY_DISABLE,
                .stop_bits = UART_STOP_BITS_1,
                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                .source_clk = UART_SCLK_DEFAULT,
            };
            uart_driver_install(DEFAULT_UART_CHANNEL, 256, 0, 0, NULL, 0);
            uart_param_config(DEFAULT_UART_CHANNEL, &uart_config);
            uart_set_pin(DEFAULT_UART_CHANNEL, serialPin, -1, -1, -1);
        }
    }
    void sendRegs()
    {
        sendPacket(GCONF, 8);
        sendPacket(CHOPCONF, 8);
    }
    static uint8_t swuart_calcCRC(uint8_t const * s, uint8_t l)
    {
        uint8_t crc= 0; // CRC located in last byte of message
        do {
            uint8_t currentByte = *s++; // Retrieve a byte to be sent from Array
            for (uint8_t j=0; j<8; j++) 
            {
                if ((crc>>7) ^ (currentByte&0x01)) crc= (crc<<1) ^ 0x07;
                else crc= crc<<1;
                currentByte>>= 1;
            }
        } while (--l!=0);
        return crc;
    }
    // send packet. Will calculate the checksum for you. size/s has to be the full packet lenght (with crc)
    // Assumes that line is high before and output mode
    void sendPacket(uint8_t *s, uint8_t size) // Send packet to all controlers with the specified address...
    {
        for (uint32_t i=0; i<4; i++)
            if (((1<<i)&validUARTAddrs)!=0) sendPacket(serialPin, s, size, i);
    }
    static void sendPacket(uint8_t pin, uint8_t *s, uint8_t size, int addr) // send 1 packet to 1 address...
    {
        s[1]= addr;
        s[size-1]= swuart_calcCRC(s, size-1);
        if (soft)
        {
            do { // send size bytes
                uint8_t v= *s++;
                portDISABLE_INTERRUPTS(); //            noInterrupts(); // Time sensitive
                CGPIO::set(pin, 0); udelay(tmc2209Delay); // start bit
                for (uint8_t i= 0; i<8; i++) { CGPIO::set(pin, v&1); udelay(tmc2209Delay); v>>=1; }
                CGPIO::set(pin, 1); udelay(tmc2209Delay); // stop bit
                portENABLE_INTERRUPTS(); // interrupts();
            } while (--size!=0);
        } else {
        // len = uart_read_bytes(DEFAULT_UART_CHANNEL, data, READ_BUF_SIZE, 100 / portTICK_PERIOD_MS);
            uart_write_bytes(UART_NUM_0, s, size);
        }
    }
    // l is the size WITH the crc and s has to be l bytes long...
    // return >=0 on packet ok. Else -1: no start bit, -2: no end bit, -3: crc error
    // will take 160 microseconds for 8 byte packet
    static int readPacket(uint8_t pin, uint8_t *s, uint8_t l)
    {
        int8_t er;
        portDISABLE_INTERRUPTS(); // noInterrupts(); // Time sensitive.. will be 8*10=80 microseconds!
        CGPIO::input(1<<pin, true);
        for (int8_t j= 0; j<l; j++) // For all packets
        {
            uint16_t t= 1000; while (CGPIO::read(pin)!=0) if (--t==0) { er= -1; goto err; } // wait for start bit
            udelay(tmc2209Delay+tmc2209Delay/2); // wait for 1.5 delay
            uint8_t b= 0; for (int8_t i=0; i<8; i++) { b=b>>1; if (CGPIO::read(pin)!=0) b|= 0x80; udelay(tmc2209Delay); } s[j]= b; // Get 8 bits
            t= 1000; while (CGPIO::read(pin)==0) if (--t==0) { er= -2; goto err; } // Wait for stop bit
        }
        // Back to normal operations
        CGPIO::output(1<<pin); CGPIO::set(pin, 1); // digitalWrite(pin, 1); pinMode(pin, OUTPUT);
        portENABLE_INTERRUPTS(); // interrupts();
        if (swuart_calcCRC(s, l-1)==s[l-1]) return 0; // verify crc and return if ok
        er= -3;
    err: CGPIO::output(1<<pin); CGPIO::set(pin, 1); portENABLE_INTERRUPTS(); return er; // return to normal and return error code
    }
    int readRegister(uint8_t r, uint8_t *v, int addr)
    {
        uint8_t p[8]= { 5, uint8_t(addr), r }; sendPacket(serialPin, p, 4, addr);
        int er= readPacket(serialPin, p, 8); if (er!=0) return er;
        memcpy(v, p+3, 4); return 0;
    }
    //void rms_current(uint16_t mA, uint16_t Rsense=110) // Rsense in milli ohm
    //{
    //    uint8_t CS= ((73*uint32_t(mA)*(Rsense+20))>>19) - 1; // Rsense= 110. Tops at 24 bits... // uint8_t CS= 32*1.41421*mA/1000.0*(Rsense+0.02)/0.325 - 1;
    //    if (CS<16) CS= ((33*uint32_t(mA)*(Rsense+20))>>17) - 1; // if CS<16, high res by turning vsense on. CS= 32*1.41421*mA/1000.0*(Rsense+0.02)/0.180 - 1;
    //    if (CS>31) CS = 31;
    //    uint8_t IHOLD_IRUN[8]= { 5, UARTAdr, IHOLD_IRUNAdr|0x80, 0, 1, CS>>1, CS }; sendPacket(serialPin, IHOLD_IRUN, 8);
    //    uint8_t v_mask[8]= { 0,0, (CS<16?2:0),2, 0,0, 0,0}; updateRegister(CHOPCONFAdr, v_mask);  // turn CHOPCONF.vsense on/off
    //}

    void TOFF(uint8_t val) // update CHOPCONF.TOFF 0:stop stepper, 3:start stepper
    { CHOPCONF[6]= (CHOPCONF[6]&~15)|val; sendPacket(CHOPCONF, 8); }
    void enable(int m)
    {
        CHOPCONF[6]= (CHOPCONF[6]&~15)|((m&1)!=0?3:0); sendPacket(serialPin, CHOPCONF, 8, 0);
        CHOPCONF[6]= (CHOPCONF[6]&~15)|((m&2)!=0?3:0); sendPacket(serialPin, CHOPCONF, 8, 1);
        CHOPCONF[6]= (CHOPCONF[6]&~15)|((m&4)!=0?3:0); sendPacket(serialPin, CHOPCONF, 8, 2);
    }
    // 0=256microsteps, 1:128, ..., 7:2, 8:1 (no microsteps)
    void microsteps(uint8_t val) // update CHOPCONF.MRES
    { CHOPCONF[3]= (CHOPCONF[3]&~15)|val; sendPacket(CHOPCONF, 8); }
    void shaft(uint8_t dir) // update GCONF.shaft 0:direction 1, 1:direction 0
    { GCONF[6]= (GCONF[6]&~8)|(dir<<3); sendPacket(GCONF, 8); }
    // 0=256microsteps, 1:128, ..., 7:2, 8:1 (no microsteps)
    void setPower(uint32_t val) // change register x10 (power control)
    {
        uint8_t power[8]=    { 5, 0, 0x10|0x80, 0, uint8_t(val>>16), uint8_t(val>>8), uint8_t(val), 0 }; sendPacket(power, 8); 
    }
};

///////////////////////////////////////
// Handles min, max, accelerations, decelerations...
class CMotor : public Ctmc2209 { public:
        uint8_t const stp;                     // pins
        uint8_t stpVal= 0, dir= 0;                     // current pin value and direction
        int32_t pos=0, dst=0; int32_t requestedSpd=0;  // Current pos, destination and desired speed (req speed it absolute value).
        uint32_t maxPos=0;                      // maximum positions in steps (min is 0!)...
        uint32_t spdMax=0, accMax=0;            // max speeds and accelerations in steps/s, steps**2/ms (carefull, this 2nd one is in units per mili seconds, not seconds!)

    CMotor(uint8_t stp, uint8_t serial, uint32_t addrs): Ctmc2209(serial, addrs), stp(stp)
    {
      // Handled at global level at the moment for memory saving in avr...
      CGPIO::output((1<<stp)); CGPIO::set(stp, 0);
    }

    void init(uint32_t maxPos, uint32_t maxStpPs, uint32_t msToFullSpeed)
    {
        maxPos*=32; maxStpPs*=32;
        currentSpd= requestedSpd=0; // kill
        this->maxPos= maxPos;
        spdMax= maxStpPs, accMax= int32_t(maxStpPs)/int32_t(msToFullSpeed);
        dst= pos= maxPos/2;
    }

    // movement controls in steps...
    bool inline isMoving() { return requestedSpd!=0; }
    void setToSteps(uint32_t position) { if (position>maxPos) dst= pos= maxPos; else dst= pos= position; } // set current position in steps unit
    void inline goToSteps(uint32_t destination) { goToSteps(destination, spdMax); } 
    void goToSteps(uint32_t destination, int32_t spdStepsPS)
    { 
        microsteps(3); // force.
        if (destination>maxPos) destination= maxPos; dst= destination;
        if (spdStepsPS>spdMax) spdStepsPS= spdMax;
        requestedSpd= spdStepsPS; 
        if (pos>=destination) { shaft(dir=0); } else { shaft(dir=1); }
        setPower(0x31f03);
    }
    void goUp(int32_t spd) { if (spd<0) return goDown(-spd); goToSteps(maxPos, spd); return; }
    void goDown(int32_t spd) { if (spd<0) return goUp(-spd); goToSteps(0, spd); return; }
    void inline stop() { requestedSpd=0; } // controled stop to here... migt be better to calculate end pos to avoid double back...
    void inline kill() { dst= pos; currentSpd= requestedSpd=0; } // hard stop!

    ///////////////////////////////////////////
    // All this is about actually moving!!!
    int32_t currentSpd=0;         // current speed in steps per second. sign indicates direction... can not be 0 if not done moving...
    uint32_t nextMs=0;            // next time we need update speed and recalculate intervals (when now arrives there...(ms timer))
    uint32_t nextStepmus=0;       // next time we need to step
    uint32_t deltaBetweenSteps=0; // delta between 2 steps during this ms slot...
    #define Abs(a) ((a)>=0?(a):(-(a)))
    // issue steps if/as needed...
    void IRAM_ATTR next(uint32_t now)
    {
        if (currentSpd==0) 
        {
            if (requestedSpd==0) return;
            nextStepmus= now; nextMs= now; // we were stopped, but now we are moving. So resync all movements...
        }

        if (int32_t(now-nextMs)>=0)
        {
            nextMs+= 1000;
            // Quantisation on a milisecond.
            // Every ms, update speed (+ or - acc) up to desirned speed...
            // then update mus delta between steps
            int32_t oldSpd= currentSpd;
            if (int32_t(Abs(currentSpd))>requestedSpd) // Slow down if we are going too fast!
            { 
                // Can we slow down to requestedSpd directly? or would that be to jerky?
                if (Abs(requestedSpd-currentSpd)<accMax) 
                { 
                    if (currentSpd<0) currentSpd= -requestedSpd; else currentSpd= requestedSpd; 
                    if (requestedSpd==0) dst= pos; // if speed 0 requested, then we are at destination..
                }
                else { if (currentSpd>0) currentSpd-= accMax; else currentSpd+= accMax; }
            } 
            int8_t changeSign;
            int32_t stepsLeft= dst-pos;
            if (stepsLeft==0) goto dstReached;   // We have arrived
            // calculate time, then distance to speed=0: sum(i=0 to n, spd-i*maxAcc)
            uint32_t msToSpd0= Abs(currentSpd)/accMax; // number of ms until we are slow enough to hard stop...
            //1/2*accMax*t²...
            // accMax is in the order of 100 at this point in time...
            // assuming it takes less than 1s to full speed... t² is smaller than 1million... and t²*accMax smaller than 1billion by a factor 10... so u32 are ok..
            // But speed is in steps per second and t in ms... so there is a division by 1e3 that will need to happen at one point as accMAx is in /ms unit...
            uint32_t distSlowDown= (accMax*msToSpd0*msToSpd0)>>11; // /2000;
            changeSign= (stepsLeft>0)?1:-1;                        // assumes the need to accelerate in the right dirrection
            if (distSlowDown>=Abs(stepsLeft)) 
            {
                //printf("slow down dst toslow:%d dst:%d\r\n", distSlowDown, Abs(stepsLeft));
                changeSign*= -4; // unless we need to slow down!
            }
            currentSpd+= accMax*changeSign;
            if (currentSpd>requestedSpd) currentSpd= requestedSpd;
            if (currentSpd<-requestedSpd) currentSpd= -requestedSpd;
            if (currentSpd!=oldSpd)
            {
                if (currentSpd==0) currentSpd= accMax>>1;
                uint32_t newDeltaBetweenSteps= 1000000 / (Abs(currentSpd));
                if (newDeltaBetweenSteps<deltaBetweenSteps) nextStepmus-= deltaBetweenSteps-newDeltaBetweenSteps;
                deltaBetweenSteps= newDeltaBetweenSteps;
                //static uint32_t lastPos= 0; printf("deltaBetweenSteps:%d speed:%d(%d) pos:%d delta:%d dst:%d\r\n", deltaBetweenSteps, currentSpd, requestedSpd, pos, pos-lastPos, dst); lastPos= pos;
            }
        }

        if (int32_t(now-nextStepmus)<0) return;    // nothing to do. we wait
        if (pos==dst) { dstReached: pos= dst; currentSpd= requestedSpd=0; return; } // destination reached!
        if (stpVal==0) { CGPIO::set(stp); stpVal= 1; } else { CGPIO::clear(stp); stpVal= 0; }
        nextStepmus+= deltaBetweenSteps;           // program allarm
        if (currentSpd>=0) pos+= 1; else pos-= 1;      // increase pos
    }
};
CMotor MFocus(FocStep, FocSer, 7);



///////////////////////////////////////////
// Keyboard and end of course sensors are on ADC channels...
class CADC { public:
    static int const nbChannels= 3;
    static int const nbSamples= 16;
    static int const sampleSize= nbChannels*nbSamples*4;
    uint8_t adcChannelToPinId[8]= {0};
    adc_continuous_handle_t handle = NULL;
    void begin() 
    {
        uint8_t const adcPins[nbChannels]={KeyOKCancel, KeyUpDown, KeyButee};
        adc_continuous_handle_cfg_t adc_config = { .max_store_buf_size = sampleSize*4, .conv_frame_size = sampleSize, .flags= 0 };
        ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));
        adc_channel_t channel[nbChannels];
        adc_digi_pattern_config_t adc_pattern[nbChannels];
        for (int i=nbChannels; --i>=0;) 
        {
            adc_unit_t unused; adc_continuous_io_to_channel(adcPins[i], &unused, &channel[i]); adcChannelToPinId[channel[i]]= i; 
            //printf("pin to channel (%d) %d %d\n", i, adcPins[i], channel[i]);
            adc_pattern[i].atten = ADC_ATTEN_DB_12;
            adc_pattern[i].channel = channel[i] & 0x7;
            adc_pattern[i].unit = ADC_UNIT_1;
            adc_pattern[i].bit_width = 12;
        }
        adc_continuous_config_t dig_cfg = { .pattern_num= nbChannels, .adc_pattern = adc_pattern, .sample_freq_hz = 20 * 1000, .conv_mode = ADC_CONV_SINGLE_UNIT_1, .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2 };
        ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));
        ESP_ERROR_CHECK(adc_continuous_start(handle));
    }
    void read(int *res)
    {
        uint32_t ret_num = 0;
        uint8_t P[sampleSize], *p= P;
        esp_err_t ret = adc_continuous_read(handle, p, sampleSize, &ret_num, 0); 
        if (ret!=ESP_OK || ret_num!=sampleSize) 
        { 
          //printf("error adc read reset\n"); 
          ESP_ERROR_CHECK(adc_continuous_stop(handle));
          ESP_ERROR_CHECK(adc_continuous_start(handle));
          return; 
        }
        //char t[400]; for (int i=0; i<sampleSize; i++) sprintf(t+2*i, "%02x", P[i]); printf("%s\n", t);
        int res2[8]; memset(res2, 0, sizeof(res2)); // populated by call back function...
        int res2cnt[8]; memset(res2cnt, 0, sizeof(res2cnt)); // populated by call back function...
        for (int i = nbChannels*nbSamples; --i>=0;) 
        {
            uint8_t b1= *p++; uint8_t b2= *p++; p+= 2;
            int chan= adcChannelToPinId[(b2>>5)&7];
            res2[chan]+= b1|(int(b2&0xf)<<8);
            res2cnt[chan]++;
        }
        for (int i=nbChannels; --i>=0;) res[i]= res2[i]/res2cnt[i];
    }
} adc;
class CKeyboard { public:
  uint32_t keymap= 0;
  int reads[3]= {0,0,0};
  void begin() { adc.begin(); }
  uint32_t read()
  {
    adc.read(reads);
    uint32_t ret= 0;
    if (reads[0]<1024) ret|= keyCancel;
    else if (reads[0]<3024) ret|= keyOK;
    if (reads[1]<1024) ret|= keyDown;
    else if (reads[1]<3024) ret|= keyUp;
    if (reads[2]<1024) ret|= ButeeUp;
    else if (reads[2]<3024) ret|= ButeeDown;
    return keymap= ret;
  }
} kbd;

//********************************************
// UI part of thing

// state variables...
enum { UIFocus, UISetup };
static int UI= 0; // UI will be one of the above and indicate the UI screen...
struct TMenuPos { int top=0, selec=0; };
static TMenuPos menu, menu2, filtreSelect; // position in menus
static uint32_t nextDisplay= 0;                // next time the display needs updating
static const int nbSpeeds= 4;
static int const manualSpeeds[]= { 314, 157, 15, 2 }; // list of possible speed in manual mode in deg per second.
static char const * const manualSpeedsTxt[]={"2mm/s", "1mm/s", ".1mm/s", "10um/s"};
static int manualSpeed= 0;                 // current speed in manualSpeeds
static uint32_t lastkbd= 0;                     // status of last keyboard scann for debounce
static const uint32_t UIDelay= 50;              // Pool time of keyboard. UI unit of time. 20 times per second...
static const uint32_t timeToScreenOffConst= 10*20;   // 10s timout on the screen. The 20 is the UI delay unit of time...
static uint32_t timeToScreenOff= timeToScreenOffConst;      // for screen off 
static int slots[4]= {-1, -1, -1, -1 }; // position save/load

static uint8_t toStr(char *s, uint32_t v) // convert number to dec representation... Returns number of characters...
{
    uint8_t nb= 0; while (true) { s[nb++]= '0'+(v%10); v/= 10; if (v==0) break; } s[nb]=0; // Write number, but backward...
    for (int8_t i=nb/2; --i>=0;) { char r= s[i]; s[i]= s[nb-1-i]; s[nb-1-i]= r; } // flip it!
    return nb;
}

static int dispMenu(char const *title, char const * const *m, int size, struct TMenuPos &pos, int keys)
{
  int x= display.text(title, 0, 0);
  int y= 0; m+= pos.top;
  for (int i=0; i<2; i++)
  {
    if (pos.selec!=i) display.text2(m[i], x+1, y);
    else { display.rect(x, y, display.W-x, 16); display.text2(m[i], x+1, y, 1, 126-x, true); }
    y+= 16;
  }
  if ((keys&keyUp)!=0)  { if (pos.selec==0) { if (pos.top!=0) pos.top--; } else pos.selec--; }
  if ((keys&keyDown)!=0) { if (pos.selec==1) { if (pos.top+2<size) pos.top++; } else pos.selec++; }
  if ((keys&keyOK)!=0) return pos.selec+pos.top+1;
  if ((keys&keyCancel)!=0) return 0;
  return -1;
}

// Main UI function!
static char ipadr[30]="unknow";
static int filterWheelPos();
static void filterWheelNext(uint32_t now);
static void filterWheelPos(int pos);
static bool doNotStop= false;
static void posToChar(char *t, int pos)
{
    pos= pos-MFocus.maxPos/2;
    pos/=32;
    pos=pos*635/100; // in microns
    if (pos>=0) t[0]='+'; else t[0]= '-', pos=-pos;
    div_t r= div(pos, 1000);
    sprintf(t+1, "%02d.%03d", r.quot, r.rem);
    if (t[1]=='0') t[1]=' ';
}

static bool volatile wificonnected= false;
static uint8_t wifiimg[]= { 7, 8, 0b10101010, 0b00101010, 0b11101010, 0b00010010, 0b11100100, 0b00001000, 0b11110000 };
static void doUI(uint32_t now, uint32_t keys)
{
    if (int32_t(now-nextDisplay)<0) return; // only look at screen/UI every 50ms or so
    nextDisplay= now+UIDelay; // + 50ms = 20* per seconds

    uint32_t newKeyDown= keys&~lastkbd; lastkbd= keys;

    // screen timeout handeling...
    if (timeToScreenOff==0) // screen is off... do we need to turn it on?
    {
        if (keys==0) return; // no keys, just return!
        display.screenOn();   // in all cases, there is no display right now
        newKeyDown= 0;        // no act on this key.
    }
    if (keys!=0) timeToScreenOff= timeToScreenOffConst; // reset timer on key down...
    if (--timeToScreenOff==0)  { display.screenOff(); return; }

  char t[20];
  display.clear();
  if (UI==0)
  { 
    int x= display.text("Cancel", 0, 0); display.text("Menu", 12, 8);
    display.text("OK spd", 0, 16); 
    strcpy(t, "F_"); if (filterWheelPos()!=0) t[1]='0'+filterWheelPos();
    display.text(t, 12, 24);
    if (wificonnected) display.blit(wifiimg, 0, 24);
    display.vline(x+1, 0, display.H-1);
    x+= 3;
    posToChar(t, MFocus.pos);
    display.text2(t, x, 0);
    display.text2(manualSpeedsTxt[manualSpeed], x, 16);
    if ((newKeyDown&keyDown)!=0)  MFocus.goDown(manualSpeeds[manualSpeed]*32);
    if ((newKeyDown&keyUp)!=0) MFocus.goUp(manualSpeeds[manualSpeed]*32);
    if ((newKeyDown&keyOK)!=0) { manualSpeed++; if (manualSpeed>=nbSpeeds) manualSpeed= 0; }
    if ((newKeyDown&keyCancel)!=0) UI= 1;
    if (newKeyDown!=0) doNotStop= false;
    if (keys==0 && !doNotStop) MFocus.stop(); 
  }
  else if (UI==1)
  {
    static char const * const txt[]= { "set0", "save", "goto", "Filtres", "IP" };
    int r= dispMenu("menu", txt, 5, menu, newKeyDown);
    if (r==1) { MFocus.pos= MFocus.maxPos/2; UI= 0; }
    else if (r==0) UI= 0;
    else if (r==2) UI= 2;
    else if (r==3) UI= 3;
    else if (r==4) UI= 4;
    else if (r==5) UI= 5;
  }
  else if (UI==2)
  {
    static char const * const txt[]= { "pos1", "pos2", "pos3", "pos4" };
    int r= dispMenu("save", txt, 4, menu2, newKeyDown);
    if (r==0) UI= 0;
    else if (r>0) { slots[r-1]= MFocus.pos; UI= 0;}
  }
  else if (UI==3)
  {
    char t2[8*4];
    for (int i=0; i<4; i++) 
    {
      if (slots[i]==-1) slots[i]= MFocus.maxPos/2; 
      posToChar(t2+i*8, slots[i]);
    }
    char * txt[]= { t2+0, t2+8, t2+16, t2+24 };
    int r= dispMenu("go", txt, 4, menu2, newKeyDown);
    if (r==0) UI= 0;
    else if (r>0) { MFocus.goToSteps(slots[r-1]); UI= 0; doNotStop= true; }
  }
  else if (UI==4)
  {
    // copy names in temp from RAF definition. Add marker for current....
    char const ftext[]="*Vide\0\0\0*G1\0\0\0\0\0*G2\0\0\0\0\0*G3\0\0\0\0\0";
    char const * txt[]= { ftext+1, ftext+1+8, ftext+1+16, ftext+1+24 };
    txt[filterWheelPos()]--;
    int r= dispMenu("Filtres", txt, 4, filtreSelect, newKeyDown);
    if (r==0) UI= 0;
    else if (r>0) { filterWheelPos(r-1); } // change filter..
  } else if (UI==5)
  {
    if (ipadr[0]>='0' && ipadr[0]<='9')
    {
        char *d= strchr(ipadr, '.');
        if (d!=nullptr)
        {
            d= strchr(d+1, '.');
            if (d!=nullptr) *d= 0;
        }
        display.text2(ipadr, 0, 0); d= ipadr+strlen(ipadr)+1;
        display.text2(d, 40, 16);
    } else {
        display.text2(ipadr, 0, 0); 
    }
    if ((newKeyDown&keyCancel)!=0) UI= 1;
  }
  display.disp();
}


// From here, we find mostly what is needed to talk to the ASCOM driver through the serial port...
static int32_t readDec(char *&s, int32_t def= 0) // read a dec int from string with default
{
    if (*s<'0' || *s>'9') return def;
    int32_t r= 0; while (*s>='0' && *s<='9') r= r*10+(*s++-'0');
    return r;
}
static uint32_t readHex(char *&s, int8_t cnt=8) // read an hex value from a string...
{
    uint32_t v= 0;
    while (--cnt>=0)
    {
        if (*s>='0' && *s<='9') v= v*16 + *s++-'0';
        else if (*s>='a' && *s<='f') v= v*16 + *s++-'a'+10;
        else if (*s>='A' && *s<='F') v= v*16 + *s++-'A'+10;
        else return v;
    }
    return v;
}

static bool IRAM_ATTR stepperTick(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    uint32_t now= micros(); 
    MFocus.next(now);
    return pdFALSE;
}
static gptimer_handle_t gptimer;
static int32_t ButeeDownPos= -1;
static uint32_t moveTimeout= 0;
void stopStartStepperTimer(uint32_t keys, uint32_t nowms)
{
    static bool timerStarted= false;
    // Stop on end of movement detection...
    if ((keys&ButeeDown)!=0 && ButeeDownPos==-1) ButeeDownPos= MFocus.pos/32;
    if (MFocus.dst<MFocus.pos && (keys&ButeeDown)!=0) MFocus.kill();
    if (MFocus.dst>MFocus.pos && (keys&ButeeUp)!=0) MFocus.kill();
    if (moveTimeout!=0 && int32_t(nowms-moveTimeout)>0) MFocus.kill(); // kill any moves after 10s. Else it looks like there is an issue some times with motor not stopping..
    // start/stop timer (helps wifi) depending on motor need to move
    if (!timerStarted && MFocus.dst!=MFocus.pos) { ESP_ERROR_CHECK(gptimer_start(gptimer)); timerStarted= true; moveTimeout= nowms+1000*10; } // start the timer
    if (timerStarted && MFocus.dst==MFocus.pos) { ESP_ERROR_CHECK(gptimer_stop(gptimer)); timerStarted= false; moveTimeout= 0;}
}
void initSteppers()
{
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
    gptimer_event_callbacks_t cbs = { .on_alarm = stepperTick, };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, nullptr));
    gptimer_alarm_config_t alarm_config1; memset(&alarm_config1, 0, sizeof(alarm_config1));
    alarm_config1.reload_count = 0;
    alarm_config1.alarm_count = 50; // period = 50us or 20k steps/s!
    alarm_config1.flags.auto_reload_on_alarm = true;
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config1));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
}


uint8_t readhex(char s)
{
    if (s>='0' && s<='9') return s-'0';
    if (s>='A' && s<='F') return s-'A'+10;
    if (s>='a' && s<='f') return s-'a'+10;
    return 0;
}

char wifi[32]="", wifipass[32]="";
CAlpaca *alpaca= nullptr;

uint8_t const camimg[]=
{128,32,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,248,3,0,0,126,9,0,128,223,33,0,192,239,0,0,224,167,6,0,240,35,17,0,120,115,31,0,184,221,11,0,252,14,23,0,124,15,31,0,220,116,13,0,94,246,13,0,206,250,3,0,190,187,2,0,110,226,3,0,94,242,1,0,254,250,3,0,46,253,0,0,164,237,0,0,176,115,0,0,176,147,0,0,248,48,0,0,24,16,0,0,64,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,252,63,0,0,254,63,0,0,6,96,0,0,6,112,0,0,14,56,0,0,28,56,0,0,0,0,0,0,0,0,0,0,128,127,0,0,248,127,0,0,126,7,0,0,14,3,0,0,254,3,0,0,248,127,0,0,0,127,0,0,0,0,0,0,0,0,0,0,254,63,0,0,254,127,0,0,126,60,0,0,56,0,0,0,224,3,0,0,192,7,0,0,240,1,0,0,126,0,0,0,254,127,0,0,254,127,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};

uint8_t const cam2[]=
{0x40,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,224,112,248,248,124,156,52,130,62,70,146,46,2,210,88,28,76,8,96,176,224,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,68,149,39,27,95,213,137,34,17,240,42,8,0,68,21,0,1,2,2,140,100,65,11,27,167,16,2,0,0,0,0,0,0,0,0,0,0,248,252,6,2,6,30,28,0,0,0,240,254,22,254,248,0,0,0,0,254,190,126,240,192,120,60,254,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,20,16,53,33,174,160,148,40,128,3,66,44,37,20,18,20,18,9,8,4,0,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,15,31,112,48,32,112,24,0,0,63,63,2,6,7,63,62,0,0,64,63,67,0,1,3,0,0,63,63,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,192,224,240,120,184,252,124,220,94,206,190,110,94,254,46,164,240,176,248,24,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,248,126,223,239,167,35,115,221,14,15,245,247,255,187,226,242,250,253,253,115,147,48,16,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,252,254,6,6,14,28,0,0,128,248,126,14,254,248,0,0,0,254,254,126,56,224,192,240,126,254,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,9,33,0,6,17,31,11,23,31,13,13,3,2,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,63,63,96,112,56,56,0,0,127,127,7,3,3,127,127,0,0,63,127,60,0,3,7,1,0,127,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};

uint8_t const cam3[]=
{0x40,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,192,224,240,248,184,252,124,188,94,174,254,46,94,190,94,172,80,168,88,8,80,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,248,127,191,79,167,65,163,85,10,85,250,85,170,85,162,80,170,85,170,87,162,17,8,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,252,254,6,6,14,28,0,0,128,248,126,14,254,248,0,0,0,84,254,126,56,240,192,112,62,254,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,25,32,0,34,17,42,23,10,21,11,5,3,1,2,1,2,0,0,0,0,0,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,63,127,32,112,56,24,0,0,127,127,3,7,3,127,126,0,0,63,127,117,0,3,3,1,0,127,63,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,7,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x20,0x70,0x20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x20,0x70,0xF8,0x70,0x20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,192,224,240,120,184,124,252,220,158,94,190,118,110,94,170,68,184,84,184,80,128,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,248,62,87,175,87,163,83,171,21,138,117,174,213,170,69,162,80,171,85,162,21,32,16,10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,252,254,6,6,14,28,0,0,128,248,126,14,254,248,0,0,0,170,254,254,124,224,192,248,124,254,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,8,33,0,5,2,21,10,23,10,5,11,5,34,5,2,1,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,63,112,112,48,56,0,0,127,127,7,2,7,127,127,0,0,62,127,42,0,3,7,0,0,127,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,7,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x20,0x70,0x20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

void HWTask(void*)
{
    millisBegin();
    display.begin();
    kbd.begin();

    usb_serial_jtag_driver_config_t usb_serial_jtag_config = { .tx_buffer_size = 128, .rx_buffer_size = 128 };
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_serial_jtag_config));
    char input[64];                         // stores serial input
    int in= 0;                              // current char in serial input

    // Write image that is grayscale
    display.disp2(cam3, sizeof(cam3));
    //                                  start 1frame stop, lines
    uint8_t const scrollOn[]={0, 0x29, 0, 7,   7,     7,   32, 0x2f}; display.send(scrollOn, sizeof(scrollOn));
    vTaskDelay(10); // .1s delay gives time to first adc read to complete.. AND display splash screen...
    for (int i=0; i<100; i++) // 10s delay that can be interrupted by a key press...
    {
        if (kbd.read()!=0) break; // Also has end of course sensors...
        if (usb_serial_jtag_read_bytes(input, 1, 0)!=0) break; // break on serial in also...
        vTaskDelay(10);
    }

    // old, single color image....
    //display.blit(camimg, 0, 0); display.disp(); vTaskDelay(400); // 4s delay gives time to first adc read to complete.. AND display splash screen...

    MFocus.init(200ULL*150, 200ULL*2, 200);
    initSteppers();

    uint32_t inFocusPos= 0;                 // focuser input position

    uint8_t const scrollOff[]={0, 0x2e}; display.send(scrollOff, 2);

    while (true)
    {
        vTaskDelay(1); // if not there, causes adc read errors...
        uint32_t keys= kbd.read(); // Also has end of course sensors...
        uint32_t now= millis();
        stopStartStepperTimer(keys, now);
        filterWheelNext(now);
        doUI(now, keys); // Do UI.

        ///////////////////////////////////////////
        // serial input processing
        // ! -> returns 38 hex characters. pos in Dec(6), Ra(6), focuser(6), 1 byte (2 chr) of status bits, millis_timer(6), meridian_value(6), uncounted_steps(6). Then a #
        // % -> returns HW configuration. RA: maxPos(6), maxSpd(6), msToSpd(6), Dec: maxPos(6), maxSpd(6), msToSpd(6), timeCompensation(8) and #

        // :w# -> request wifi info. returns it in the same format as descibed bellow, but without the :W on front...
        // :WSssidpass# // S is sizeof ssid-'A' Sets wifi ssid/password. might require reboot to work..
        //   ':SN????#' -> set motor target position in hex
        //   ':FG#' -> start the motor toward destination
        //   ':FQ#' -> stop the motor. Keep current position
        //   ':Fo??????#' -> focuser out at ????? speed(steps/s)
        //   ':Fi??????#' -> focuser in at ????? speed(steps/s)
        //   ':fx#' -> engage filter wheel x (0 to 3)
        //   ':enx#' -> enable motors x where x is a bit field. so 7 is all and 0 is none...

        char *src= input+in+2;
        int len= usb_serial_jtag_read_bytes(src, sizeof(input)-in-2, 0);
        for (int i=0; i<len; i++)
        {
            char c= *src++;
            if (c<=' ') continue;                                  // ignore blanks
            if (c=='!')   // My "get info" command
            {
                char t[64];
                sprintf(t, "000000000000%06X%02X%06X00000000000%c#", (unsigned int)MFocus.pos/32, (MFocus.isMoving()?2:0)|(((keys&ButeeDown)!=0)?4:0)|(((keys&ButeeUp)!=0)?8:0) ,(unsigned int)now/1000, '0'+filterWheelPos());
                // bit 1: focus moving, bit 2: butee down, bit 3: butee up
                usb_serial_jtag_write_bytes(t, strlen(t), 1);
                continue;
            }
            if (c=='%')   // My "get config" command
            {
                char t[64];
                sprintf(t, "000000000000%06X00000000#", (unsigned int)MFocus.maxPos/32);
                usb_serial_jtag_write_bytes(t, strlen(t), 1);
                continue;
            }

            if (in==0 && c!=':') continue;                         // nothing until ':' (line start)...
            input[in++]= c; if (in==sizeof(input)-2) { in= 0; continue; } // save new character and overflow detection...
            input[in]= 0;
            if (c!='#' && c!='\n') continue;                       // not end of line... get next character
            in=0;                                                  // reset line
            char *s= input+3; // in most cases, numbers start at input+3. init once only
            // now look for commands in line...
            // Focuser commands
        #define t2(c1,c2) input[1]==c1 && input[2]==c2
            if (t2('F', 'G')) { doNotStop= true; MFocus.goToSteps(inFocusPos*32, MFocus.spdMax);  continue; } // -> start the motor toward destination
            if (t2('F', 'Q')) { doNotStop= false; MFocus.stop(); continue; } // -> stop the motor. Keep current position
            if (t2('F', 'o')) { doNotStop= true; MFocus.goUp(readDec(s, MFocus.spdMax));    continue; } // :Mo# move out
            if (t2('F', 'i')) { doNotStop= true; MFocus.goDown(readDec(s, MFocus.spdMax));  continue; } // :Mi# move in
            if (t2('S', 'N')) { inFocusPos= readHex(s); continue; } // ????-> set motor target position in hex
            if (input[1]=='f') { filterWheelPos(input[2]-'0'); continue; }
            if (t2('e', 'n'))
            {
                int m= input[3]-'0'; if (m<0 || m>7) continue;
                MFocus.enable(m);
                continue;
            }
            if (input[1]=='w' && input[2]=='#')  // sends wifi
            { 
                char t[128];
                sprintf(t, "%c%s%s#", 'A'+strlen(wifi), wifi, wifipass);
                usb_serial_jtag_write_bytes(t, strlen(t), 1);
                continue;

            }
            if (input[1]=='W')  // updates wifi
            {
                int pos= 3, ssidlen= input[2]-'A'; 
                int len= strlen(input+3); if (input[len+3-1]=='#' || input[len+3-1]<' ') len--;
                printf("received %s %d %d(total) %d(pass)\r\n", input, ssidlen, len, len-ssidlen);
                if (ssidlen>=len) continue; // problem!
                memcpy(wifi, input+3, ssidlen); wifi[ssidlen]= 0;
                memcpy(wifipass, input+3+ssidlen, len-ssidlen); wifipass[len-ssidlen]= 0;
                printf("ssid len %d pass %d\r\n", ssidlen, len-ssidlen);
                alpaca->saveLoadBegin();
                alpaca->save("wifi", wifi);
                alpaca->save("wifipass", wifipass);
                alpaca->saveLoadEnd();
            }
        #undef t2
        }
    }
}


class CMyFocuser : public CFocuser
{ public:
    CMyFocuser(int id): CFocuser(id, "CdB Focuser Driver", "1", "CdB Alpaca Focuser", "Focuser for RC600") { }
    bool get_absolute() override { return true; }
    bool get_ismoving() override { return MFocus.isMoving(); }
    int32_t get_maxincrement() override { return MFocus.maxPos/32; }
    int32_t get_maxstep() override { return MFocus.maxPos/32; }
    int32_t get_position() override { return MFocus.pos/32; }
    int32_t get_stepsize() override { return 6; }
    TAlpacaErr put_halt() override { doNotStop= false; MFocus.stop(); return ALPACA_OK; };
    TAlpacaErr put_move(int32_t position) override { doNotStop= true; MFocus.goToSteps(position*32, MFocus.spdMax); return ALPACA_OK; };
    void subSetup(CAlpaca *Alpaca, int sock, bool get, char *data, CMyStr &s) override // This allows you to add stuff in the HTML or handle inputs...
    {
        static char savedPosString[1024]; Alpaca->load("savedPos", "", savedPosString, sizeof(savedPosString)); // (name\0x8pos\0x7)*
        if (data!=nullptr)
        {   
            char const *d= getStrData(data, "home"); if (d!=nullptr) put_move(0);

            d= getStrData(data, "savePos");
            if (d!=nullptr)
            {
                char name[21]; int i; for (i=0; i<19; i++) if (*d<' ' || *d>'z') break; else name[i]= *d++; name[i++]= 8; name[i]= 0; // extract name
                size_t l= strlen(savedPosString); if (l>0 && savedPosString[l-1]!=7) { savedPosString[l-1]= 7; l++; } // make sure ends with 7
                char *d= strstr(savedPosString, name); // find existing name in the list...
                if (d!=nullptr) // if there...
                {
                    char *s= d; while (*s!=0 && *s!=7) s++; if (*s==7) s++; // go to end of pos
                    strcpy(d, s); // and erase by copying whatever is after over start...
                    l= strlen(savedPosString);
                }
                sprintf(savedPosString+l, "%s%d\007", name, int(MFocus.pos/32)); // add current post at end of string
                Alpaca->save("savedPos", savedPosString); // and save
            }

            d= getStrData(data, "erasePos");
            if (d!=nullptr)
            {
                char name[21]; int i; for (i=0; i<19; i++) if (*d<' ' || *d>'z') break; else name[i]= *d++; name[i++]= 8; name[i]= 0; // extract name
                size_t l= strlen(savedPosString); if (l>0 && savedPosString[l-1]!=7) { savedPosString[l-1]= 7; l++; } // make sure ends with 7
                char *d= strstr(savedPosString, name); // find existing name in the list...
                if (d!=nullptr) // if there...
                {
                    char *s= d; while (*s!=0 && *s!=7) s++; if (*s==7) s++; // go to end of pos
                    strcpy(d, s); // and erase by copying whatever is after over start...
                    Alpaca->save("savedPos", savedPosString); // and save
                }
            }
        }
        CFocuser::subSetup(Alpaca, sock, get, data, s);
        s.printf("<h1>Homming</h1>"
                "<form action=\"/setup/v1/%s/%d/setup\">"
                "  <input type=\"hidden\" id=\"home\" name=\"home\" value=\"0\">"
                "  <input type=\"submit\" value=\"Find Home\">"
                "</form>", get_type(), id);
        if (ButeeDownPos==-1) s.printf("<h2>Home not yet known<h2><br>");
        else { 
            s.printf("<h2>Home found at pos %d</h2><br>", ButeeDownPos);
            s.printf("<form action=\"/setup/v1/%s/%d/setup\">"
                "  <label for=\"savePos\">Position Name:</label>"
                "  <input type=\"text\" id=\"savePos\" name=\"savePos\" value=\"name\">"
                "  <input type=\"submit\" value=\"Save\">"
                "</form>",get_type(), id);
            char *S= savedPosString; bool hasone= false;
            while (*S!=0)
            {
                char name[20]; int i; for (i=0; i<19; i++) if (*S<=8) break; else name[i]= *S++; name[i]= 0; if (*S==8) S++;
                char pos[20]; int j; for (j=0; j<19; j++) if (*S<=7) break; else pos[j]= *S++; pos[j]= 0; if (*S==7) S++;
                if (i==0 || j==0) break;
                if (!hasone) { s.printf("<h2>Saved Positions</h2><br><table align=\"center\">"); hasone= true; }
                s.printf("<tr><th><form action=\"/setup/v1/%s/%d/setup\">"
                    "  <label for=\"position\">%s:</label>"
                    "  <input type=\"text\" id=\"position\" name=\"position\" value=\"%s\">"
                    "  <input type=\"submit\" value=\"GoTo\">"
                    "</form></th>"
                    "<th><form action=\"/setup/v1/%s/%d/setup\">"
                    "  <input type=\"hidden\" id=\"erasePos\" name=\"erasePos\" value=\"%s\">"
                    "  <input type=\"submit\" value=\"Erase\">"
                    "</form></th></tr>"
                    , get_type(), id, name, pos, get_type(), id, name);
            }
            if (hasone) { s.printf("</table>"); hasone= true; }
        }
    }
};

class CMyFilterWheel : public CFilterWheel { public: 
    static CMyFilterWheel *singleton;
    CMyFilterWheel(int id): CFilterWheel(id, "CdB Filters Driver", "1", "CdB Alpaca Filters", "Filters for RC600") { singleton= this; }
    int get_position() { return pos; };
    TAlpacaErr put_position(int32_t position) 
    { 
        if (position<0 && position>4) return ALPACA_ERR_INVALID_VALUE;
        requestedPos= position;
        return ALPACA_OK; 
    }
    int pos= 0;               // if 0, all motors are in off state, or getting there...
                              // else, motor "n" is activated, or getting there...
    uint32_t posOpenedAt= 0;  // time at which destination will be reached. pos can not be changed until this is 0
    uint32_t requestedPos= 0; // 4 bytes. Each is a position with the high bit set to 1 if requested... bit 6 set if openning... which leave 64 possible filters... 
    static int const closePwm= 50, openPwm= 102, poMovingCompensate=5;
    bool hasInit= false;
    //unsigned int volatile * const LEDC_HSCH0_CONF0_REG= (unsigned int  volatile *)0x60019000; 
    //unsigned int volatile * const LEDC_HSCH0_CONF1_REG= (unsigned int  volatile *)0x6001900c; 
    //unsigned int volatile * const EDC_HSCH0_HPOINT_REG= (unsigned int  volatile *)0x60019004; 
    //unsigned int volatile * const EDC_HSCH0_DUTY_REG= (unsigned int  volatile *)0x60019008; 

    void next(uint32_t nowms)
    {
        if (!hasInit) begin();
        if (int(posOpenedAt-nowms)>0) return; // no timer beeped...
        if (requestedPos==pos) // no changes
        {
            if (!MFocus.isMoving())
            {
                // full open all opened filters!
                if (pos!=1) analogWrite(LEDC_CHANNEL_0, closePwm);
                if (pos!=2) analogWrite(LEDC_CHANNEL_1, closePwm);
                if (pos!=3) analogWrite(LEDC_CHANNEL_2, closePwm);
            } else {
                // 20% close all opened filters!
                if (pos!=1) analogWrite(LEDC_CHANNEL_0, closePwm+poMovingCompensate);
                if (pos!=2) analogWrite(LEDC_CHANNEL_1, closePwm+poMovingCompensate);
                if (pos!=3) analogWrite(LEDC_CHANNEL_2, closePwm+poMovingCompensate);
            }
            return;        
        }
        int percent; ledc_channel_t p;
        if (pos!=0) { p= ledc_channel_t(pos-1); pos= 0; percent= closePwm+poMovingCompensate; }
        else { pos= requestedPos; percent= openPwm; p= ledc_channel_t(pos-1); }
        //analogWrite(p, percent);
        ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, p, percent, 800);
        ledc_fade_start(LEDC_LOW_SPEED_MODE, p, LEDC_FADE_NO_WAIT);            
//        printf("c0:%08x c1:%08x point:%08x duty:%08x\r\n", *LEDC_HSCH0_CONF0_REG, *LEDC_HSCH0_CONF1_REG, *EDC_HSCH0_HPOINT_REG, *EDC_HSCH0_DUTY_REG);
        posOpenedAt= nowms+1024; // do not do anything for 1 s!!!
    }

    void begin()
    {
        hasInit= true;
        ledc_timer_config_t ledc_timer = { .speed_mode=LEDC_LOW_SPEED_MODE, .duty_resolution= LEDC_TIMER_10_BIT, .timer_num= LEDC_TIMER_0, .freq_hz= 50, .clk_cfg= LEDC_AUTO_CLK };
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
        ledc_channel_config_t ledc_channel = {.gpio_num=filter1PWM, .speed_mode=LEDC_LOW_SPEED_MODE, .channel=LEDC_CHANNEL_0, .intr_type=LEDC_INTR_DISABLE, .timer_sel=LEDC_TIMER_0, .duty= closePwm, .hpoint=0 };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
        ledc_channel.gpio_num=filter2PWM; ledc_channel.channel=LEDC_CHANNEL_1; ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
        ledc_channel.gpio_num=filter3PWM; ledc_channel.channel=LEDC_CHANNEL_2; ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
        ledc_fade_func_install(0);
        analogWrite(LEDC_CHANNEL_0, closePwm); analogWrite(LEDC_CHANNEL_1, closePwm); analogWrite(LEDC_CHANNEL_2, closePwm);
// current freq: 100hz.  100% is 10ms pulse.
// doc says: 1ms (10% of 256 = 25) is open and 2ms (20% of 256 = 51) is closed...
        //if (pos==0) { analogWrite(LEDC_CHANNEL_0, closePwm); analogWrite(LEDC_CHANNEL_1, closePwm); analogWrite(LEDC_CHANNEL_2, closePwm); }
        //if (pos==1) { analogWrite(LEDC_CHANNEL_0, openPwm); analogWrite(LEDC_CHANNEL_1, closePwm); analogWrite(LEDC_CHANNEL_2, closePwm); }
        //if (pos==2) { analogWrite(LEDC_CHANNEL_0, closePwm); analogWrite(LEDC_CHANNEL_1, openPwm); analogWrite(LEDC_CHANNEL_2, closePwm); }
        //if (pos==3) { analogWrite(LEDC_CHANNEL_0, closePwm); analogWrite(LEDC_CHANNEL_1, closePwm); analogWrite(LEDC_CHANNEL_2, openPwm); }
    }

    void analogWrite(ledc_channel_t pin, int val)
    {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, pin, val));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, pin));
    }
};

CMyFilterWheel *CMyFilterWheel::singleton= nullptr;
int filterWheelPos() { return CMyFilterWheel::singleton->pos; }
void filterWheelPos(int pos) { CMyFilterWheel::singleton->put_position(pos); }
void filterWheelNext(uint32_t now) { CMyFilterWheel::singleton->next(now); }



static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) { esp_wifi_connect(); wificonnected= false; } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        esp_wifi_connect();
        ESP_LOGI(TAG, "retry to connect to the AP");
        wificonnected= false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        sprintf(ipadr, IPSTR, IP2STR(&event->ip_info.ip));
        wificonnected= true;
    }
}

void startWifi(char const *net, char const *pass, char const *hostname)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) { ESP_ERROR_CHECK(nvs_flash_erase()); ret = nvs_flash_init(); }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *netif= esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(netif, hostname);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));
    wifi_config_t wifi_config; memset(&wifi_config, 0, sizeof(wifi_config));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    memcpy(wifi_config.sta.ssid, net, strlen(net));
    memcpy(wifi_config.sta.password, pass, strlen(pass));
    wifi_config.sta.pmf_cfg.capable= true;
    wifi_config.sta.pmf_cfg.required= false;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    if (strlen(net)!=0)
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

extern "C" void app_main(void)
{
  xTaskCreate(HWTask, "HWTask", 4096, nullptr, 2, nullptr);
  alpaca= new CAlpaca("FocusServer", "CdB", "AlpacaRCFoc", "Mars");
  alpaca->load("wifi", "", wifi, sizeof(wifi));
  alpaca->load("wifipass", "", wifipass, sizeof(wifipass));
  startWifi(wifi, wifipass, alpaca->ServerName);
  alpaca->addDevice(new CMyFocuser(0));
  alpaca->addDevice(new CMyFilterWheel(0));
  alpaca->start(80);
  // main task will return and be destroyed. alpaca devices have been created and will stay alive...
}

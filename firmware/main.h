typedef enum 
{
    Left = 0,
    Right = 1
} Direction;

typedef enum 
{
    FastMove = 0,
    Calibrate = 1,
    Normal = 2
} Mode;

#define BUFFERSIZE              32
#define M_PI                    3.14159265359
#define ACCELERATION_WINDOW     150
#define LIMITSWITCH_LEFT        PORTFbits.RF6
#define LIMITSWITCH_RIGHT       PORTAbits.RA12

//  6500 = 0.90ms  12rpm
//  7000 = 0.96ms  40rpm
//  7250 = 1.00ms  ??rpm
//  7500 = 1.03ms 290rpm <--- fastest without skipping
//  8000 = 1.10ms 273rpm
//  8250 = 1.14ms 264rpm
//  8500 = 1.17ms 257rpm
//  9000 = 1.24ms 242rpm
//  9500 = 1.31ms 229rpm
// 10000 = 1.37ms 218rpm
//  20000 =  2.74ms 109rpm
//  30000 =  4.10ms  73rpm
//  50000 =  6.84ms  44rpm
// 100000 = 13.65ms  ??rpm
// 200000 = 27.29ms  ??rpm
// 300000 = 41.01ms
// 500000 = 68.2ms
#define TILT_SPEED_MIN          500000
#define TILT_SPEED_MAX          7500


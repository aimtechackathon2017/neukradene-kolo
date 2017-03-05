#include "mbed.h"
#include "x_nucleo_iks01a1.h"

const int ACC_TRESHOLD = 100;
const int THIEF_WARMDOWN = 10 * 60 * 10; // 10 minutes

/* Instantiate the expansion board */
static X_NUCLEO_IKS01A1* mems_expansion_board = X_NUCLEO_IKS01A1::Instance(D14, D15);

static MotionSensor* accelerometer = mems_expansion_board->GetAccelerometer();

DigitalIn mybutton(USER_BUTTON);
DigitalOut myled(LED1);

/* Serial port over USB */
Serial pc(USBTX, USBRX);

/* Serial connection to sigfox modem */
Serial modem(PA_9, PA_10);
//Serial modem(PA_2, PA_3);

// serial to gps
Serial gps_device(PC_10, PC_11);

// GPS coordinate structure, 12 bytes size on 32 bits platforms
struct gpscoord {
    float a_latitude; // 4 bytes
    float a_longitude; // 4 bytes
};

int locked = 0;

int main()
{
    int32_t axes[3];
    float avg[3];
    float last_avg[3];
    int32_t sum[3] = { 0, 0, 0 };
    int count = 0;
    float diff;
    int warmup = 3; // how many diffs to ignore
    int warmdown = 0;

    // gps
    float latitude = 49.740440f;
    float longitude = 13.365906f;

    char buffer[32];

    while (1) {
        if (mybutton == 0) { // Button is pressed
            myled = !myled; // Toggle the LED state

            if (locked == 0) {
                locked = 1;
                printf("Zamknul jsi kolo!\r\n");
            }
            else {
                locked = 0;
                printf("Odemknul jsi kolo!\r\n");
            }

            warmup = 3;
            warmdown = 0;
        }

        //    pc.printf("Is locked? %d\r\n", locked);

        accelerometer->Get_X_Axes(axes);
        //    printf("LSM6DS0 [acc/mg]:      %6ld, %6ld, %6ld\r\n", axes[0], axes[1], axes[2]);

        sum[0] += axes[0];
        sum[1] += axes[1];
        sum[2] += axes[2];

        if (++count == 10) {
            avg[0] = sum[0] / count;
            avg[1] = sum[1] / count;
            avg[2] = sum[2] / count;

            count = 0;
            sum[0] = 0;
            sum[1] = 0;
            sum[2] = 0;

            diff = abs(abs(avg[0]) - abs(last_avg[0]));
            diff += abs(abs(avg[1]) - abs(last_avg[1]));
            diff += abs(abs(avg[2]) - abs(last_avg[2]));

            printf("diff: %f\r\n", diff);

            //        printf("LSM6DS0 [acc/mg]:      %f, %f, %f\r\n", avg[0], avg[1], avg[2]);
            if (locked == 1) {
                if (warmup-- < 0 && diff > ACC_TRESHOLD && warmdown-- == 0) {
                    printf("\r\nCovece kradou ti KOLO!\r\n");
                    warmdown = THIEF_WARMDOWN;

                    // Store coordinates into dedicated structure
                    gpscoord coords = { latitude, longitude };

                    // send to sigfox
                    pc.printf("AT$SF=%x\r\n", coords);
                    modem.printf("AT$SF=%x\r\n", coords);
                }
            }
            last_avg[0] = avg[0];
            last_avg[1] = avg[1];
            last_avg[2] = avg[2];
        }

        printf(".");
        wait(0.1);
    }
}

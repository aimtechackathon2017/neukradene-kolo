#include "mbed.h"
#include "x_nucleo_iks01a1.h"

const int ACC_TRESHOLD = 100;

/* Instantiate the expansion board */
static X_NUCLEO_IKS01A1 *mems_expansion_board = X_NUCLEO_IKS01A1::Instance(D14, D15);

static MotionSensor *accelerometer = mems_expansion_board->GetAccelerometer();
 
DigitalIn mybutton(USER_BUTTON);
DigitalOut myled(LED1);

/* Serial port over USB */
Serial pc(USBTX, USBRX);

/* Serial connection to sigfox modem */
//Serial modem(PA_9, PA_10);
//Serial modem(PA_2, PA_3);
Serial modem(PC_10, PC_11);


int locked = 0;

// fce prototypes
void sertmout();
bool modem_command_check_ok(char * command);
void modem_setup();
void txData(uint8_t btn);

bool ser_timeout = false;
 
int main() {
  int32_t axes[3];
  float avg[3];
  float last_avg[3];
  int32_t sum[3] = {0,0,0};
  int count = 0;
  float diff;
  
  wait(3);
  
    pc.printf("is readable?\r\n");
  
    /* first clear serial data buffers */
    while(modem.readable()) modem.getc();
    
    pc.printf("modem readable\r\n");
  modem_setup();
  
  while(1) {
    if (mybutton == 0) { // Button is pressed
      myled = !myled; // Toggle the LED state
            
      locked = locked == 0 ? 1 : 0; 
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
        
        modem_command_check_ok("AT");
        
        if (diff > ACC_TRESHOLD) {
            printf("\r\nCovece kradou ti KOLO!\r\n");
        }
        
        last_avg[0] = avg[0];
        last_avg[1] = avg[1];
        last_avg[2] = avg[2];
    }
    
    printf(".");
    wait(0.1);
  }
}



/* TX data over Sigfox */
//void txData (uint8_t btn)
//{
//
//    float    tAvg;
//    char     command[32];
//    sensor.Sense();
//    tAvg = sensor.GetAverageTemp();
//    char temperature[6] ="";
//    sprintf(temperature, "%3.1f", tAvg);
//    for(int i = 0; i < 5; i++)
//        if(temperature[i]==0) temperature[i] = ' ';
//    sprintf(command, "AT$SF=0142544e%x20%x%x%x%x%x43,2,0\n", btn+48, temperature[0],temperature[1],temperature[2],temperature[3],temperature[4]);
//    pc.printf("Sending pressed button %d and temperature %s C over Sigfox.\n", btn, temperature);
//    pc.printf("using modem command: %s", command);
//    modem_command_check_ok(command);
//    LED_1 = 1;
//    LED_2 = 1;
//}

void modem_setup()
{
    /* Reset to factory defaults */
    if(modem_command_check_ok("AT&F")) {
        pc.printf("Factory reset succesfull\r\n");
    } else {
        pc.printf("Factory reset TD120x failed\r\n");
    }
    /* Disable local echo */
    modem.printf("ATE0\n");
    if(modem_command_check_ok("ATE0")) {
        pc.printf("Local echo disabled\r\n");
    }
    /* Write to mem */
    if(modem_command_check_ok("AT&W")) {
        pc.printf("Settings saved!\r\n");
    }
}

/* ISR for serial timeout */
void sertmout()
{
    pc.printf("serial timeouted!\r\n");
    ser_timeout = true;
}

bool modem_command_check_ok(char * command)
{
    /* first clear serial data buffers */
    while(modem.readable()) modem.getc();
    /* Timeout for response of the modem */
    Timeout tmout;
    ser_timeout = false;
    /* Buffer for incoming data */
    char responsebuffer[6];
    /* Flag to set when we get 'OK' response */
    bool ok = false;
    bool error = false;
    /* Print command to TD120x */
    modem.printf(command);
    /* Newline to activate command */
    modem.printf("\n");
    /* Wait untill serial feedback, max 3 seconds before timeout */
    tmout.attach(&sertmout, 3.0);
    while(!modem.readable()&& ser_timeout == false);
    while(!ok && !ser_timeout && !error) {
        if(modem.readable()) {
            for(int i = 0; i < 5; i++) {
                responsebuffer[i] = responsebuffer[i+1];
            }
            responsebuffer[5] = modem.getc();
            if(responsebuffer[0] == '\r' && responsebuffer[1] == '\n' && responsebuffer[2] == 'O' && responsebuffer[3] == 'K' && responsebuffer[4] == '\r' && responsebuffer[5] == '\n' ) {
                ok = true;
            } else if(responsebuffer[0] == '\r' && responsebuffer[1] == '\n' && responsebuffer[2] == 'E' && responsebuffer[3] == 'R' && responsebuffer[4] == 'R' && responsebuffer[5] == 'O' ) {
                error = true;
            }
        }
    }
    tmout.detach();
    return ok;
}
 

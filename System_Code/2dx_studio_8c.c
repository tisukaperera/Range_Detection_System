// 2DX3 Final Demo Code
// Student: W. Tisuka Perera
// MacID: pererw2
// Student #: 400318373
// Date: April 11th, 2022

#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "vl53l1x_api.h"
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include <math.h>




#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable

#define MAXRETRIES              5           // number of receive attempts before giving up
void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           													// activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;          												// activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};																		// ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           																	// 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             																	// 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             																	// 5) enable digital I/O on PB2,3
//    GPIO_PORTB_AMSEL_R &= ~0x0C;          																// 7) disable analog functionality on PB2,3

                                                                            // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                      													// 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                       	// 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
//    I2C0_MTPR_R = 0x3B;                                        						// 8) configure for 100 kbps clock
        
}

void PortH_Init(void){
	//Use PortK pins for output
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;				// activate clock for Port N
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R7) == 0){};	// allow time for clock to stabilize
	GPIO_PORTH_DIR_R |= 0xFF;        								// make PN0 out (PN0 built-in LED1)
  GPIO_PORTH_AFSEL_R &= ~0xFF;     								// disable alt funct on PN0
  GPIO_PORTH_DEN_R |= 0xFF;        								// enable digital I/O on PN0
																									// configure PN1 as GPIO
  //GPIO_PORTM_PCTL_R = (GPIO_PORTM_PCTL_R&0xFFFFFF0F)+0x00000000;
  GPIO_PORTH_AMSEL_R &= ~0xFF;     								// disable analog functionality on PN0		
	return;
}

void PortE_Init(void){
SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4; //activate the clock for Port E
while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R4) == 0){}; //allow time for clock to stabilize
	GPIO_PORTE_DIR_R = 0b00000000; // Make PE0 input, reading if the button is pressed or not
	GPIO_PORTE_DEN_R = 0b00000001; // Enable PE0
return;
}

//Turns on D2, D1
void PortN0N1_Init(void){
SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12; //activate the clock for Port N
while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};//allow time for clock to stabilize
GPIO_PORTN_DIR_R=0b00000011; //Make PN0 and PN1 outputs, to turn on LED's
GPIO_PORTN_DEN_R=0b00000011; //Enable PN0 and PN1
return;
}

//Turns on D3, D4
void PortF0F4_Init(void){
SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5; //activate the clock for Port F
while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R5) == 0){};//allow time for clock to stabilize
GPIO_PORTF_DIR_R=0b00010001; //Make PF0 and PF4 outputs, to turn on LED's
GPIO_PORTF_DEN_R=0b00010001;
return;
}

//The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0

    return;
}

//XSHUT     This pin is an active-low shutdown input; 
//					the board pulls it up to VDD to enable the sensor by default. 
//					Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
//    FlashAllLEDs();
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
    
}


//*********************************************************************************************************
//*********************************************************************************************************
//***********					MAIN Function				*****************************************************************
//*********************************************************************************************************
//*********************************************************************************************************
uint16_t	dev = 0x29;			//address of the ToF sensor as an I2C slave peripheral
int status=0;

void spin(){
	for(int i=0; i<64; i++){ // 32 steps (for a complete rotation) * 64 (gear ratio) / 4 (steps per for loop) / 8 (360 deg / 8 = 45 deg)		
		GPIO_PORTH_DATA_R = 0b00001100;
		SysTick_Wait100us(25);
		GPIO_PORTH_DATA_R = 0b00000110;
		SysTick_Wait100us(25);
		GPIO_PORTH_DATA_R = 0b00000011;
		SysTick_Wait100us(25);
		GPIO_PORTH_DATA_R = 0b00001001;
		SysTick_Wait100us(25);
	}
	GPIO_PORTH_DATA_R = 0b00000000;
	
	GPIO_PORTF_DATA_R = 0b00000000; 
	GPIO_PORTN_DATA_R = 0b00000001;
	SysTick_Wait10ms(25);
	GPIO_PORTF_DATA_R = 0b00000000; 
	GPIO_PORTN_DATA_R = 0b00000000;
}

void spinBack(){
	for(int i=0; i<64; i++){ // 32 steps (for a complete rotation) * 64 (gear ratio) / 4 (steps per for loop) / 8 (360 deg / 8 = 45 deg)		
		GPIO_PORTH_DATA_R = 0b00001001;
		SysTick_Wait100us(25);
		GPIO_PORTH_DATA_R = 0b00000011;
		SysTick_Wait100us(25);
		GPIO_PORTH_DATA_R = 0b00000110;
		SysTick_Wait100us(25);
		GPIO_PORTH_DATA_R = 0b00001100;
		SysTick_Wait100us(25);
	}
	GPIO_PORTH_DATA_R = 0b00000000;
}

int z=0;

int main(void) {
  uint8_t byteData, byte2Data, sensorState=0, myByteArray[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} , i=0;
  uint16_t wordData;
  uint16_t Distance;
  uint16_t SignalRate;
  uint16_t AmbientRate;
  uint16_t SpadNum; 
  uint8_t RangeStatus;
  uint8_t dataReady;
	double rad_angle;
	double x;
	double y;

	//initialize
	PLL_Init();	
	SysTick_Init();
	PortH_Init();	
	PortE_Init();
	PortN0N1_Init();
	PortF0F4_Init();
	onboardLEDs_Init();
	I2C_Init();
	UART_Init();
	
	// hello world!
	//UART_printf("Program Begins\r\n");
	//int mynumber = 1;
	//sprintf(printf_buffer,"2DX4 Program Studio Code %d\r\n",mynumber);
	//UART_printf(printf_buffer);


/* Those basic I2C read functions can be used to check your own I2C functions */
	status = VL53L1X_GetSensorId(dev, &wordData);

	//sprintf(printf_buffer,"(Model_ID, Module_Type)=0x%x\r\n",wordData);
	//UART_printf(printf_buffer);

	// Booting ToF chip
	while(sensorState==0){
		status = VL53L1X_BootState(dev, &sensorState);
		SysTick_Wait10ms(5);
  }
	FlashAllLEDs();
	//UART_printf("ToF Chip Booted!\r\n Please Wait...\r\n");
	
	status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
	
  /* This function must to be called to initialize the sensor with the default setting  */
  status = VL53L1X_SensorInit(dev);
	Status_Check("SensorInit", status);

	
  /* Optional functions to be used to change the main ranging parameters according the application requirements to get the best ranging performances */
//  status = VL53L1X_SetDistanceMode(dev, 2); /* 1=short, 2=long */
//  status = VL53L1X_SetTimingBudgetInMs(dev, 100); /* in ms possible values [20, 50, 100, 200, 500] */
//  status = VL53L1X_SetInterMeasurementInMs(dev, 200); /* in ms, IM must be > = TB */

  status = VL53L1X_StartRanging(dev);   // This function has to be called to enable the ranging

	
	// Get the Distance Measures 50 times
	for(int ii = 0; ii < 361; ii+=45) {
		
		//wait until the ToF sensor's data is ready
	  while (dataReady == 0){
		  status = VL53L1X_CheckForDataReady(dev, &dataReady);
      FlashLED3(1);
      VL53L1_WaitMs(dev, 10);
	  }
		dataReady = 0;
		spin();
	  
		//read the data values from ToF sensor
		status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
	  status = VL53L1X_GetDistance(dev, &Distance);					//The Measured Distance value
		status = VL53L1X_GetSignalRate(dev, &SignalRate);
		status = VL53L1X_GetAmbientRate(dev, &AmbientRate);
		status = VL53L1X_GetSpadNb(dev, &SpadNum);
    
		FlashLED4(1);

	  status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
		
		rad_angle = ii*3.14159265/180;
		x = Distance * cos(rad_angle);
		y = Distance * sin(rad_angle);
		
		// print the resulted readings to UART
		sprintf(printf_buffer,"%f %f %d %u\r\n", round(x), round(y), z, Distance);		
		UART_printf(printf_buffer);
	  SysTick_Wait10ms(24);
  }
	
	for(int ii = 0; ii < 8; ii++) {
		spinBack();
	}
  
	VL53L1X_StopRanging(dev);
  while(1) {
		if((GPIO_PORTE_DATA_R | 0b11111110) == 0b11111110){
			z+=10;
			main();
		} 
	}

}
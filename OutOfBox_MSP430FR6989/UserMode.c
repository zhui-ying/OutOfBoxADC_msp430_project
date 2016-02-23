/*******************************************************************************
 *
 * UserMode.c
 *
 * Simple thermometer application that uses the external temperature sensor to
 * measure and compare with external temperature sensor and display temperature 
 * on the segmented LCD screen and send value to PC through bluetooth
 *
 * October 2015
 * Alex.Liu
 ******************************************************************************/

#include <driverlib.h>
#include "UserMode.h"
#include "hal_LCD.h"
#include <stdio.h>

volatile int temp_degC;                       // Celcius measurement for inner temperature
volatile int temp_degF;                       // Fahrenheit measurement for inner temperature

volatile int pot_degC;                       // Celcius measurement for potentiometer 
volatile int pot_degF;                       // Fahrenheit measurement for potentiometer 

int fputc(int ch,FILE *f)
{
  EUSCI_A_UART_transmitData(EUSCI_A1_BASE,(uint8_t)ch);
  return ch;
}

// TimerA UpMode Configuration Parameter
Timer_A_initUpModeParam initUpParam_A1_USER =
{
    TIMER_A_CLOCKSOURCE_ACLK,               // ACLK Clock Source
    TIMER_A_CLOCKSOURCE_DIVIDER_1,          // ACLK/1 = 32768Hz
    0x2000,                                 // Timer period
    TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
    TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE ,   // Disable CCR0 interrupt
    TIMER_A_DO_CLEAR                        // Clear value
};

Timer_A_initCompareModeParam initCompParam_USER =
{
    TIMER_A_CAPTURECOMPARE_REGISTER_1,        // Compare register 1
    TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE, // Disable Compare interrupt
    TIMER_A_OUTPUTMODE_RESET_SET,             // Timer output mode 7
    0x1000                                    // Compare value
};

void uart1Init(void)
{
      /*
     * Select Port 2d
     * Set Pin 0, 1 to input Secondary Module Function, (UCA0TXD/UCA0SIMO, UCA0RXD/UCA0SOMI).
     */
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P3,
        GPIO_PIN4 + GPIO_PIN5,
        GPIO_PRIMARY_MODULE_FUNCTION
        );

    /*
     * Disable the GPIO power-on default high-impedance mode to activate
     * previously configured port settings
     */
    PMM_unlockLPM5();

    EUSCI_A_UART_disable(EUSCI_A1_BASE);
    
        // Configure UART
    // Baud Rate calculation
    // 1000000/115200 = 8.6806;
    // Fractional portion = 0.6806
    // User's Guide Table 21-4: UCBRSx = 0xD6
    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK; //1MHz
    param.clockPrescalar = 8;
    //param.firstModReg = 0;
    param.secondModReg = 0xD6;
    
    param.parity = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode = EUSCI_A_UART_MODE;
    param.overSampling = EUSCI_A_UART_LOW_FREQUENCY_BAUDRATE_GENERATION;

    if(STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A1_BASE, &param))
    {
        return;
    }

    EUSCI_A_UART_enable(EUSCI_A1_BASE);
}

void userModeInit(void)
{
  userModeRunning = 1;
  tempUnit = 0;//show temperature value
  displayScrollText("USER MODE");
  
  uart1Init();
  
  //disable temperature interrupt
  ADC12_B_disableInterrupt(ADC12_B_BASE,
                            ADC12_B_IE0,
                            0,
                            0);
    
    //Set P1.3 as Ternary Module Function Output.
    /*
     * Select Port 1
     * Set Pin 3 to output Ternary Module Function, (A3, C1, VREF+, VeREF+).
     */
    GPIO_setAsPeripheralModuleFunctionOutputPin(
            GPIO_PORT_P1,
            GPIO_PIN3,
            GPIO_TERNARY_MODULE_FUNCTION
            );

    // Select internal ref = 1.2V
    Ref_A_setReferenceVoltage(REF_A_BASE,
                              REF_A_VREF1_2V);
    // Internal Reference ON
    Ref_A_enableReferenceVoltage(REF_A_BASE);

    // Enables the internal temperature sensor
    Ref_A_enableTempSensor(REF_A_BASE);

    while(!Ref_A_isVariableReferenceVoltageOutputReady(REF_A_BASE));

    // Initialize the ADC12B Module
    /*
     * Base address of ADC12B Module
     * Use internal ADC12B bit as sample/hold signal to start conversion
     * USE MODOSC 5MHZ Digital Oscillator as clock source
     * Use default clock divider/pre-divider of 1
     * Use Temperature Sensor internal channel
     */
    ADC12_B_initParam initParam = {0};
    initParam.sampleHoldSignalSourceSelect = ADC12_B_SAMPLEHOLDSOURCE_4;
    initParam.clockSourceSelect = ADC12_B_CLOCKSOURCE_ADC12OSC;
    initParam.clockSourceDivider = ADC12_B_CLOCKDIVIDER_1;
    initParam.clockSourcePredivider = ADC12_B_CLOCKPREDIVIDER__1;
    initParam.internalChannelMap = ADC12_B_TEMPSENSEMAP;
    ADC12_B_init(ADC12_B_BASE, &initParam);

    // Enable the ADC12B module
    ADC12_B_enable(ADC12_B_BASE);

    /*
     * Base address of ADC12B Module
     * For memory buffers 0-7 sample/hold for 256 clock cycles
     * For memory buffers 8-15 sample/hold for 4 clock cycles (default)
     * Disable Multiple Sampling
     */
    ADC12_B_setupSamplingTimer(ADC12_B_BASE,
                               ADC12_B_CYCLEHOLD_256_CYCLES,
                               ADC12_B_CYCLEHOLD_4_CYCLES,
                               ADC12_B_MULTIPLESAMPLESDISABLE);

    // Configure Memory Buffer
    /*
     * Base address of the ADC12B Module
     * Configure memory buffer 0
     * Map input A30 to memory buffer 0
     * Vref+ = VRef+
     * Vref- = Vref-
     */
    ADC12_B_configureMemoryParam configureMemoryParam = {0};
    configureMemoryParam.memoryBufferControlIndex = ADC12_B_MEMORY_0;
    configureMemoryParam.inputSourceSelect = ADC12_B_INPUT_TCMAP;
    configureMemoryParam.refVoltageSourceSelect =
        ADC12_B_VREFPOS_INTBUF_VREFNEG_VSS;
    configureMemoryParam.endOfSequence = ADC12_B_NOTENDOFSEQUENCE;//not end seq  modfied by LC 2015.10.24 15:06
    configureMemoryParam.windowComparatorSelect =
        ADC12_B_WINDOW_COMPARATOR_DISABLE;
    configureMemoryParam.differentialModeSelect =
        ADC12_B_DIFFERENTIAL_MODE_DISABLE;
    ADC12_B_configureMemory(ADC12_B_BASE, &configureMemoryParam);

    //add by LC 2015.10.24 15:07
    /*
     * Base address of the ADC12B Module
     * Configure memory buffer 1
     * Map input A3 to memory buffer 1
     * Vref+ = VRef+
     * Vref- = Vref-
     * Memory buffer 1 is the end of a sequence
     */
    ADC12_B_configureMemoryParam configureMemoryParaA3 = {0};
    configureMemoryParaA3.memoryBufferControlIndex = ADC12_B_MEMORY_1;
    configureMemoryParaA3.inputSourceSelect = ADC12_B_INPUT_A3;
    configureMemoryParaA3.refVoltageSourceSelect =
        ADC12_B_VREFPOS_INTBUF_VREFNEG_VSS;
    configureMemoryParaA3.endOfSequence = ADC12_B_ENDOFSEQUENCE;
    configureMemoryParaA3.windowComparatorSelect =
        ADC12_B_WINDOW_COMPARATOR_DISABLE;
    configureMemoryParaA3.differentialModeSelect =
        ADC12_B_DIFFERENTIAL_MODE_DISABLE;
    ADC12_B_configureMemory(ADC12_B_BASE, &configureMemoryParaA3);

    ADC12_B_clearInterrupt(ADC12_B_BASE,
                           0,
                           ADC12_B_IFG1
                           );

    // Enable memory buffer 1 interrupt
    ADC12_B_enableInterrupt(ADC12_B_BASE,
                            ADC12_B_IE1,
                            0,
                            0);
    //end by LC 2015.10.24 15:07
    // Start ADC conversion
    ADC12_B_startConversion(ADC12_B_BASE, ADC12_B_START_AT_ADC12MEM0, ADC12_B_REPEATED_SEQOFCHANNELS);//change to seq sample  add by LC 2015.10.24 15:11

    // TimerA1.1 (125ms ON-period) - ADC conversion trigger signal
    Timer_A_initUpMode(TIMER_A1_BASE, &initUpParam_A1_USER);

    // Initialize compare mode to generate PWM1
    Timer_A_initCompareMode(TIMER_A1_BASE, &initCompParam_USER);

    // Start timer A1 in up mode
    Timer_A_startCounter(TIMER_A1_BASE,
        TIMER_A_UP_MODE
        );

     // Check if any button is pressed
    Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam_A0);
    __bis_SR_register(LPM3_bits | GIE);         // enter LPM3
    __no_operation(); 
}



void userMode(void)
{
  while(mode == USER_MODE)
  {
        __bis_SR_register(LPM3_bits | GIE);                       // LPM3 with interrupts enabled
        __no_operation();                                         // Only for debugger

        if (userModeRunning)
        {
                          
            // Turn LED1 on when waking up to calculate temperature and update display
            P1OUT |= BIT0;

            // Calculate inner Temperature in degree C and F
            signed short temp = (ADC12MEM0 - CALADC_12V_30C);
            temp_degC = ((long)temp * 10 * (85-30) * 10)/((CALADC_12V_85C-CALADC_12V_30C)*10) + 300;
            temp_degF = (temp_degC) * 9 / 5 + 320;

            //Calculate potentiometer in degree C and F
            signed short potentiometer = (ADC12MEM1 - CALADC_12V_30C);
            pot_degC = ((long)potentiometer * 10 * (85-30) * 10)/((CALADC_12V_85C-CALADC_12V_30C)*10) + 300;
            pot_degF = (pot_degC) * 9 / 5 + 320;           
            // Update temperature on LCD
            displayValue();
            
            //Turn LED2 on if potentiometer value > inner Temperature value
            if(pot_degC > temp_degC)
            {
              P9OUT |= BIT7;
            }else
            {
              P9OUT &= ~BIT7;
            }

            P1OUT &= ~BIT0;
        }
  }
}

void displayValue()
{
  //Send values to PC
    printf("value=%d,%d\r\n",temp_degC,pot_degC);
    
    clearLCD();
    // show temp_degC or pot_degC
    int deg;
    showChar('C',pos6);
    if (tempUnit == 0)
    {
        deg = temp_degC;
    }
    else
    {
        deg = pot_degC;
    }

    // Handle negative values
    if (deg < 0)
    {
        deg *= -1;
        // Negative sign
        LCDMEM[pos1+1] |= 0x04;
    }

    // Handles displaying up to 999.9 degrees
    if (deg>=1000)
        showChar((deg/1000)%10 + '0',pos2);
    if (deg>=100)
        showChar((deg/100)%10 + '0',pos3);
    if (deg>=10)
        showChar((deg/10)%10 + '0',pos4);
    if (deg>=1)
        showChar((deg/1)%10 + '0',pos5);

    // Decimal point
    LCDMEM[pos4+1] |= 0x01;

    // Degree symbol
    LCDMEM[pos5+1] |= 0x04;
}

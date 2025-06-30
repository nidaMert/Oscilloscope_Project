#include <msp430.h>
#include <stdint.h>
//--------------------Defines-------------------------

#define SAMPLING_INTERVAL 2000
#define UART_TXD BIT2
#define UART_RXD BIT1

#define ADC_INPUT_CHANNEL INCH_7
#define ADC_INPUT_PIN BIT7

#define RESISTOR_DIV_R1 22000
#define RESISTOR_DIV_R2 8200


char adcBuffer[10];
unsigned int adcValue;
unsigned int adcSampleFlag = 0;
float adcInput = 0.0;
float Vref = 3.3f;
float resDividerInput;
float vADC;
float vINPUT;
//+++++++++++++++++ Uart Initialize +++++++++++++++

void uartInit(void)
{
    P1SEL |= UART_TXD + UART_RXD;
    P1SEL2 |= UART_RXD + UART_TXD;

    // Setup UART

        UCA0CTL1 |= UCSWRST;                                // Reset State
        UCA0CTL1 |= UCSSEL_2;                               // SMCLK

        UCA0BR0 = 139;                                      // @ 16MHZ 115200 baud
        UCA0BR1 = 0;
        UCA0MCTL = UCBRS2;                                  // Modulation

        UCA0CTL1 &= ~UCSWRST;                               // Release from reset
}


//+++++++++++++++++ Uart Send Char +++++++++++++++


void uartSendChar (char c)
{
    while(!(IFG2 & UCA0TXIFG));
    UCA0TXBUF = c;
}


//+++++++++++++++++ Uart Send String +++++++++++++++


void uartSendString(const char *str)
{
    while (*str != '\0') {
        while (!(IFG2 & UCA0TXIFG));
        UCA0TXBUF = *str;
        str++;
        }
}

//+++++++++++++++++ ADC Initialize +++++++++++++++


void adcInit(void)
{
    ADC10CTL1 = ADC_INPUT_CHANNEL;
    ADC10CTL0 = SREF_0 | ADC10SHT_3 | ADC10ON | ADC10IE;
    ADC10AE0 |= ADC_INPUT_PIN;
}



//+++++++++++++++++ ADC Read +++++++++++++++


unsigned int adcRead(void)
{
    ADC10CTL0 |= ENC + ADC10SC;
    while(ADC10CTL1 & ADC10BUSY);
    return ADC10MEM;
}



//+++++++++++++++++ Unsigned Int to String +++++++++++++++


void uintToStr(unsigned int value, char* str)
{
    char temp[10];
    int i = 0, j = 0;

    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    while (value > 0) {
        temp[i++] = (value % 10) + '0';
        value /= 10;
    }

    // reverse
    for (j = 0; j < i; j++) {
        str[j] = temp[i - j - 1];
    }

    str[j] = '\0';
}


//+++++++++++++++++ Timer Initialize +++++++++++++++


void timerInit(void)
{
    TA0CTL = TASSEL_2 | MC_1 | ID_3;
    TA0CCR0 = SAMPLING_INTERVAL;
    TA0CCTL0 = CCIE;
    TA0R = 0;
}

//+++++++++++++++++ Float to String (xx.xx) +++++++++++++++

char* ftoa_2_2(char *buffer, float value)
{
    uint8_t int_part = (uint8_t)value;
    uint16_t frac_part = (uint16_t)((value - int_part) * 100 + 0.5f);

    buffer[0] = '0' + (int_part / 10);
    buffer[1] = '0' + (int_part % 10);
    buffer[2] = '.';
    buffer[3] = '0' + (frac_part / 10);
    buffer[4] = '0' + (frac_part % 10);
    buffer[5] = '\0';

    return buffer;
}

//+++++++++++++++++ Main Function +++++++++++++++

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    BCSCTL1 = CALBC1_16MHZ;
    DCOCTL = CALDCO_16MHZ;

    P1DIR |= BIT3;
    P1OUT |= BIT3;

    uartInit();
    adcInit();
    timerInit();



    __enable_interrupt();

    while(1)
    {
        __bis_SR_register(CPUOFF + GIE);
        if(adcSampleFlag)
        {
            vADC = (adcValue * Vref) / 1023.0f;
            vINPUT = vADC * ((RESISTOR_DIV_R1 + RESISTOR_DIV_R2) / (float)RESISTOR_DIV_R2);

            char buffer[10];
            uartSendString("ADC: ");
            //uintToStr(adcValue, buffer);
            ftoa_2_2(buffer, vINPUT);
            uartSendString(buffer);
            uartSendString("\n\r");
            adcSampleFlag = 0;

        }

    }

    return 0;
}


#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{

    ADC10CTL0 |= ENC + ADC10SC;
}


#pragma vector= ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
    adcValue = ADC10MEM;
    adcSampleFlag = 1;
    __bic_SR_register_on_exit(CPUOFF);

}

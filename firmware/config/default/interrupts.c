/*******************************************************************************
 System Interrupts File

  Company:
    Microchip Technology Inc.

  File Name:
    interrupt.c

  Summary:
    Interrupt vectors mapping

  Description:
    This file maps all the interrupt vectors to their corresponding
    implementations. If a particular module interrupt is used, then its ISR
    definition can be found in corresponding PLIB source file. If a module
    interrupt is not used, then its ISR implementation is mapped to dummy
    handler.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include "configuration.h"
#include "interrupts.h"
#include "definitions.h"



// *****************************************************************************
// *****************************************************************************
// Section: System Interrupt Vector Functions
// *****************************************************************************
// *****************************************************************************


/* All the handlers are defined here.  Each will call its PLIB-specific function. */
// *****************************************************************************
// *****************************************************************************
// Section: System Interrupt Vector declarations
// *****************************************************************************
// *****************************************************************************
void CORE_TIMER_Handler (void);
void UART1_FAULT_Handler (void);
void UART1_RX_Handler (void);
void UART1_TX_Handler (void);
void I2C1_BUS_Handler (void);
void I2C1_MASTER_Handler (void);
void USB_Handler (void);
void USB_DMA_Handler (void);
void DMA0_Handler (void);
void DMA1_Handler (void);
void DMA2_Handler (void);
void DMA3_Handler (void);
void SPI2_RX_Handler (void);
void SPI2_TX_Handler (void);
void UART2_FAULT_Handler (void);
void UART2_RX_Handler (void);
void UART2_TX_Handler (void);
void I2C2_BUS_Handler (void);
void I2C2_MASTER_Handler (void);
void ETHERNET_Handler (void);
void SPI3_RX_Handler (void);
void SPI3_TX_Handler (void);
void FLASH_CONTROL_Handler (void);
void SQI1_Handler (void);
void UART4_FAULT_Handler (void);
void UART4_RX_Handler (void);
void UART4_TX_Handler (void);
void UART5_FAULT_Handler (void);
void UART5_RX_Handler (void);
void UART5_TX_Handler (void);
void I2C5_BUS_Handler (void);
void I2C5_MASTER_Handler (void);


// *****************************************************************************
// *****************************************************************************
// Section: System Interrupt Vector definitions
// *****************************************************************************
// *****************************************************************************
void __ISR(_CORE_TIMER_VECTOR, ipl1SRS) CORE_TIMER_Handler (void)
{
    CORE_TIMER_InterruptHandler();
}

void __ISR(_UART1_FAULT_VECTOR, ipl1SRS) UART1_FAULT_Handler (void)
{
    UART1_FAULT_InterruptHandler();
}

void __ISR(_UART1_RX_VECTOR, ipl1SRS) UART1_RX_Handler (void)
{
    UART1_RX_InterruptHandler();
}

void __ISR(_UART1_TX_VECTOR, ipl1SRS) UART1_TX_Handler (void)
{
    UART1_TX_InterruptHandler();
}

void __ISR(_I2C1_BUS_VECTOR, ipl1SRS) I2C1_BUS_Handler (void)
{
    I2C1_BUS_InterruptHandler();
}

void __ISR(_I2C1_MASTER_VECTOR, ipl1SRS) I2C1_MASTER_Handler (void)
{
    I2C1_MASTER_InterruptHandler();
}

void __ISR(_USB_VECTOR, ipl1SRS) USB_Handler (void)
{
    DRV_USBHS_InterruptHandler();
}

void __ISR(_USB_DMA_VECTOR, ipl1SRS) USB_DMA_Handler (void)
{
    DRV_USBHS_DMAInterruptHandler();
}

void __ISR(_DMA0_VECTOR, ipl1SRS) DMA0_Handler (void)
{
    DMA0_InterruptHandler();
}

void __ISR(_DMA1_VECTOR, ipl1SRS) DMA1_Handler (void)
{
    DMA1_InterruptHandler();
}

void __ISR(_DMA2_VECTOR, ipl1SRS) DMA2_Handler (void)
{
    DMA2_InterruptHandler();
}

void __ISR(_DMA3_VECTOR, ipl1SRS) DMA3_Handler (void)
{
    DMA3_InterruptHandler();
}

void __ISR(_SPI2_RX_VECTOR, ipl1SRS) SPI2_RX_Handler (void)
{
    SPI2_RX_InterruptHandler();
}

void __ISR(_SPI2_TX_VECTOR, ipl1SRS) SPI2_TX_Handler (void)
{
    SPI2_TX_InterruptHandler();
}

void __ISR(_UART2_FAULT_VECTOR, ipl1SRS) UART2_FAULT_Handler (void)
{
    UART2_FAULT_InterruptHandler();
}

void __ISR(_UART2_RX_VECTOR, ipl1SRS) UART2_RX_Handler (void)
{
    UART2_RX_InterruptHandler();
}

void __ISR(_UART2_TX_VECTOR, ipl1SRS) UART2_TX_Handler (void)
{
    UART2_TX_InterruptHandler();
}

void __ISR(_I2C2_BUS_VECTOR, ipl1SRS) I2C2_BUS_Handler (void)
{
    I2C2_BUS_InterruptHandler();
}

void __ISR(_I2C2_MASTER_VECTOR, ipl1SRS) I2C2_MASTER_Handler (void)
{
    I2C2_MASTER_InterruptHandler();
}

void __ISR(_ETHERNET_VECTOR, ipl1SRS) ETHERNET_Handler (void)
{
    ETHERNET_InterruptHandler();
}

void __ISR(_SPI3_RX_VECTOR, ipl1SRS) SPI3_RX_Handler (void)
{
    SPI3_RX_InterruptHandler();
}

void __ISR(_SPI3_TX_VECTOR, ipl1SRS) SPI3_TX_Handler (void)
{
    SPI3_TX_InterruptHandler();
}

void __ISR(_FLASH_CONTROL_VECTOR, ipl1SRS) FLASH_CONTROL_Handler (void)
{
    NVM_InterruptHandler();
}

void __ISR(_SQI1_VECTOR, ipl1SRS) SQI1_Handler (void)
{
    SQI1_InterruptHandler();
}

void __ISR(_UART4_FAULT_VECTOR, ipl1SRS) UART4_FAULT_Handler (void)
{
    UART4_FAULT_InterruptHandler();
}

void __ISR(_UART4_RX_VECTOR, ipl1SRS) UART4_RX_Handler (void)
{
    UART4_RX_InterruptHandler();
}

void __ISR(_UART4_TX_VECTOR, ipl1SRS) UART4_TX_Handler (void)
{
    UART4_TX_InterruptHandler();
}

void __ISR(_UART5_FAULT_VECTOR, ipl1SRS) UART5_FAULT_Handler (void)
{
    UART5_FAULT_InterruptHandler();
}

void __ISR(_UART5_RX_VECTOR, ipl1SRS) UART5_RX_Handler (void)
{
    UART5_RX_InterruptHandler();
}

void __ISR(_UART5_TX_VECTOR, ipl1SRS) UART5_TX_Handler (void)
{
    UART5_TX_InterruptHandler();
}

void __ISR(_I2C5_BUS_VECTOR, ipl1SRS) I2C5_BUS_Handler (void)
{
    I2C5_BUS_InterruptHandler();
}

void __ISR(_I2C5_MASTER_VECTOR, ipl1SRS) I2C5_MASTER_Handler (void)
{
    I2C5_MASTER_InterruptHandler();
}





/*******************************************************************************
 End of File
*/

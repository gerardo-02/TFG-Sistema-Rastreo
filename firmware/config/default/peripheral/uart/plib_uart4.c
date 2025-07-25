/*******************************************************************************
  UART4 PLIB

  Company:
    Microchip Technology Inc.

  File Name:
    plib_uart4.c

  Summary:
    UART4 PLIB Implementation File

  Description:
    None

*******************************************************************************/

/*******************************************************************************
* Copyright (C) 2019 Microchip Technology Inc. and its subsidiaries.
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

#include "device.h"
#include "plib_uart4.h"
#include "interrupts.h"

// *****************************************************************************
// *****************************************************************************
// Section: UART4 Implementation
// *****************************************************************************
// *****************************************************************************

volatile static UART_OBJECT uart4Obj;

void static UART4_ErrorClear( void )
{
    UART_ERROR errors = UART_ERROR_NONE;
    uint8_t dummyData = 0u;

    errors = (UART_ERROR)(U4STA & (_U4STA_OERR_MASK | _U4STA_FERR_MASK | _U4STA_PERR_MASK));

    if(errors != UART_ERROR_NONE)
    {
        /* If it's a overrun error then clear it to flush FIFO */
        if((U4STA & _U4STA_OERR_MASK) != 0U)
        {
            U4STACLR = _U4STA_OERR_MASK;
        }

        /* Read existing error bytes from FIFO to clear parity and framing error flags */
        while((U4STA & _U4STA_URXDA_MASK) != 0U)
        {
            dummyData = (uint8_t)U4RXREG;
        }

        /* Clear error interrupt flag */
        IFS5CLR = _IFS5_U4EIF_MASK;

        /* Clear up the receive interrupt flag so that RX interrupt is not
         * triggered for error bytes */
        IFS5CLR = _IFS5_U4RXIF_MASK;
    }

    // Ignore the warning
    (void)dummyData;
}

void UART4_Initialize( void )
{
    /* Set up UxMODE bits */
    /* STSEL  = 0 */
    /* PDSEL = 0 */
    /* UEN = 0 */

    U4MODE = 0x8;

    /* Enable UART4 Receiver and Transmitter */
    U4STASET = (_U4STA_UTXEN_MASK | _U4STA_URXEN_MASK | _U4STA_UTXISEL1_MASK );

    /* BAUD Rate register Setup */
    U4BRG = 216;

    /* Disable Interrupts */
    IEC5CLR = _IEC5_U4EIE_MASK;

    IEC5CLR = _IEC5_U4RXIE_MASK;

    IEC5CLR = _IEC5_U4TXIE_MASK;

    /* Initialize instance object */
    uart4Obj.rxBuffer = NULL;
    uart4Obj.rxSize = 0;
    uart4Obj.rxProcessedSize = 0;
    uart4Obj.rxBusyStatus = false;
    uart4Obj.rxCallback = NULL;
    uart4Obj.txBuffer = NULL;
    uart4Obj.txSize = 0;
    uart4Obj.txProcessedSize = 0;
    uart4Obj.txBusyStatus = false;
    uart4Obj.txCallback = NULL;
    uart4Obj.errors = UART_ERROR_NONE;

    /* Turn ON UART4 */
    U4MODESET = _U4MODE_ON_MASK;
}

bool UART4_SerialSetup( UART_SERIAL_SETUP *setup, uint32_t srcClkFreq )
{
    bool status = false;
    uint32_t baud;
    uint32_t status_ctrl;
    uint32_t uxbrg = 0;

    if(uart4Obj.rxBusyStatus == true)
    {
        /* Transaction is in progress, so return without updating settings */
        return status;
    }
    if (uart4Obj.txBusyStatus == true)
    {
        /* Transaction is in progress, so return without updating settings */
        return status;
    }

    if (setup != NULL)
    {
        baud = setup->baudRate;

        if ((baud == 0U) || ((setup->dataWidth == UART_DATA_9_BIT) && (setup->parity != UART_PARITY_NONE)))
        {
            return status;
        }

        if(srcClkFreq == 0U)
        {
            srcClkFreq = UART4_FrequencyGet();
        }

        /* Calculate BRG value */
        uxbrg = (((srcClkFreq >> 2) + (baud >> 1)) / baud);
        /* Check if the baud value can be set with low baud settings */
        if (uxbrg < 1U)
        {
            return status;
        }

        uxbrg -= 1U;

        if (uxbrg > UINT16_MAX)
        {
            return status;
        }

        /* Turn OFF UART4. Save UTXEN, URXEN and UTXBRK bits as these are cleared upon disabling UART */

        status_ctrl = U4STA & (_U4STA_UTXEN_MASK | _U4STA_URXEN_MASK | _U4STA_UTXBRK_MASK);

        U4MODECLR = _U4MODE_ON_MASK;

        if(setup->dataWidth == UART_DATA_9_BIT)
        {
            /* Configure UART4 mode */
            U4MODE = (U4MODE & (~_U4MODE_PDSEL_MASK)) | setup->dataWidth;
        }
        else
        {
            /* Configure UART4 mode */
            U4MODE = (U4MODE & (~_U4MODE_PDSEL_MASK)) | setup->parity;
        }

        /* Configure UART4 mode */
        U4MODE = (U4MODE & (~_U4MODE_STSEL_MASK)) | setup->stopBits;

        /* Configure UART4 Baud Rate */
        U4BRG = uxbrg;

        U4MODESET = _U4MODE_ON_MASK;

        /* Restore UTXEN, URXEN and UTXBRK bits. */
        U4STASET = status_ctrl;

        status = true;
    }

    return status;
}

bool UART4_AutoBaudQuery( void )
{
    bool autobaudqcheck = false;
    if((U4MODE & _U4MODE_ABAUD_MASK) != 0U)
    {

       autobaudqcheck = true;
    }
    return autobaudqcheck;
}

void UART4_AutoBaudSet( bool enable )
{
    if( enable == true )
    {
        U4MODESET = _U4MODE_ABAUD_MASK;
    }

    /* Turning off ABAUD if it was on can lead to unpredictable behavior, so that
       direction of control is not allowed in this function.                      */
}

bool UART4_Read(void* buffer, const size_t size )
{
    bool status = false;

    if(buffer != NULL)
    {
        /* Check if receive request is in progress */
        if(uart4Obj.rxBusyStatus == false)
        {
            /* Clear error flags and flush out error data that may have been received when no active request was pending */
            UART4_ErrorClear();

            uart4Obj.rxBuffer = buffer;
            uart4Obj.rxSize = size;
            uart4Obj.rxProcessedSize = 0;
            uart4Obj.rxBusyStatus = true;
            uart4Obj.errors = UART_ERROR_NONE;
            status = true;

            /* Enable UART4_FAULT Interrupt */
            IEC5SET = _IEC5_U4EIE_MASK;

            /* Enable UART4_RX Interrupt */
            IEC5SET = _IEC5_U4RXIE_MASK;
        }
    }

    return status;
}

bool UART4_Write( void* buffer, const size_t size )
{
    bool status = false;

    if(buffer != NULL)
    {
        /* Check if transmit request is in progress */
        if(uart4Obj.txBusyStatus == false)
        {
            uart4Obj.txBuffer = buffer;
            uart4Obj.txSize = size;
            uart4Obj.txProcessedSize = 0;
            uart4Obj.txBusyStatus = true;
            status = true;

            size_t txProcessedSize = uart4Obj.txProcessedSize;
            size_t txSize = uart4Obj.txSize;

            /* Initiate the transfer by writing as many bytes as we can */
            while(((U4STA & _U4STA_UTXBF_MASK) == 0U) && (txSize > txProcessedSize) )
            {
                if (( U4MODE & (_U4MODE_PDSEL0_MASK | _U4MODE_PDSEL1_MASK)) == (_U4MODE_PDSEL0_MASK | _U4MODE_PDSEL1_MASK))
                {
                    /* 9-bit mode */
                    U4TXREG = ((uint16_t*)uart4Obj.txBuffer)[txProcessedSize];
                    txProcessedSize++;
                }
                else
                {
                    /* 8-bit mode */
                    U4TXREG = ((uint8_t*)uart4Obj.txBuffer)[txProcessedSize];
                    txProcessedSize++;
                }
            }

            uart4Obj.txProcessedSize = txProcessedSize;

            IEC5SET = _IEC5_U4TXIE_MASK;
        }
    }

    return status;
}

UART_ERROR UART4_ErrorGet( void )
{
    UART_ERROR errors = uart4Obj.errors;

    uart4Obj.errors = UART_ERROR_NONE;

    /* All errors are cleared, but send the previous error state */
    return errors;
}

void UART4_ReadCallbackRegister( UART_CALLBACK callback, uintptr_t context )
{
    uart4Obj.rxCallback = callback;

    uart4Obj.rxContext = context;
}

bool UART4_ReadIsBusy( void )
{
    return uart4Obj.rxBusyStatus;
}

size_t UART4_ReadCountGet( void )
{
    return uart4Obj.rxProcessedSize;
}

bool UART4_ReadAbort(void)
{
    if (uart4Obj.rxBusyStatus == true)
    {
        /* Disable the fault interrupt */
        IEC5CLR = _IEC5_U4EIE_MASK;

        /* Disable the receive interrupt */
        IEC5CLR = _IEC5_U4RXIE_MASK;

        uart4Obj.rxBusyStatus = false;

        /* If required application should read the num bytes processed prior to calling the read abort API */
        uart4Obj.rxSize = 0U;
        uart4Obj.rxProcessedSize = 0U;
    }

    return true;
}

void UART4_WriteCallbackRegister( UART_CALLBACK callback, uintptr_t context )
{
    uart4Obj.txCallback = callback;

    uart4Obj.txContext = context;
}

bool UART4_WriteIsBusy( void )
{
    return uart4Obj.txBusyStatus;
}

size_t UART4_WriteCountGet( void )
{
    return uart4Obj.txProcessedSize;
}

void __attribute__((used)) UART4_FAULT_InterruptHandler (void)
{
    /* Save the error to be reported later */
    uart4Obj.errors = (U4STA & (_U4STA_OERR_MASK | _U4STA_FERR_MASK | _U4STA_PERR_MASK));

    /* Disable the fault interrupt */
    IEC5CLR = _IEC5_U4EIE_MASK;

    /* Disable the receive interrupt */
    IEC5CLR = _IEC5_U4RXIE_MASK;

    /* Clear rx status */
    uart4Obj.rxBusyStatus = false;

    UART4_ErrorClear();

    /* Client must call UARTx_ErrorGet() function to get the errors */
    if( uart4Obj.rxCallback != NULL )
    {
        uintptr_t rxContext = uart4Obj.rxContext;

        uart4Obj.rxCallback(rxContext);
    }
}

void __attribute__((used)) UART4_RX_InterruptHandler (void)
{
    if(uart4Obj.rxBusyStatus == true)
    {
        size_t rxSize = uart4Obj.rxSize;
        size_t rxProcessedSize = uart4Obj.rxProcessedSize;

        while((_U4STA_URXDA_MASK == (U4STA & _U4STA_URXDA_MASK)) && (rxSize > rxProcessedSize) )
        {
            if (( U4MODE & (_U4MODE_PDSEL0_MASK | _U4MODE_PDSEL1_MASK)) == (_U4MODE_PDSEL0_MASK | _U4MODE_PDSEL1_MASK))
            {
                /* 9-bit mode */
                ((uint16_t*)uart4Obj.rxBuffer)[rxProcessedSize] = (uint16_t)(U4RXREG);
            }
            else
            {
                /* 8-bit mode */
                ((uint8_t*)uart4Obj.rxBuffer)[rxProcessedSize] = (uint8_t)(U4RXREG);
            }
            rxProcessedSize++;
        }

        uart4Obj.rxProcessedSize = rxProcessedSize;

        /* Clear UART4 RX Interrupt flag */
        IFS5CLR = _IFS5_U4RXIF_MASK;

        /* Check if the buffer is done */
        if(uart4Obj.rxProcessedSize >= rxSize)
        {
            uart4Obj.rxBusyStatus = false;

            /* Disable the fault interrupt */
            IEC5CLR = _IEC5_U4EIE_MASK;

            /* Disable the receive interrupt */
            IEC5CLR = _IEC5_U4RXIE_MASK;


            if(uart4Obj.rxCallback != NULL)
            {
                uintptr_t rxContext = uart4Obj.rxContext;

                uart4Obj.rxCallback(rxContext);
            }
        }
    }
    else
    {
        /* Nothing to process */
    }
}

void __attribute__((used)) UART4_TX_InterruptHandler (void)
{
    if(uart4Obj.txBusyStatus == true)
    {
        size_t txSize = uart4Obj.txSize;
        size_t txProcessedSize = uart4Obj.txProcessedSize;

        while(((U4STA & _U4STA_UTXBF_MASK) == 0U) && (txSize > txProcessedSize) )
        {
            if (( U4MODE & (_U4MODE_PDSEL0_MASK | _U4MODE_PDSEL1_MASK)) == (_U4MODE_PDSEL0_MASK | _U4MODE_PDSEL1_MASK))
            {
                /* 9-bit mode */
                U4TXREG = ((uint16_t*)uart4Obj.txBuffer)[txProcessedSize];
            }
            else
            {
                /* 8-bit mode */
                U4TXREG = ((uint8_t*)uart4Obj.txBuffer)[txProcessedSize];
            }
            txProcessedSize++;
        }

        uart4Obj.txProcessedSize = txProcessedSize;

        /* Clear UART4TX Interrupt flag */
        IFS5CLR = _IFS5_U4TXIF_MASK;

        /* Check if the buffer is done */
        if(uart4Obj.txProcessedSize >= txSize)
        {
            uart4Obj.txBusyStatus = false;

            /* Disable the transmit interrupt, to avoid calling ISR continuously */
            IEC5CLR = _IEC5_U4TXIE_MASK;

            if(uart4Obj.txCallback != NULL)
            {
                uintptr_t txContext = uart4Obj.txContext;

                uart4Obj.txCallback(txContext);
            }
        }
    }
    else
    {
        // Nothing to process
        ;
    }
}



bool UART4_TransmitComplete( void )
{
    bool transmitComplete = false;

    if((U4STA & _U4STA_TRMT_MASK) != 0U)
    {
        transmitComplete = true;
    }

    return transmitComplete;
}
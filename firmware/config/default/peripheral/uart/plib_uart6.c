/*******************************************************************************
  UART6 PLIB

  Company:
    Microchip Technology Inc.

  File Name:
    plib_uart6.c

  Summary:
    UART6 PLIB Implementation File

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
#include "plib_uart6.h"

// *****************************************************************************
// *****************************************************************************
// Section: UART6 Implementation
// *****************************************************************************
// *****************************************************************************

UART_RING_BUFFER_OBJECT uart6Obj;

#define UART6_READ_BUFFER_SIZE      128
#define UART6_READ_BUFFER_SIZE_9BIT (128 >> 1)
#define UART6_RX_INT_DISABLE()      IEC5CLR = _IEC5_U6RXIE_MASK;
#define UART6_RX_INT_ENABLE()       IEC5SET = _IEC5_U6RXIE_MASK;

static uint8_t UART6_ReadBuffer[UART6_READ_BUFFER_SIZE];

#define UART6_WRITE_BUFFER_SIZE     128
#define UART6_WRITE_BUFFER_SIZE_9BIT       (128 >> 1)
#define UART6_TX_INT_DISABLE()      IEC5CLR = _IEC5_U6TXIE_MASK;
#define UART6_TX_INT_ENABLE()       IEC5SET = _IEC5_U6TXIE_MASK;

static uint8_t UART6_WriteBuffer[UART6_WRITE_BUFFER_SIZE];

#define UART6_IS_9BIT_MODE_ENABLED()    ( U6MODE & (_U6MODE_PDSEL0_MASK | _U6MODE_PDSEL1_MASK)) == (_U6MODE_PDSEL0_MASK | _U6MODE_PDSEL1_MASK) ? true:false

void static UART6_ErrorClear( void )
{
    UART_ERROR errors = UART_ERROR_NONE;
    uint8_t dummyData = 0u;

    errors = (UART_ERROR)(U6STA & (_U6STA_OERR_MASK | _U6STA_FERR_MASK | _U6STA_PERR_MASK));

    if(errors != UART_ERROR_NONE)
    {
        /* If it's a overrun error then clear it to flush FIFO */
        if(U6STA & _U6STA_OERR_MASK)
        {
            U6STACLR = _U6STA_OERR_MASK;
        }

        /* Read existing error bytes from FIFO to clear parity and framing error flags */
        while(U6STA & _U6STA_URXDA_MASK)
        {
            dummyData = U6RXREG;
        }

        /* Clear error interrupt flag */
        IFS5CLR = _IFS5_U6EIF_MASK;

        /* Clear up the receive interrupt flag so that RX interrupt is not
         * triggered for error bytes */
        IFS5CLR = _IFS5_U6RXIF_MASK;

    }

    // Ignore the warning
    (void)dummyData;
}

void UART6_Initialize( void )
{
    /* Set up UxMODE bits */
    /* STSEL  = 0 */
    /* PDSEL = 0 */

    U6MODE = 0x8;

    /* Enable UART6 Receiver and Transmitter */
    U6STASET = (_U6STA_UTXEN_MASK | _U6STA_URXEN_MASK | _U6STA_UTXISEL1_MASK );

    /* BAUD Rate register Setup */
    U6BRG = 214;

    /* Disable Interrupts */
    IEC5CLR = _IEC5_U6EIE_MASK;

    IEC5CLR = _IEC5_U6RXIE_MASK;

    IEC5CLR = _IEC5_U6TXIE_MASK;

    /* Initialize instance object */
    uart6Obj.rdCallback = NULL;
    uart6Obj.rdInIndex = 0;
    uart6Obj.rdOutIndex = 0;
    uart6Obj.isRdNotificationEnabled = false;
    uart6Obj.isRdNotifyPersistently = false;
    uart6Obj.rdThreshold = 0;

    uart6Obj.wrCallback = NULL;
    uart6Obj.wrInIndex = 0;
    uart6Obj.wrOutIndex = 0;
    uart6Obj.isWrNotificationEnabled = false;
    uart6Obj.isWrNotifyPersistently = false;
    uart6Obj.wrThreshold = 0;

    uart6Obj.errors = UART_ERROR_NONE;

    if (UART6_IS_9BIT_MODE_ENABLED())
    {
        uart6Obj.rdBufferSize = UART6_READ_BUFFER_SIZE_9BIT;
        uart6Obj.wrBufferSize = UART6_WRITE_BUFFER_SIZE_9BIT;
    }
    else
    {
        uart6Obj.rdBufferSize = UART6_READ_BUFFER_SIZE;
        uart6Obj.wrBufferSize = UART6_WRITE_BUFFER_SIZE;
    }


    /* Turn ON UART6 */
    U6MODESET = _U6MODE_ON_MASK;

    /* Enable UART6_FAULT Interrupt */
    IEC5SET = _IEC5_U6EIE_MASK;

    /* Enable UART6_RX Interrupt */
    IEC5SET = _IEC5_U6RXIE_MASK;
}

bool UART6_SerialSetup( UART_SERIAL_SETUP *setup, uint32_t srcClkFreq )
{
    bool status = false;
    uint32_t baud;
    int32_t brgValHigh = 0;
    int32_t brgValLow = 0;
    uint32_t brgVal = 0;
    uint32_t uartMode;

    if (setup != NULL)
    {
        baud = setup->baudRate;

        if (baud == 0)
        {
            return status;
        }

        if(srcClkFreq == 0)
        {
            srcClkFreq = UART6_FrequencyGet();
        }

        /* Calculate BRG value */
        brgValLow = (((srcClkFreq >> 4) + (baud >> 1)) / baud ) - 1;
        brgValHigh = (((srcClkFreq >> 2) + (baud >> 1)) / baud ) - 1;

        /* Check if the baud value can be set with low baud settings */
        if((brgValLow >= 0) && (brgValLow <= UINT16_MAX))
        {
            brgVal =  brgValLow;
            U6MODECLR = _U6MODE_BRGH_MASK;
        }
        else if ((brgValHigh >= 0) && (brgValHigh <= UINT16_MAX))
        {
            brgVal = brgValHigh;
            U6MODESET = _U6MODE_BRGH_MASK;
        }
        else
        {
            return status;
        }

        if(setup->dataWidth == UART_DATA_9_BIT)
        {
            if(setup->parity != UART_PARITY_NONE)
            {
               return status;
            }
            else
            {
               /* Configure UART6 mode */
               uartMode = U6MODE;
               uartMode &= ~_U6MODE_PDSEL_MASK;
               U6MODE = uartMode | setup->dataWidth;
            }
        }
        else
        {
            /* Configure UART6 mode */
            uartMode = U6MODE;
            uartMode &= ~_U6MODE_PDSEL_MASK;
            U6MODE = uartMode | setup->parity ;
        }

        /* Configure UART6 mode */
        uartMode = U6MODE;
        uartMode &= ~_U6MODE_STSEL_MASK;
        U6MODE = uartMode | setup->stopBits ;

        /* Configure UART6 Baud Rate */
        U6BRG = brgVal;

        if (UART6_IS_9BIT_MODE_ENABLED())
        {
            uart6Obj.rdBufferSize = UART6_READ_BUFFER_SIZE_9BIT;
            uart6Obj.wrBufferSize = UART6_WRITE_BUFFER_SIZE_9BIT;
        }
        else
        {
            uart6Obj.rdBufferSize = UART6_READ_BUFFER_SIZE;
            uart6Obj.wrBufferSize = UART6_WRITE_BUFFER_SIZE;
        }

        status = true;
    }

    return status;
}

/* This routine is only called from ISR. Hence do not disable/enable USART interrupts. */
static inline bool UART6_RxPushByte(uint16_t rdByte)
{
    uint32_t tempInIndex;
    bool isSuccess = false;

    tempInIndex = uart6Obj.rdInIndex + 1;

    if (tempInIndex >= uart6Obj.rdBufferSize)
    {
        tempInIndex = 0;
    }

    if (tempInIndex == uart6Obj.rdOutIndex)
    {
        /* Queue is full - Report it to the application. Application gets a chance to free up space by reading data out from the RX ring buffer */
        if(uart6Obj.rdCallback != NULL)
        {
            uart6Obj.rdCallback(UART_EVENT_READ_BUFFER_FULL, uart6Obj.rdContext);

            /* Read the indices again in case application has freed up space in RX ring buffer */
            tempInIndex = uart6Obj.rdInIndex + 1;

            if (tempInIndex >= uart6Obj.rdBufferSize)
            {
                tempInIndex = 0;
            }
        }
    }

    /* Attempt to push the data into the ring buffer */
    if (tempInIndex != uart6Obj.rdOutIndex)
    {
        if (UART6_IS_9BIT_MODE_ENABLED())
        {
            ((uint16_t*)&UART6_ReadBuffer)[uart6Obj.rdInIndex] = rdByte;
        }
        else
        {
            UART6_ReadBuffer[uart6Obj.rdInIndex] = (uint8_t)rdByte;
        }

        uart6Obj.rdInIndex = tempInIndex;

        isSuccess = true;
    }
    else
    {
        /* Queue is full. Data will be lost. */
    }

    return isSuccess;
}

/* This routine is only called from ISR. Hence do not disable/enable USART interrupts. */
static void UART6_ReadNotificationSend(void)
{
    uint32_t nUnreadBytesAvailable;

    if (uart6Obj.isRdNotificationEnabled == true)
    {
        nUnreadBytesAvailable = UART6_ReadCountGet();

        if(uart6Obj.rdCallback != NULL)
        {
            if (uart6Obj.isRdNotifyPersistently == true)
            {
                if (nUnreadBytesAvailable >= uart6Obj.rdThreshold)
                {
                    uart6Obj.rdCallback(UART_EVENT_READ_THRESHOLD_REACHED, uart6Obj.rdContext);
                }
            }
            else
            {
                if (nUnreadBytesAvailable == uart6Obj.rdThreshold)
                {
                    uart6Obj.rdCallback(UART_EVENT_READ_THRESHOLD_REACHED, uart6Obj.rdContext);
                }
            }
        }
    }
}

size_t UART6_Read(uint8_t* pRdBuffer, const size_t size)
{
    size_t nBytesRead = 0;
    uint32_t rdOutIndex = 0;
    uint32_t rdInIndex = 0;

    /* Take a snapshot of indices to avoid creation of critical section */
    rdOutIndex = uart6Obj.rdOutIndex;
    rdInIndex = uart6Obj.rdInIndex;

    while (nBytesRead < size)
    {
        if (rdOutIndex != rdInIndex)
        {
            if (UART6_IS_9BIT_MODE_ENABLED())
            {
                ((uint16_t*)pRdBuffer)[nBytesRead++] = ((uint16_t*)&UART6_ReadBuffer)[rdOutIndex++];
            }
            else
            {
                pRdBuffer[nBytesRead++] = UART6_ReadBuffer[rdOutIndex++];
            }

            if (rdOutIndex >= uart6Obj.rdBufferSize)
            {
                rdOutIndex = 0;
            }
        }
        else
        {
            /* No more data available in the RX buffer */
            break;
        }
    }

    uart6Obj.rdOutIndex = rdOutIndex;

    return nBytesRead;
}

size_t UART6_ReadCountGet(void)
{
    size_t nUnreadBytesAvailable;
    uint32_t rdInIndex;
    uint32_t rdOutIndex;

    /* Take a snapshot of indices to avoid processing in critical section */
    rdInIndex = uart6Obj.rdInIndex;
    rdOutIndex = uart6Obj.rdOutIndex;

    if ( rdInIndex >=  rdOutIndex)
    {
        nUnreadBytesAvailable =  rdInIndex -  rdOutIndex;
    }
    else
    {
        nUnreadBytesAvailable =  (uart6Obj.rdBufferSize -  rdOutIndex) + rdInIndex;
    }

    return nUnreadBytesAvailable;
}

size_t UART6_ReadFreeBufferCountGet(void)
{
    return (uart6Obj.rdBufferSize - 1) - UART6_ReadCountGet();
}

size_t UART6_ReadBufferSizeGet(void)
{
    return (uart6Obj.rdBufferSize - 1);
}

bool UART6_ReadNotificationEnable(bool isEnabled, bool isPersistent)
{
    bool previousStatus = uart6Obj.isRdNotificationEnabled;

    uart6Obj.isRdNotificationEnabled = isEnabled;

    uart6Obj.isRdNotifyPersistently = isPersistent;

    return previousStatus;
}

void UART6_ReadThresholdSet(uint32_t nBytesThreshold)
{
    if (nBytesThreshold > 0)
    {
        uart6Obj.rdThreshold = nBytesThreshold;
    }
}

void UART6_ReadCallbackRegister( UART_RING_BUFFER_CALLBACK callback, uintptr_t context)
{
    uart6Obj.rdCallback = callback;

    uart6Obj.rdContext = context;
}

/* This routine is only called from ISR. Hence do not disable/enable USART interrupts. */
static bool UART6_TxPullByte(uint16_t* pWrByte)
{
    bool isSuccess = false;
    uint32_t wrOutIndex = uart6Obj.wrOutIndex;
    uint32_t wrInIndex = uart6Obj.wrInIndex;

    if (wrOutIndex != wrInIndex)
    {
        if (UART6_IS_9BIT_MODE_ENABLED())
        {
            *pWrByte = ((uint16_t*)&UART6_WriteBuffer)[wrOutIndex++];
        }
        else
        {
            *pWrByte = UART6_WriteBuffer[wrOutIndex++];
        }

        if (wrOutIndex >= uart6Obj.wrBufferSize)
        {
            wrOutIndex = 0;
        }

        uart6Obj.wrOutIndex = wrOutIndex;

        isSuccess = true;
    }

    return isSuccess;
}

static inline bool UART6_TxPushByte(uint16_t wrByte)
{
    uint32_t tempInIndex;
    bool isSuccess = false;

    uint32_t wrOutIndex = uart6Obj.wrOutIndex;
    uint32_t wrInIndex = uart6Obj.wrInIndex;

    tempInIndex = wrInIndex + 1;

    if (tempInIndex >= uart6Obj.wrBufferSize)
    {
        tempInIndex = 0;
    }
    if (tempInIndex != wrOutIndex)
    {
        if (UART6_IS_9BIT_MODE_ENABLED())
        {
            ((uint16_t*)&UART6_WriteBuffer)[wrInIndex] = wrByte;
        }
        else
        {
            UART6_WriteBuffer[wrInIndex] = (uint8_t)wrByte;
        }

        uart6Obj.wrInIndex = tempInIndex;

        isSuccess = true;
    }
    else
    {
        /* Queue is full. Report Error. */
    }

    return isSuccess;
}

/* This routine is only called from ISR. Hence do not disable/enable USART interrupts. */
static void UART6_WriteNotificationSend(void)
{
    uint32_t nFreeWrBufferCount;

    if (uart6Obj.isWrNotificationEnabled == true)
    {
        nFreeWrBufferCount = UART6_WriteFreeBufferCountGet();

        if(uart6Obj.wrCallback != NULL)
        {
            if (uart6Obj.isWrNotifyPersistently == true)
            {
                if (nFreeWrBufferCount >= uart6Obj.wrThreshold)
                {
                    uart6Obj.wrCallback(UART_EVENT_WRITE_THRESHOLD_REACHED, uart6Obj.wrContext);
                }
            }
            else
            {
                if (nFreeWrBufferCount == uart6Obj.wrThreshold)
                {
                    uart6Obj.wrCallback(UART_EVENT_WRITE_THRESHOLD_REACHED, uart6Obj.wrContext);
                }
            }
        }
    }
}

static size_t UART6_WritePendingBytesGet(void)
{
    size_t nPendingTxBytes;

    /* Take a snapshot of indices to avoid processing in critical section */

    uint32_t wrOutIndex = uart6Obj.wrOutIndex;
    uint32_t wrInIndex = uart6Obj.wrInIndex;

    if ( wrInIndex >=  wrOutIndex)
    {
        nPendingTxBytes =  wrInIndex - wrOutIndex;
    }
    else
    {
        nPendingTxBytes =  (uart6Obj.wrBufferSize -  wrOutIndex) + wrInIndex;
    }

    return nPendingTxBytes;
}

size_t UART6_WriteCountGet(void)
{
    size_t nPendingTxBytes;

    nPendingTxBytes = UART6_WritePendingBytesGet();

    return nPendingTxBytes;
}

size_t UART6_Write(uint8_t* pWrBuffer, const size_t size )
{
    size_t nBytesWritten  = 0;

    while (nBytesWritten < size)
    {
        if (UART6_IS_9BIT_MODE_ENABLED())
        {
            if (UART6_TxPushByte(((uint16_t*)pWrBuffer)[nBytesWritten]) == true)
            {
                nBytesWritten++;
            }
            else
            {
                /* Queue is full, exit the loop */
                break;
            }
        }
        else
        {
            if (UART6_TxPushByte(pWrBuffer[nBytesWritten]) == true)
            {
                nBytesWritten++;
            }
            else
            {
                /* Queue is full, exit the loop */
                break;
            }
        }

    }

    /* Check if any data is pending for transmission */
    if (UART6_WritePendingBytesGet() > 0)
    {
        /* Enable TX interrupt as data is pending for transmission */
        UART6_TX_INT_ENABLE();
    }

    return nBytesWritten;
}

size_t UART6_WriteFreeBufferCountGet(void)
{
    return (uart6Obj.wrBufferSize - 1) - UART6_WriteCountGet();
}

size_t UART6_WriteBufferSizeGet(void)
{
    return (uart6Obj.wrBufferSize - 1);
}

bool UART6_WriteNotificationEnable(bool isEnabled, bool isPersistent)
{
    bool previousStatus = uart6Obj.isWrNotificationEnabled;

    uart6Obj.isWrNotificationEnabled = isEnabled;

    uart6Obj.isWrNotifyPersistently = isPersistent;

    return previousStatus;
}

void UART6_WriteThresholdSet(uint32_t nBytesThreshold)
{
    if (nBytesThreshold > 0)
    {
        uart6Obj.wrThreshold = nBytesThreshold;
    }
}

void UART6_WriteCallbackRegister( UART_RING_BUFFER_CALLBACK callback, uintptr_t context)
{
    uart6Obj.wrCallback = callback;

    uart6Obj.wrContext = context;
}

UART_ERROR UART6_ErrorGet( void )
{
    UART_ERROR errors = uart6Obj.errors;

    uart6Obj.errors = UART_ERROR_NONE;

    /* All errors are cleared, but send the previous error state */
    return errors;
}

bool UART6_AutoBaudQuery( void )
{
    if(U6MODE & _U6MODE_ABAUD_MASK)
        return true;
    else
        return false;
}

void UART6_AutoBaudSet( bool enable )
{
    if( enable == true )
    {
        U6MODESET = _U6MODE_ABAUD_MASK;
    }

    /* Turning off ABAUD if it was on can lead to unpredictable behavior, so that
       direction of control is not allowed in this function.                      */
}

void UART6_FAULT_InterruptHandler (void)
{
    /* Save the error to be reported later */
    uart6Obj.errors = (UART_ERROR)(U6STA & (_U6STA_OERR_MASK | _U6STA_FERR_MASK | _U6STA_PERR_MASK));

    UART6_ErrorClear();

    /* Client must call UARTx_ErrorGet() function to clear the errors */
    if( uart6Obj.rdCallback != NULL )
    {
        uart6Obj.rdCallback(UART_EVENT_READ_ERROR, uart6Obj.rdContext);
    }
}

void UART6_RX_InterruptHandler (void)
{
    /* Keep reading until there is a character availabe in the RX FIFO */
    while((U6STA & _U6STA_URXDA_MASK) == _U6STA_URXDA_MASK)
    {
        if (UART6_RxPushByte( (uint16_t )(U6RXREG) ) == true)
        {
            UART6_ReadNotificationSend();
        }
        else
        {
            /* UART RX buffer is full */
        }
    }

    /* Clear UART6 RX Interrupt flag */
    IFS5CLR = _IFS5_U6RXIF_MASK;
}

void UART6_TX_InterruptHandler (void)
{
    uint16_t wrByte;

    /* Check if any data is pending for transmission */
    if (UART6_WritePendingBytesGet() > 0)
    {
        /* Keep writing to the TX FIFO as long as there is space */
        while(!(U6STA & _U6STA_UTXBF_MASK))
        {
            if (UART6_TxPullByte(&wrByte) == true)
            {
                if (UART6_IS_9BIT_MODE_ENABLED())
                {
                    U6TXREG = wrByte;
                }
                else
                {
                    U6TXREG = (uint8_t)wrByte;
                }

                /* Send notification */
                UART6_WriteNotificationSend();
            }
            else
            {
                /* Nothing to transmit. Disable the data register empty interrupt. */
                UART6_TX_INT_DISABLE();
                break;
            }
        }

        /* Clear UART6TX Interrupt flag */
        IFS5CLR = _IFS5_U6TXIF_MASK;
    }
    else
    {
        /* Nothing to transmit. Disable the data register empty interrupt. */
        UART6_TX_INT_DISABLE();

        /* Clear UART6TX Interrupt flag */
        IFS5CLR = _IFS5_U6TXIF_MASK;
    }
}


/*******************************************************************************
  GPIO PLIB

  Company:
    Microchip Technology Inc.

  File Name:
    plib_gpio.h UUUUUUUUU

  Summary:
    GPIO PLIB Header File

  Description:
    This library provides an interface to control and interact with Parallel
    Input/Output controller (GPIO) module.

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

#ifndef PLIB_GPIO_H
#define PLIB_GPIO_H

#include <device.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

    extern "C" {

#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Data types and constants
// *****************************************************************************
// *****************************************************************************


/*** Macros for EXPAN_IO_08 pin ***/
#define EXPAN_IO_08_Set()               (LATESET = (1U<<5))
#define EXPAN_IO_08_Clear()             (LATECLR = (1U<<5))
#define EXPAN_IO_08_Toggle()            (LATEINV= (1U<<5))
#define EXPAN_IO_08_OutputEnable()      (TRISECLR = (1U<<5))
#define EXPAN_IO_08_InputEnable()       (TRISESET = (1U<<5))
#define EXPAN_IO_08_Get()               ((PORTE >> 5) & 0x1U)
#define EXPAN_IO_08_PIN                  GPIO_PIN_RE5

/*** Macros for SD_CD pin ***/
#define SD_CD_Set()               (LATESET = (1U<<6))
#define SD_CD_Clear()             (LATECLR = (1U<<6))
#define SD_CD_Toggle()            (LATEINV= (1U<<6))
#define SD_CD_OutputEnable()      (TRISECLR = (1U<<6))
#define SD_CD_InputEnable()       (TRISESET = (1U<<6))
#define SD_CD_Get()               ((PORTE >> 6) & 0x1U)
#define SD_CD_PIN                  GPIO_PIN_RE6

/*** Macros for SD_CS pin ***/
#define SD_CS_Set()               (LATESET = (1U<<7))
#define SD_CS_Clear()             (LATECLR = (1U<<7))
#define SD_CS_Toggle()            (LATEINV= (1U<<7))
#define SD_CS_OutputEnable()      (TRISECLR = (1U<<7))
#define SD_CS_InputEnable()       (TRISESET = (1U<<7))
#define SD_CS_Get()               ((PORTE >> 7) & 0x1U)
#define SD_CS_PIN                  GPIO_PIN_RE7

/*** Macros for SD_MOSI pin ***/
#define SD_MOSI_Get()               ((PORTC >> 1) & 0x1U)
#define SD_MOSI_PIN                  GPIO_PIN_RC1

/*** Macros for ETH1_TXD0 pin ***/
#define ETH1_TXD0_Get()               ((PORTJ >> 8) & 0x1U)
#define ETH1_TXD0_PIN                  GPIO_PIN_RJ8

/*** Macros for ETH1_TXD1 pin ***/
#define ETH1_TXD1_Get()               ((PORTJ >> 9) & 0x1U)
#define ETH1_TXD1_PIN                  GPIO_PIN_RJ9

/*** Macros for SIMCOM_RST pin ***/
#define SIMCOM_RST_Set()               (LATJSET = (1U<<12))
#define SIMCOM_RST_Clear()             (LATJCLR = (1U<<12))
#define SIMCOM_RST_Toggle()            (LATJINV= (1U<<12))
#define SIMCOM_RST_OutputEnable()      (TRISJCLR = (1U<<12))
#define SIMCOM_RST_InputEnable()       (TRISJSET = (1U<<12))
#define SIMCOM_RST_Get()               ((PORTJ >> 12) & 0x1U)
#define SIMCOM_RST_PIN                  GPIO_PIN_RJ12

/*** Macros for SIMCOM_STATUS pin ***/
#define SIMCOM_STATUS_Set()               (LATJSET = (1U<<10))
#define SIMCOM_STATUS_Clear()             (LATJCLR = (1U<<10))
#define SIMCOM_STATUS_Toggle()            (LATJINV= (1U<<10))
#define SIMCOM_STATUS_OutputEnable()      (TRISJCLR = (1U<<10))
#define SIMCOM_STATUS_InputEnable()       (TRISJSET = (1U<<10))
#define SIMCOM_STATUS_Get()               ((PORTJ >> 10) & 0x1U)
#define SIMCOM_STATUS_PIN                  GPIO_PIN_RJ10

/*** Macros for EXPAN_IO_09 pin ***/
#define EXPAN_IO_09_Set()               (LATCSET = (1U<<2))
#define EXPAN_IO_09_Clear()             (LATCCLR = (1U<<2))
#define EXPAN_IO_09_Toggle()            (LATCINV= (1U<<2))
#define EXPAN_IO_09_OutputEnable()      (TRISCCLR = (1U<<2))
#define EXPAN_IO_09_InputEnable()       (TRISCSET = (1U<<2))
#define EXPAN_IO_09_Get()               ((PORTC >> 2) & 0x1U)
#define EXPAN_IO_09_PIN                  GPIO_PIN_RC2

/*** Macros for SIMCOM_NETLIGHT pin ***/
#define SIMCOM_NETLIGHT_Set()               (LATCSET = (1U<<3))
#define SIMCOM_NETLIGHT_Clear()             (LATCCLR = (1U<<3))
#define SIMCOM_NETLIGHT_Toggle()            (LATCINV= (1U<<3))
#define SIMCOM_NETLIGHT_OutputEnable()      (TRISCCLR = (1U<<3))
#define SIMCOM_NETLIGHT_InputEnable()       (TRISCSET = (1U<<3))
#define SIMCOM_NETLIGHT_Get()               ((PORTC >> 3) & 0x1U)
#define SIMCOM_NETLIGHT_PIN                  GPIO_PIN_RC3

/*** Macros for SD_MISO pin ***/
#define SD_MISO_Get()               ((PORTC >> 4) & 0x1U)
#define SD_MISO_PIN                  GPIO_PIN_RC4

/*** Macros for SD_SCLK pin ***/
#define SD_SCLK_Get()               ((PORTG >> 6) & 0x1U)
#define SD_SCLK_PIN                  GPIO_PIN_RG6

/*** Macros for EXPAN_IO_17 pin ***/
#define EXPAN_IO_17_Set()               (LATGSET = (1U<<7))
#define EXPAN_IO_17_Clear()             (LATGCLR = (1U<<7))
#define EXPAN_IO_17_Toggle()            (LATGINV= (1U<<7))
#define EXPAN_IO_17_OutputEnable()      (TRISGCLR = (1U<<7))
#define EXPAN_IO_17_InputEnable()       (TRISGSET = (1U<<7))
#define EXPAN_IO_17_Get()               ((PORTG >> 7) & 0x1U)
#define EXPAN_IO_17_PIN                  GPIO_PIN_RG7

/*** Macros for EXPAN_IO_16 pin ***/
#define EXPAN_IO_16_Set()               (LATGSET = (1U<<8))
#define EXPAN_IO_16_Clear()             (LATGCLR = (1U<<8))
#define EXPAN_IO_16_Toggle()            (LATGINV= (1U<<8))
#define EXPAN_IO_16_OutputEnable()      (TRISGCLR = (1U<<8))
#define EXPAN_IO_16_InputEnable()       (TRISGSET = (1U<<8))
#define EXPAN_IO_16_Get()               ((PORTG >> 8) & 0x1U)
#define EXPAN_IO_16_PIN                  GPIO_PIN_RG8

/*** Macros for O_RELAY_1 pin ***/
#define O_RELAY_1_Set()               (LATKSET = (1U<<0))
#define O_RELAY_1_Clear()             (LATKCLR = (1U<<0))
#define O_RELAY_1_Toggle()            (LATKINV= (1U<<0))
#define O_RELAY_1_OutputEnable()      (TRISKCLR = (1U<<0))
#define O_RELAY_1_InputEnable()       (TRISKSET = (1U<<0))
#define O_RELAY_1_Get()               ((PORTK >> 0) & 0x1U)
#define O_RELAY_1_PIN                  GPIO_PIN_RK0

/*** Macros for EXPAN_IO_15 pin ***/
#define EXPAN_IO_15_Set()               (LATGSET = (1U<<9))
#define EXPAN_IO_15_Clear()             (LATGCLR = (1U<<9))
#define EXPAN_IO_15_Toggle()            (LATGINV= (1U<<9))
#define EXPAN_IO_15_OutputEnable()      (TRISGCLR = (1U<<9))
#define EXPAN_IO_15_InputEnable()       (TRISGSET = (1U<<9))
#define EXPAN_IO_15_Get()               ((PORTG >> 9) & 0x1U)
#define EXPAN_IO_15_PIN                  GPIO_PIN_RG9

/*** Macros for EXPAN_IO_14 pin ***/
#define EXPAN_IO_14_Set()               (LATASET = (1U<<0))
#define EXPAN_IO_14_Clear()             (LATACLR = (1U<<0))
#define EXPAN_IO_14_Toggle()            (LATAINV= (1U<<0))
#define EXPAN_IO_14_OutputEnable()      (TRISACLR = (1U<<0))
#define EXPAN_IO_14_InputEnable()       (TRISASET = (1U<<0))
#define EXPAN_IO_14_Get()               ((PORTA >> 0) & 0x1U)
#define EXPAN_IO_14_PIN                  GPIO_PIN_RA0

/*** Macros for EXPAN_IO_13 pin ***/
#define EXPAN_IO_13_Set()               (LATESET = (1U<<8))
#define EXPAN_IO_13_Clear()             (LATECLR = (1U<<8))
#define EXPAN_IO_13_Toggle()            (LATEINV= (1U<<8))
#define EXPAN_IO_13_OutputEnable()      (TRISECLR = (1U<<8))
#define EXPAN_IO_13_InputEnable()       (TRISESET = (1U<<8))
#define EXPAN_IO_13_Get()               ((PORTE >> 8) & 0x1U)
#define EXPAN_IO_13_PIN                  GPIO_PIN_RE8

/*** Macros for EXPAN_IO_12 pin ***/
#define EXPAN_IO_12_Set()               (LATESET = (1U<<9))
#define EXPAN_IO_12_Clear()             (LATECLR = (1U<<9))
#define EXPAN_IO_12_Toggle()            (LATEINV= (1U<<9))
#define EXPAN_IO_12_OutputEnable()      (TRISECLR = (1U<<9))
#define EXPAN_IO_12_InputEnable()       (TRISESET = (1U<<9))
#define EXPAN_IO_12_Get()               ((PORTE >> 9) & 0x1U)
#define EXPAN_IO_12_PIN                  GPIO_PIN_RE9

/*** Macros for EXPAN_IO_11 pin ***/
#define EXPAN_IO_11_Set()               (LATBSET = (1U<<5))
#define EXPAN_IO_11_Clear()             (LATBCLR = (1U<<5))
#define EXPAN_IO_11_Toggle()            (LATBINV= (1U<<5))
#define EXPAN_IO_11_OutputEnable()      (TRISBCLR = (1U<<5))
#define EXPAN_IO_11_InputEnable()       (TRISBSET = (1U<<5))
#define EXPAN_IO_11_Get()               ((PORTB >> 5) & 0x1U)
#define EXPAN_IO_11_PIN                  GPIO_PIN_RB5

/*** Macros for EXPAN_IO_10 pin ***/
#define EXPAN_IO_10_Set()               (LATBSET = (1U<<4))
#define EXPAN_IO_10_Clear()             (LATBCLR = (1U<<4))
#define EXPAN_IO_10_Toggle()            (LATBINV= (1U<<4))
#define EXPAN_IO_10_OutputEnable()      (TRISBCLR = (1U<<4))
#define EXPAN_IO_10_InputEnable()       (TRISBSET = (1U<<4))
#define EXPAN_IO_10_Get()               ((PORTB >> 4) & 0x1U)
#define EXPAN_IO_10_PIN                  GPIO_PIN_RB4

/*** Macros for ETH1_EREF_CLK pin ***/
#define ETH1_EREF_CLK_Get()               ((PORTJ >> 11) & 0x1U)
#define ETH1_EREF_CLK_PIN                  GPIO_PIN_RJ11

/*** Macros for O_RELAY_2 pin ***/
#define O_RELAY_2_Set()               (LATJSET = (1U<<13))
#define O_RELAY_2_Clear()             (LATJCLR = (1U<<13))
#define O_RELAY_2_Toggle()            (LATJINV= (1U<<13))
#define O_RELAY_2_OutputEnable()      (TRISJCLR = (1U<<13))
#define O_RELAY_2_InputEnable()       (TRISJSET = (1U<<13))
#define O_RELAY_2_Get()               ((PORTJ >> 13) & 0x1U)
#define O_RELAY_2_PIN                  GPIO_PIN_RJ13

/*** Macros for O_RELAY_3 pin ***/
#define O_RELAY_3_Set()               (LATJSET = (1U<<14))
#define O_RELAY_3_Clear()             (LATJCLR = (1U<<14))
#define O_RELAY_3_Toggle()            (LATJINV= (1U<<14))
#define O_RELAY_3_OutputEnable()      (TRISJCLR = (1U<<14))
#define O_RELAY_3_InputEnable()       (TRISJSET = (1U<<14))
#define O_RELAY_3_Get()               ((PORTJ >> 14) & 0x1U)
#define O_RELAY_3_PIN                  GPIO_PIN_RJ14

/*** Macros for O_RELAY_4 pin ***/
#define O_RELAY_4_Set()               (LATJSET = (1U<<15))
#define O_RELAY_4_Clear()             (LATJCLR = (1U<<15))
#define O_RELAY_4_Toggle()            (LATJINV= (1U<<15))
#define O_RELAY_4_OutputEnable()      (TRISJCLR = (1U<<15))
#define O_RELAY_4_InputEnable()       (TRISJSET = (1U<<15))
#define O_RELAY_4_Get()               ((PORTJ >> 15) & 0x1U)
#define O_RELAY_4_PIN                  GPIO_PIN_RJ15

/*** Macros for EXPAN_IO_06 pin ***/
#define EXPAN_IO_06_Set()               (LATBSET = (1U<<3))
#define EXPAN_IO_06_Clear()             (LATBCLR = (1U<<3))
#define EXPAN_IO_06_Toggle()            (LATBINV= (1U<<3))
#define EXPAN_IO_06_OutputEnable()      (TRISBCLR = (1U<<3))
#define EXPAN_IO_06_InputEnable()       (TRISBSET = (1U<<3))
#define EXPAN_IO_06_Get()               ((PORTB >> 3) & 0x1U)
#define EXPAN_IO_06_PIN                  GPIO_PIN_RB3

/*** Macros for EXPAN_IO_05 pin ***/
#define EXPAN_IO_05_Set()               (LATBSET = (1U<<2))
#define EXPAN_IO_05_Clear()             (LATBCLR = (1U<<2))
#define EXPAN_IO_05_Toggle()            (LATBINV= (1U<<2))
#define EXPAN_IO_05_OutputEnable()      (TRISBCLR = (1U<<2))
#define EXPAN_IO_05_InputEnable()       (TRISBSET = (1U<<2))
#define EXPAN_IO_05_Get()               ((PORTB >> 2) & 0x1U)
#define EXPAN_IO_05_PIN                  GPIO_PIN_RB2

/*** Macros for EXPAN_IO_04 pin ***/
#define EXPAN_IO_04_Set()               (LATBSET = (1U<<1))
#define EXPAN_IO_04_Clear()             (LATBCLR = (1U<<1))
#define EXPAN_IO_04_Toggle()            (LATBINV= (1U<<1))
#define EXPAN_IO_04_OutputEnable()      (TRISBCLR = (1U<<1))
#define EXPAN_IO_04_InputEnable()       (TRISBSET = (1U<<1))
#define EXPAN_IO_04_Get()               ((PORTB >> 1) & 0x1U)
#define EXPAN_IO_04_PIN                  GPIO_PIN_RB1

/*** Macros for EXPAN_IO_03 pin ***/
#define EXPAN_IO_03_Set()               (LATBSET = (1U<<0))
#define EXPAN_IO_03_Clear()             (LATBCLR = (1U<<0))
#define EXPAN_IO_03_Toggle()            (LATBINV= (1U<<0))
#define EXPAN_IO_03_OutputEnable()      (TRISBCLR = (1U<<0))
#define EXPAN_IO_03_InputEnable()       (TRISBSET = (1U<<0))
#define EXPAN_IO_03_Get()               ((PORTB >> 0) & 0x1U)
#define EXPAN_IO_03_PIN                  GPIO_PIN_RB0

/*** Macros for EXPAN_IO_01 pin ***/
#define EXPAN_IO_01_Set()               (LATASET = (1U<<9))
#define EXPAN_IO_01_Clear()             (LATACLR = (1U<<9))
#define EXPAN_IO_01_Toggle()            (LATAINV= (1U<<9))
#define EXPAN_IO_01_OutputEnable()      (TRISACLR = (1U<<9))
#define EXPAN_IO_01_InputEnable()       (TRISASET = (1U<<9))
#define EXPAN_IO_01_Get()               ((PORTA >> 9) & 0x1U)
#define EXPAN_IO_01_PIN                  GPIO_PIN_RA9

/*** Macros for EXPAN_IO_02 pin ***/
#define EXPAN_IO_02_Set()               (LATASET = (1U<<10))
#define EXPAN_IO_02_Clear()             (LATACLR = (1U<<10))
#define EXPAN_IO_02_Toggle()            (LATAINV= (1U<<10))
#define EXPAN_IO_02_OutputEnable()      (TRISACLR = (1U<<10))
#define EXPAN_IO_02_InputEnable()       (TRISASET = (1U<<10))
#define EXPAN_IO_02_Get()               ((PORTA >> 10) & 0x1U)
#define EXPAN_IO_02_PIN                  GPIO_PIN_RA10

/*** Macros for O_RELAY_5 pin ***/
#define O_RELAY_5_Set()               (LATHSET = (1U<<2))
#define O_RELAY_5_Clear()             (LATHCLR = (1U<<2))
#define O_RELAY_5_Toggle()            (LATHINV= (1U<<2))
#define O_RELAY_5_OutputEnable()      (TRISHCLR = (1U<<2))
#define O_RELAY_5_InputEnable()       (TRISHSET = (1U<<2))
#define O_RELAY_5_Get()               ((PORTH >> 2) & 0x1U)
#define O_RELAY_5_PIN                  GPIO_PIN_RH2

/*** Macros for O_RELAY_6 pin ***/
#define O_RELAY_6_Set()               (LATHSET = (1U<<3))
#define O_RELAY_6_Clear()             (LATHCLR = (1U<<3))
#define O_RELAY_6_Toggle()            (LATHINV= (1U<<3))
#define O_RELAY_6_OutputEnable()      (TRISHCLR = (1U<<3))
#define O_RELAY_6_InputEnable()       (TRISHSET = (1U<<3))
#define O_RELAY_6_Get()               ((PORTH >> 3) & 0x1U)
#define O_RELAY_6_PIN                  GPIO_PIN_RH3

/*** Macros for ETH2_INT pin ***/
#define ETH2_INT_Get()               ((PORTB >> 8) & 0x1U)
#define ETH2_INT_PIN                  GPIO_PIN_RB8

/*** Macros for ETH2_SDO pin ***/
#define ETH2_SDO_Get()               ((PORTB >> 9) & 0x1U)
#define ETH2_SDO_PIN                  GPIO_PIN_RB9

/*** Macros for ETH2_SDI pin ***/
#define ETH2_SDI_Get()               ((PORTB >> 10) & 0x1U)
#define ETH2_SDI_PIN                  GPIO_PIN_RB10

/*** Macros for ETH2_CS pin ***/
#define ETH2_CS_Set()               (LATBSET = (1U<<11))
#define ETH2_CS_Clear()             (LATBCLR = (1U<<11))
#define ETH2_CS_Toggle()            (LATBINV= (1U<<11))
#define ETH2_CS_OutputEnable()      (TRISBCLR = (1U<<11))
#define ETH2_CS_InputEnable()       (TRISBSET = (1U<<11))
#define ETH2_CS_Get()               ((PORTB >> 11) & 0x1U)
#define ETH2_CS_PIN                  GPIO_PIN_RB11

/*** Macros for O_RELAY_7 pin ***/
#define O_RELAY_7_Set()               (LATKSET = (1U<<1))
#define O_RELAY_7_Clear()             (LATKCLR = (1U<<1))
#define O_RELAY_7_Toggle()            (LATKINV= (1U<<1))
#define O_RELAY_7_OutputEnable()      (TRISKCLR = (1U<<1))
#define O_RELAY_7_InputEnable()       (TRISKSET = (1U<<1))
#define O_RELAY_7_Get()               ((PORTK >> 1) & 0x1U)
#define O_RELAY_7_PIN                  GPIO_PIN_RK1

/*** Macros for O_RELAY_8 pin ***/
#define O_RELAY_8_Set()               (LATKSET = (1U<<2))
#define O_RELAY_8_Clear()             (LATKCLR = (1U<<2))
#define O_RELAY_8_Toggle()            (LATKINV= (1U<<2))
#define O_RELAY_8_OutputEnable()      (TRISKCLR = (1U<<2))
#define O_RELAY_8_InputEnable()       (TRISKSET = (1U<<2))
#define O_RELAY_8_Get()               ((PORTK >> 2) & 0x1U)
#define O_RELAY_8_PIN                  GPIO_PIN_RK2

/*** Macros for SIMCOM_GPS_TXD pin ***/
#define SIMCOM_GPS_TXD_Get()               ((PORTF >> 13) & 0x1U)
#define SIMCOM_GPS_TXD_PIN                  GPIO_PIN_RF13

/*** Macros for SIMCOM_GPS_RXD pin ***/
#define SIMCOM_GPS_RXD_Get()               ((PORTF >> 12) & 0x1U)
#define SIMCOM_GPS_RXD_PIN                  GPIO_PIN_RF12

/*** Macros for ETH2_SCK pin ***/
#define ETH2_SCK_Get()               ((PORTB >> 14) & 0x1U)
#define ETH2_SCK_PIN                  GPIO_PIN_RB14

/*** Macros for ETH1_RX_ER pin ***/
#define ETH1_RX_ER_Get()               ((PORTH >> 4) & 0x1U)
#define ETH1_RX_ER_PIN                  GPIO_PIN_RH4

/*** Macros for ETH1_RXD1 pin ***/
#define ETH1_RXD1_Get()               ((PORTH >> 5) & 0x1U)
#define ETH1_RXD1_PIN                  GPIO_PIN_RH5

/*** Macros for LED_VERDE pin ***/
#define LED_VERDE_Set()               (LATHSET = (1U<<6))
#define LED_VERDE_Clear()             (LATHCLR = (1U<<6))
#define LED_VERDE_Toggle()            (LATHINV= (1U<<6))
#define LED_VERDE_OutputEnable()      (TRISHCLR = (1U<<6))
#define LED_VERDE_InputEnable()       (TRISHSET = (1U<<6))
#define LED_VERDE_Get()               ((PORTH >> 6) & 0x1U)
#define LED_VERDE_PIN                  GPIO_PIN_RH6

/*** Macros for LED_AMBAR pin ***/
#define LED_AMBAR_Set()               (LATHSET = (1U<<7))
#define LED_AMBAR_Clear()             (LATHCLR = (1U<<7))
#define LED_AMBAR_Toggle()            (LATHINV= (1U<<7))
#define LED_AMBAR_OutputEnable()      (TRISHCLR = (1U<<7))
#define LED_AMBAR_InputEnable()       (TRISHSET = (1U<<7))
#define LED_AMBAR_Get()               ((PORTH >> 7) & 0x1U)
#define LED_AMBAR_PIN                  GPIO_PIN_RH7

/*** Macros for CONSOLE_RX pin ***/
#define CONSOLE_RX_Get()               ((PORTD >> 14) & 0x1U)
#define CONSOLE_RX_PIN                  GPIO_PIN_RD14

/*** Macros for CONSOLE_TX pin ***/
#define CONSOLE_TX_Get()               ((PORTD >> 15) & 0x1U)
#define CONSOLE_TX_PIN                  GPIO_PIN_RD15

/*** Macros for EXT_IN_03_U4RX_RS232 pin ***/
#define EXT_IN_03_U4RX_RS232_Get()               ((PORTF >> 2) & 0x1U)
#define EXT_IN_03_U4RX_RS232_PIN                  GPIO_PIN_RF2

/*** Macros for EXT_IN_04_U4TX_RS232 pin ***/
#define EXT_IN_04_U4TX_RS232_Get()               ((PORTF >> 8) & 0x1U)
#define EXT_IN_04_U4TX_RS232_PIN                  GPIO_PIN_RF8

/*** Macros for ETH1_RXD0 pin ***/
#define ETH1_RXD0_Get()               ((PORTH >> 8) & 0x1U)
#define ETH1_RXD0_PIN                  GPIO_PIN_RH8

/*** Macros for BAT_OTG_VAP pin ***/
#define BAT_OTG_VAP_Set()               (LATHSET = (1U<<10))
#define BAT_OTG_VAP_Clear()             (LATHCLR = (1U<<10))
#define BAT_OTG_VAP_Toggle()            (LATHINV= (1U<<10))
#define BAT_OTG_VAP_OutputEnable()      (TRISHCLR = (1U<<10))
#define BAT_OTG_VAP_InputEnable()       (TRISHSET = (1U<<10))
#define BAT_OTG_VAP_Get()               ((PORTH >> 10) & 0x1U)
#define BAT_OTG_VAP_PIN                  GPIO_PIN_RH10

/*** Macros for ETH1_nRST pin ***/
#define ETH1_nRST_Set()               (LATHSET = (1U<<11))
#define ETH1_nRST_Clear()             (LATHCLR = (1U<<11))
#define ETH1_nRST_Toggle()            (LATHINV= (1U<<11))
#define ETH1_nRST_OutputEnable()      (TRISHCLR = (1U<<11))
#define ETH1_nRST_InputEnable()       (TRISHSET = (1U<<11))
#define ETH1_nRST_Get()               ((PORTH >> 11) & 0x1U)
#define ETH1_nRST_PIN                  GPIO_PIN_RH11

/*** Macros for BAT_SCL pin ***/
#define BAT_SCL_Get()               ((PORTA >> 2) & 0x1U)
#define BAT_SCL_PIN                  GPIO_PIN_RA2

/*** Macros for BAT_SDA pin ***/
#define BAT_SDA_Get()               ((PORTA >> 3) & 0x1U)
#define BAT_SDA_PIN                  GPIO_PIN_RA3

/*** Macros for BAT_CHG_OK pin ***/
#define BAT_CHG_OK_Set()               (LATASET = (1U<<4))
#define BAT_CHG_OK_Clear()             (LATACLR = (1U<<4))
#define BAT_CHG_OK_Toggle()            (LATAINV= (1U<<4))
#define BAT_CHG_OK_OutputEnable()      (TRISACLR = (1U<<4))
#define BAT_CHG_OK_InputEnable()       (TRISASET = (1U<<4))
#define BAT_CHG_OK_Get()               ((PORTA >> 4) & 0x1U)
#define BAT_CHG_OK_PIN                  GPIO_PIN_RA4

/*** Macros for RTC_SDA pin ***/
#define RTC_SDA_Get()               ((PORTF >> 4) & 0x1U)
#define RTC_SDA_PIN                  GPIO_PIN_RF4

/*** Macros for RTC_SCL pin ***/
#define RTC_SCL_Get()               ((PORTF >> 5) & 0x1U)
#define RTC_SCL_PIN                  GPIO_PIN_RF5

/*** Macros for EXT_IN_08 pin ***/
#define EXT_IN_08_Set()               (LATKSET = (1U<<4))
#define EXT_IN_08_Clear()             (LATKCLR = (1U<<4))
#define EXT_IN_08_Toggle()            (LATKINV= (1U<<4))
#define EXT_IN_08_OutputEnable()      (TRISKCLR = (1U<<4))
#define EXT_IN_08_InputEnable()       (TRISKSET = (1U<<4))
#define EXT_IN_08_Get()               ((PORTK >> 4) & 0x1U)
#define EXT_IN_08_PIN                  GPIO_PIN_RK4

/*** Macros for EXT_IN_09 pin ***/
#define EXT_IN_09_Set()               (LATKSET = (1U<<5))
#define EXT_IN_09_Clear()             (LATKCLR = (1U<<5))
#define EXT_IN_09_Toggle()            (LATKINV= (1U<<5))
#define EXT_IN_09_OutputEnable()      (TRISKCLR = (1U<<5))
#define EXT_IN_09_InputEnable()       (TRISKSET = (1U<<5))
#define EXT_IN_09_Get()               ((PORTK >> 5) & 0x1U)
#define EXT_IN_09_PIN                  GPIO_PIN_RK5

/*** Macros for EERAM_HS pin ***/
#define EERAM_HS_Set()               (LATKSET = (1U<<6))
#define EERAM_HS_Clear()             (LATKCLR = (1U<<6))
#define EERAM_HS_Toggle()            (LATKINV= (1U<<6))
#define EERAM_HS_OutputEnable()      (TRISKCLR = (1U<<6))
#define EERAM_HS_InputEnable()       (TRISKSET = (1U<<6))
#define EERAM_HS_Get()               ((PORTK >> 6) & 0x1U)
#define EERAM_HS_PIN                  GPIO_PIN_RK6

/*** Macros for EERAM_SCL pin ***/
#define EERAM_SCL_Get()               ((PORTA >> 14) & 0x1U)
#define EERAM_SCL_PIN                  GPIO_PIN_RA14

/*** Macros for EERAM_SDA pin ***/
#define EERAM_SDA_Get()               ((PORTA >> 15) & 0x1U)
#define EERAM_SDA_PIN                  GPIO_PIN_RA15

/*** Macros for EXT_IN_10 pin ***/
#define EXT_IN_10_Set()               (LATDSET = (1U<<9))
#define EXT_IN_10_Clear()             (LATDCLR = (1U<<9))
#define EXT_IN_10_Toggle()            (LATDINV= (1U<<9))
#define EXT_IN_10_OutputEnable()      (TRISDCLR = (1U<<9))
#define EXT_IN_10_InputEnable()       (TRISDSET = (1U<<9))
#define EXT_IN_10_Get()               ((PORTD >> 9) & 0x1U)
#define EXT_IN_10_PIN                  GPIO_PIN_RD9

/*** Macros for EXT_IN_11 pin ***/
#define EXT_IN_11_Set()               (LATDSET = (1U<<10))
#define EXT_IN_11_Clear()             (LATDCLR = (1U<<10))
#define EXT_IN_11_Toggle()            (LATDINV= (1U<<10))
#define EXT_IN_11_OutputEnable()      (TRISDCLR = (1U<<10))
#define EXT_IN_11_InputEnable()       (TRISDSET = (1U<<10))
#define EXT_IN_11_Get()               ((PORTD >> 10) & 0x1U)
#define EXT_IN_11_PIN                  GPIO_PIN_RD10

/*** Macros for ETH1_MDC pin ***/
#define ETH1_MDC_Get()               ((PORTD >> 11) & 0x1U)
#define ETH1_MDC_PIN                  GPIO_PIN_RD11

/*** Macros for ETH1_CRS_DV pin ***/
#define ETH1_CRS_DV_Get()               ((PORTH >> 13) & 0x1U)
#define ETH1_CRS_DV_PIN                  GPIO_PIN_RH13

/*** Macros for BAT_CELL_CONTROL pin ***/
#define BAT_CELL_CONTROL_Set()               (LATHSET = (1U<<15))
#define BAT_CELL_CONTROL_Clear()             (LATHCLR = (1U<<15))
#define BAT_CELL_CONTROL_Toggle()            (LATHINV= (1U<<15))
#define BAT_CELL_CONTROL_OutputEnable()      (TRISHCLR = (1U<<15))
#define BAT_CELL_CONTROL_InputEnable()       (TRISHSET = (1U<<15))
#define BAT_CELL_CONTROL_Get()               ((PORTH >> 15) & 0x1U)
#define BAT_CELL_CONTROL_PIN                  GPIO_PIN_RH15

/*** Macros for EXT_IN_12 pin ***/
#define EXT_IN_12_Set()               (LATDSET = (1U<<0))
#define EXT_IN_12_Clear()             (LATDCLR = (1U<<0))
#define EXT_IN_12_Toggle()            (LATDINV= (1U<<0))
#define EXT_IN_12_OutputEnable()      (TRISDCLR = (1U<<0))
#define EXT_IN_12_InputEnable()       (TRISDSET = (1U<<0))
#define EXT_IN_12_Get()               ((PORTD >> 0) & 0x1U)
#define EXT_IN_12_PIN                  GPIO_PIN_RD0

/*** Macros for ETH1_nINT pin ***/
#define ETH1_nINT_Set()               (LATCSET = (1U<<13))
#define ETH1_nINT_Clear()             (LATCCLR = (1U<<13))
#define ETH1_nINT_Toggle()            (LATCINV= (1U<<13))
#define ETH1_nINT_OutputEnable()      (TRISCCLR = (1U<<13))
#define ETH1_nINT_InputEnable()       (TRISCSET = (1U<<13))
#define ETH1_nINT_Get()               ((PORTC >> 13) & 0x1U)
#define ETH1_nINT_PIN                  GPIO_PIN_RC13

/*** Macros for RTC_MFP pin ***/
#define RTC_MFP_Get()               ((PORTC >> 14) & 0x1U)
#define RTC_MFP_PIN                  GPIO_PIN_RC14

/*** Macros for EXT_IN_02_U2TX_D_RS485 pin ***/
#define EXT_IN_02_U2TX_D_RS485_Get()               ((PORTD >> 1) & 0x1U)
#define EXT_IN_02_U2TX_D_RS485_PIN                  GPIO_PIN_RD1

/*** Macros for BAT_PROCHOT pin ***/
#define BAT_PROCHOT_Get()               ((PORTD >> 2) & 0x1U)
#define BAT_PROCHOT_PIN                  GPIO_PIN_RD2

/*** Macros for EXT_IN_01_U2RX_R_RS485 pin ***/
#define EXT_IN_01_U2RX_R_RS485_Get()               ((PORTD >> 12) & 0x1U)
#define EXT_IN_01_U2RX_R_RS485_PIN                  GPIO_PIN_RD12

/*** Macros for DE_RS485_DIR pin ***/
#define DE_RS485_DIR_Set()               (LATDSET = (1U<<13))
#define DE_RS485_DIR_Clear()             (LATDCLR = (1U<<13))
#define DE_RS485_DIR_Toggle()            (LATDINV= (1U<<13))
#define DE_RS485_DIR_OutputEnable()      (TRISDCLR = (1U<<13))
#define DE_RS485_DIR_InputEnable()       (TRISDSET = (1U<<13))
#define DE_RS485_DIR_Get()               ((PORTD >> 13) & 0x1U)
#define DE_RS485_DIR_PIN                  GPIO_PIN_RD13

/*** Macros for RE_RS485_DIR pin ***/
#define RE_RS485_DIR_Set()               (LATJSET = (1U<<0))
#define RE_RS485_DIR_Clear()             (LATJCLR = (1U<<0))
#define RE_RS485_DIR_Toggle()            (LATJINV= (1U<<0))
#define RE_RS485_DIR_OutputEnable()      (TRISJCLR = (1U<<0))
#define RE_RS485_DIR_InputEnable()       (TRISJSET = (1U<<0))
#define RE_RS485_DIR_Get()               ((PORTJ >> 0) & 0x1U)
#define RE_RS485_DIR_PIN                  GPIO_PIN_RJ0

/*** Macros for ETH1_MDIO pin ***/
#define ETH1_MDIO_Get()               ((PORTJ >> 1) & 0x1U)
#define ETH1_MDIO_PIN                  GPIO_PIN_RJ1

/*** Macros for FLASH_SQICS0 pin ***/
#define FLASH_SQICS0_Get()               ((PORTD >> 4) & 0x1U)
#define FLASH_SQICS0_PIN                  GPIO_PIN_RD4

/*** Macros for FLASH_SQICS1 pin ***/
#define FLASH_SQICS1_Get()               ((PORTD >> 5) & 0x1U)
#define FLASH_SQICS1_PIN                  GPIO_PIN_RD5

/*** Macros for ETH1_TXEN pin ***/
#define ETH1_TXEN_Get()               ((PORTD >> 6) & 0x1U)
#define ETH1_TXEN_PIN                  GPIO_PIN_RD6

/*** Macros for EXT_IN_07 pin ***/
#define EXT_IN_07_Set()               (LATDSET = (1U<<7))
#define EXT_IN_07_Clear()             (LATDCLR = (1U<<7))
#define EXT_IN_07_Toggle()            (LATDINV= (1U<<7))
#define EXT_IN_07_OutputEnable()      (TRISDCLR = (1U<<7))
#define EXT_IN_07_InputEnable()       (TRISDSET = (1U<<7))
#define EXT_IN_07_Get()               ((PORTD >> 7) & 0x1U)
#define EXT_IN_07_PIN                  GPIO_PIN_RD7

/*** Macros for SIMCOM_RXD pin ***/
#define SIMCOM_RXD_Get()               ((PORTF >> 0) & 0x1U)
#define SIMCOM_RXD_PIN                  GPIO_PIN_RF0

/*** Macros for SIMCOM_TXD pin ***/
#define SIMCOM_TXD_Get()               ((PORTF >> 1) & 0x1U)
#define SIMCOM_TXD_PIN                  GPIO_PIN_RF1

/*** Macros for EXT_IN_06 pin ***/
#define EXT_IN_06_Set()               (LATGSET = (1U<<1))
#define EXT_IN_06_Clear()             (LATGCLR = (1U<<1))
#define EXT_IN_06_Toggle()            (LATGINV= (1U<<1))
#define EXT_IN_06_OutputEnable()      (TRISGCLR = (1U<<1))
#define EXT_IN_06_InputEnable()       (TRISGSET = (1U<<1))
#define EXT_IN_06_Get()               ((PORTG >> 1) & 0x1U)
#define EXT_IN_06_PIN                  GPIO_PIN_RG1

/*** Macros for EXT_IN_05 pin ***/
#define EXT_IN_05_Set()               (LATGSET = (1U<<0))
#define EXT_IN_05_Clear()             (LATGCLR = (1U<<0))
#define EXT_IN_05_Toggle()            (LATGINV= (1U<<0))
#define EXT_IN_05_OutputEnable()      (TRISGCLR = (1U<<0))
#define EXT_IN_05_InputEnable()       (TRISGSET = (1U<<0))
#define EXT_IN_05_Get()               ((PORTG >> 0) & 0x1U)
#define EXT_IN_05_PIN                  GPIO_PIN_RG0

/*** Macros for FLASH_SQICLK pin ***/
#define FLASH_SQICLK_Get()               ((PORTA >> 6) & 0x1U)
#define FLASH_SQICLK_PIN                  GPIO_PIN_RA6

/*** Macros for FLASH_SQID3 pin ***/
#define FLASH_SQID3_Get()               ((PORTA >> 7) & 0x1U)
#define FLASH_SQID3_PIN                  GPIO_PIN_RA7

/*** Macros for FLASH_SQID2 pin ***/
#define FLASH_SQID2_Get()               ((PORTG >> 14) & 0x1U)
#define FLASH_SQID2_PIN                  GPIO_PIN_RG14

/*** Macros for FLASH_SQID1 pin ***/
#define FLASH_SQID1_Get()               ((PORTG >> 12) & 0x1U)
#define FLASH_SQID1_PIN                  GPIO_PIN_RG12

/*** Macros for FLASH_SQID0 pin ***/
#define FLASH_SQID0_Get()               ((PORTG >> 13) & 0x1U)
#define FLASH_SQID0_PIN                  GPIO_PIN_RG13

/*** Macros for SIMCOM_GPS_EN pin ***/
#define SIMCOM_GPS_EN_Set()               (LATESET = (1U<<2))
#define SIMCOM_GPS_EN_Clear()             (LATECLR = (1U<<2))
#define SIMCOM_GPS_EN_Toggle()            (LATEINV= (1U<<2))
#define SIMCOM_GPS_EN_OutputEnable()      (TRISECLR = (1U<<2))
#define SIMCOM_GPS_EN_InputEnable()       (TRISESET = (1U<<2))
#define SIMCOM_GPS_EN_Get()               ((PORTE >> 2) & 0x1U)
#define SIMCOM_GPS_EN_PIN                  GPIO_PIN_RE2

/*** Macros for EXPAN_IO_07 pin ***/
#define EXPAN_IO_07_Set()               (LATESET = (1U<<3))
#define EXPAN_IO_07_Clear()             (LATECLR = (1U<<3))
#define EXPAN_IO_07_Toggle()            (LATEINV= (1U<<3))
#define EXPAN_IO_07_OutputEnable()      (TRISECLR = (1U<<3))
#define EXPAN_IO_07_InputEnable()       (TRISESET = (1U<<3))
#define EXPAN_IO_07_Get()               ((PORTE >> 3) & 0x1U)
#define EXPAN_IO_07_PIN                  GPIO_PIN_RE3

/*** Macros for SIMCOM_POWER pin ***/
#define SIMCOM_POWER_Set()               (LATESET = (1U<<4))
#define SIMCOM_POWER_Clear()             (LATECLR = (1U<<4))
#define SIMCOM_POWER_Toggle()            (LATEINV= (1U<<4))
#define SIMCOM_POWER_OutputEnable()      (TRISECLR = (1U<<4))
#define SIMCOM_POWER_InputEnable()       (TRISESET = (1U<<4))
#define SIMCOM_POWER_Get()               ((PORTE >> 4) & 0x1U)
#define SIMCOM_POWER_PIN                  GPIO_PIN_RE4


// *****************************************************************************
/* GPIO Port

  Summary:
    Identifies the available GPIO Ports.

  Description:
    This enumeration identifies the available GPIO Ports.

  Remarks:
    The caller should not rely on the specific numbers assigned to any of
    these values as they may change from one processor to the next.

    Not all ports are available on all devices.  Refer to the specific
    device data sheet to determine which ports are supported.
*/


#define    GPIO_PORT_A  (0)
#define    GPIO_PORT_B  (1)
#define    GPIO_PORT_C  (2)
#define    GPIO_PORT_D  (3)
#define    GPIO_PORT_E  (4)
#define    GPIO_PORT_F  (5)
#define    GPIO_PORT_G  (6)
#define    GPIO_PORT_H  (7)
#define    GPIO_PORT_J  (8)
#define    GPIO_PORT_K  (9)
typedef uint32_t GPIO_PORT;

typedef enum
{
    GPIO_INTERRUPT_ON_MISMATCH,
    GPIO_INTERRUPT_ON_RISING_EDGE,
    GPIO_INTERRUPT_ON_FALLING_EDGE,
    GPIO_INTERRUPT_ON_BOTH_EDGES,
}GPIO_INTERRUPT_STYLE;

// *****************************************************************************
/* GPIO Port Pins

  Summary:
    Identifies the available GPIO port pins.

  Description:
    This enumeration identifies the available GPIO port pins.

  Remarks:
    The caller should not rely on the specific numbers assigned to any of
    these values as they may change from one processor to the next.

    Not all pins are available on all devices.  Refer to the specific
    device data sheet to determine which pins are supported.
*/


#define     GPIO_PIN_RA0  (0U)
#define     GPIO_PIN_RA1  (1U)
#define     GPIO_PIN_RA2  (2U)
#define     GPIO_PIN_RA3  (3U)
#define     GPIO_PIN_RA4  (4U)
#define     GPIO_PIN_RA5  (5U)
#define     GPIO_PIN_RA6  (6U)
#define     GPIO_PIN_RA7  (7U)
#define     GPIO_PIN_RA9  (9U)
#define     GPIO_PIN_RA10  (10U)
#define     GPIO_PIN_RA14  (14U)
#define     GPIO_PIN_RA15  (15U)
#define     GPIO_PIN_RB0  (16U)
#define     GPIO_PIN_RB1  (17U)
#define     GPIO_PIN_RB2  (18U)
#define     GPIO_PIN_RB3  (19U)
#define     GPIO_PIN_RB4  (20U)
#define     GPIO_PIN_RB5  (21U)
#define     GPIO_PIN_RB6  (22U)
#define     GPIO_PIN_RB7  (23U)
#define     GPIO_PIN_RB8  (24U)
#define     GPIO_PIN_RB9  (25U)
#define     GPIO_PIN_RB10  (26U)
#define     GPIO_PIN_RB11  (27U)
#define     GPIO_PIN_RB12  (28U)
#define     GPIO_PIN_RB13  (29U)
#define     GPIO_PIN_RB14  (30U)
#define     GPIO_PIN_RB15  (31U)
#define     GPIO_PIN_RC1  (33U)
#define     GPIO_PIN_RC2  (34U)
#define     GPIO_PIN_RC3  (35U)
#define     GPIO_PIN_RC4  (36U)
#define     GPIO_PIN_RC12  (44U)
#define     GPIO_PIN_RC13  (45U)
#define     GPIO_PIN_RC14  (46U)
#define     GPIO_PIN_RC15  (47U)
#define     GPIO_PIN_RD0  (48U)
#define     GPIO_PIN_RD1  (49U)
#define     GPIO_PIN_RD2  (50U)
#define     GPIO_PIN_RD3  (51U)
#define     GPIO_PIN_RD4  (52U)
#define     GPIO_PIN_RD5  (53U)
#define     GPIO_PIN_RD6  (54U)
#define     GPIO_PIN_RD7  (55U)
#define     GPIO_PIN_RD9  (57U)
#define     GPIO_PIN_RD10  (58U)
#define     GPIO_PIN_RD11  (59U)
#define     GPIO_PIN_RD12  (60U)
#define     GPIO_PIN_RD13  (61U)
#define     GPIO_PIN_RD14  (62U)
#define     GPIO_PIN_RD15  (63U)
#define     GPIO_PIN_RE0  (64U)
#define     GPIO_PIN_RE1  (65U)
#define     GPIO_PIN_RE2  (66U)
#define     GPIO_PIN_RE3  (67U)
#define     GPIO_PIN_RE4  (68U)
#define     GPIO_PIN_RE5  (69U)
#define     GPIO_PIN_RE6  (70U)
#define     GPIO_PIN_RE7  (71U)
#define     GPIO_PIN_RE8  (72U)
#define     GPIO_PIN_RE9  (73U)
#define     GPIO_PIN_RF0  (80U)
#define     GPIO_PIN_RF1  (81U)
#define     GPIO_PIN_RF2  (82U)
#define     GPIO_PIN_RF3  (83U)
#define     GPIO_PIN_RF4  (84U)
#define     GPIO_PIN_RF5  (85U)
#define     GPIO_PIN_RF8  (88U)
#define     GPIO_PIN_RF12  (92U)
#define     GPIO_PIN_RF13  (93U)
#define     GPIO_PIN_RG0  (96U)
#define     GPIO_PIN_RG1  (97U)
#define     GPIO_PIN_RG6  (102U)
#define     GPIO_PIN_RG7  (103U)
#define     GPIO_PIN_RG8  (104U)
#define     GPIO_PIN_RG9  (105U)
#define     GPIO_PIN_RG12  (108U)
#define     GPIO_PIN_RG13  (109U)
#define     GPIO_PIN_RG14  (110U)
#define     GPIO_PIN_RG15  (111U)
#define     GPIO_PIN_RH0  (112U)
#define     GPIO_PIN_RH1  (113U)
#define     GPIO_PIN_RH2  (114U)
#define     GPIO_PIN_RH3  (115U)
#define     GPIO_PIN_RH4  (116U)
#define     GPIO_PIN_RH5  (117U)
#define     GPIO_PIN_RH6  (118U)
#define     GPIO_PIN_RH7  (119U)
#define     GPIO_PIN_RH8  (120U)
#define     GPIO_PIN_RH9  (121U)
#define     GPIO_PIN_RH10  (122U)
#define     GPIO_PIN_RH11  (123U)
#define     GPIO_PIN_RH12  (124U)
#define     GPIO_PIN_RH13  (125U)
#define     GPIO_PIN_RH14  (126U)
#define     GPIO_PIN_RH15  (127U)
#define     GPIO_PIN_RJ0  (128U)
#define     GPIO_PIN_RJ1  (129U)
#define     GPIO_PIN_RJ2  (130U)
#define     GPIO_PIN_RJ3  (131U)
#define     GPIO_PIN_RJ4  (132U)
#define     GPIO_PIN_RJ5  (133U)
#define     GPIO_PIN_RJ6  (134U)
#define     GPIO_PIN_RJ7  (135U)
#define     GPIO_PIN_RJ8  (136U)
#define     GPIO_PIN_RJ9  (137U)
#define     GPIO_PIN_RJ10  (138U)
#define     GPIO_PIN_RJ11  (139U)
#define     GPIO_PIN_RJ12  (140U)
#define     GPIO_PIN_RJ13  (141U)
#define     GPIO_PIN_RJ14  (142U)
#define     GPIO_PIN_RJ15  (143U)
#define     GPIO_PIN_RK0  (144U)
#define     GPIO_PIN_RK1  (145U)
#define     GPIO_PIN_RK2  (146U)
#define     GPIO_PIN_RK3  (147U)
#define     GPIO_PIN_RK4  (148U)
#define     GPIO_PIN_RK5  (149U)
#define     GPIO_PIN_RK6  (150U)
#define     GPIO_PIN_RK7  (151U)

    /* This element should not be used in any of the GPIO APIs.
       It will be used by other modules or application to denote that none of the GPIO Pin is used */
#define    GPIO_PIN_NONE   (-1)

typedef uint32_t GPIO_PIN;


void GPIO_Initialize(void);

// *****************************************************************************
// *****************************************************************************
// Section: GPIO Functions which operates on multiple pins of a port
// *****************************************************************************
// *****************************************************************************

uint32_t GPIO_PortRead(GPIO_PORT port);

void GPIO_PortWrite(GPIO_PORT port, uint32_t mask, uint32_t value);

uint32_t GPIO_PortLatchRead ( GPIO_PORT port );

void GPIO_PortSet(GPIO_PORT port, uint32_t mask);

void GPIO_PortClear(GPIO_PORT port, uint32_t mask);

void GPIO_PortToggle(GPIO_PORT port, uint32_t mask);

void GPIO_PortInputEnable(GPIO_PORT port, uint32_t mask);

void GPIO_PortOutputEnable(GPIO_PORT port, uint32_t mask);

// *****************************************************************************
// *****************************************************************************
// Section: GPIO Functions which operates on one pin at a time
// *****************************************************************************
// *****************************************************************************

static inline void GPIO_PinWrite(GPIO_PIN pin, bool value)
{
	 uint32_t xvalue = (uint32_t)value;
    GPIO_PortWrite((pin>>4U), (uint32_t)(0x1U) << (pin & 0xFU), (xvalue) << (pin & 0xFU));
}

static inline bool GPIO_PinRead(GPIO_PIN pin)
{
    return ((((GPIO_PortRead((GPIO_PORT)(pin>>4U))) >> (pin & 0xFU)) & 0x1U) != 0U);
}

static inline bool GPIO_PinLatchRead(GPIO_PIN pin)
{
    return (((GPIO_PortLatchRead((GPIO_PORT)(pin>>4U)) >> (pin & 0xFU)) & 0x1U) != 0U);
}

static inline void GPIO_PinToggle(GPIO_PIN pin)
{
    GPIO_PortToggle((pin>>4U), (uint32_t)0x1U << (pin & 0xFU));
}

static inline void GPIO_PinSet(GPIO_PIN pin)
{
    GPIO_PortSet((pin>>4U), (uint32_t)0x1U << (pin & 0xFU));
}

static inline void GPIO_PinClear(GPIO_PIN pin)
{
    GPIO_PortClear((pin>>4U), (uint32_t)0x1U << (pin & 0xFU));
}

static inline void GPIO_PinInputEnable(GPIO_PIN pin)
{
    GPIO_PortInputEnable((pin>>4U), (uint32_t)0x1U << (pin & 0xFU));
}

static inline void GPIO_PinOutputEnable(GPIO_PIN pin)
{
    GPIO_PortOutputEnable((pin>>4U), (uint32_t)0x1U << (pin & 0xFU));
}


// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

    }

#endif
// DOM-IGNORE-END
#endif // PLIB_GPIO_H

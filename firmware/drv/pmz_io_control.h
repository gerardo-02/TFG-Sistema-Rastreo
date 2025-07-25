#ifndef IO_CONTROL_H
#define	IO_CONTROL_H

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#ifdef	__cplusplus
extern "C" {
#endif

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Constants                                                         */
    /* ************************************************************************** */

    /* ************************************************************************** */
    typedef enum {
        IO_OUTPUT_GREEN_LED = 0,
        IO_OUTPUT_AMBER_LED,
        IO_OUTPUT_RELE_ID_1,
        IO_OUTPUT_RELE_ID_2,
        IO_OUTPUT_RELE_ID_3,
        IO_OUTPUT_RELE_ID_4,
        IO_OUTPUT_RELE_ID_5,
        IO_OUTPUT_RELE_ID_6,
        IO_OUTPUT_RELE_ID_7,
        IO_OUTPUT_RELE_ID_8,
        IO_OUTPUT_SIMCOM_POWER,
        //
        IO_OUTPUT_MAX,
    } t_IO_OUTPUT_ID;

    typedef enum {
        IO_INPUT_ID_01 = 0,
        IO_INPUT_ID_02,
        IO_INPUT_ID_03,
        IO_INPUT_ID_04,
        IO_INPUT_ID_05,
        IO_INPUT_ID_06,
        IO_INPUT_ID_07,
        IO_INPUT_ID_08,
        IO_INPUT_ID_09,
        IO_INPUT_ID_10,
        IO_INPUT_ID_11,
        IO_INPUT_ID_12,
        //
        IO_INPUT_MAX,
    } t_IO_INPUT_ID;

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Data types & Definitions                                          */
    /* ************************************************************************** */

    /* ************************************************************************** */
    typedef enum {
        IO_INPUT_STATE_DISABLE = 0,
        IO_INPUT_STATE_UNKNOWN,
        IO_INPUT_STATE_LOW,
        IO_INPUT_STATE_LOW_TO_HIGH,
        IO_INPUT_STATE_HIGH,
        IO_INPUT_STATE_HIGH_TO_LOW,
    } t_IO_INPUT_STATES;
    typedef void (*IO_INPUT_CALL_BACK)(t_IO_INPUT_ID id, t_IO_INPUT_STATES state, void *localCtx);

    typedef enum {
        IO_OUTPUT_STATE_LOW = 0,
        IO_OUTPUT_STATE_HIGH,
    } t_IO_OUTPUT_STATES;

    typedef enum {
        IO_EDGE_RISING = 0,
        IO_EDGE_FALLING,
        IO_EDGE_BOTH,
        //
        IO_MAX_EDGES
    } t_IO_EDGE;

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */
    void io_initialize(void);
    t_IO_INPUT_STATES io_input_get_state(t_IO_INPUT_ID id);
    void io_register_input_cb(t_IO_INPUT_ID id, t_IO_EDGE edge, IO_INPUT_CALL_BACK cb, void *localCtx);
    t_IO_OUTPUT_STATES io_output_get_state(t_IO_OUTPUT_ID id);
    int io_output_set_state(t_IO_OUTPUT_ID id, t_IO_OUTPUT_STATES state);
    int io_output_toggle(t_IO_OUTPUT_ID id);
    int io_output_pulse(t_IO_OUTPUT_ID id, t_IO_OUTPUT_STATES state, uint32_t ms_operation_time);
    int io_output_blink(t_IO_OUTPUT_ID id, uint16_t ms_pulse_high, uint16_t ms_pulse_low, uint32_t ms_operation_time);

#ifdef	__cplusplus
}
#endif

#endif	/* IO_CONTROL_H */


/*
 * Trace Recorder for Tracealyzer v4.8.1
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Main configuration parameters for the trace recorder library.
 * More settings can be found in trcStreamingConfig.h and trcSnapshotConfig.h.
 */

#ifndef TRC_CONFIG_H
#define TRC_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include of processor header file
 *
 * Here you may need to include the header file for your processor. This is
 * required at least for the ARM Cortex-M port, that uses the ARM CMSIS API.
 * Try that in case of build problems. Otherwise, remove the #error line below.
 *****************************************************************************/
//#error "Trace Recorder: Please include your processor's header file here and remove this line."
#include "stm32f0xx.h"

/**
 * @def TRC_CFG_HARDWARE_PORT
 * @brief Specify what hardware port to use (i.e., the "timestamping driver").
 *
 * All ARM Cortex-M MCUs are supported by "TRC_HARDWARE_PORT_ARM_Cortex_M".
 * This port uses the DWT cycle counter for Cortex-M3/M4/M7 devices, which is
 * available on most such devices. In case your device don't have DWT support,
 * you will get an error message opening the trace. In that case, you may
 * force the recorder to use SysTick timestamping instead, using this define:
 *
 * #define TRC_CFG_ARM_CM_USE_SYSTICK
 *
 * For ARM Cortex-M0/M0+ devices, SysTick mode is used automatically.
 *
 * See trcHardwarePort.h for available ports and information on how to
 * define your own port, if not already present.
 */
#define TRC_CFG_HARDWARE_PORT TRC_HARDWARE_PORT_ARM_Cortex_M

#define TRC_CFG_SCHEDULING_ONLY             0
#define TRC_CFG_INCLUDE_MEMMANG_EVENTS      1
#define TRC_CFG_INCLUDE_USER_EVENTS         1
#define TRC_CFG_INCLUDE_ISR_TRACING         0
#define TRC_CFG_INCLUDE_READY_EVENTS        1
#define TRC_CFG_INCLUDE_OSTICK_EVENTS       0

#define TRC_CFG_ENABLE_STACK_MONITOR        1
#define TRC_CFG_STACK_MONITOR_MAX_TASKS     10
#define TRC_CFG_STACK_MONITOR_MAX_REPORTS   1

#define TRC_CFG_CTRL_TASK_PRIORITY          1
#define TRC_CFG_CTRL_TASK_DELAY             50
#define TRC_CFG_CTRL_TASK_STACK_SIZE        configMINIMAL_STACK_SIZE

#define TRC_CFG_RECORDER_BUFFER_ALLOCATION  TRC_RECORDER_BUFFER_ALLOCATION_STATIC
#define TRC_CFG_MAX_ISR_NESTING             8

#define TRC_CFG_ISR_TAILCHAINING_THRESHOLD  0
#define TRC_CFG_RECORDER_DATA_INIT          1

#define TRC_CFG_RECORDER_DATA_ATTRIBUTE 

#define TRC_CFG_USE_TRACE_ASSERT            0

#ifdef __cplusplus
}
#endif

#endif /* _TRC_CONFIG_H */

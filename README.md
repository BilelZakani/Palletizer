# Project Palletizer (Palettiseur) - Real-Time Embedded System

This project focuses on the implementation of a real-time control system for an industrial **Palletizer**, developed as part of the SE4 Embedded Systems program (2024-2027) at **Polytech Montpellier**.

## Introduction
The objective of this project is to manage the real-time control of an industrial palletizing system simulated within the **RealGames Factory I/O** environment. The system is designed to construct a pallet consisting of two layers, each containing six boxes. Control is handled by a **STM32F072RB Nucleo** board running **FreeRTOS**.

## Work Summary

### 1. Real-Time Architecture
The system architecture is decomposed into several parallel tasks to manage different industrial components:
* **Communication Bridge:** A Python bridge interfaces the Nucleo board and the simulation via UART (115200 bauds), using a specific 8-byte frame format.
* **Task Management:** Specific tasks handle the logic for boxes (**Carton**), the barrier (**Barriere**), the pusher and door (**Poussoir and porte**), the elevator (**Ascenseur**), and the pallet delivery (**Palette**).
* **Synchronization:** Communication between tasks is secured using FreeRTOS kernel objects such as semaphores, mutexes, and message queues.

### 2. Memory & CPU Optimization
* **Memory Management:** Stack sizes for each task were initially set at 150 words and subsequently refined using **High Water Mark (HWM)** debug data to optimize RAM usage.
* **CPU Utilization:** Performance analysis via **Percepio Tracealyzer 4** revealed that the CPU remains in an IDLE state approximately **92.7%** of the time during standard operation.

### 3. Energy Consumption Reduction
Significant efforts were made to reduce the energy footprint of the hardware:
* **Frequency Scaling:** Reducing the CPU frequency from **48 MHz to 8 MHz** lowered consumption from 16.13 mA to 4.8 mA.
* **Low-Power Mode:** Implementing a `vApplicationIdleHook` to trigger a Sleep mode (`_WFI()`) during IDLE periods further reduced final consumption to **3.49 mA**.

## Project Documentation
For a detailed technical breakdown, including functional diagrams, memory mapping, and detailed performance results, please refer to the final project report:
* **Palettiseur_BZF.pdf**

---
**Author:** Bilel ZAKANI-FADILI  
**Supervising Professor:** Mr. Laurent LATORRE

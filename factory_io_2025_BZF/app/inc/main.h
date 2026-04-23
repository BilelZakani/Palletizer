/*
 * main.h
 *
 *  Created on: 16 ao�t 2018
 *      Author: Laurent
 */

#ifndef APP_INC_MAIN_H_
#define APP_INC_MAIN_H_

#include "stm32f0xx.h"
#include "bsp.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"
#include "stream_buffer.h"

//Define actuators
#define Distribution_cartons 	 		(uint32_t)(1)
#define Tapis_distrib_cartons 	  		(uint32_t)(1 << 1)
#define Blocage_entree_palettiseur 		(uint32_t)(1 << 2)
#define Porte 	 						(uint32_t)(1 << 3)
#define Poussoir 						(uint32_t)(1 << 4)
#define Clamp 	 						(uint32_t)(1 << 5)
#define Monter_ascenseur				(uint32_t)(1 << 6)
#define Descendre_ascenseur				(uint32_t)(1 << 8)
#define Ascenseur_to_limit				(uint32_t)(1 << 9)
#define Distribution_palette			(uint32_t)(1 << 10)
#define Charger_palette					(uint32_t)(1 << 11)
#define Tapis_carton_vers_palettiseur	(uint32_t)(1 << 12)
#define Tourner_carton					(uint32_t)(1 << 13)
#define Decharger_palettiseur			(uint32_t)(1 << 15)
#define Tapis_Charger_palettiseur		(uint32_t)(1 << 16)
#define Tapis_decharger_palette			(uint32_t)(1 << 17)
#define Tapis_Palette_Vers_Ascenseur 	(uint32_t)(1 << 18)
#define Tapis_Distribution_Palette 		(uint32_t)(1 << 19)
#define Tapis_Fin 						(uint32_t)(1 << 20)
#define Remover   						(uint32_t)(1 << 21)


//Define sensors
#define Carton_distribue 	 			(uint32_t)(1)
#define Carton_envoye 	  				(uint32_t)(1 << 1)
#define Entree_palletiseur  	  		(uint32_t)(1 << 2)
#define Porte_ouverte 	  				(uint32_t)(1 << 3)
#define Limite_poussoir 	  			(uint32_t)(1 << 4)
#define Clamped 	  					(uint32_t)(1 << 5)
#define Ascenseur_etage_RDC 	  		(uint32_t)(1 << 6)
#define Ascenseur_etage1 	  			(uint32_t)(1 << 8)
#define Ascenseur_etage2     			(uint32_t)(1 << 9)
#define Sortie_palette     				(uint32_t)(1 << 10)
#define Limite_porte     				(uint32_t)(1 << 11)
#define Ascenseur_en_mvt     			(uint32_t)(1 << 12)
#define Entree_palette     				(uint32_t)(1 << 13)

// Others defines
#define TX_DMA_BUFFER_SIZE					60
#define debug_console_offset 				12

/* Global functions */

int my_printf	(const char *format, ...);
int my_sprintf	(char *out, const char *format, ...);


#endif /* APP_INC_MAIN_H_ */

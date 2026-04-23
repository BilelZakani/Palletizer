

#include "stm32f0xx.h"
#include "main.h"
#include "bsp.h"
#include "factory_io.h"
#include "delay.h"
#include "string.h"


/*
 * Local Static Functions
 */

static uint8_t 	SystemClock_Config	(void);
void            my_new_printf       (const char* msg);

// FreeRTOS tasks
void vTaskcarton 					(void *pvParameters);
void vTaskBarriere 					(void *pvParameters);
void vTaskPoussoir_and_Porte 		(void *pvParameters);
void vTask_Write 					(void *pvParameters);
void vTask_Publish 					(void *pvParameters);
void vTaskAscenseur 				(void *pvParameters);
void vTaskPalette 					(void *pvParameters);
//void vTaskPrint 					(void *pvParameters);
void vTaskHWM 						(void *pvParameters);


// Kernel Objects
xQueueHandle 	 xSubscribeQueue;
xQueueHandle 	 xWriteQueue;
xSemaphoreHandle xConsoleMutex;

xSemaphoreHandle xSem_DMA_TC;
xSemaphoreHandle xSemTask2;
xSemaphoreHandle xSemTask1;
xSemaphoreHandle xSemTask3;
xSemaphoreHandle xSemTask4;
xSemaphoreHandle xSem_cartons_bloque;
xSemaphoreHandle xSem_poussoir_ready;
xSemaphoreHandle xSem_porte_ready;

xTaskHandle 	 xSemTask1_sync;
xTaskHandle 	 xTask2_HWM;
xTaskHandle      xSem_layer_ready;
xTaskHandle 	 xSem_ascenseur_ready;
xTaskHandle 	 xSem_palette_ready;
xTaskHandle 	 xTask_Publish;
xTaskHandle 	 xTask_Write;

xTaskHandle 	 vTaskHWM_handle;

uint8_t tx_dma_buffer[TX_DMA_BUFFER_SIZE];
uint8_t	rx_dma_buffer[FRAME_LENGTH];

uint8_t count_boxes;
uint8_t count_pallets;

typedef struct
{
	uint32_t mask; 		   		// Actionner ID
	uint32_t state;       		// Actionner State
} message_t;

typedef struct
{
	uint32_t sensor_mask;  		// Awaited sensor ID
	uint32_t sensor_state; 		// Awaited sensor State
	xSemaphoreHandle *xSem;		// Semaphore linked to the response
} subscribe_message_t;

#define TAILLE_TABLEAU 5
typedef subscribe_message_t sub_tab[TAILLE_TABLEAU];


/*
 * Project Entry Point
 */

int main(void)
{
	// Configure System Clock for 48MHz from 8MHz HSE
	//SystemClock_Config();

	// Initialize LED and USER Button
	BSP_LED_Init();
	BSP_PB_Init();

	// Initialize Debug Console
	BSP_Console_Init();
	BSP_Console_Init_2();
	my_printf("%c[0m",   0x1B);	// Remove all text attributes
	my_printf("%c[2J",   0x1B); 	// Clear console
	my_printf("\r\nConsole Ready!\r\n");
	my_printf("SYSCLK = %d Hz\r\n", SystemCoreClock);

	// Read all states from the scene
	FACTORY_IO_update();

	// Wait here for user button
	while(BSP_PB_GetState() == 0);

	count_boxes   = 0;
	count_pallets = 0;

	// Start Trace Recording
	vTraceEnable(TRC_START);

	// Queues
	xSubscribeQueue = xQueueCreate(4, sizeof(subscribe_message_t));
	xWriteQueue = xQueueCreate(4, sizeof(message_t));

	// Create a Mutex for accessing the console
	xConsoleMutex = xSemaphoreCreateMutex();

	// Semaphores
	xSem_DMA_TC 		= 	xSemaphoreCreateBinary();
    xSemTask1			=	xSemaphoreCreateBinary();
	xSemTask2			=	xSemaphoreCreateBinary();
	xSemTask3			=	xSemaphoreCreateBinary();
    xSemTask4			=	xSemaphoreCreateBinary();
	xSem_cartons_bloque =   xSemaphoreCreateBinary();
	xSem_poussoir_ready =   xSemaphoreCreateBinary();
	xSem_porte_ready	=	xSemaphoreCreateBinary();

	// Create Tasks
	xTaskCreate(vTaskcarton, 				"vTaskcarton", 				140, NULL, 5, &xSemTask1_sync);
	xTaskCreate(vTaskBarriere, 				"vTaskBarriere", 			140, NULL, 2, &xTask2_HWM);
	xTaskCreate(vTaskPoussoir_and_Porte, 	"vTaskPoussoir_and_Porte", 	150, NULL, 3, &xSem_ascenseur_ready);
	xTaskCreate(vTaskAscenseur,			 	"vTaskAscenseur", 			160, NULL, 4, &xSem_layer_ready);
	xTaskCreate(vTaskPalette, 				"vTaskPalette", 			128, NULL, 3, &xSem_palette_ready);
	xTaskCreate(vTask_Publish, 				"Task_Publish", 			140, NULL, 6, &xTask_Publish);
	xTaskCreate(vTask_Write, 				"Task_Write", 				128, NULL, 7, &xTask_Write);
	xTaskCreate(vTaskHWM,		 			"vTaskHWM_handle", 			128, NULL, 1, &vTaskHWM_handle);

	// Give a nice name to the Queue in the trace recorder
	vTraceSetQueueName(xSubscribeQueue, "Publisher Queue");
	vTraceSetQueueName(xWriteQueue, "Writer Queue");

	// Give a nice name to the Mutex in the trace recorder
	vTraceSetMutexName(xConsoleMutex, "Console Mutex");

	// Give a nice name to the Semaphore in the trace recorder
	vTraceSetSemaphoreName(xSem_DMA_TC, 		"xSem_DMA_TC");
	vTraceSetSemaphoreName(xSemTask1, 			"xSemTask1");
	vTraceSetSemaphoreName(xSemTask2, 			"xSemTask2");
	vTraceSetSemaphoreName(xSemTask3, 			"xSemTask3");
	vTraceSetSemaphoreName(xSemTask4, 			"xSemTask4");
	vTraceSetSemaphoreName(xSem_cartons_bloque, "xSem_cartons_bloque");
	vTraceSetSemaphoreName(xSem_poussoir_ready, "xSem_poussoir_ready");
	vTraceSetSemaphoreName(xSem_porte_ready,    "xSem_porte_ready");

	// Start the Scheduler
	vTaskStartScheduler();

	// Loop forever
	while(1)
	{

	}
}



/*
 *	Gestion des cartons
 */
void vTaskcarton (void *pvParameters)
{
	//variable
	uint8_t i;
	uint32_t mask_off=0x00000000;

	// Tapis
	message_t msg_tapis={(Tapis_distrib_cartons|Tapis_carton_vers_palettiseur|Tapis_Charger_palettiseur|Tapis_Palette_Vers_Ascenseur|Tapis_Distribution_Palette|Charger_palette|Tapis_Fin),(Tapis_distrib_cartons|Tapis_carton_vers_palettiseur|Tapis_Charger_palettiseur|Tapis_Palette_Vers_Ascenseur|Charger_palette|Tapis_Distribution_Palette|Tapis_Fin)};

	// Remover
	message_t msg_remover={Remover, Remover};

	// Cartons
	message_t msg_task1_distrib_on={Distribution_cartons,Distribution_cartons};
	message_t msg_task1_distrib_off={Distribution_cartons,mask_off};

	//message_publish
	subscribe_message_t carton_distribue={Carton_distribue,mask_off,&xSemTask1};
	subscribe_message_t carton_envoyé={Carton_envoye,mask_off,&xSemTask1};

	//Tapis
	xQueueSendToBack(xWriteQueue, &msg_tapis, 0);
	xQueueSendToBack(xWriteQueue, &msg_remover, 0);

	while(1)
	{
		//Wait here for notif
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		for(i=0;i<2;i++)
		{
			xQueueSendToBack(xWriteQueue, &msg_task1_distrib_on, 0);
			xQueueSendToBack(xSubscribeQueue, &carton_distribue, 0);
			xSemaphoreTake(xSemTask1, portMAX_DELAY);
			xQueueSendToBack(xWriteQueue, &msg_task1_distrib_off, 0);
			xQueueSendToBack(xSubscribeQueue, &carton_envoyé, 0);
			xSemaphoreTake(xSemTask1, portMAX_DELAY);
		}

	}

}

/*
 *	Gestion de la barriere
 */

void vTaskBarriere (void *pvParameters)
{
	uint32_t mask_off=0x00000000;

	//Actionneur
	message_t msg_blocage_entree_palettiseur_on={Blocage_entree_palettiseur,Blocage_entree_palettiseur};
	message_t msg_blocage_entree_palettiseur_off={Blocage_entree_palettiseur,mask_off};
	message_t msg_tapis_carton_vers_palettiseur_On={Tapis_carton_vers_palettiseur,Tapis_carton_vers_palettiseur};
	message_t msg_tapis_carton_vers_palettiseur_Off={Tapis_carton_vers_palettiseur,mask_off};

	//message_publish
	subscribe_message_t entree_palletiseur_off={Entree_palletiseur,mask_off,&xSemTask2};
	subscribe_message_t entree_palletiseur_on={Entree_palletiseur,Entree_palletiseur,&xSemTask2};

	xSemaphoreGive(xSem_poussoir_ready);

	while(1)
	{
		// Attente porte prête
		xSemaphoreTake(xSem_porte_ready, portMAX_DELAY);

		// Activation bloqueur
		xQueueSendToBack(xWriteQueue, &msg_blocage_entree_palettiseur_on, 0);

		// Bloqueur actif -> Autorisation de distribution de cartons
		xTaskNotifyGive(xSemTask1_sync);

		// Attente 1er carton
		xQueueSendToBack(xSubscribeQueue, &entree_palletiseur_off, 0);
		xSemaphoreTake(xSemTask2, portMAX_DELAY);
		xQueueSendToBack(xSubscribeQueue, &entree_palletiseur_on, 0);
		xSemaphoreTake(xSemTask2, portMAX_DELAY);

		// Attente 2eme carton
		xQueueSendToBack(xSubscribeQueue, &entree_palletiseur_off, 0);
		xSemaphoreTake(xSemTask2, portMAX_DELAY);

		// Attente poussoir prêt
		xSemaphoreTake(xSem_poussoir_ready, portMAX_DELAY);

		// Notification Poussoir
		xSemaphoreGive(xSem_cartons_bloque);

		// Desactivation du tapis de distribution de cartons
		xQueueSendToBack(xWriteQueue, &msg_tapis_carton_vers_palettiseur_Off, 0);

		// Bloqueur inactif
		xQueueSendToBack(xWriteQueue, &msg_blocage_entree_palettiseur_off, 0);

		vTaskDelay(700);

		// Reactivation du tapis
		xQueueSendToBack(xWriteQueue, &msg_tapis_carton_vers_palettiseur_On, 0);

	}

}

/*
 *	Gestion du poussoir et de la porte
 */

void vTaskPoussoir_and_Porte (void *pvParameters)
{
	//variable
	uint32_t mask_off=0x00000000;
	uint8_t compteur=0;

	//Actionneur
	message_t msg_poussoir_ON={Poussoir,Poussoir};
	message_t msg_poussoir_OFF={Poussoir,mask_off};
	message_t clamp_On={Clamp,Clamp};
	message_t clamp_Off={Clamp,mask_off};
	message_t porte_On={Porte,Porte};
	message_t porte_Off={Porte,mask_off};

	//message_publish
	subscribe_message_t limite_poussoir={Limite_poussoir,Limite_poussoir,&xSemTask3};
	subscribe_message_t clamped={Clamped,Clamped,&xSemTask3};
	subscribe_message_t porte_ouverte={Porte_ouverte,Porte_ouverte,&xSemTask3};
	subscribe_message_t porte_ferme={Porte_ouverte | Limite_porte,Limite_porte,&xSemTask3};

	xSemaphoreGive(xSem_porte_ready);

	while(1)
	{
		// Attente de cartons bloques
		xSemaphoreTake(xSem_cartons_bloque, portMAX_DELAY);

		// Attente de l'arrivé des cartons en butee
		vTaskDelay(3500);

		// Activation du poussoir
		xQueueSendToBack(xWriteQueue, &msg_poussoir_ON, 0);

		count_boxes += 2;

		vTaskDelay(500);

		// Attente poussoir au bout
		xQueueSendToBack(xSubscribeQueue, &limite_poussoir, 0);
		xSemaphoreTake(xSemTask3, portMAX_DELAY);

		// Desactivation du poussoir
		xQueueSendToBack(xWriteQueue, &msg_poussoir_OFF, 0);

		// Attente retour du poussoir
		xQueueSendToBack(xSubscribeQueue, &limite_poussoir, 0);
		xSemaphoreTake(xSemTask3, portMAX_DELAY);

		// Fin poussoir -> blocage
		xSemaphoreGive(xSem_poussoir_ready);

		compteur++;

		if(compteur==3)
		{
			// Attente de la dispo de l'ascenseur
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

			// Clamp
			xQueueSendToBack(xWriteQueue, &clamp_On, 0);

			// Attente du clamped
			xQueueSendToBack(xSubscribeQueue, &clamped, 0);
			xSemaphoreTake(xSemTask3, portMAX_DELAY);

			// Ouverture porte
			xQueueSendToBack(xWriteQueue, &porte_On, 0);

			// Attente fin ouverture de la porte
			xQueueSendToBack(xSubscribeQueue, &porte_ouverte, 0);
			xSemaphoreTake(xSemTask3, portMAX_DELAY);

			// Relachement
			xQueueSendToBack(xWriteQueue, &clamp_Off, 0);

			// Notification a l'ascenseur
			xTaskNotifyGive(xSem_layer_ready);

			// Attente de la dispo de l'ascenseur
			vTaskDelay(2000);

			// Fermeture porte
			xQueueSendToBack(xWriteQueue, &porte_Off, 0);

			// Attente fin fermeture de la porte
			xQueueSendToBack(xSubscribeQueue, &porte_ferme, 0);
			xSemaphoreTake(xSemTask3, portMAX_DELAY);

			// Reinitialisation du compteur
			compteur = 0;

		}

		// La porte est prête
		xSemaphoreGive(xSem_porte_ready);

	}

}


/*
 *	Gestion de l'ascenseur
 */

void vTaskAscenseur (void *pvParameters)
{
	//variable
	uint32_t mask_off=0x00000000;

	//message_publish
	subscribe_message_t sortie_ascenseur_On={Sortie_palette,Sortie_palette,&xSemTask4};
	subscribe_message_t sortie_ascenseur_Off={Sortie_palette,mask_off,&xSemTask4};
	subscribe_message_t ascenseur_en_mvt_Off={Ascenseur_en_mvt,mask_off,&xSemTask4};
	subscribe_message_t etageRDC={Ascenseur_etage_RDC,Ascenseur_etage_RDC,&xSemTask4};
	subscribe_message_t etage1={Ascenseur_etage1,Ascenseur_etage1,&xSemTask4};
	subscribe_message_t etage2={Ascenseur_etage2,Ascenseur_etage2,&xSemTask4};

	//Actionneur
	message_t tapis_ascenseur_ON={Charger_palette,Charger_palette};
	message_t tapis_ascenseur_OFF={Charger_palette,mask_off};
	message_t monter_to_limit_ON={Monter_ascenseur|Ascenseur_to_limit,Monter_ascenseur|Ascenseur_to_limit};
	message_t monter_to_limit_OFF={Monter_ascenseur|Ascenseur_to_limit,mask_off};
	message_t descendre_etage_On={Descendre_ascenseur,Descendre_ascenseur};
	message_t descendre_etage_Off={Descendre_ascenseur,mask_off};
	message_t descendre_to_limit={Ascenseur_to_limit,Ascenseur_to_limit};


	while(1)
	{
		// Attente detection de la palette au milieu de l'ascenseur
		xQueueSendToBack(xSubscribeQueue, &sortie_ascenseur_On, 0);
		xSemaphoreTake(xSemTask4, portMAX_DELAY);
		//xEventGroupWaitBits(event_palette_in_ascenseur, mask_event_group, pdTRUE, pdTRUE, portMAX_DELAY);

		// Desactivation du tapis de l'ascenseur
		xQueueSendToBack(xWriteQueue, &tapis_ascenseur_OFF, 0);

		// Ascenseur monte à l'etage 1
		xQueueSendToBack(xWriteQueue, &monter_to_limit_ON, 0);

		// Attente fin mouvement ascenseur
		xQueueSendToBack(xSubscribeQueue, &ascenseur_en_mvt_Off, 0);
		xSemaphoreTake(xSemTask4, portMAX_DELAY);

		// Verifie position de la palette a l'étage 1 (capteur etage 1 et etage 2 on)
		xQueueSendToBack(xSubscribeQueue, &etage1, 0);
		xSemaphoreTake(xSemTask4, portMAX_DELAY);
		xQueueSendToBack(xSubscribeQueue, &etage2, 0);
		xSemaphoreTake(xSemTask4, portMAX_DELAY);

		// Desactivation du mouvement jusqu'a la limite
		xQueueSendToBack(xWriteQueue, &monter_to_limit_OFF, 0);

		// Notification a la porte
		xTaskNotifyGive(xSem_ascenseur_ready);

		// Attente layer de cartons
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Ascenseur descends à l'étage 2
		xQueueSendToBack(xWriteQueue, &descendre_etage_On, 0);

		// Attente fin mouvement ascenseur
		xQueueSendToBack(xSubscribeQueue, &ascenseur_en_mvt_Off, 0);
		xSemaphoreTake(xSemTask4, portMAX_DELAY);

		// Notification a la porte
		xTaskNotifyGive(xSem_ascenseur_ready);

		// Attente layer de cartons
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Ascenseur descends au RDC
		xQueueSendToBack(xWriteQueue, &descendre_to_limit, 0);

		// Attente Ascenseur au RDC
		xQueueSendToBack(xSubscribeQueue, &etageRDC, 0);
		xSemaphoreTake(xSemTask4, portMAX_DELAY);

		// Desactivation de la descente de l'ascenseur
		xQueueSendToBack(xWriteQueue, &descendre_etage_Off, 0);

		// Demarrage tapis palette
		xQueueSendToBack(xWriteQueue, &tapis_ascenseur_ON, 0);

		// Attente que la palette soit sortie de l'ascenseur
		xQueueSendToBack(xSubscribeQueue, &sortie_ascenseur_Off, 0);
		xSemaphoreTake(xSemTask4, portMAX_DELAY);

		count_pallets++;

		// Envoi de la synchro pour remove
		xTaskNotifyGive(xSem_palette_ready);
	}
}

/*
 *	Gestion des palettes
 */

void vTaskPalette (void *pvParameters)
{
	//variable
	uint32_t mask_off=0x00000000;
	//Actionneur
	message_t distribution_palette_ON={Distribution_palette,Distribution_palette};
	message_t distribution_palette_OFF={Distribution_palette,mask_off};

	while(1)
	{
		// Creation de la palette
		xQueueSendToBack(xWriteQueue, &distribution_palette_ON, 0);
		vTaskDelay(1000);
		xQueueSendToBack(xWriteQueue, &distribution_palette_OFF, 0);

		// Attente palette prête
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	}


}


/*
 *	Task_write
 */
void vTask_Write (void *pvParameters)
{
	message_t msg;

	uint8_t count = 0;
	uint32_t n;

	// Set priority 1 for DMA1_Channel 5 interrupt
	NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, configMAX_API_CALL_INTERRUPT_PRIORITY + 0);

	// Enable DMA1_Channel 5 interrupt
	NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);

	// Take Console Mutex
	xSemaphoreTake(xConsoleMutex, portMAX_DELAY);

	// Console
	my_printf("%c[1;92;40m", 0x1B); 	// Vert vif sur du noir
	my_printf("%c[5;0H", 0x1B);
	my_printf("---------- PALLETIZER SUMMARY ----------");
	my_printf("%c[7;0H", 0x1B);
	my_printf("Palletized Boxes :");
	my_printf("%c[9;0H", 0x1B);
	my_printf("Pallets :");
	my_printf("%c[11;0H", 0x1B);
	my_printf("----------------------------------------");

	// Release Console Mutex
	xSemaphoreGive(xConsoleMutex);

	uint32_t actual_state=0x00000000;

	while(1)
	{
		// Wait for something in the message Queue
		xQueueReceive(xWriteQueue, &msg, portMAX_DELAY);

		actual_state&=~msg.mask;
		actual_state|=msg.state;

		// Calcul du CRC
		n = actual_state;
		while (n > 0) {
			n = n & (n - 1);
			count++;
		}

		// Prepare frame buffer
		tx_dma_buffer[0] = TAG_ACTUATORS; 									// Actuators tag

		tx_dma_buffer[1] = (uint8_t) (actual_state & 0x000000FF);			// data byte #1
		tx_dma_buffer[2] = (uint8_t)((actual_state & 0x0000FF00) >>8U );	// data byte #2
		tx_dma_buffer[3] = (uint8_t)((actual_state & 0x00FF0000) >>16U);	// data byte #3
		tx_dma_buffer[4] = (uint8_t)((actual_state & 0xFF000000) >>24U);	// data byte #4

		tx_dma_buffer[5] = (uint8_t)((count        & 0x0000001F) <<5U );    // CRC
		tx_dma_buffer[6] = '\n';											// End byte


		DMA1_Channel4->CNDTR = 7;
		DMA1_Channel4->CCR |= DMA_CCR_EN;
		USART2->CR3 |= USART_CR3_DMAT;

		// Wait for Semaphore endlessly
		xSemaphoreTake(xSem_DMA_TC, portMAX_DELAY);

		// Disable DMA1 Channel 4
		DMA1_Channel4->CCR &= ~DMA_CCR_EN;

		// Enable USART2 DMA Request on TX
		USART2->CR3 &= ~USART_CR3_DMAT;

		// Take Console Mutex
		xSemaphoreTake(xConsoleMutex, portMAX_DELAY);

		// Console
		my_printf("%c[1;92;40m", 0x1B); 	// Vert vif sur du noir
		my_printf("%c[7;20H", 0x1B);
		my_printf("%3d", count_boxes);
		my_printf("%c[9;20H", 0x1B);
		my_printf("%3d", count_pallets);

		// Release Console Mutex
		xSemaphoreGive(xConsoleMutex);

		vTaskDelay(100);
	}
}


void vTask_Publish (void *pvParameters)
{
	portTickType	xLastWakeTime;
	// Initialize timing
	xLastWakeTime = xTaskGetTickCount();
	subscribe_message_t msg;
	uint32_t sstates = 0x00000000;
	uint32_t state_desired;
	sub_tab sub;
	uint8_t j;
	uint8_t i=0;
	uint8_t a;
	uint8_t exist=0;

	memset(sub, 0, sizeof(sub));

	while(1)
	{
		// Wait for something in the message Queue
		if(xQueueReceive(xSubscribeQueue, &msg, 0))
		{
			for(j=0;j<TAILLE_TABLEAU;j++)
			{
				if((sub[j].sensor_mask==msg.sensor_mask) && (sub[j].sensor_state==msg.sensor_state))
				{
					exist=1;
				}
			}
			if(exist==0)
			{
				while(exist!=1)
				{
					if((sub[i].sensor_mask==0) && (sub[i].sensor_state==0) && (sub[i].xSem==NULL))
					{
						sub[i]=msg;
						exist=1;
					}
					i++;
				}
				i=0;
			}
			exist=0;
		}
		for(a=0;a<TAILLE_TABLEAU;a++)
		{
			if(sub[a].sensor_mask != 0)
			{
				//Etape1
				sstates |= rx_dma_buffer[1];
				sstates |= (rx_dma_buffer[2] <<8U );
				sstates |= (rx_dma_buffer[3] <<16U);
				sstates |= (rx_dma_buffer[4] <<24U);
				//Etape2
				state_desired=sub[a].sensor_mask & sstates;
				//Etape3
				if(state_desired == sub[a].sensor_state)
				{
					xSemaphoreGive(*(sub[a].xSem));
					sub[a].sensor_mask=0;
					sub[a].sensor_state=0;
					sub[a].xSem=NULL;
				}
			}
			sstates = 0x00000000;
		}

		// Wait here for 200ms since last wakeup
		vTaskDelayUntil (&xLastWakeTime, (200/portTICK_RATE_MS));

	}
}



/*
 * vTaskHWM
 */
void vTaskHWM (void *pvParameters)
{
	uint32_t	count;
	uint16_t	hwm_Task_cartons, hwm_Task_barriere, hwm_Task_poussoir_and_porte,hwm_Task_ascenseur,
				hwm_Task_palette, hwm_Task_publish, hwm_Task_write, hwm_TaskHWM;
	uint32_t	free_heap_size;

	count = 0;

	// Take Console Mutex
	xSemaphoreTake(xConsoleMutex, portMAX_DELAY);

	// Prepare console layout using ANSI escape sequences
	my_printf("%c[0m",   0x1B);	// Remove all text attributes

	my_printf("%c[0;30;107m", 0x1B); 	// black over white
	my_printf("%c[%d;0H", 0x1B,debug_console_offset+1);	// Move cursor [1:0]
	my_printf("High Water Marks");

	my_printf("%c[0m",   0x1B);	// Remove all text attributes

	my_printf("%c[%d;0H", 0x1B,debug_console_offset+3);	// Move cursor line 3
	my_printf("Iterations");

	my_printf("%c[%d;0H", 0x1B,debug_console_offset+4);	// Move cursor line 4
	my_printf("Task Cartons");

	my_printf("%c[%d;0H", 0x1B,debug_console_offset+5);	// Move cursor line 5
	my_printf("Task Barriere");

	my_printf("%c[%d;0H", 0x1B,debug_console_offset+6);	// Move cursor line 6
	my_printf("Task Poussoir and Porte");

	my_printf("%c[%d;0H", 0x1B,debug_console_offset+7);	// Move cursor line 7
	my_printf("Task Ascenseur");

	my_printf("%c[%d;0H", 0x1B,debug_console_offset+8);	// Move cursor line 8
	my_printf("Task Palette");

	my_printf("%c[%d;0H", 0x1B,debug_console_offset+9);	// Move cursor line 9
	my_printf("Task Publish");

	my_printf("%c[%d;0H", 0x1B,debug_console_offset+10);	// Move cursor line 10
	my_printf("Task Write");

	my_printf("%c[%d;0H", 0x1B,debug_console_offset+11);	// Move cursor line 11
	my_printf("Task HWM");

	my_printf("%c[%d;0H", 0x1B,debug_console_offset+12);	// Move cursor line 12
	my_printf("Free Heap");

	// Release Console Mutex
	xSemaphoreGive(xConsoleMutex);

	while(1)
	{
	  // Gather High Water Marks
	  hwm_Task_cartons				= uxTaskGetStackHighWaterMark(xSemTask1_sync);
	  hwm_Task_barriere				= uxTaskGetStackHighWaterMark(xTask2_HWM);
	  hwm_Task_poussoir_and_porte   = uxTaskGetStackHighWaterMark(xSem_ascenseur_ready);
	  hwm_Task_ascenseur     		= uxTaskGetStackHighWaterMark(xSem_layer_ready);
	  hwm_Task_palette       		= uxTaskGetStackHighWaterMark(xSem_palette_ready);
	  hwm_Task_publish       		= uxTaskGetStackHighWaterMark(xTask_Publish);
	  hwm_Task_write         		= uxTaskGetStackHighWaterMark(xTask_Write);
	  hwm_TaskHWM					= uxTaskGetStackHighWaterMark(vTaskHWM_handle);

	  // Get free Heap size
	  free_heap_size = xPortGetFreeHeapSize();

	  // Take Console Mutex
	  xSemaphoreTake(xConsoleMutex, portMAX_DELAY);

	  // Display results into console
	  my_printf("%c[0;31;40m", 0x1B); 	// Red over black

	  my_printf("%c[%d;30H", 0x1B,debug_console_offset+3);
	  my_printf("%5d", count);

	  my_printf("%c[1;30;107m", 0x1B); 	// black over white

	  my_printf("%c[%d;30H", 0x1B,debug_console_offset+4);
	  my_printf("%5d", hwm_Task_cartons);

	  my_printf("%c[%d;30H", 0x1B,debug_console_offset+5);
	  my_printf("%5d", hwm_Task_barriere);

	  my_printf("%c[%d;30H", 0x1B,debug_console_offset+6);
	  my_printf("%5d", hwm_Task_poussoir_and_porte);

	  my_printf("%c[%d;30H", 0x1B,debug_console_offset+7);
	  my_printf("%5d", hwm_Task_ascenseur);

	  my_printf("%c[%d;30H", 0x1B,debug_console_offset+8);
	  my_printf("%5d", hwm_Task_palette);

	  my_printf("%c[%d;30H", 0x1B,debug_console_offset+9);
	  my_printf("%5d", hwm_Task_publish);

	  my_printf("%c[%d;30H", 0x1B,debug_console_offset+10);
	  my_printf("%5d", hwm_Task_write);

	  my_printf("%c[%d;30H", 0x1B,debug_console_offset+11);
	  my_printf("%5d", hwm_TaskHWM);

	  my_printf("%c[1;35;40m", 0x1B); 	// Majenta over black
	  my_printf("%c[%d;30H", 0x1B,debug_console_offset+12);
	  my_printf("%5d", free_heap_size);

	  my_printf("%c[0m", 0x1B); 		// Remove all text attributes

	  // Release Console Mutex
	  xSemaphoreGive(xConsoleMutex);

	  count++;

	  // Wait for 200ms
	  vTaskDelay(200);
	}
}


void vApplicationIdleHook(void){

	// Go into Sleep mode
	__WFI();
}


/*
 * 	Clock configuration for the Nucleo STM32F072RB board
 * 	HSE input Bypass Mode 			-> 8MHz
 * 	SYSCLK, AHB, APB1 				-> 48MHz
 *  PA8 as MCO with /16 prescaler 	-> 3MHz
 */

static uint8_t SystemClock_Config()
{
	uint32_t	status;
	uint32_t	timeout;

	// Start HSE in Bypass Mode
	RCC->CR |= RCC_CR_HSEBYP;
	RCC->CR |= RCC_CR_HSEON;

	// Wait until HSE is ready
	timeout = 1000;

	do
	{
		status = RCC->CR & RCC_CR_HSERDY_Msk;
		timeout--;
	} while ((status == 0) && (timeout > 0));

	if (timeout == 0) return (1);	// HSE error


	// Select HSE as PLL input source
	RCC->CFGR &= ~RCC_CFGR_PLLSRC_Msk;
	RCC->CFGR |= (0x02 <<RCC_CFGR_PLLSRC_Pos);

	// Set PLL PREDIV to /1
	RCC->CFGR2 = 0x00000000;

	// Set PLL MUL to x6
	RCC->CFGR &= ~RCC_CFGR_PLLMUL_Msk;
	RCC->CFGR |= (0x04 <<RCC_CFGR_PLLMUL_Pos);

	// Enable the main PLL
	RCC-> CR |= RCC_CR_PLLON;

	// Wait until PLL is ready
	timeout = 1000;

	do
	{
		status = RCC->CR & RCC_CR_PLLRDY_Msk;
		timeout--;
	} while ((status == 0) && (timeout > 0));

	if (timeout == 0) return (2);	// PLL error


	// Set AHB prescaler to /1
	RCC->CFGR &= ~RCC_CFGR_HPRE_Msk;
	RCC->CFGR |= RCC_CFGR_HPRE_DIV1;

	//Set APB1 prescaler to /1
	RCC->CFGR &= ~RCC_CFGR_PPRE_Msk;
	RCC->CFGR |= RCC_CFGR_PPRE_DIV1;

	// Enable FLASH Prefetch Buffer and set Flash Latency (required for high speed)
	FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;

	// Select the main PLL as system clock source
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL;

	// Wait until PLL becomes main switch input
	timeout = 1000;

	do
	{
		status = (RCC->CFGR & RCC_CFGR_SWS_Msk);
		timeout--;
	} while ((status != RCC_CFGR_SWS_PLL) && (timeout > 0));

	if (timeout == 0) return (3);	// SW error


	// Set MCO source as SYSCLK (48MHz)
	RCC->CFGR &= ~RCC_CFGR_MCO_Msk;
	RCC->CFGR |=  RCC_CFGR_MCOSEL_SYSCLK;

	// Set MCO prescaler to /16 -> 3MHz
	RCC->CFGR &= ~RCC_CFGR_MCOPRE_Msk;
	RCC->CFGR |=  RCC_CFGR_MCOPRE_DIV16;

	// Enable GPIOA clock
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	// Configure PA8 as Alternate function
	GPIOA->MODER &= ~GPIO_MODER_MODER8_Msk;
	GPIOA->MODER |= (0x02 <<GPIO_MODER_MODER8_Pos);

	// Set to AF0 (MCO output)
	GPIOA->AFR[1] &= ~(0x0000000F);
	GPIOA->AFR[1] |=  (0x00000000);

	// Update SystemCoreClock global variable
	SystemCoreClockUpdate();
	return (0);
}



//-----------------------------------------------------------------------------
// MASTER_Config_Globale.c
//
//Gestion des trames de commandes et d'informations
//Réception sur l'UART des commandes et stockage dans un tableau binaire
//Lecture dans un tableau des informations et envoie par l'UART
//-----------------------------------------------------------------------------

#include <C8051F020.h>
#include "c8051F020_SFR16.h"

//-----------------------------------------------------------------------------
// Initialisation clock du Microcontrôleur (sysclock à 22MHz)
//-----------------------------------------------------------------------------
void Oscillator_init(void){
	int i = 0;
	OSCXCN    = 0x67;  // Config de l'horloge externe - Quartz > 6,7 MHz
	for (i = 0; i < 3000; i++);  // attente stabilisation Fosc quartz  
	while ((OSCXCN & 0x80) == 0); // validation stabilité du quartz
	OSCICN    = 0x0C;  // Commutation sur oscillateur externe
}
//-----------------------------------------------------------------------------
// Initialisation Ports I/O du Microcontrôleur
//-----------------------------------------------------------------------------
void Port_IO_init(void){
	
	XBR0 = (1<<2);
	XBR2 = (1<<6);
	
}
//-----------------------------------------------------------------------------
// Initialisation Sources de reset du Microcontrôleur
//-----------------------------------------------------------------------------

void Reset_Sources_init(void){
	//Watchdog
	WDTCN = 0xDE;
	WDTCN = 0XAD;
	
	//Autres
}	

//-----------------------------------------------------------------------------
// Initialisation globale du Microcontrôleur
//-----------------------------------------------------------------------------
void Init_Device(void)
{
	Oscillator_init();
	Port_IO_init();
	Reset_Sources_init();
}

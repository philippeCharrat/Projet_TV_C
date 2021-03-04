//-----------------------------------------------------------------------------
// F0-M01.c
//
//Gestion des trames de commandes et d'informations
//Réception sur l'UART des commandes et stockage dans un tableau binaire
//Lecture dans un tableau des informations et envoie par l'UART
//-----------------------------------------------------------------------------

#include <C8051F020.h>
#include <stdio.h>
#include <string.h>
#include "c8051F020_SFR16.h"
#include "MASTER_Config_Globale.h"

// Prototypes de Fonctions
//Config
void Config_UART0(void);
void Config_interrupt(void);
void Config_Timer(void);

//Utiles
void Send_char(char c);
void Interpretation_commande(void);
void Reception_chaine_UART(char* ptr_String_UART);
void Send_string(char*);
void Transmettre(char caractere, bit LF);
char* split_element(char* ptr_commande);
void Convertion(void);

//Interrupt
void INT_UART0(void);

//Variables utiles
//Reception de chaine
char fin_de_commande;
char bit_reception_UART;
char Lecture_String_UART[50] = '\r';
char* ptr_String_UART = &Lecture_String_UART[0];
char commande[16];
char params[5];
int i,j,k,m,l;
void Affichage_UART(char*);
char* ptrcommande; 
    
//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------

void main (void) {
	// Appel des configurations globales
	Init_Device();  
	Config_Timer();
	Config_UART0();
	
	//Init variables
	fin_de_commande = 0;
	Send_string("SYSTEME OK !");

	while (1){
		
		if (RI0 == 1){
		
			RI0 = 0;
			REN0 = 0;
			
			bit_reception_UART = SBUF0;
			commande[i] = bit_reception_UART;
			i++;
			*(ptr_String_UART) = bit_reception_UART;
			*(ptr_String_UART + 1) = '\0';
			
			Send_char(bit_reception_UART);
			
			if (bit_reception_UART == '\r'){
				ptr_String_UART = &Lecture_String_UART[0];
				Send_char('\n');
				Send_string(commande);
				i = 0;
				Convertion();
			}
			else{	ptr_String_UART++;}
			
			REN0 = 1;			
		}
	}
}

//-----------------------------------------------------------------------------
// Fonctions de configuration des divers périphériques et interruptions
//-----------------------------------------------------------------------------
void Config_interrupt(){
}

void Config_UART0(void){
	//SMOD0 dans PCON.7 à 0 pour garder la baud rate/2
	//SM00 SM10 : 01 (mode 1)
	//SM20 : 0 (valeur du bit de STOP ignorée)
	//REN0 : 0 (reception desactivée)
	//TB80 RB80 : 00 (valeur du bit lors de la Transmission/Reception dans le mode 2 ou 3)
	//TI0 RI0 : 00 (flag lors d'une fin de transmission/reception)
	SCON0 = (1<<6);
}

void Config_Timer() {
	
	//TIMER 2 (POUR UART)
	
	//TF2 EXF2 = 00 (flags interrupt)
	//RCLK0 TCLK0 : 11 (mode 2 baud rate generator receive et transmit)
	//EXEN2 : 0 (T2EX ignored)
	//TR2 : 0 (TIMER2 disabled)
	//C/T2 : 0 (SYSCLK used)
	//CP/RL2 : 0 (ignored in mode 2)
	RCAP2 = 0xFFDC; //Baud-rate de 19200
	T2CON = (3<<4);
	TR2 = 1; //start timer
}

//-----------------------------------------------------------------------------
// Fonctions utiles
//-----------------------------------------------------------------------------

void Affichage_UART(char* mot){
		while (*mot != '\r'){
			if(*(mot+1) == '\0'){
				Transmettre(*mot, 1); //Fin de chaine
			}
			else {
				Transmettre(*mot, 0); //milieu du mot
			}
			mot++;
		}
}

//Recupere le caractere en attente dans SBUF0
//et stocke la valeur dans le buffer de commande
void Reception_chaine_UART(char* ptr_String_UART){
	char reception = SBUF0;
	//Lecture du caractere et stockage
	*(ptr_String_UART) = reception;
	
	//Si on recoit le caractere de fin de chaine c'est qu'on a recu la totalité de la commande
	//Arguments inclus
	if (reception == 0x0D){
		fin_de_commande = 1;
	}
	
	//MAJ de la position d'ecriture pour la commande en cours de reception
	ptr_String_UART++;
}

void Interpretation_commande(void){
	
}

void Send_string(char* mot){
		while (*mot != '\0'){
			if(*(mot+1) == '\r'){
				Transmettre(*mot, 1); //Fin de chaine
			}
			else {
				Transmettre(*mot, 0); //milieu du mot
			}
			mot++;
		}
}

void Send_char(char c){
	
	//Desactive reception
	REN0 = 0;
	SBUF0 = c;
	
	//Attente fin de transmission
	while(!TI0){}
		
			//Remise à 0 du flag d'envoi une fois qu'on est sur que le caractere a été transmis
	TI0 = 0;
	REN0 = 1;
}

void Transmettre(char caractere, bit LF){
	
	EA = 0;
	//desactive la reception
	REN0 = 0;
	
	//Ecrit la valeur dans SBUF0 pour transmettre
	SBUF0 = caractere;
	
	//Attente de la bonne transmission
	while(!TI0){}
		
	//Remise à 0 du flag d'envoi une fois qu'on est sur que le caractere a été transmis
	TI0 = 0;
	REN0 = 1;
		
	if(LF){Transmettre(0x0D, 0);
	Transmettre(0x0A, 0);} //Retour à la ligne
	EA = 1;
}

//-----------------------------------------------------------------------------
// Fonctions d'interruptions
//-----------------------------------------------------------------------------
INT_UART0(void) interrupt 4{
	
//Cas interruption car on a fini une transmission
	if (TI0 == 1){TI0 = 0;}
	
//Cas interruption parce qu'on recoit une info
	if (RI0 == 1){
		
	//Remise des flag et desactivation temporaire de la reception
		REN0 = 0;
		RI0 = 0;
		
		//Stockage du caractere recu dans la chaine representant la commande en cours de reception
		Reception_chaine_UART(ptr_String_UART);
		Send_string(ptr_String_UART);
		REN0 = 1;
		//Si on a toute la commande il faut voir si elle est correcte, et dans le cas echeant la mettre dans une structure
		//Elle meme stockee dans un tableau
			if (fin_de_commande == 1){
				ptr_String_UART = &Lecture_String_UART[0];
				//interpretation /!\ desactiver les interruptions et la reception pendant le traitement pour eviter de refaire la lecture 
				//si une commande arrive pendant le traitement
				Send_string("Commande bien recue");
				REN0 = 1;
				//stockage
		}
	}
}
void Convertion(void) {
		k = 1;
		ptrcommande = &commande;
		ptrcommande = split_element(ptrcommande);
		if(k == 0) {
        //TODO : Fin de commande
    } else {
        if (params[0] == 'D' || params[0] == 'E' || params[0] == 'Q') {
            //commande_epreuve(params[0],k);
						Send_string(params);
        }    else if (params[0] == 'A' || params[0] == 'B' || params[0] == 'S' || params[0] == 'G'&& strcmp(params, "RD") == 0 && strcmp(params, "RG") == 0 && strcmp(params, "RA") == 0) {
            //commande_mouvement(params[0],k);
        }    else if (strcmp(params, "ASS") == 0) {
            //TODO : ACQ
        }    else if (strcmp(params, "MOB") == 0 ) {
            //TODO : DCT
        }    else if (strcmp(params, "CS") == 0) {
            //commande_servo();
        } else if (strcmp(params, "MI") == 0 && strcmp(params, "ME") == 0 ) {
            // TODO : Energie
        }else if (strcmp(params, "IPO") == 0 && strcmp(params, "POS") == 0 ) {
            //commande_position();
        }else if (strcmp(params, "L") == 0 && strcmp(params, "LS") == 0 ) {
            //commande_lumiere();
        }else if (strcmp(params, "PPH") == 0 && strcmp(params, "SPH") == 0 ) {
            //commande_photo();
        } else {
            //erreur_commande();
        }
    }
}   

char* split_element(char* ptr_commande) {
  m = 0;
	while( *ptr_commande != ' ') {
		params[m] = *ptr_commande;
		m++;
		ptr_commande++;
	}		
}
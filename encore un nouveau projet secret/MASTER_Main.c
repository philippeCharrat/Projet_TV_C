//-----------------------------------------------------------------------------
// F0-M01.c
//
//Gestion des trames de commandes et d'informations
//Réception sur l'UART des commandes et stockage dans un tableau binaire
//Lecture dans un tableau des informations et envoie par l'UART
//-----------------------------------------------------------------------------

// Import des bibliothèques ---
#include <C8051F020.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "c8051F020_SFR16.h"
#include "MASTER_Config_Globale.h"
#include "FO_M1__Structures_COMMANDES_INFORMATIONS_CentraleDeCommande.h"
// ---

// Prototypes de Fonctions

// Partie : Configuration 
void Config_UART0(void);
void Config_interrupt(void);
void Config_Timer(void);

// Partie : Envoies et Réception des messages 
void Send_char(char c);
void Interpretation_commande(void);
void Reception_chaine_UART(char* ptr_String_UART);
void Send_string(char*);
void Transmettre(char caractere, bit LF);
void Affichage_UART(char*);

// Partie : Fonctions utiles 
char* split_element(char* ptr_commande);
int convertion_str_int(int k, char* ptr);
struct argument_complexe param_complexe(char* params);
int int_neg_or_positiv(int min, char* params);

// Partie : Convertion 
void Convertion(void);
void Convertion_Etat(char etat, char* ptrcommande);
void Convertion_Mouvement(char *mouvement, char* ptrcommande);
void Convertion_Servomoteur(char* ptrcommande);
void Convertion_Coord(char* params,char* ptrcommande);
void Convertion_Lumineux(char* coord, char* ptrcommande);

// Partie : Interrupt
void INT_UART0(void);

// Variables globales utiles
char fin_de_commande;
char bit_reception_UART;

int i,j,k,m,fin_commande;

// Variables pointeurs 
char* ptrcommande; 

// Variables char[] 
char buffer[3];
char Lecture_String_UART[16] = '\r';
char* ptr_String_UART = &Lecture_String_UART[0];
char commande[16];
char params[5];

// Définition des structures 
typedef struct argument_complexe argument_complexe;
struct argument_complexe {
	char param;
	int valeur;
};
argument_complexe args;
struct COMMANDES commandeenvoieStA;

//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------

void main (void) {
	// Appel des configurations globales
	Init_Device();  
	Config_Timer();
	Config_UART0();
	
	//Initialisation de variables
	fin_de_commande = 0;
	Send_string("SYSTEME OK !");

	while (1){
		
		if (RI0 == 1){
			RI0 = 0;
			REN0 = 0;
			// Récupération du char dans le buffer
			bit_reception_UART = SBUF0;
			// Ajout du char dans la string commande 
			commande[i] = bit_reception_UART;
			// Incrément du tab commande
			i++;
			// Utile ?
			*(ptr_String_UART) = bit_reception_UART;
			*(ptr_String_UART + 1) = '\0';
			
			//Send_char(bit_reception_UART);
			// Si fin de commande 
			if (bit_reception_UART == '\r'){
				// RAZ pointeur et commande 
				ptr_String_UART = &Lecture_String_UART[0];
				
				// Affichage en console
				Send_char('\n');
				Send_string("Commande recue : ");
				Send_string(commande);
				// Convertion de commande vers struct
				Convertion();
				i = 0;
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
	// But : Envoyer char par char et vérifier si fin de la chaine ou non. 
	// Input : 
	//		- mot : string avec les chars à envoyer
	// Output : 
	//		none
	// Tant que le char n'est pas la fin de la commande ('\r') 
	while (*mot != '\r'){
		// Si p
		if(*(mot+1) == '\0'){	Transmettre(*mot, 1); }
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
			} else { 
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
char* split_element(char* ptr_commande) {
  m = 0;
	while( *ptr_commande != ' ') {
		if (*ptr_commande == '\r') {
			fin_commande = 1; 
			break;
		} else {
			params[m] = *ptr_commande;
			m++;
			ptr_commande++;
		}
	}
	return ptr_commande; 
}

struct argument_complexe param_complexe(char* params) {
	args.param = params[0];
	args.valeur = int_neg_or_positiv(2, params);
	return args;
}

int convertion_str_int(int k, char* params) {
	for(j=k;j<10; j++){
		if(params[j] != '\r' ||params[j] != ' ') {buffer[j-k]=params[j];}
		else { break; }
	}
	return atoi(buffer);		
}

int int_neg_or_positiv(int min, char* params) {
	if (params[min] == '-') {	
		i = convertion_str_int(min+1, params);	
		i = 0-i;
	} else { i = convertion_str_int(min, params); }
	return i; 
} 
void Convertion(void) {
		ptrcommande = &commande;
		ptrcommande = split_element(ptrcommande);
		if (params[0] == 'D' || params[0] == 'E' || params[0] == 'Q') {
				Convertion_Etat(params[0],ptrcommande);
		} else if (params[0] == 'A' || params[0] == 'B' || params[0] == 'S' || params[0] == 'G'|| strcmp(params, "RD") == 0 || strcmp(params, "RG") == 0 || strcmp(params, "RA") == 0) {
				Convertion_Mouvement(params,ptrcommande);
		} else if (strcmp(params, "ASS") == 0) {
				ptrcommande = split_element(ptrcommande);
				i = int_neg_or_positiv(0, params);
				commandeenvoieStA.Vitesse = i;
		} else if (strcmp(params, "MOB") == 0 ) { 
				ptrcommande = split_element(ptrcommande);
				i = int_neg_or_positiv(0, params);
				if (i == 0) {	commandeenvoieStA.Etat_DCT_Obst = DCT_non;}
				else if (i == 1) {commandeenvoieStA.Etat_DCT_Obst = oui_180;}
				else {commandeenvoieStA.Etat_DCT_Obst = oui_360;}
				ptrcommande = split_element(ptrcommande);
				if (params[0] == 'A' ) {
					args = param_complexe(params);
					if (args.valeur > 5 && args.valeur < 45) {commandeenvoieStA.DCT_Obst_Resolution = args.valeur;}
					else { commandeenvoieStA.DCT_Obst_Resolution = 30; }
				} else { commandeenvoieStA.DCT_Obst_Resolution = 30; }
		} else if (strcmp(params, "CS") == 0) {
				Convertion_Servomoteur(ptrcommande);
		} else if (strcmp(params, "MI") == 0 || strcmp(params, "ME") == 0 ) {
				if (strcmp(params, "MI") == 0) {	commandeenvoieStA.Etat_Position = Mesure_I; }
				else { commandeenvoieStA.Etat_Position = Mesure_E;}
		}else if (strcmp(params, "IPO") == 0 || strcmp(params, "POS") == 0) {
				Convertion_Coord(params,ptrcommande);
		} else if (strcmp(params, "L") == 0 && strcmp(params, "LS") == 0 ) {
				Convertion_Lumineux(params, ptrcommande);
		}else if (strcmp(params, "PPH") == 0 && strcmp(params, "SPH") == 0 ) {
				//commande_photo();
		} else {
				//erreur_commande();
		}
} 
   
void Convertion_Etat(char etat, char* ptrcommande) {
	if (etat == "D") {
		ptrcommande = split_element(ptrcommande);
		i = int_neg_or_positiv(0,params);
		if ( i == 1 ) {commandeenvoieStA.Etat_Epreuve = epreuve1;}
		else if ( i == 2 ) {commandeenvoieStA.Etat_Epreuve = epreuve2;}
		else if ( i == 3 ) {commandeenvoieStA.Etat_Epreuve = epreuve3;}
		else if ( i == 4 ) {commandeenvoieStA.Etat_Epreuve = epreuve4;}
		else if ( i == 5 ) {commandeenvoieStA.Etat_Epreuve = epreuve5;}
		else if ( i == 6 ) {commandeenvoieStA.Etat_Epreuve = epreuve6;}
		else if ( i == 7 ) {commandeenvoieStA.Etat_Epreuve = epreuve7;}
		else if ( i == 8 ) {commandeenvoieStA.Etat_Epreuve = epreuve8;}
		//else { commandeenvoieStA.Etat_Epreuve == epreuve1; }
	} else if (etat == "E") {commandeenvoieStA.Etat_Epreuve = Fin_Epreuve; }
	else { commandeenvoieStA.Etat_Epreuve = Stop_Urgence; }
}

void Convertion_Mouvement(char *mouvement, char* ptrcommande) {
	ptrcommande = split_element(ptrcommande);
	if (strcmp(mouvement, "TV") == 0) {
				j = int_neg_or_positiv(0, params);
				if (j > 5 && j< 100) { commandeenvoieStA.Vitesse = j;	}
	} else if (mouvement[0] == 'A'|| mouvement[0] == 'B') {
				// Convertion du param
				j = int_neg_or_positiv(0, params);
				// 1er étape : déinition du mouvement 
				if (mouvement[0] == 'A') { commandeenvoieStA.Etat_Mouvement = Avancer; }
				else { commandeenvoieStA.Etat_Mouvement = Reculer; }
				// 2ème étape : définition de la vitesse	
				if (j > 5 && j<100) {commandeenvoieStA.Vitesse = j;	}
				else {commandeenvoieStA.Vitesse = 20;}
	} else if (mouvement[0]== 'S') {
				commandeenvoieStA.Etat_Mouvement = Stopper;
	} else if (strcmp(mouvement, "RD") == 0) {
				commandeenvoieStA.Etat_Mouvement = Rot_90D;
	} else if (strcmp(mouvement, "RG") == 0) {
				commandeenvoieStA.Etat_Mouvement = Rot_90G;
	} else if (strcmp(mouvement, "RC") == 0) {
				if (params[0] == 'D') { commandeenvoieStA.Etat_Mouvement = Rot_180D; }
				else { commandeenvoieStA.Etat_Mouvement = Rot_180G; }
	} else if (strcmp(mouvement, "RA") == 0) {
				args = param_complexe(params);
				if (args.param == 'D') {commandeenvoieStA.Etat_Mouvement =Rot_AngD; }
				else {commandeenvoieStA.Etat_Mouvement =RotAngG;}
				commandeenvoieStA.Angle = args.valeur; 
	}				
}

void Convertion_Servomoteur(char* ptrcommande) {
	// Valeurs par défauts
	commandeenvoieStA.Servo_Angle = 0;
	commandeenvoieStA.Etat_Servo = Servo_H;
	// Récupération de la commande
	while (fin_commande == 0) {
		ptrcommande = split_element(ptrcommande);
		if (params[0] == 'H') { commandeenvoieStA.Etat_Servo = Servo_H; }
		if (params[0] == 'V') { commandeenvoieStA.Etat_Servo = Servo_V; }
		if (params[0] == 'A') {
			args = param_complexe(params);	
			commandeenvoieStA.Servo_Angle = args.valeur;
		}
	}
}

void Convertion_Coord(char* params,char* ptrcommande) {
	if (strcmp(params, "IPO") == 0) {
		// Valeurs par défaults
		commandeenvoieStA.Etat_Position = Init_Position;
		commandeenvoieStA.Pos_Angle = 0;
		// Boucle pour récupérer les paramètres
		while (fin_commande == 0) {
			// Récupération des paramètres
			ptrcommande = split_element(ptrcommande);
			args = param_complexe(params);
			// Différents cas : 
			if ( strcmp(args.param, "X") == 0) { commandeenvoieStA.Pos_Coord_X = args.valeur; } 
			if ( strcmp(args.param, "Y") == 0) { commandeenvoieStA.Pos_Coord_Y = args.valeur; }
			if ( strcmp(args.param, "A") == 0) { commandeenvoieStA.Pos_Angle = args.valeur; }
		}
	}	else {commandeenvoieStA.Etat_Position = Demande_Position;}			
}

void Convertion_Lumineux(char* coord, char* ptrcommande) {
	if (strcmp(params, "LS") == 0) {
		commandeenvoieStA.Etat_Lumiere = Eteindre;
	} else {
		// Valeurs par défaults
		commandeenvoieStA.Etat_Lumiere = Allumer;
		commandeenvoieStA.Lumiere_Intensite = 100;			
		commandeenvoieStA.Lumiere_Duree = 99;
		commandeenvoieStA.Lumire_Extinction = 0;
		commandeenvoieStA.Lumiere_Intensite = 1;
		// Boucle pour récupérer les paramètres 
		while (fin_commande == 0) {
			// Récupérations et convertions des paramètres 
			ptrcommande = split_element(ptrcommande);
			args = param_complexe(params);
			// Différents cas possibles 
			if ( args.param[0] == 'I') {
				if ( args.valeur > 0 && args.valeur < 101) {
					commandeenvoieStA.Lumiere_Intensite = args.valeur;
				}
			} else if ( args.param[0] == 'D') {
				if ( args.valeur > 0 && args.valeur < 101) {
					commandeenvoieStA.Lumiere_Duree = args.valeur;
				}
			} else if ( args.param[0] == 'E') {
				if ( args.valeur > 0 && args.valeur < 101) {
					commandeenvoieStA.Lumire_Extinction = args.valeur;
				}
			} else if ( args.param[0] == 'N') {
				if ( args.valeur > 0 && args.valeur < 101) {
					commandeenvoieStA.Lumiere_Intensite = args.valeur;
				}
			}
		}
	}
}

void Convertion_Photo(char* params, char* ptrcommande) {
	if (strcmp(params, "PPH") == 0) {
		commandeenvoieStA.Etat_Photo = Photo_1;
		commandeenvoieStA.Photo_Duree = 1;
		commandeenvoieStA.Photo_Nbre = 1;
		while (fin_commande == 0) {
			// Récupérations et convertions des paramètres 
			ptrcommande = split_element(ptrcommande);
			// Différents cas possibles 
			if ( params[0] == 'O') {
				commandeenvoieStA.Etat_Photo = Photo_1;
			}
			if ( params[0] == 'S') {
				commandeenvoieStA.Etat_Photo = Photo_Multiple;
			}
			if ( params[0] == 'E') {
				args = param_complexe(params);
				commandeenvoieStA.Photo_Duree = args.valeur;
			}
			if ( params[0] == 'N') {
				args = param_complexe(params);
				commandeenvoieStA.Photo_Nbre = args.valeur;
			}
		}
	} else {
			commandeenvoieStA.Etat_Photo = Photo_stop;
	}
}
	

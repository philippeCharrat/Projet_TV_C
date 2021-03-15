//-----------------------------------------------------------------------------
// F0-M01.c
//
// Derni�re Modification : 15/03/2021 (aucune erreur d�clar�es et 3 warnings) 
// Auteur : 
// 	- Maxime LERICHE
//  - Philippe CHARRAT
// TODO : 
//  - fonction d'erreur 
//  - Convertion Struct to String 
//  - 
//-----------------------------------------------------------------------------

// Import des biblioth�ques ---
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

// Partie : Envoies et R�ception des messages 
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
void Convertion_S_to_A(void);
void Convertion_Etat(char etat, char* ptrcommande);
void Convertion_Mouvement(char *mouvement, char* ptrcommande);
void Convertion_Servomoteur(char* ptrcommande);
void Convertion_Coord(char* params,char* ptrcommande);
void Convertion_Lumineux(char* params, char* ptrcommande);
void Convertion_Photo(char* params, char* ptrcommande);

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

// D�finition des structures 
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
			// R�cup�ration du char dans le buffer
			bit_reception_UART = SBUF0;
			// Ajout du char dans la string commande 
			commande[i] = bit_reception_UART;
			// Incr�ment du tab commande
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
				Convertion_S_to_A();
				i = 0;
			}
			else{	ptr_String_UART++;}
			
			REN0 = 1;			
		}
	}
}

//-----------------------------------------------------------------------------
// Fonctions de configuration des divers p�riph�riques et interruptions
//-----------------------------------------------------------------------------
void Config_interrupt(){
	//TODO 
}

void Config_UART0(void){
	// But : Configuration de l'UART 0
	SCON0 = (1<<6);
}

void Config_Timer() {
	// But : Configuration du TIMER 2
	RCAP2 = 0xFFDC; //Baud-rate de 19200
	T2CON = (3<<4);
	TR2 = 1; //start timer
}

//-----------------------------------------------------------------------------
// Fonctions UART et d'envoie
//-----------------------------------------------------------------------------


void Reception_chaine_UART(char* ptr_String_UART){
	// But : Recupere le caractere en attente dans SBUF0 et stocke la valeur dans le buffer de commande 
	// Input : 
	//		- *ptr_String_UART : pointeur vers le buffer de commande
	// Output : 
	//		none
	char reception = SBUF0;
	//Lecture du caractere et stockage
	*(ptr_String_UART) = reception;
	//Si on recoit le caractere de fin de chaine c'est qu'on a recu la totalit� de la commande
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
	// But : Fonction pour envoyer une string de mani�re automatique 
	// Input : 
	//		- mot : string avec les chars � envoyer (via pointeur)
	// Output : 
	//		none
	// Tant que le char n'est pas la fin de la commande ('\r') 
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
	// But : Fonction pour envoyer un caract�re dans l'UART
	// Input : 
	//		- c : caract�re � envoyer
	// Output : 
	//		none
	//Desactive reception
	REN0 = 0;
	SBUF0 = c;
	
	//Attente fin de transmission
	while(!TI0){}
		
	//Remise � 0 du flag d'envoi une fois qu'on est sur que le caractere a �t� transmis
	TI0 = 0;
	REN0 = 1;
}

void Transmettre(char caractere, bit LF){
	// But : Fonction pour envoyer un caract�re et la 
	// Input : 
	//		- caractere : char � envoyer
	// Output : 
	//		none
	EA = 0;
	//desactive la reception
	REN0 = 0;
	
	//Ecrit la valeur dans SBUF0 pour transmettre
	SBUF0 = caractere;
	
	//Attente de la bonne transmission
	while(!TI0){}
		
	//Remise � 0 du flag d'envoi une fois qu'on est sur que le caractere a �t� transmis
	TI0 = 0;
	REN0 = 1;
		
	if(LF){Transmettre(0x0D, 0);
	Transmettre(0x0A, 0);} //Retour � la ligne
	EA = 1;
}

//-----------------------------------------------------------------------------
// Fonctions utiles
//-----------------------------------------------------------------------------

char* split_element(char* ptr_commande) {
	// But : R�cup�rer un param�tre (plusieurs char) et les placer dans un tableau globale. La fonction va aussi passer un flag � 1 si la commande est finie ('\r'). 
	// Input : 
	//		- *ptr_commande : pointeur vers le buffer de la commande compl�te. 
	// Output : 
	//		- *ptr_commande : pointeur (incr�ment� des m caract�res parcourus) vers le buffer de la commande compl�te. 
  m = 0;
	// Tant que le char n'est pas un espace 
	while( *ptr_commande != ' ') {
		// Cas : le char est la fin de la commande  
		if (*ptr_commande == '\r') {
			// flag de fin modifi�
			fin_commande = 1;
			break;
			// Cas : char est "quelconque"
		} else {
			// Ajout dans le tableau params 
			params[m] = *ptr_commande;
			m++;
			ptr_commande++;
		}
	}
	// Saut de l'espace 
	ptr_commande++;
	return ptr_commande; 
}

struct argument_complexe param_complexe(char* params) {
	// But : Convertir string en param�tres complexe, forme : param:valeur (exemple : A:12) 
	// Input : 
	//		- *param : pointeur vers la string param�tres 
	// Output : 
	//		- args : structure compos�e de deux champs :
	//					* param : nom du param�tre (1 char) 
	//					* valeur : entier 
	// Remarque : Si param plus de 1 char ? Id�e : boucle jusqu'� ':'
	args.param = params[0];
	args.valeur = int_neg_or_positiv(2, params);
	return args;
}

int int_neg_or_positiv(int min, char* params) {
	// But : Gestion du signe pour la convertion str vers int : ['-','1','2'] => -12
	// Input : 
	//		- min : int pour indiquer le d�but du nombre
	//		- *param : pointeur vers la string contenat les chars 
	// Output : 
	//		- i : int sign� 
	// Cas : int n�gatif 
	if (params[min] == '-') {
		// R�cup�ration de sa valeur absolue 
		i = convertion_str_int(min+1, params);
		// Passage en n�gatif 
		i = 0-i;
		// Cas : int positif
	} else { i = convertion_str_int(min, params); }
	return i; 
} 

int convertion_str_int(int k, char* params) {
	// But : Convertir plusieurs chars vers un int (absolue) : ['1','2'] => 12
	// Input : 
	//		- k : int pour indiquer le d�but du nombre
	//		- *param : pointeur vers la string contenat les chars 
	// Output : 
	//		- atoi(buffer) : int non sign�
	// Remarque : atoi() permet une convertion de char[] en int
	// Boucle jusqu'� 10 (valeur arbitraire) 
	for(j=k;j<10; j++){
		// Si le char est un digit
		if( params[j] == '1' || params[j] == '2'|| params[j] == '3'|| params[j] == '4'|| params[j] == '5'|| params[j] == '6'|| params[j] == '7'|| params[j] == '8'|| params[j] == '9'|| params[j] == '0') {
			buffer[j-k]=params[j];
		}
		else { break; }
	}
	return atoi(buffer);		
}


void Convertion_S_to_A(void) {
	// But : R�cup�rer la partie commande et appeler les fonctions correspondante
	// Input : 
	//		none
	// Output : 
	//		none
	// Remarque : strcmp() permet de chercher une suite de char dans un autre.
	// Remarque : A v�rifier que je n'ai pas oublier de fonction comme le son ou MOU ... 
	// Initialisation d'un pointeur vers la commande.  
	ptrcommande = &commande;
	// R�cup�ration de la partie commande dans le tableau params
	ptrcommande = split_element(ptrcommande);
	// Test des diff�rents cas de figures 
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
			Convertion_Photo(params, ptrcommande);
	} else {
			//erreur_commande();
	}
} 
   
void Convertion_Etat(char etat, char* ptrcommande) {
	// But : Fonction pour les modifications de l'�tat d'�preuve
	// Input : 
	//		- etat : char qui d�finie l'�tat 
	//		- *ptrcommande : pointeur vers les param�tres possibles 
	// Output : 
	//		none
	// Cas D : Commencez �tape   
	if (etat == "D") {
		// Valeur par d�faut 
		commandeenvoieStA.Etat_Epreuve = epreuve1;
		// R�cup�ration et convertion de l'�tape : 
		ptrcommande = split_element(ptrcommande);
		i = int_neg_or_positiv(0,params);
		// Modification de la structure en fonction 
		if ( i == 1 ) {commandeenvoieStA.Etat_Epreuve = epreuve1;}
		else if ( i == 2 ) {commandeenvoieStA.Etat_Epreuve = epreuve2;}
		else if ( i == 3 ) {commandeenvoieStA.Etat_Epreuve = epreuve3;}
		else if ( i == 4 ) {commandeenvoieStA.Etat_Epreuve = epreuve4;}
		else if ( i == 5 ) {commandeenvoieStA.Etat_Epreuve = epreuve5;}
		else if ( i == 6 ) {commandeenvoieStA.Etat_Epreuve = epreuve6;}
		else if ( i == 7 ) {commandeenvoieStA.Etat_Epreuve = epreuve7;}
		else if ( i == 8 ) {commandeenvoieStA.Etat_Epreuve = epreuve8;}
		// Cas E : Fin de l'�preuve 
	} else if (etat == "E") {commandeenvoieStA.Etat_Epreuve = Fin_Epreuve; }
	// Cas Q : Arr�t de l'�preuve
	else { commandeenvoieStA.Etat_Epreuve = Stop_Urgence; }
}

void Convertion_Mouvement(char *mouvement, char* ptrcommande) {
	// But : Fonction de gestion des mouvements
	// Input : 
	//		- *mouvement : pointeur pour connaitre le type de mouvement 
	//		- *ptrcommande : pointeur vers les param�tres possibles 
	// Output : 
	//		none
	// R�cup�ration du premier param�tre
	ptrcommande = split_element(ptrcommande);
	// Cas : Modification vitesse 
	if (strcmp(mouvement, "TV") == 0) {
				j = int_neg_or_positiv(0, params);
				if (j > 5 && j< 100) { commandeenvoieStA.Vitesse = j;	}
	// Cas : Avancer ou reculer
	} else if (mouvement[0] == 'A'|| mouvement[0] == 'B') {
				// Convertion du param
				j = int_neg_or_positiv(0, params);
				// Modification de la structure avec l'action 
				if (mouvement[0] == 'A') { commandeenvoieStA.Etat_Mouvement = Avancer; }
				else { commandeenvoieStA.Etat_Mouvement = Reculer; }
				// Modification de la vitesse
				if (j > 5 && j<100) {commandeenvoieStA.Vitesse = j;	}
				else {commandeenvoieStA.Vitesse = 20;}
	// Cas : Stop le mouvement
	} else if (mouvement[0]== 'S') {
				commandeenvoieStA.Etat_Mouvement = Stopper;
	// Cas : Tourner � droite de 90� 
	} else if (strcmp(mouvement, "RD") == 0) {
				commandeenvoieStA.Etat_Mouvement = Rot_90D;
	// Cas : Tourner � gauche de 90�
	} else if (strcmp(mouvement, "RG") == 0) {
				commandeenvoieStA.Etat_Mouvement = Rot_90G;
	// Cas : Rotation de 180�
	} else if (strcmp(mouvement, "RC") == 0) {
				// Cas : Droite 
				if (params[0] == 'D') { commandeenvoieStA.Etat_Mouvement = Rot_180D; }
				// Cas : Gauche
				else { commandeenvoieStA.Etat_Mouvement = Rot_180G; }
				// Cas : Rotation d'un angle
	} else if (strcmp(mouvement, "RA") == 0) {
				// Valeurs par d�faut 
				commandeenvoieStA.Etat_Mouvement =Rot_AngD;
				commandeenvoieStA.Angle = 90; 
				// Convertion du param en struct
				args = param_complexe(params);
				// Cas : Droite ou Gauche
				if (args.param == 'D') {commandeenvoieStA.Etat_Mouvement =Rot_AngD; }
				else {commandeenvoieStA.Etat_Mouvement =RotAngG;}
				// Ajout de l'angle
				commandeenvoieStA.Angle = args.valeur; 
	}				
}

void Convertion_Servomoteur(char* ptrcommande) {	
	// But : Fonction de gestion des servomoteurs
	// Input : 
	//		- *ptrcommande : pointeur vers les param�tres possibles 
	// Output : 
	//		none
	// Valeurs par d�fauts
	commandeenvoieStA.Servo_Angle = 0;
	commandeenvoieStA.Etat_Servo = Servo_H;
	// Tant que l'on a des param�tres : 
	while (fin_commande == 0) {
		// R�cup�ration des param�tres
		ptrcommande = split_element(ptrcommande);
		// Cas : Servomoteur Horizontale
		if (params[0] == 'H') { commandeenvoieStA.Etat_Servo = Servo_H; }
		// Cas : Servomoteur Verticale
		if (params[0] == 'V') { commandeenvoieStA.Etat_Servo = Servo_V; }
		// Ajout de l'angle
		if (params[0] == 'A') {
			args = param_complexe(params);	
			commandeenvoieStA.Servo_Angle = args.valeur;
		}
	}
}

void Convertion_Coord(char* params,char* ptrcommande) {	
	// But : Fonction de gestion des coordonn�es
	// Input : 
	//		- *params : pointeur vers le type de commande
	//		- *ptrcommande : pointeur vers les param�tres possibles 
	// Output : 
	//		none
	// Cas : Initialisation de coord
	if (strcmp(params, "IPO") == 0) {
		// Valeurs par d�faults
		commandeenvoieStA.Etat_Position = Init_Position;
		commandeenvoieStA.Pos_Angle = 0;
		// Boucle pour r�cup�rer les param�tres
		while (fin_commande == 0) {
			// R�cup�ration des param�tres
			ptrcommande = split_element(ptrcommande);
			args = param_complexe(params);
			// Diff�rents cas : 
			if ( args.param == 'X') { commandeenvoieStA.Pos_Coord_X = args.valeur; } 
			if ( args.param == 'Y') { commandeenvoieStA.Pos_Coord_Y = args.valeur; }
			if ( args.param == 'A') { commandeenvoieStA.Pos_Angle = args.valeur; }
		}
		// Sinon : R�cup�ration de coord
	}	else {commandeenvoieStA.Etat_Position = Demande_Position;}			
}

void Convertion_Lumineux(char* params, char* ptrcommande) {
	// But : Fonction de gestion du pointeur lumineux
	// Input : 
	//		- *mouvement : pointeur pour connaitre le type de commandes 
	//		- *ptrcommande : pointeur vers les param�tres possibles 
	// Output : 
	//		none
	// Cas : Fin  
	if (strcmp(params, "LS") == 0) {
		commandeenvoieStA.Etat_Lumiere = Eteindre;
		// Cas : Initialisation 
	} else {
		// Valeurs par d�faults
		commandeenvoieStA.Etat_Lumiere = Allumer;
		commandeenvoieStA.Lumiere_Intensite = 100;			
		commandeenvoieStA.Lumiere_Duree = 99;
		commandeenvoieStA.Lumire_Extinction = 0;
		commandeenvoieStA.Lumiere_Intensite = 1;
		// Boucle pour r�cup�rer les param�tres 
		while (fin_commande == 0) {
			// R�cup�rations et convertions des param�tres 
			ptrcommande = split_element(ptrcommande);
			args = param_complexe(params);
			// Diff�rents cas possibles 
			if ( args.param == 'I') {
				if ( args.valeur > 0 && args.valeur < 101) {
					commandeenvoieStA.Lumiere_Intensite = args.valeur;
				}
			} else if ( args.param == 'D') {
				if ( args.valeur > 0 && args.valeur < 101) {
					commandeenvoieStA.Lumiere_Duree = args.valeur;
				}
			} else if ( args.param == 'E') {
				if ( args.valeur > 0 && args.valeur < 101) {
					commandeenvoieStA.Lumire_Extinction = args.valeur;
				}
			} else if ( args.param == 'N') {
				if ( args.valeur > 0 && args.valeur < 101) {
					commandeenvoieStA.Lumiere_Intensite = args.valeur;
				}
			}
		}
	}
}

void Convertion_Photo(char* params, char* ptrcommande) {
	// But : Fonction de gestion des photos
	// Input : 
	//		- *params : pointeur pour connaitre le type de commandes 
	//		- *ptrcommande : pointeur vers les param�tres possibles 
	// Output : 
	//		none
	// Cas : Initialisation  
	if (strcmp(params, "PPH") == 0) {
		// Valeurs par d�faut 
		commandeenvoieStA.Etat_Photo = Photo_1;
		commandeenvoieStA.Photo_Duree = 1;
		commandeenvoieStA.Photo_Nbre = 1;
		
		while (fin_commande == 0) {
			// R�cup�rations et convertions des param�tres 
			ptrcommande = split_element(ptrcommande);
			// Diff�rents cas possibles 
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
		// Cas : Fin de photo
	} else {
			commandeenvoieStA.Etat_Photo = Photo_stop;
	}
}
	

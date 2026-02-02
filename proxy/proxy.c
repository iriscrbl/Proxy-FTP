/* Projet C : Proxy FTP

Equipe composée de : 
- OUMERRETANE Emmy
- NGUYEN Phuong 
- CORBILLE Iris

GROUPE B, R3.05
*/

#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdbool.h>
#include "./simpleSocketAPI.h"


#define SERVADDR "127.0.0.1"        // Définition de l'adresse IP d'écoute (locale)
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement(automatiquement)
#define LISTENLEN 1                 // Taille de la file des demandes de connexion (nombre max de clients en attente)
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données (taille max des messages échangés)
#define MAXHOSTLEN 64               // Taille d'un nom de machine (nom du serveur)
#define MAXPORTLEN 64               // Taille d'un numéro de port
#define MAXLOGINLEN 64              // Taille d'un login

/*
Cette fonction gère UN client FTP : 
sockServeur : scoket de communication avec le client
serverPortFTP : port du Serveur FTP
*/
int gererClient(int sockClient, char *serverPortFTP);

int main(int argc, char* argv[]){
    int ecode;                          // Code retour des fonctions
    char adresseServeur[MAXHOSTLEN];    // Adresse du serveur
    char portServeur[MAXPORTLEN];       // Port du server
    int sockServeur;                    // Descripteur de socket de rendez-vous
    int sockClient;                     // Descripteur de socket de communication
    struct addrinfo hints;              // Contrôle la fonction getaddrinfo
    struct addrinfo *res;               // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo;     // Informations sur la connexion de RDV
    struct sockaddr_storage from;       // Informations sur le client connecté
    socklen_t len;                      // Variable utilisée pour stocker les longueurs des structures de socket
    
    char buffer[MAXBUFFERLEN];          // Tampon de communication entre le client et le serveur

    // création de la socket serveur IPv4/TCP : socket(int domain, int type, int protocol)
    sockServeur = socket(AF_INET, SOCK_STREAM, 0);
    // -1 = erreur durant la création du socket
    // on sort du programme s'il y a une erreur de création de socket
    if (sockServeur == -1) {
         perror("Erreur création socket RDV\n");
         exit(2);
    }

    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initialisation de hints
    hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par la fonction getaddrinfo

    // Récupération des informations du serveur
    // SERVADDR : adresse du serveur (nom DNS ou IP)
    // SERVPORT : port d'écoute
    // &hints : préférences (IPv4, IPv6, TCP, etc.)
    // &res : pointeur vers la liste des résultats (rempli par la fonction)
    ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);

    // en cas d'erreur, un message s'affiche
    // on sort du programme 
    if (ecode) {
        fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
        exit(1);
    }

    // Publication de la socket
    // associe une adresse IP et un port définis dans &res
    // ai_addr : adresse IP + port
    // ai_addrlen : taille de la structure
    ecode = bind(sockServeur, res->ai_addr, res->ai_addrlen);

    // -1 = erreur lors de la liaison de la socket du serveur
    // on sort du programme
    if (ecode == -1) {
        perror("Erreur liaison de la socket de RDV");
        exit(3);
    }

    // Nous n'avons plus besoin de cette liste chainée addrinfo
    // adresse déjà utilisée lors du bind() donc plus nécessaire
    freeaddrinfo(res);

    // Récupération du nom de la machine et du numéro de port pour affichage à l'écran
    len = sizeof(struct sockaddr_storage);
    // récupère l'adresse locale associée à la socket (IP et port)
    // &myinfo : buffer qui reçoit l'adresse locale de la socket
    // &len : taille du buffer de l'adresse
    getsockname(sockServeur, (struct sockaddr *) &myinfo, &len);

    // transforme une adresse binaire en chaîne de caractères
    getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo),
                adresseServeur, MAXHOSTLEN,
                portServeur, MAXPORTLEN,
                NI_NUMERICHOST | NI_NUMERICSERV);

    // affichage de l'adresse d'écoute
    printf("L'adresse d'ecoute est: %s\n", adresseServeur);
    // affichage du port d'écoute
    printf("Le port d'ecoute est: %s\n", portServeur);

    // Attente connexion du client
    // LISTENLEN : taille de la file d'attente des connexions (nombre max de clients en attente avant accept())
    listen(sockServeur, LISTENLEN);



    // ==== BOUCLE QUI PERMETS DE GÉRER PLUSIEURS CLIENTS ====

    while (1) {
        // indique la taille du buffer pour stocker l'adresse du client
        len = sizeof(struct sockaddr_storage);
        // Lorsque demande de connexion, creation d'une socket de communication avec le client
        // connexion du client au serveur
        // &from : reçoit l'adresse IP + port du client
        sockClient = accept(sockServeur, (struct sockaddr *) &from, &len);

        // crée un nouevau processus pour gérer plusieurs clients en même temps
        pid_t pid = fork();

        // 0 = processus fils créé
        // on ferme le serveur (inutile)
        // on traîte la communication client (port d'écoute en paramètre)
        // on sort du programme
        if (pid == 0) {
            close(sockServeur);
            gererClient(sockClient, argv[1]);
            exit(0);
        // X processus père, on ferme la communication
        // retour sur accept() pour le client suivant 
        } else {
            close(sockClient);
        }
    }
}

//------------------
// Fonction gererClient
//------------------

/*
Cette fonction gère UN client FTP : 
sockServeur : scoket de communication avec le client
serverPortFTP : port du Serveur FTP
*/
int gererClient(int sockClient, char *serverPortFTP) {
    char buffer[MAXBUFFERLEN];      // boîte pour stocket ce qui arrive du réseau
    char login[MAXLOGINLEN];        // stocke le nom d'utilisateur FTP
    char serveur[MAXHOSTLEN];       // stocke le nom du serveur FTP
    int ecode;                      // stocke le code du résultat de read/write
    int sockServeur;                // socket serveur FTP
    int descSockDataServer = -1;    // socket DATA proxy -> serveur FTP
    int descSockDataClient = -1;    // socket DATA proxy -> client FTP


    // ======= 1ERE ETAPE : CONNEXIONS DE CONTRÔLE CLIENT -> PROXY =======

    // affiche PID du processus qui gère ce client
    printf("(PROXY) Client connecté : PID = %d)\n", getpid());

    // copie et envoi du message "Bienvenue" de code 220
    strcpy(buffer, "220 Bienvenue\r\n");
    // lit ce que le client envoi (anonymous@ftp.fr.debian.org)
    write(sockClient, buffer, strlen(buffer));
     
    // vide le buffer
    memset(buffer, 0, MAXBUFFERLEN);
    ecode = read(sockClient, buffer, MAXBUFFERLEN);       
    
    // -1 = echec de la lecture
    // on sort du programme si la lecture a échoué
    if (ecode == -1) {
        perror("Impossible de lire les données de l'utilisateur.\n");
        exit(1);
    }
    
    // affichage de la commande reçue du client
    printf("(PROXY) commande reçue : %s\n", buffer);

    // 1) extraction de la commande (anonymous@ftp.fr.debian.org)
    // vérifie si la commande est valide
    if (sscanf(buffer, "USER %63s", login) != 1) {
        printf("(PROXY) commande USER invalide\n");
        exit(1); 
    }
    
    // 2) séparation du login et du serveur
    // cherche le caractère "@"
    // vérifie le format
    // on sort du programme si le format est mauvais
    // at pointant sur l'index du "@" dans le login
    char *at = strchr(login, '@');  
    if (at == NULL) {
        printf("(PROXY) format attendu : USER login@serveur\n");
        exit(1);
    }

    // copie ce qui suit "@" (ftp.fr.debian.org) dans le serveur
    strncpy(serveur, at + 1, MAXHOSTLEN - 1); 
    // \0 = met fin à la chaîne 
    serveur[MAXHOSTLEN - 1] = '\0';
    // coupe la chaîne au niveau du "@"
    // le login (anonymous@ftp.fr.debian.org) ne contient à présent que (anonymous)
    *at = '\0';

    // affiche le login (anonymous)
    printf("(PROXY) login = %s\n", login);
    // affiche le serveur (ftp.fr.debian.org)
    printf("(PROXY) serveur = %s\n", serveur);
        
    /*
    3)Maintenant qu'on a les login et serveur, on peut les utiliser pour connecter le client au serveur
    */

    // récupère le port du serveur FTP (distant)
    char *portServeur = serverPortFTP;

    printf("(PROXY) Tentative de connexion au serveur %s:%s...\n", serveur, portServeur);
    ecode = connect2Server(serveur, portServeur, &sockServeur);
    
    // -1 = echec de la connexion
    // on déconnecte le client
    // on sort du programme si la connexion a échoué
    if (ecode == -1) {
        printf("(PROXY) Erreur: impossible de se connecter au serveur.\n");
        close(sockClient);
        exit(9);
    }
    
    // connexion réussie
    printf("(PROXY) Connecté au serveur.\n");
    
    
    // vide le buffer
    memset(buffer, 0, MAXBUFFERLEN);
    // lit le message du serveur FTP
	ecode = read(sockServeur, buffer, MAXBUFFERLEN);
	if (ecode > 0) {
	    printf("(PROXY) Message de bienvenue du serveur: %s", buffer);
	}

    // 4) construction de la commande USER pour le serveur
    sprintf(buffer, "USER %s\r\n", login);
    // affichage de ce qui est envoyé (login)
    printf("(PROXY) Envoi au serveur: %s", buffer);

    // 5) envoi de la commande USER au serveur FTP
    write(sockServeur, buffer, strlen(buffer));
    // vide le buffer
    memset(buffer, 0, MAXBUFFERLEN);
    
    // 6) lit la réponse du serveur FTP
    ecode = read(sockServeur, buffer, MAXBUFFERLEN);
    if (ecode > 0) {
        printf("(PROXY) Réponse serveur: %s", buffer);
        // Relayer la réponse au client
        write(sockClient, buffer, ecode);
    }
        
    // boucle infinie : en attente de commande de la part du client (tant que la connexion est en cours)
    while (1) {
        // vide le buffer
        memset(buffer, 0, MAXBUFFERLEN);
        // lit la commande du client
        // stocke le résultat dans le buffer
        ecode = read(sockClient, buffer, MAXBUFFERLEN);

        // 0 = close(), le client s'est déconnecté
        // on sort de la boucle
        if (ecode == 0) {
            printf("(PROXY) Le client est déconnecté");
            break;
        // -1 = erreur dans la lecture des données
        // on sort de la boucle
        } else if (ecode == -1) {
            perror("Erreur lors de la lecture des données");
            break;
        }

        // affichage de la commande du client
        printf("(PROXY) Client: %s", buffer);



        //  ======= 2ERE ETAPE : CONNEXION DE DONNEES CLIENT (ACTIF) -> PROXY <- SERVEUR (PASSIF) =======

        // Si commande PORT, la transformer en PASV
        // vérifie si la commande reçue est "PORT"
        if (strncmp(buffer, "PORT", 4) == 0) {

 

            // 1. Parsing de la commande PORT pour extraire l'IP et le Port du client
            // affichage de la commande reçue 
            printf("(PROXY) Commande reçue : %s\n", buffer);

            // FTP : la commande "PORT" est de la forme <PORT h1, h2, etc.> (PORT 15,205,66,7,195,80 par exemple)
            int h1, h2, h3, h4, p1, p2;

            // extrait les 6 nombres :
            // h1.h2.h3.h4 : adresse IP du client
            // p1, p2 : port du client
            // s'il n'y a pas 6 valeurs, erreur de syntaxe de code 501
            // on attend la commande suivante
            sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);

            // calcul du port client (p1=195, p2=80 : port = 195*256+80 = 50000)
            int clientPort = p1 * 256 + p2;

            char clientIP[16];
            char portClientStr[16];
    
            // création de l'IP du client et du port à partir des informations reçues
            sprintf(clientIP, "%d.%d.%d.%d", h1, h2, h3, h4);
            sprintf(portClientStr, "%d", clientPort);

            // affichage de l'IP et port client
            printf("(PROXY) IP client: %s, port client: %d\n", clientIP, clientPort);

            // 2.  Connection proxy -> client 

            // permet au proxy de se connecter au client vers IP et port du client
            // -1 = échec de la connexion
            // on passe à la prochaine commande
            ecode = connect2Server(clientIP, portClientStr, &descSockDataClient);
            if (ecode == -1) {
                perror("Erreur connexion données client");
                write(sockClient, "425 Can't open data connection\r\n", 33);
                continue;      //nous fais revenir au début de la boucle while(1)
            }

            printf("(PROXY) Connecté au client\n");

            // 3. demande PASV au serveur pour se connecter
            write(sockServeur, "PASV\r\n", 6);
            printf("(PROXY) Envoi au serveur: PASV\r\n");   

            // 4. lit la réponse PASV du serveur (227 Entering Passive Mode ...)
            // vide le buffer
            memset(buffer, 0, MAXBUFFERLEN);
            // lit les réponses du serveur
            ecode = read(sockServeur, buffer, MAXBUFFERLEN);

            // <= 0 = erreur système 
            // on sort de la boucle
			if (ecode <= 0) {
				perror("Erreur lecture PASV");
				break;
			}


            // affichage de la réponse du serveur
            printf("(PROXY) Réponse PASV du serveur: %s", buffer);


            // 6. Parser la réponse 227 pour extraire l'IP et le port du serveur
            int sh1, sh2, sh3, sh4, sp1, sp2;

            // extrait les 6 nombres : adresse IP et port du serveur
            sscanf(buffer,"227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &sh1, &sh2, &sh3, &sh4, &sp1, &sp2);
            
            // calcul du port du serveur
            int portServeurData = sp1 * 256 + sp2;
            
            // création de l'IP du serveur
            char serverIP[16];
            char portStr[16];

            // passer l'IP et le port composés d'entiers en des chaînes de caractères
            sprintf(serverIP, "%d.%d.%d.%d", sh1, sh2, sh3, sh4);
            sprintf(portStr, "%d", portServeurData);

            // affichage de l'IP et du port du serveur
            printf("(PROXY) IP serveur: %s, port serveur: %d\n", serverIP, portServeurData);

            // permet au proxy de se connecter au serveur vers IP et port du serveur
            // -1 = échec de la connexion
            // on passe à la prochaine commande
            ecode = connect2Server(serverIP, portStr, &descSockDataServer);     
            if (ecode == -1) {
                perror("Erreur connexion données serveur");
                close(descSockDataClient);
                descSockDataClient = -1;
                write(sockClient, "425 Can't open data connection\r\n", 33);
                continue;
            }
            
            printf("(PROXY) Connecté au serveur\n");

            // envoi au client de la réussite de la commande passée
            write(sockClient, "200 PORT command successful\r\n", 30);
            continue;
        }


        // compare les 4 premiers caractères
        // LIST : liste des fichiers 
        else if (strncmp(buffer, "LIST", 4) == 0) {
			
			// envoi de la commande reçue du client au serveur FTP
			write(sockServeur, buffer, ecode);
			
			//lire la réponse 150
			// vide le buffer
            memset(buffer, 0, MAXBUFFERLEN);
			// lit la réponse du serveur 
			// récupère le nombre d'octets lus
            ecode = read(sockServeur, buffer, MAXBUFFERLEN);
            if (ecode <= 0) {
                perror("Erreur lecture 150");
                break;
            }
			
			
			// affichage de la réponse du serveur
			printf("(PROXY) Réponse serveur: %s", buffer);

			// envoi de la réponse au client
			write(sockClient, buffer, ecode);
			

			//transfert des données
			printf("(PROXY) Transfert de données entre serveur et client\n");

            
            char dataBuffer[MAXBUFFERLEN];
            int n;

            // lecture des données du serveur
            
            // tant que le serveur envoie des données, on les lit et on les envoie au client
			while ((n = read(descSockDataServer, dataBuffer, MAXBUFFERLEN)) > 0) {
                write(descSockDataClient, dataBuffer, n);
            }
			
			printf("(PROXY) Transfert terminé\n");

			//fermeture des sockets de données
            close(descSockDataServer);
            close(descSockDataClient);
            descSockDataServer = -1;
            descSockDataClient = -1;

            // Lire la réponse finale du serveur (226 Transfer complete)
            memset(buffer, 0, MAXBUFFERLEN);
            ecode = read(sockServeur, buffer, MAXBUFFERLEN);
            if (ecode > 0) {
                printf("(PROXY) Réponse serveur: %s", buffer);
                write(sockClient, buffer, ecode);
            }
			
            
			continue;
        
        }
		
		// ===== AUTRES COMMANDES =====
		else {
			
			// Envoyer au serveur
            write(sockServeur, buffer, ecode);
            
            // Lire la réponse
            memset(buffer, 0, MAXBUFFERLEN);
            ecode = read(sockServeur, buffer, MAXBUFFERLEN);
            if (ecode <= 0) {
                perror("Erreur lecture réponse serveur");
                break;
            }
            
            printf("(PROXY) Serveur: %s", buffer);
            
            // Envoyer au client
            write(sockClient, buffer, ecode);
        }

        // Vérifier QUIT
        if (strncmp(buffer, "221", 3) == 0) {
            printf("(PROXY) Client a fait QUIT, fermeture\n");
            break;
        }
    }


    // fermeture des sockets client et serveur
    // fin du programme
    if (descSockDataServer != -1) {
        close(descSockDataServer);
    }
    if (descSockDataClient != -1) {
        close(descSockDataClient);
    }
    close(sockClient);
    close(sockServeur);
    printf("(PROXY) Client fermé (PID %d)\n", getpid());
    exit(0);
}

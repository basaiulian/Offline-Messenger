/* cliTCPIt.c - Client TCP pentru Offline Messenger
   Trimite o comanda la server; primeste de la server raspuns; in caz de succes, va introduce un username pentru logare/inregistrare, in functie de comanda introdusa.
         
   Autor: Iulian-Danut Basa  <iulian.basa@info.uaic.ro>
*/
#include <sys/types.h>

#include <sys/socket.h>

#include <netinet/in.h>

#include <errno.h>

#include <unistd.h>

#include <stdio.h>

#include <stdlib.h>

#include <netdb.h>

#include <string.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;
int port; //portul de conectare la server
int sd; // descriptorul de socket
struct sockaddr_in server; // structura folosita pentru conectare 
char msg[100]; // mesajul
char cmd[100]; // comanda trimisa
char cmd2[100];
char sendUser[25];
char sendPwd[18];
char receiver[25];
char message[250]; //MESAJUL DE TRIMIS CATRE ALT UTILIZATOR
char rcvCheck[1000];
void corrString(char string[300]) {
  char * pstr;
  pstr = strstr(string, "\n");
  if (pstr != NULL) { //daca am gasit o pozitie pentru \n, inlocuiesc aparitia sa cu ""
    strncpy(pstr, "", 1);
  }
}

int logRegUser() { //LOGIN DUPA REGISTER
  printf("\nM-am inregistrat, acum fac login.\n\n");

  if (read(sd, msg, 100) <= 0) {
    perror("[client]Eroare la read() de la server.\n");
    return errno;
  }

  /* afisam mesajul primit */
  printf("%s\n", msg);
  bzero(sendUser, 25);
  fflush(stdout);
  read(0, sendUser, 25);
  if (write(sd, sendUser, 25) <= 0) {
    perror("[client]Eroare la write() spre server.\n");
    return errno;
  } else
    printf("[client]Username-ul introdus este: %s \n", sendUser);
  if (read(sd, msg, 100) <= 0) {
    perror("[client]Eroare la read() de la server.\n");
    return errno;
  } else printf("%s\n", msg);

  bzero(sendPwd, 18);
  fflush(stdout);
  read(0, sendPwd, 18);
  if (write(sd, sendPwd, 18) <= 0) {
    perror("[client]Eroare la write() spre server.\n");
    return errno;
  } else
    printf("[client]Password-ul introdus este: %s \n", sendPwd);

  /*citesc raspunsul dat de server la incercarea de login*/
  if (read(sd, msg, 100) <= 0) {
    perror("[client]Eroare la read() de la server.\n");
    return errno;
  }
  int status;
  if (strcmp(msg, "[server]Te-ai logat cu succes.") == 0) {
    printf("%s\n", msg);
    return 1;
  } else {
    printf("[server]Nu te-ai logat.");
    return 0;
  }

}

int loginUser() {
  if (write(sd, cmd, 100) <= 0) {
    perror("[client]Eroare la write() spre server.\n");
    return errno;
  }
  printf("%s\n", cmd);
  /* citirea raspunsului dat de server 
          apel blocant pina cind serverul raspunde) */
  if (read(sd, msg, 100) <= 0) {
    perror("[client]Eroare la read() de la server.\n");
    return errno;
  }

  /* afisam mesajul primit */
  printf("%s\n", msg);
  bzero(sendUser, 25);
  fflush(stdout);
  read(0, sendUser, 25);
  if (write(sd, sendUser, 25) <= 0) {
    perror("[client]Eroare la write() spre server.\n");
    return errno;
  } else
    printf("[client]Username-ul introdus este: %s \n", sendUser);
  if (read(sd, msg, 100) <= 0) {
    perror("[client]Eroare la read() de la server.\n");
    return errno;
  } else printf("%s\n", msg);

  bzero(sendPwd, 18);
  fflush(stdout);
  read(0, sendPwd, 18);
  if (write(sd, sendPwd, 18) <= 0) {
    perror("[client]Eroare la write() spre server.\n");
    return errno;
  } else
    printf("[client]Password-ul introdus este: %s \n", sendPwd);

  /*citesc raspunsul dat de server la incercarea de login*/
  if (read(sd, msg, 100) <= 0) {
    perror("[client]Eroare la read() de la server.\n");
    return errno;
  }
  int status;
  if (strcmp(msg, "[server]Te-ai logat cu succes.") == 0) {
    printf("%s\n", msg);
    return 1;
  } else {
    printf("[server]Nu te-ai logat.\n");
    return 0;
  }
  return 1;
}

int logoutUser() {
  if (read(sd, msg, 100) <= 0) {
    perror("[client]Eroare la read(logout).\n");
    return errno;
  } else printf("%s\n", msg);
}

int registerUser() {
  /*scriu comanda*/
  if (write(sd, cmd, 100) <= 0) {
    perror("[client]Eroare la write() spre server.\n");
    return errno;
  }
  printf("%s\n", cmd);
  if (read(sd, msg, 100) <= 0) {
    printf("[client]Eroare la read(cerere username).\n");
    return 0;
  } else {
    printf("[client]Am primit mesajul: %s.\n", msg);
  }
  /*trebuie sa scriu un username*/
  bzero(sendUser, 25);
  fflush(stdout);
  read(0, sendUser, 25);
  if (write(sd, sendUser, 25) <= 0) {
    perror("[client]Eroare la write(username) spre server.\n");
    return 0;
  } else {
    printf("Username-ul introdus este: %s\n", sendUser);
  }
  if (read(sd, msg, 100) <= 0) {
    printf("[client]Eroare la read(cerere password).\n");
    return 0;
  } else {
    printf("[client]Am primit mesajul: %s.\n", msg);
  }
  /*trebuie sa scriu password*/
  bzero(sendPwd, 18);
  fflush(stdout);
  read(0, sendPwd, 18);
  if (write(sd, sendPwd, 18) <= 0) {
    perror("[client]Eroare la write(password) spre server.\n");
    return 0;
  } else {
    printf("Password-ul introdus este: %s\n", sendPwd);
  }

  /*citesc raspunsul la incercarea de inregistrare*/
  bzero(msg, 100);
  if (read(sd, msg, 100) <= 0) {
    perror("[client]Eroare la read(raspuns inregistrare).\n");
    return 0;
  } else {
    if (strcmp(msg, "Te-ai inregistrat cu succes!") == 0) {
      printf("[client]M-am inregistrat.\n");
      return 1;
    } else {
      printf("[client]Nu m-am inregistrat.\n");
      return 0;
    }
  }

}

int main(int argc, char * argv[]) {

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3) {
    printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    return -1;
  }

  /* stabilim portul */
  port = atoi(argv[2]);

  /* cream socketul */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Eroare la socket().\n");
    return errno;
  }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons(port);

  /* ne conectam la server */
  if (connect(sd, (struct sockaddr * ) & server, sizeof(struct sockaddr)) == -1) {
    perror("[client]Eroare la connect().\n");
    return errno;
  }

  int end=0;
  /* citirea mesajului */
  bzero(cmd, 100);
  printf("Pentru a va conecta folositi comanda: login \n");
  printf("Pentru a va inregistra folositi comanda: register \n");
  printf("[client]Introduceti o comanda: ");
  fflush(stdout);
  read(0, cmd, 100);
  corrString(cmd);
  if (strcmp(cmd, "login") == 0) {
    /* trimiterea comenzii la server */
    
      if (loginUser() == 1) {
	int ok1=0,ok2=0;
	printf("Te-ai conectat cu succes.\n");
	
	bzero(rcvCheck, 1000);
	  if(read(sd,rcvCheck,1000)<=0){
		perror("[client]Eroare la read(mesaje offline)\n");
		return errno;
	  } else {
		printf("Caut mesajele primite cat timp ai fost offline.\n");
		printf(".");fflush(stdout);sleep(1);printf(".");fflush(stdout);sleep(1);printf(".");sleep(1);fflush(stdout);			
                printf("\nMesajele necitite sunt:\n %s \n", rcvCheck); }
	while((ok1==0 || ok2==0) && end!=1 ){
      printf("Comenzi disponibile:\n");
      printf("writeto | history | reply\n");
      printf("logout | help\n");
      bzero(cmd2, 100);
      read(0, cmd2, 100);

      if (write(sd, cmd2, 100) <= 0) {
        perror("[client]Eroare la scrierea comenzii.\n");
        return errno;
      } else {
        printf("[client]Am scris comanda catre server.\n");
      }
      corrString(cmd2);

      if (strcmp(cmd2, "help") == 0) {
        printf("\n writeto => trimite mesaj \n history => verifica istoricul mesajelor cu un utilizator");
        printf("\n reply => raspunde unui anumit mesaj \n logout => deconecteaza-te de la server \n delacc => sterge-ti contul \n");

      } else if (strcmp(cmd2, "writeto") == 0) {
        if (read(sd, msg, 100) <= 0) {
          perror("[client]Eroare la citire(raspuns writeto)\n");
          return errno;
        } else {
          printf("%s\n", msg);

        }
        bzero(receiver, 25);
        read(0, receiver, 25);
        corrString(receiver);
        if (write(sd, receiver, 25) <= 0) {
          perror("[client]Eroare la write(destinatar).\n");
          return errno;
        } else {

          printf("[client]Am scris destinatarul cu succes(%s).\n", receiver);
	  if(strcmp(receiver,"-end")==0)
		{ok1=1; end=1;}
        }
	if(ok1==0){
        //am trimis destinatarul, trimit mesajul
        if (read(sd, msg, 100) <= 0) {
          perror("[client]Eroare la citire.\n");
          return errno;
        } else {
          printf("%s\n", msg);
        }
        bzero(message, 250);
        read(0, message, 250);
        corrString(message);
        if (write(sd, message, 250) <= 0) {
          perror("[client]Eroare la scrierea mesajului.\n");
          return errno;
        } else {
          printf("Am scris mesajul.\n");
        }
	}
      } /*else if (strcmp(cmd2, "check") == 0) {
	  printf("SUNT IN CHECK BAAA");
          bzero(rcvCheck, 1000);
	  if(read(sd,msg,100)<=0){
		perror("[client]Eroare la read(raspuns check)\n");
		return errno;
	  } else printf("%s\n",msg); 
          if (read(sd, rcvCheck, 1000) <= 0) {
            perror("[client]Eroare la read(rcvCheck).\n");
            return errno;
          } else {
            printf("Mesajele necitite sunt:\n %s \n", rcvCheck);
          }

        }*/ else if (strcmp(cmd2, "delacc") == 0) {
          printf("Am sters contul. M-am deconectat.\n");
          end=1; continue;

        } else if (strcmp(cmd2, "logout") == 0) {
          	printf("M-am deconectat.\n"); end=1; continue;
        } else {printf("[EROARE] Comanda %s nu exista. \n",cmd2);
      }
	
    }

}
    /*aici inchid loginul*/
  } else if (strcmp(cmd, "register") == 0) {
    /*trimiterea comenzii la server*/

    if (registerUser() == 1) {

      if (logRegUser() == 1) {
	bzero(rcvCheck, 1000);
	  if(read(sd,msg,100)<=0){
		perror("[client]Eroare la read(raspuns check)\n");
		return errno;
	  } else printf("%s\n",msg); 
          if (read(sd, rcvCheck, 1000) <= 0) {
            perror("[client]Eroare la read(rcvCheck).\n");
            return errno;
          } else {
            printf("Mesajele necitite sunt:\n %s \n", rcvCheck);
	}
	while(1){
        printf("Esti logat. Poti folosi:\n");
        printf("writeto | check | history | reply\n");
        printf("logout | help\n");
        bzero(cmd2, 100);
        read(0, cmd2, 100);

        if (write(sd, cmd2, 100) <= 0) {
          perror("[client]Eroare la scrierea comenzii.\n");
          return errno;
        } else {
          printf("[client]Am scris comanda catre server.\n");
        }
        corrString(cmd2);

        if (strcmp(cmd2, "help") == 0) {
          printf("\n writeto => trimite mesaj \n check => verifica daca ai mesaje necitite \n history => verifica istoricul mesajelor cu un utilizator");
          printf("\n reply => raspunde unui anumit mesaj \n logout => deconecteaza-te de la server \n delacc => sterge-ti contul \n");

        } else if (strcmp(cmd2, "writeto") == 0) {
          if (read(sd, msg, 100) <= 0) {
            perror("[client]Eroare la citire(raspuns writeto)\n");
            return errno;
          } else {
            printf("%s\n", msg);

          }
          bzero(receiver, 25);
          read(0, receiver, 25);
          corrString(receiver);
          if (write(sd, receiver, 25) <= 0) {
            perror("[client]Eroare la write(destinatar).\n");
            return errno;
          } else {

            printf("[client]Am scris destinatarul cu succes(%s).\n", receiver);
	    
          }
          //am trimis destinatarul, trimit mesajul
          if (read(sd, msg, 100) <= 0) {
            perror("[client]Eroare la citire.\n");
            return errno;
          } else {
            printf("%s\n", msg);
          }
          bzero(message, 250);
          read(0, message, 250);
          corrString(message);
          if (write(sd, message, 250) <= 0) {
            perror("[client]Eroare la scrierea mesajului.\n");
            return errno;
          } else {
            printf("Am scris mesajul.\n");
          }
          if (read(sd, msg, 100) <= 0) {
            perror("[client]Eroare la citire.\n");
            return errno;
          } else {
            printf("[server]%s\n", msg);
          }
        } /*else if (strcmp(cmd2, "check") == 0) {
	  printf("SUNT IN CHECK BAAA");
          bzero(rcvCheck, 1000);
	  if(read(sd,msg,100)<=0){
		perror("[client]Eroare la read(raspuns check)\n");
		return errno;
	  } else printf("%s\n",msg); 
          if (read(sd, rcvCheck, 1000) <= 0) {
            perror("[client]Eroare la read(rcvCheck).\n");
            return errno;
          } else {
            printf("Mesajele necitite sunt:\n %s \n", rcvCheck);
          }*/

         else if (strcmp(cmd2, "delacc") == 0) {
          bzero(msg, 100);
          if (read(sd, msg, 100) <= 0) {
            perror("[client]Eroare la citire(raspuns delAcc.\n");
            return errno;
          } else {printf("%s\n", msg); /*continue;*/}

        } else if (strcmp(cmd2, "logout") == 0) {
          bzero(msg, 100);
          if (read(sd, msg, 100) <= 0) {
            perror("[client]Eroare la citire(raspuns logout.\n");
            return errno;
          } else {printf("%s\n", msg); /*continue;*/ }
        }
      }
     }
    }
	

  }


	
  /* inchidem conexiunea, am terminat */
  close(sd);
}


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <sqlite3.h>
#define PORT 1801

extern int errno; // codul de eroare returnat de anumite apeluri 
struct sockaddr_in server;
struct sockaddr_in from;
char recUser[25];
char recPwd[18];
char servCmd[100];//comanda server
char cmd[100];//comanda trimisa de client catre procesul copil
char cmd2[100];
char rsp[250];//raspunsul primit de client de la procesul copil
char msg[250];//MESAJUL TRIMIS
char receiver[25];
char sender[25];
int sd;//socket
int client;
int length=sizeof(from);
pid_t child;
int changes;




/*preluat de pe net*/
   sqlite3 *db;
   char *zErrMsg = 0;
   sqlite3_stmt *stmt;
   int rc;
   char *sql;
   char sqlCmd[250];

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}
/*sfarsit preluat de pe net */


void corrString(char string[300]){
		char *pstr;//va pointa la pozitia primei aparitii a "\n"
		pstr=strstr(string,"\n");
		if(pstr!=NULL){//daca am gasit o pozitie pentru \n, inlocuiesc aparitia sa cu ""
			strncpy(pstr,"",1);
		}
}

void child_handler(){
	time_t rawtime;
	struct tm * timeinfo;
	time (&rawtime);
	timeinfo=localtime(&rawtime);
	char lastActive[100];
	strcpy(lastActive,asctime(timeinfo));
	corrString(lastActive);
	/*fac update in baza de date la lastactive*/
	printf("[server]Un proces copil si-a terminat executia la ora %s.\n",lastActive);
	printf("---------------------------------------------------------------------\n");
	wait(NULL);
}

int loginUser(){
	
		
		printf("[server]Clientul a solicitat sa se logheze.\n");
	//pregatim mesajul de raspuns
      		bzero(rsp,100);//curat
      		strcpy(rsp,"[server]Astept username pentru login...\n");

		printf("[server]Trimitem mesajul inapoi...\n");
      
 	// returnam mesajul clientului
     	 	if (write (client, rsp, 100) <= 0){

	  		perror ("[server]Eroare la write() catre client.\n");
	  		return 0; 
		}
      		else
			printf ("[server]Mesajul a fost transmis cu succes.\n");
	
		if(read(client,recUser,100)<=0){
			perror("[server-login]Eroare la primirea username-ului.\n");
			close(client);
			return 0;
		
		}
		else    {	
	//corectez username-ul, recUser avand la final "\n"
			corrString(recUser);
			printf("[server-login]Username-ul primit este: %s.\n",recUser);

		bzero(rsp,250);
		strcpy(rsp,"[server]Astept parola pentru login...\n");

		printf("[server]Trimitem mesajul inapoi...\n");
      
 	// returnam mesajul clientului
     	 	if (write (client, rsp, 100) <= 0){

	  		perror ("[server]Eroare la write() catre client.\n");
	  		return 0; 
		}
      		else
			printf ("[server]Mesajul a fost transmis cu succes.\n");
	
		if(read(client,recPwd,18)<=0){
			perror("[server-login]Eroare la primirea password-ului.\n");
			close(client);
			return 0;
		
		}
		else    {	
	//corectez password-ul, recPwd avand la final "\n"
			corrString(recPwd);
			printf("[server-login]Password-ul primit este: %s.\n",recPwd);

		bzero(rsp,100);
	

	//BD
   		rc = sqlite3_open("users.db", &db);
   
   		if( rc ) {
      			printf("Can't open database: %s\n", sqlite3_errmsg(db));
      			return(0);
   		}
	 
   		else {
			char sqlCmd[250];
        		printf("Opened database successfully\n");
			bzero(sqlCmd,250);
			sprintf(sqlCmd,"UPDATE users SET status=1 WHERE username='%s' and password='%s';",recUser,recPwd);
			changes=sqlite3_total_changes(db);
			rc=sqlite3_prepare_v2(db,sqlCmd,255,&stmt,0);
			
			if(rc==SQLITE_OK){
				//sqlite3_bind(stmt,1,3);
			} else{
				printf("Failed to execute statement: %s\n",sqlite3_errmsg(db));
			  }
			int step=sqlite3_step(stmt);
			if(step==SQLITE_ROW){
				printf("%s: ",sqlite3_column_text(stmt,0));
				printf("%s\n",sqlite3_column_text(stmt,1));
			}
			
			if(sqlite3_total_changes(db)!=changes){
				printf("ultima instructiune SQL a modificat o coloana.\n");
				
				rc = sqlite3_open("users.db", &db);
   
   				if( rc ) {
      					printf("Can't open database: %s\n", sqlite3_errmsg(db));
      					return(0);
   				}
	 
   				else {
					sprintf(sqlCmd,"INSERT INTO loggedIn (username,password) values('%s','%s');",recUser,recPwd);
					rc=sqlite3_exec(db,sqlCmd,callback,0,&zErrMsg);
					if(rc!=SQLITE_OK){
						printf("Failed to execute statement: %s\n",sqlite3_errmsg(db));
						write(client,"[server]Nu te-ai logat.",100);
					} else { 
						printf("Perechea username->password s-a inserat cu succes in tabela loggedIn.\n");
						write(client,"[server]Te-ai logat cu succes.",100);
						strcpy(sender,recUser);
					  }
					sqlite3_close(db);
				}
			}
			else { printf("ultima instructiune SQL NU A MODIFICAT O COLOANA.\n");		       
			       write(client,"[server]Nu te-ai logat.",100);	
			}
			sqlite3_finalize(stmt);
   		}
	 sqlite3_close(db);
	}

	

		}
	return 1;
}

int registerUser(){
	 
	 printf("[server]Clientul a solicitat sa se inregistreze.\n");
	 strcpy(rsp,"Astept username pentru register...");
	 if(write(client,rsp,100)<=0){
		printf("[server]Eroare la write(cerere username).\n");
		return errno;
	 } else {
	  	printf("[server]Am trimis mesajul.\n");	 
	   }
	 bzero(recUser,25);
	 fflush(stdout);
	 if(read(client,recUser,100)<=0){
		printf("[server]Eroare la read(username).\n");
		return errno;
	 } else {
		corrString(recUser);
		printf("[server]Username-ul primit este:%s.\n",recUser);
	}
	
	/*AM USERNAME*/
	/*CER PAROLA*/
	bzero(rsp,100);
	strcpy(rsp,"Astept password pentru register...");
	if(write(client,rsp,100)<=0){
		printf("[server]Eroare la write(cerere password).\n");
		return errno;
	} else {
		printf("[server]Am trimis mesajul.\n");
	  }
	bzero(recPwd,18);
	fflush(stdout);
	if(read(client,recPwd,100)<=0){
		printf("[server]Eroare la read(password).\n");
		return errno;
	}else{
		corrString(recPwd);
		printf("[server]Password-ul primit este:%s.\n",recPwd);
	 }
	/*AM USERNAME SI PAROLA*/
	
	/*INCEP LUCRUL CU BAZA DE DATE*/
	rc = sqlite3_open("users.db", &db);
   	if( rc ) {
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
      		return(0);
   	} else {
	 	printf("Database opened successfully.\n");
		bzero(rsp,100);
		char sqlCmd[250];
		sprintf(sqlCmd,"INSERT INTO users (username,password) values ('%s','%s');",recUser,recPwd);
		rc=sqlite3_exec(db,sqlCmd,callback,0,&zErrMsg);
		if(rc!=SQLITE_OK){
				printf("Failed to execute statement: %s\n",sqlite3_errmsg(db));
				strcpy(rsp,"Nu te-ai inregistrat cu succes!");
				if(write(client,rsp,100)<=0){
					perror("[server]Eroare la write(rezultat inregistrare).\n");
					return errno;
			  	} else{
					printf("[server]Am scris cu succes rezultatul inregistrarii.\n");
					
			    	}
		} else{
				printf("[server]Clientul cu username-ul '%s' s-a inregistrat.\n",recUser);
				strcpy(rsp,"Te-ai inregistrat cu succes!");
				if(write(client,rsp,100)<=0){
					perror("[server]Eroare la write(rezultat inregistrare).\n");
					return errno;
				} else{
					printf("[server]Am scris cu succes rezultatul inregistrarii.\n");
						
	        		      }

 		 }

	 sqlite3_close(db);
	}
return 1;
}

int logoutUser(char user[25]){
	printf("Utilizatorul %s doreste sa se delogheze.\n",user);
	rc=sqlite3_open("users.db",&db);
	if(rc){
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
		return 0;
	}else{
		sprintf(sqlCmd,"DELETE FROM loggedIn where username='%s';",user);
		rc=sqlite3_exec(db,sqlCmd,callback,0,&zErrMsg);
		if(rc!=SQLITE_OK){
			printf("Failed to execute statement: %s\n",sqlite3_errmsg(db));
			bzero(rsp,100);
			strcpy(rsp,"Contul nu a fost deconectat.");
			if(write(client,rsp,100)<=0){
				perror("[server]Eroare la write(rezultat logout).\n");
				return errno;
			} else {
				printf("[server]Am scris cu succes rezultatul la logout.\n");	
			  }
		} else {
			printf("Contul cu username-ul %s a fost deconectat.\n",user);
			bzero(rsp,100);
			strcpy(rsp,"Contul a fost deconectat.");
			if(write(client,rsp,100)<=0){
				perror("[server]Eroare la write(rezultat logout).\n");
				return errno;
			} else {
				printf("[server]Am scris cu succes rezultatul la logout.\n");	
			  }
			
		  }
	sqlite3_close(db);
	} 
return 1;
}

int deleteAccount(char user[25]){
	printf("Utilizatorul %s doreste sa isi stearga contul.\n",user);
	rc=sqlite3_open("users.db",&db);
	if(rc){
		printf("Can't open database: %s\n",sqlite3_errmsg(db));
		return 0;
	} else {
		bzero(sqlCmd,100);
		sprintf(sqlCmd,"DELETE FROM users where username='%s';",user);
		rc=sqlite3_exec(db,sqlCmd,callback,0,&zErrMsg);
		if(rc!=SQLITE_OK){
			printf("Failed to execute statement: %s\n",sqlite3_errmsg(db));
			bzero(rsp,100);
			strcpy(rsp,"Contul nu a fost sters.\n");
			if(write(client,rsp,100)<=0){
				perror("[server]Eroare la write(rezultat deleteAccount).\n");
				return errno;
			} else {
				printf("[server]Am scris cu succes rezultatul la stergerea contului.\n");	
			  }
		} else {
			printf("Contul cu username-ul %s a fost sters.\n",user);
			bzero(rsp,100);
			strcpy(rsp,"Contul a fost sters.");
			if(write(client,rsp,100)<=0){
				perror("[server]Eroare la write(rezultat deleteAccount).\n");
				return errno;
			} else {
				printf("[server]Am scris cu succes rezultatul la stergerea contului.\n");	
			  }
			
		  }
	sqlite3_close(db);
			
		}
	 

return 1;
}

//presupun ca am receiver deja primit de la client			PARAMETRI: char sender[25],char receiver[25]
int writeTo(){
	strcpy(rsp,"Introdu destinatar: ");
	if(write(client,rsp,100)<=0){
		perror("[server]Eroare la write(cerere destinatar).\n");
		return errno;
	} else {
			printf("[server]Am scris cu succes mesajul.\n");
	       }
	if(read(client,receiver,25)<=0){
		perror("[server]Eroare la read(destinatar).\n");
		return errno;
	} else {	
			corrString(receiver);
			printf("[server]Am citit cu succes destinatarul:%s.\n",receiver);
	  }

	//am destinatarul, cer mesajul
	
	strcpy(rsp,"Introdu mesajul: ");
	if(write(client,rsp,100)<=0){
		perror("[server]Eroare la write(cerere mesaj).\n");
		return errno;
	} else {
			printf("[server]Am scris cu succes mesajul.\n");
	  }
	if(read(client,msg,100)<=0){
		perror("[server]Eroare la read(mesaj).\n");
		return errno;
	} else {
		//corectez string?
		printf("\n%s\n",msg);
	  }
	printf("Trimit mesajul:%s\n",msg);
	printf("De la:%s\n",sender);
	printf("La:%s\n",receiver);

return 1;
}


int main(){

	

/* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }

  /* pregatirea structurilor de date */
  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));
  
  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;	
  /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl (INADDR_ANY);
  /* utilizam un port utilizator */
    server.sin_port = htons (PORT);
  
  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 5) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }
	
	rc=sqlite3_open("users.db",&db);
	bzero(sqlCmd,100);
	sprintf(sqlCmd,"delete from loggedIn;");
	rc=sqlite3_exec(db,sqlCmd,callback,0,&zErrMsg);
	bzero(sqlCmd,100);

    while(1){
	
	
	

	printf ("[server]Asteptam la portul %d...\n",PORT);
        fflush (stdout);
	
	/* acceptam un client (stare blocanta pina la realizarea conexiunii) */
      	client = accept (sd, (struct sockaddr *) &from, &length);
      
      	signal(SIGCHLD,child_handler);//scap de procesul copil atunci cand acesta trimite SIGCHLD

      	/*creez un proces copil pentru clientul care tocmai a solicitat o conexiune cu serverul*/
      	child=fork();
	if(child==0){
	
		if(client<0){
			perror("[server]Eroare la accept.\n");
			continue;
		}
		
		/* s-a realizat conexiunea, se astepta comanda */
      		bzero (cmd, 100);//curat
      		printf ("[server]Astept comanda...\n");
      		fflush (stdout);
		
		/* citirea comenzii */
      		if (read (client, cmd, 100) <= 0){

	  			perror ("[server]Eroare la read(cmd) de la client.\n");
	  			close (client);	/* inchidem conexiunea cu clientul */
	  			continue;	/* continuam sa ascultam */
		}
	
      		printf ("[server]Comanda a fost receptionata...%s\n", cmd);
	
	/*corectez comanda, cmd avand la final "\n"*/
	corrString(cmd);

	if(strcmp(cmd,"login")==0){	
	
		if(loginUser()==0)
			continue;
		else {
			printf("[server]Astept comenzi.\n");
			bzero(cmd2,100);
			fflush(stdout);
			if(read(client,cmd2,100)<=0){
				perror("[server]Eroare la read(cmd2) de la client.\n");
				close(client);
				continue;
			} else {
				corrString(cmd2);
				printf("Comanda primita este %s.\n",cmd2);
				if(strcmp(cmd2,"logout")==0){
					logoutUser(recUser);
				} else 
				if(strcmp(cmd2,"delAcc")==0){
					deleteAccount(recUser);
					logoutUser(recUser);
				} else
				if(strcmp(cmd2,"writeto")==0){
					writeTo();
					
				}				
			  }
		}
	
	 }//sfarsit login
	
	else if(strcmp(cmd,"register")==0){
		if(registerUser()==0)
			continue;
		if(loginUser()==0)
			continue;
		else {
			printf("[server]Astept comenzi.\n");
			bzero(cmd2,100);
			fflush(stdout);
			if(read(client,cmd2,100)<=0){
				perror("[server]Eroare la read(cmd2) de la client.\n");
				close(client);
				continue;
			} else {
				corrString(cmd2);
				printf("Comanda primita este %s.\n",cmd2);
				if(strcmp(cmd2,"logout")==0){
					logoutUser(recUser);
				} else 
				if(strcmp(cmd2,"delAcc")==0){
					deleteAccount(recUser);
					logoutUser(recUser);
					bzero(rsp,100);
					strcpy(rsp,"Contul a fost sters.\n");
					if(write(client,rsp,100)<=0){
						perror("[server]Eroare la write(raspuns delAcc).\n");
						return errno;
					} 
				} else
				if(strcmp(cmd2,"writeto")==0){
					writeTo();
				}			
			  }
		}
	}/*sfarsit register*/

	else if(strcmp(cmd,"logout")==0){
		if(logoutUser(recUser)==0)
			continue;
	}
	close(client);/*am terminat cu acest client, inchidem conexiunea*/
	exit(1);
	}/*sfarsit copil*/
  	
	
     }/*sfarsit while*/

return 0;
}

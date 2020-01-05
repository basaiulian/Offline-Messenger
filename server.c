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
#include <pthread.h>
#define PORT 1805

extern int errno;	// codul de eroare returnat de anumite apeluri 
struct sockaddr_in server;
struct sockaddr_in from;
char recUser[25];	//username-ul primit la login
char recPwd[18];	//password-ul primit la login
char servCmd[100];	//comanda server
char cmd[100];	//comanda trimisa de client catre procesul copil
char cmd2[100];	//a doua comanda trimisa de client catre procesul copil
char rsp[250];	//raspunsul primit de client de la procesul copil
char msg[250];	//mesajul primit
char convMsg[250];	//mesajul trimis la speakto
char onlineUser[25];	//username-ul pt eOnline
char hist[100][250];	//istoric
char sndHistory[5000];	//istoric
char chk[100][250];	//mesajele primite cand se conecteaza un client
char receiver[25];	//destinatarul 
char sender[25];	//cel ce trimite mesajele
char mess[1000][1000];	//pentru callback
char mesaj[1000];	//mesajele cand sunt in speakto
int sd;	//socket
int client;
int length = sizeof(from);
pid_t child;
int changes;
int m = 0;	
int mm = 0;	 
int h = 1;
int hh = 1;
int j=1;
int max=1;

typedef struct thData
{
	int client;
	char *rcv;
	char *snd;
}
thData;


sqlite3 * db;
char *zErrMsg = 0;
sqlite3_stmt * stmt;
int rc;
char *sql;
char sqlCmd[500];

int callbackOriginal(void *NotUsed, int argc, char **argv, char **azColName) {
    
    for (int i = 0; i < argc; i++) {

        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    
    printf("\n");
    
    return 0;
}

static int cb(void *NotUsed, int argc, char **argv, char **azColName){
   int i,j;
   for(i=0; i<argc; i++){ 
      /*printf("%s|%s\n", azColName[i], argv[i] ? argv[i] : "NULL");*/
      strcpy(mess[j],argv[i]);
      j++;
      max++;  
   }

	for(j=0;j<=max;j++){
		strcat(mesaj,mess[j]);	
		strcat(mesaj,"\n");
		
	}
	
   return 0;
}

static int callbackO(void *NotUsed, int argc, char **argv, char **azColName)
{
	bzero(onlineUser, 25);
	for (int i = 0; i < argc; i++)
		strcpy(onlineUser, argv[i]);
	return 0;
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	
	for (i = 0; i < argc; i++)
	{
		h++;
		strcpy(chk[h], argv[i]);
		m++;
	}
	return 0;
}

static int callbackH(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for (i = 0; i < argc; i++)
	{

		//printf("%s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		hh++;
		strcpy(hist[hh], argv[i]);
		mm++;
	}
	return 0;
}

/*sfarsit preluat de pe net */

void corrString(char string[300])
{
	char *pstr;	//va pointa la pozitia primei aparitii a "\n"
	pstr = strstr(string, "\n");
	if (pstr != NULL)
	{
		//daca am gasit o pozitie pentru \n, inlocuiesc aparitia sa cu ""
		strncpy(pstr, "", 1);
	}
}

int eOnline(char user[])
{
	rc = sqlite3_open("users.db", &db);
	sprintf(sqlCmd, "SELECT username FROM loggedIn where username='%s';", user);
	rc = sqlite3_exec(db, sqlCmd, callbackO, 0, &zErrMsg);
	if (rc != SQLITE_OK)
	{
		printf("eroare:%s\n", zErrMsg);
	}
	else
	{
		corrString(onlineUser);
		sqlite3_close(db);
		if (strcmp(onlineUser, user) == 0)
		{
			bzero(onlineUser, 25);
			return 1;
		}
		else
		{
			bzero(onlineUser, 25);
			return -1;
		}
	}
	sqlite3_close(db);
}

void child_handler()
{
	time_t rawtime;//preluat
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char lastActive[100];
	strcpy(lastActive, asctime(timeinfo));
	corrString(lastActive);
	printf("[server]Un proces copil si-a terminat executia la ora %s.\n", lastActive);
	printf("---------------------------------------------------------------------\n");
	wait(NULL);
}

int loginUser()
{

	printf("[server]Clientul a solicitat sa se logheze.\n");
	bzero(rsp, 100);
	strcpy(rsp, "[server]Astept username pentru login...\n");

	printf("[server]Trimitem mesajul inapoi...\n");

	// returnam mesajul clientului
	if (write(client, rsp, 100) <= 0)
	{

		perror("[server]Eroare la write() catre client.\n");
		return 0;
	}
	else
		printf("[server]Mesajul a fost transmis cu succes.\n");

	if (read(client, recUser, 100) <= 0)
	{
		perror("[server-login]Eroare la primirea username-ului.\n");
		close(client);
		return 0;
	}
	else
	{
		//corectez username-ul, recUser avand la final "\n"
		corrString(recUser);
		printf("[server-login]Username-ul primit este: %s.\n", recUser);

		bzero(rsp, 250);
		strcpy(rsp, "[server]Astept parola pentru login...\n");

		printf("[server]Trimitem mesajul inapoi...\n");

		// returnam mesajul clientului
		if (write(client, rsp, 100) <= 0)
		{

			perror("[server]Eroare la write() catre client.\n");
			return 0;
		}
		else
			printf("[server]Mesajul a fost transmis cu succes.\n");

		if (read(client, recPwd, 18) <= 0)
		{
			perror("[server-login]Eroare la primirea password-ului.\n");
			close(client);
			return 0;
		}
		else
		{
			//corectez password-ul, recPwd avand la final "\n"
			corrString(recPwd);
			printf("[server-login]Password-ul primit este: %s.\n", recPwd);

			bzero(rsp, 100);

			rc = sqlite3_open("users.db", &db);

			if (rc)
			{
				printf("Can't open database: %s\n", sqlite3_errmsg(db));
				return (0);
			}
			else
			{
				char sqlCmd[500];
				printf("Opened database successfully\n");
				bzero(sqlCmd, 500);
				sprintf(sqlCmd, "UPDATE users SET status=1 WHERE username='%s' and password='%s';", recUser, recPwd);
				changes = sqlite3_total_changes(db);
				rc = sqlite3_prepare_v2(db, sqlCmd, 255, &stmt, 0);

				if (rc != SQLITE_OK)
				{
					printf("Failed to execute statement: %s\n", sqlite3_errmsg(db));
				}
				int step = sqlite3_step(stmt);
				if (step == SQLITE_ROW)
				{
					printf("%s: ", sqlite3_column_text(stmt, 0));
					printf("%s\n", sqlite3_column_text(stmt, 1));
				}

				if (sqlite3_total_changes(db) != changes)
				{
					printf("ultima instructiune SQL a modificat o coloana.\n");

					rc = sqlite3_open("users.db", &db);

					if (rc)
					{
						printf("Can't open database: %s\n", sqlite3_errmsg(db));
						return (0);
					}
					else
					{
						sprintf(sqlCmd, "INSERT INTO loggedIn (username,password) values('%s','%s');", recUser, recPwd);
						rc = sqlite3_exec(db, sqlCmd, callback, 0, &zErrMsg);
						if (rc != SQLITE_OK)
						{
							printf("Failed to execute statement: %s\n", sqlite3_errmsg(db));
							write(client, "[server]Nu te-ai logat.", 100);
						}
						else
						{
							printf("Perechea username->password s-a inserat cu succes in tabela loggedIn.\n");
							write(client, "[server]Te-ai logat cu succes.", 100);
							strcpy(sender, recUser);
						}
						sqlite3_close(db);
					}
				}
				else
				{
					printf("ultima instructiune SQL NU A MODIFICAT O COLOANA.\n");
					write(client, "[server]Nu te-ai logat.", 100);
				}
				sqlite3_finalize(stmt);
			}
			sqlite3_close(db);
		}
	}
	return 1;
}

int registerUser()
{

	printf("[server]Clientul a solicitat sa se inregistreze.\n");
	strcpy(rsp, "Astept username pentru register...");
	if (write(client, rsp, 100) <= 0)
	{
		printf("[server]Eroare la write(cerere username).\n");
		return errno;
	}
	else
	{
		printf("[server]Am trimis mesajul.\n");
	}
	bzero(recUser, 25);
	fflush(stdout);
	if (read(client, recUser, 100) <= 0)
	{
		printf("[server]Eroare la read(username).\n");
		return errno;
	}
	else
	{
		corrString(recUser);
		printf("[server]Username-ul primit este:%s.\n", recUser);
	}

	/*AM USERNAME*/
	/*CER PAROLA*/
	bzero(rsp, 100);
	strcpy(rsp, "Astept password pentru register...");
	if (write(client, rsp, 100) <= 0)
	{
		printf("[server]Eroare la write(cerere password).\n");
		return errno;
	}
	else
	{
		printf("[server]Am trimis mesajul.\n");
	}
	bzero(recPwd, 18);
	fflush(stdout);
	if (read(client, recPwd, 100) <= 0)
	{
		printf("[server]Eroare la read(password).\n");
		return errno;
	}
	else
	{
		corrString(recPwd);
		printf("[server]Password-ul primit este:%s.\n", recPwd);
	}
	/*AM USERNAME SI PAROLA*/

	rc = sqlite3_open("users.db", &db);
	if (rc)
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
		return (0);
	}
	else
	{
		printf("Database opened successfully.\n");
		bzero(rsp, 100);
		char sqlCmd[500];
		sprintf(sqlCmd, "INSERT INTO users (username,password) values ('%s','%s');", recUser, recPwd);
		rc = sqlite3_exec(db, sqlCmd, callback, 0, &zErrMsg);
		if (rc != SQLITE_OK)
		{
			printf("Failed to execute statement: %s\n", sqlite3_errmsg(db));
			strcpy(rsp, "Nu te-ai inregistrat cu succes!");
			if (write(client, rsp, 100) <= 0)
			{
				perror("[server]Eroare la write(rezultat inregistrare).\n");
				return errno;
			}
			else
			{
				printf("[server]Am scris cu succes rezultatul inregistrarii.\n");
			}
		}
		else
		{
			printf("[server]Clientul cu username-ul '%s' s-a inregistrat.\n", recUser);
			strcpy(rsp, "Te-ai inregistrat cu succes!");
			if (write(client, rsp, 100) <= 0)
			{
				perror("[server]Eroare la write(rezultat inregistrare).\n");
				return errno;
			}
			else
			{
				printf("[server]Am scris cu succes rezultatul inregistrarii.\n");
			}
		}

		sqlite3_close(db);
	}
	return 1;
}

int logoutUser(char user[25])
{
	printf("Utilizatorul %s doreste sa se delogheze.\n", user);
	rc = sqlite3_open("users.db", &db);
	if (rc)
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	else
	{
		sprintf(sqlCmd, "DELETE FROM loggedIn where username='%s';", user);
		rc = sqlite3_exec(db, sqlCmd, callback, 0, &zErrMsg);
		if (rc != SQLITE_OK)
		{
			printf("Failed to execute statement: %s\n", sqlite3_errmsg(db));
			return 0;
		}

		sprintf(sqlCmd, "UPDATE users SET status=0 where username='%s';", user);
		rc = sqlite3_exec(db, sqlCmd, callback, 0, &zErrMsg);
		if (rc != SQLITE_OK)
		{
			printf("Failed to execute statement: %s\n", sqlite3_errmsg(db));
			return 0;
		}
	}
	sqlite3_close(db);
	return 1;
}

int deleteAccount(char user[25])
{
	printf("Utilizatorul %s doreste sa isi stearga contul.\n", user);
	rc = sqlite3_open("users.db", &db);
	if (rc)
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	else
	{
		bzero(sqlCmd, 500);
		sprintf(sqlCmd, "DELETE FROM users where username='%s';", user);
		rc = sqlite3_exec(db, sqlCmd, callback, 0, &zErrMsg);
		if (rc != SQLITE_OK)
		{
			printf("Failed to execute statement: %s\n", sqlite3_errmsg(db));
			return 0;
		}
	}

	sqlite3_close(db);
	return 1;
}

int writeTo()
{

	strcpy(rsp, "Introdu destinatar: ");
	if (write(client, rsp, 100) <= 0)
	{
		perror("[server]Eroare la write(cerere destinatar).\n");
		return errno;
	}
	else
	{
		printf("[server]Am scris cu succes mesajul.\n");
	}
	if (read(client, receiver, 25) <= 0)
	{
		perror("[server]Eroare la read(destinatar).\n");
		return errno;
	}
	else
	{
		corrString(receiver);
		printf("[server]Am citit cu succes destinatarul:%s.\n", receiver);
	}

	if (eOnline(receiver) == 1)
	{
		strcpy(rsp, "%s este online.\n");
	}

	if (strcmp(receiver, "-end") == 0)
		return -1;

	//am destinatarul, cer mesajul

	strcpy(rsp, "Introdu mesajul: ");
	if (write(client, rsp, 100) <= 0)
	{
		perror("[server]Eroare la write(cerere mesaj).\n");
		return errno;
	}
	else
	{
		printf("[server]Am scris cu succes mesajul.\n");
	}
	if (read(client, msg, 100) <= 0)
	{
		perror("[server]Eroare la read(mesaj).\n");
		return errno;
	}
	else
	{
		printf("\n%s\n", msg);
	}
	printf("Trimit mesajul:%s\n", msg);
	printf("De la:%s\n", sender);
	printf("La:%s\n", receiver);

	//am senderul,destinatarul si mesajul => incep lucrul cu baza de date

	rc = sqlite3_open("users.db", &db);
	if (rc)
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	else
	{
		sprintf(sqlCmd, "INSERT INTO messages (snd,rcv,msg) VALUES ('%s','%s','%s');", sender, receiver, msg);
		rc = sqlite3_exec(db, sqlCmd, callback, 0, &zErrMsg);
		if (rc != SQLITE_OK)
		{
			printf("Failed to execute statement: %s\n", sqlite3_errmsg(db));
			bzero(rsp, 100);
			strcpy(rsp, "Mesajul nu a fost trimis.\n");
			if (write(client, rsp, 100) <= 0)
			{
				perror("[server]Eroare la write(rezultat trimitere mesaj).\n");
				return errno;
			}
			else
			{
				printf("[server]Am scris cu succes rezultatul la trimiterea mesajului.\n");
			}
		}
		else
		{
			printf("Mesajul lui %s a fost trimis catre %s.\n", sender, receiver);
		}
		fflush(stdout);
		sqlite3_close(db);
	}

	return 1;
}

int check(char user[25])
{
	char sndCheck[1000];	// raspuns la check
	char *out;
	rc = sqlite3_open("users.db", &db);
	sprintf(sqlCmd, "SELECT snd,msg,id from messages where rcv='%s' and status=0 order by snd,timestamp asc;", user);
	rc = sqlite3_exec(db, sqlCmd, callback, &out, &zErrMsg);
	if (rc != SQLITE_OK)
	{
		printf("eroare:%s\n", zErrMsg);
	}
	bzero(sndCheck, 1000);
	for (int z = 2; z <= m+1; z = z + 3)
	{
	
		strcat(sndCheck,chk[z+1]);
		strcat(sndCheck,"(");
		strcat(sndCheck,chk[z]);
		strcat(sndCheck,",");
		strcat(sndCheck,chk[z+2]);
		strcat(sndCheck,")\n");
		if(strstr(sndCheck,",)")!=0)
		strcat(sndCheck," \n");
		
	}


	sprintf(sqlCmd, "UPDATE messages set status=1 where rcv='%s';", user);
	rc = sqlite3_exec(db, sqlCmd, callback, 0, &zErrMsg);
	if (rc != SQLITE_OK)
	{
		printf("eroare:%s\n", zErrMsg);
	}

	sqlite3_close(db);

	if (write(client, sndCheck, 1000) <= 0)
	{
		perror("[server]Eroare la write.\n");
		return errno;
	}
	else printf("Am trimis mesajele necitite.\n");
	return 1;
}

int history(char user[25])
{
	strcpy(rsp, "Introdu userul pentru care doresti istoricul mesajelor: ");
	if (write(client, rsp, 100) <= 0)
	{
		perror("[server]Eroare la write(cerere destinatar).\n");
		return errno;
	}
	else
	{
		printf("[server]Am scris cu succes mesajul.\n");
	}
	if (read(client, receiver, 25) <= 0)
	{
		perror("[server]Eroare la read(destinatar).\n");
		return errno;
	}
	else
	{
		corrString(receiver);
		printf("[server]Am citit cu succes destinatarul:%s.\n", receiver);
	}
	bzero(sndHistory,5000);	
	char *outH;
	rc = sqlite3_open("users.db", &db);
	sprintf(sqlCmd, "SELECT snd,msg,id from messages where (snd='%s' AND rcv='%s') or (snd='%s' and rcv='%s');", receiver, user, user, receiver);
	rc = sqlite3_exec(db, sqlCmd, callbackH, &outH, &zErrMsg);
	if (rc != SQLITE_OK)
	{
		printf("eroare:%s\n", zErrMsg);
	}
	for (int z = 2; z <= mm+1; z = z + 3)
	{
		strcat(sndHistory,hist[z+1]);
		strcat(sndHistory,"(");
		strcat(sndHistory,hist[z]);
		strcat(sndHistory,",");
		strcat(sndHistory,hist[z+2]);
		strcat(sndHistory,")\n");
		if(strstr(sndHistory,",)")!=0)
			strcat(sndHistory," \n");
	}
	mm = 0;
	hh = 1;
	sqlite3_close(db);
	if (write(client, sndHistory, 5000) <= 0)
	{
		perror("[server]Eroare la write.\n");
		return errno;
	}
	else
	{
		printf("Am trimis istoricul lui %s cu %s.\n", user, receiver);
	}
	return 1;
}

void *cautareMesaje(void *arg)
	{
    struct thData thd;
    thd = *((struct thData *) arg);
    int rc;
    char *zErrMsg = 0;
    sqlite3 *db;

	rc = sqlite3_open("users.db", &db);

        if (rc) {
            fprintf(stderr, "Nu se poate face conexiunea la baza de date: %s\n", sqlite3_errmsg(db));
        } 

    while (1) {
        sleep(1);
	sprintf(sqlCmd,"SELECT msg from messages where rcv='%s' and snd='%s' and status=0;",thd.snd,thd.rcv);
        rc=sqlite3_exec(db,sqlCmd,cb,0,&zErrMsg);
	
        if (rc != SQLITE_OK) {
            
            sqlite3_close(db);
        }
	corrString(mesaj);
            if (write(thd.client, mesaj, 1000) <= 0) {
                perror("Eroare la write() catre client.\n");
            }

		bzero(mesaj,1000);
		fflush(stdout);
	
          
	    sprintf(sqlCmd,"UPDATE messages SET status=1 WHERE rcv='%s' AND status=0 AND snd='%s';",thd.snd,thd.rcv);
            rc = sqlite3_exec(db, sqlCmd, NULL, 0, &zErrMsg);
            if (rc != SQLITE_OK) {
                sqlite3_close(db);
            } 

        


        
    }
	sqlite3_close(db);
	}

		int main()
		{

			/*crearea unui socket */
			if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			{
				perror("[server]Eroare la socket().\n");
				return errno;
			}

			/*pregatirea structurilor de date */
			bzero(&server, sizeof(server));
			bzero(&from, sizeof(from));

			/*umplem structura folosita de server */
			/*stabilirea familiei de socket-uri */
			server.sin_family = AF_INET;
			/*acceptam orice adresa */
			server.sin_addr.s_addr = htonl(INADDR_ANY);
			/*utilizam un port utilizator */
			server.sin_port = htons(PORT);

			/*atasam socketul */
			if (bind(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1)
			{
				perror("[server]Eroare la bind().\n");
				return errno;
			}

			/*punem serverul sa asculte daca vin clienti sa se conecteze */
			if (listen(sd, 5) == -1)
			{
				perror("[server]Eroare la listen().\n");
				return errno;
			}

			rc = sqlite3_open("users.db", &db);
			bzero(sqlCmd, 500);
			sprintf(sqlCmd, "delete from loggedIn;");
			rc = sqlite3_exec(db, sqlCmd, callback, 0, &zErrMsg);
			bzero(sqlCmd, 500);

			while (1)
			{

				printf("[server]Asteptam la portul %d...\n", PORT);
				fflush(stdout);

				/*acceptam un client (stare blocanta pina la realizarea conexiunii) */
				client = accept(sd, (struct sockaddr *) &from, &length);

				signal(SIGCHLD, child_handler);	//scap de procesul copil atunci cand acesta trimite SIGCHLD

				/*creez un proces copil pentru clientul care tocmai a solicitat o conexiune cu serverul*/
				child = fork();
				if (child == 0)
				{
					int end = 0;
					if (client < 0)
					{
						perror("[server]Eroare la accept.\n");
						continue;
					}

					/*s-a realizat conexiunea, se astepta comanda */
					bzero(cmd, 100);	//curat
					printf("[server]Astept comanda...\n");
					fflush(stdout);

					/*citirea comenzii */
					if (read(client, cmd, 100) <= 0)
					{

						perror("[server]Eroare la read(cmd) de la client.\n");
						close(client); /*inchidem conexiunea cu clientul */
						continue; /*continuam sa ascultam */
					}

					printf("[server]Comanda a fost receptionata...%s\n", cmd);

					/*corectez comanda, cmd avand la final "\n"*/
					corrString(cmd);

					if (strcmp(cmd, "login") == 0)
					{

						if (loginUser() == 0)
							continue;
						else
						{
							check(recUser);
							while (1 && end != 1)
							{
								printf("[server]Astept comenzi.\n");
								bzero(cmd2, 100);
								fflush(stdout);
								if (read(client, cmd2, 100) <= 0)
								{
								 						
									close(client);
									continue;
								}
								else
								{
									corrString(cmd2);
									printf("Comanda primita este %s.\n", cmd2);
									if (strcmp(cmd2, "logout") == 0)
									{
										logoutUser(recUser);
										end = 1;
										continue;
									}
									else
									if (strcmp(cmd2, "delacc") == 0)
									{
										deleteAccount(recUser);
										logoutUser(recUser);
										end = 1;
										continue;
									}
									else
									if (strcmp(cmd2, "writeto") == 0)
									{
										writeTo();
									}
									else
									if (strcmp(cmd2, "history") == 0)
									{
										history(recUser);
									}
									else if (strcmp(cmd2, "speakto") == 0)
									{
										bzero(rsp, 100);
        strcat(rsp, "Introdu numele utilizatorului cu care doresti sa porti o conversatie \n");

        if (write(client, rsp, 100) <= 0) {
            perror("Eroare la write(rsp) catre client.\n");
        } else {
            printf("Mesajul a fost transmis cu succes.\n");
        }

        //aici primim numele celui cu care va vorbi
        if (read(client, receiver, 100) <= 0) {
            perror("Eroare la read(receiver) de la client.\n");
        }
        printf("Mesajul primit este: %s\n", receiver);
        corrString(receiver);
	
        if (eOnline(receiver) == -1) {
            bzero(rsp, 100);
            strcpy(rsp, "Client offline.");
            if (write(client, rsp, 100) <= 0) {
                perror("Eroare la write() catre client.\n");
            } else {
                printf("Mesajul a fost transmis cu succes.\n");
                printf("Mesajul este: %s \n", rsp);
            }
        } else {
            bzero(rsp, 100);
            strcpy(rsp, "Client online.");
            if (write(client, rsp, 100) <= 0) {
                perror("Eroare la write() catre client.\n");
            } else {
                printf("Mesajul a fost transmis cu succes.\n");
                printf("Mesajul este: %s \n", rsp);
            }
        }

	if(strcmp(rsp,"Client online.")==0){
        thData *thd;
        thd = (struct thData *) malloc(sizeof(struct thData));
        thd->client = client;
        thd->rcv = receiver;
        thd->snd = recUser;
        pthread_t td;
        pthread_create(&td, NULL, &cautareMesaje, thd);
	rc=sqlite3_open("users.db",&db);
	if(rc){
		printf("eroare la deschiderea bazei de date.\n");
		return 0;
	} printf("BAZA DE DATE DESCHISA\n");
        bzero(msg, 100);
        if (read(client, msg, 100) <= 0) {
            perror("Eroare la read() de la client.\n");
        }
        printf("Mesajul a fost receptionat... %s\n", msg);
        corrString(msg);
        int k = 1;
        while (strcmp(msg, "-end") != 0) {
            if (k > 1) {
                
                if (read(client, msg, 100) <= 0) {
                    perror("Eroare la read() de la client.\n");
                }
                printf("Mesajul a fost receptionat... %s\n", msg);
            }
            k++;

            if (strcmp(msg, "-end") != 0) {
		sprintf(sqlCmd,"INSERT INTO messages (snd,rcv,msg,status) values ('%s','%s','%s',0);",recUser,receiver,msg);
                rc = sqlite3_exec(db, sqlCmd, callbackOriginal, 0, &zErrMsg);

                if (rc != SQLITE_OK) {
                    printf("Nu s-a putut adauga la BD.[%s]\n",zErrMsg);
                } else {
                    printf("Mesajul '%s' a fost adaugat la BD. \n", msg);
                }
            }


        }
        pthread_detach(td);
        pthread_cancel(td);
        sqlite3_close(db);
	}
										
									}	//sfarsit speakto

								}
							}
						}
					}	//sfarsit login
					else if (strcmp(cmd, "register") == 0)
					{
						if (registerUser() == 0)
						{
							continue;
						}
						if (loginUser() == 0)
						{
							continue;
						}
						else
						{

							check(recUser);
							while (1 && end != 1)
							{
								printf("[server]Astept comenzi.\n");
								bzero(cmd2, 100);
								fflush(stdout);
								if (read(client, cmd2, 100) <= 0)
								{
								 						
									close(client);
									continue;
								}
								else
								{
									corrString(cmd2);
									printf("Comanda primita este %s.\n", cmd2);
									if (strcmp(cmd2, "logout") == 0)
									{
										logoutUser(recUser);
										end = 1;
										continue;
									}
									else
									if (strcmp(cmd2, "delacc") == 0)
									{
										deleteAccount(recUser);
										logoutUser(recUser);
										end = 1;
										continue;
									}
									else
									if (strcmp(cmd2, "writeto") == 0)
									{
										writeTo();
									}
									else
									if (strcmp(cmd2, "history") == 0)
									{
										history(recUser);
									}
									else if (strcmp(cmd2, "speakto") == 0)
									{
										bzero(rsp, 100);
        strcat(rsp, "Introdu numele utilizatorului cu care doresti sa porti o conversatie \n");

        if (write(client, rsp, 100) <= 0) {
            perror("Eroare la write(rsp) catre client.\n");
        } else {
            printf("Mesajul a fost transmis cu succes.\n");
        }

        //aici primim numele celui cu care va vorbi
        if (read(client, receiver, 100) <= 0) {
            perror("Eroare la read(receiver) de la client.\n");
        }
        printf("Mesajul primit este: %s\n", receiver);
        corrString(receiver);
	
        if (eOnline(receiver) == -1) {
            bzero(rsp, 100);
            strcpy(rsp, "Client offline.");
            if (write(client, rsp, 100) <= 0) {
                perror("Eroare la write() catre client.\n");
            } else {
                printf("Mesajul a fost transmis cu succes.\n");
                printf("Mesajul este: %s \n", rsp);
            }
        } else {
            bzero(rsp, 100);
            strcpy(rsp, "Client online.");
            if (write(client, rsp, 100) <= 0) {
                perror("Eroare la write() catre client.\n");
            } else {
                printf("Mesajul a fost transmis cu succes.\n");
                printf("Mesajul este: %s \n", rsp);
            }
        }

	if(strcmp(rsp,"Client online.")==0){
        thData *thd;
        thd = (struct thData *) malloc(sizeof(struct thData));
        thd->client = client;
        thd->rcv = receiver;
        thd->snd = recUser;
        pthread_t td;
        pthread_create(&td, NULL, &cautareMesaje, thd);
	rc=sqlite3_open("users.db",&db);
	if(rc){
		printf("eroare la deschiderea bazei de date.\n");
		return 0;
	} printf("BAZA DE DATE DESCHISA\n");
        bzero(msg, 100);
        if (read(client, msg, 100) <= 0) {
            perror("Eroare la read() de la client.\n");
        }
        printf("Mesajul a fost receptionat... %s\n", msg);
        corrString(msg);
        int k = 1;
        while (strcmp(msg, "-end") != 0) {
            if (k > 1) {
                
                if (read(client, msg, 100) <= 0) {
                    perror("Eroare la read() de la client.\n");
                }
                printf("Mesajul a fost receptionat... %s\n", msg);
            }
            k++;

            if (strcmp(msg, "-end") != 0) {
		sprintf(sqlCmd,"INSERT INTO messages (snd,rcv,msg,status) values ('%s','%s','%s',0);",recUser,receiver,msg);
                rc = sqlite3_exec(db, sqlCmd, callbackOriginal, 0, &zErrMsg);

                if (rc != SQLITE_OK) {
                    printf("Nu s-a putut adauga la BD.[%s]\n",zErrMsg);
                } else {
                    printf("Mesajul '%s' a fost adaugat la BD. \n", msg);
                }
            }


        }
        pthread_detach(td);
        pthread_cancel(td);
        sqlite3_close(db);
	}
										
									}	//sfarsit speakto

								}
							}
						}
					} /*sfarsit register*/

					close(client); /*am terminat cu acest client, inchidem conexiunea*/
					exit(1);
				} /*sfarsit copil*/

			} /*sfarsit while*/

			return 0;
		}

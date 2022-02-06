#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include "structs.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>

//FILE DESCRIPTOR
int s_fifo_fd_cliente,s_fifo_fd_medico, c_fifo_fd,m_fifo_fd;
int n_medicos;
int n_clientes;
int repeat_time=30;
pthread_mutex_t mutex;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	
int max(int a, int b){
	return (a>b) ? a:b;
}
void trataSinal(int i){
	fprintf(stderr,RED "\nAbortada conexao :C \n" WHITE);
	close(s_fifo_fd_cliente);
	close(s_fifo_fd_medico);
	unlink(SERVER_FIFO_CLIENTE);
	unlink(SERVER_FIFO_MEDICO);
	exit(EXIT_SUCCESS);
}
void apaga_lista_med(int pid, struct Especialidades lista[],int consulta_bypass){
	for(int i=0;i<5;i++){
			if(lista[i].inicioMed != NULL){
					struct Medico* j = lista[i].inicioMed;		
					if(j->pid == pid){ 
						if(!j->em_consulta || consulta_bypass){
							//se for o primeiro da lista para apagar
							lista[i].inicioMed = j->prox;
							fprintf(stderr,"\nA remover Medico com o pid: %d\n",pid);
							kill(pid,SIGUSR1);
							unlink(j->fifo_nome);
							free(j);
							break;
						}
						fprintf(stderr,RED "O medico nao pode ser eliminado pois esta numa consulta" WHITE);
						
					}
				for(struct Medico* k = lista[i].inicioMed;k != NULL;k = k->prox){	// se estiver no meio
					struct Medico* temp = k->prox;
					if(temp != NULL){ 
						if(temp->pid == pid){
							if(!temp->em_consulta || consulta_bypass){
								fprintf(stderr,"\nA remover Medico com o pid: %d\n",pid);
								kill(pid,SIGUSR1);
								rcv_t rcv;
								k->prox = temp->prox;
								unlink(temp->fifo_nome);
								free(temp);
								break;
							}
							fprintf(stderr,RED "O medico nao pode ser eliminado pois esta numa consulta" WHITE);
						}
					}
					else{
						fprintf(stderr,RED "o PID introduzido nao foi encontrado." WHITE);
						break;
					}
				}
			}
		}
}
void apaga_lista_cliente(int pid,struct Especialidades lista[],int consulta_bypass){
	for(int i=0;i<5;i++){
			if(lista[i].inicio != NULL){
					struct Cliente* j = lista[i].inicio;		
					if(j->pid == pid){ 
						if(!j->em_consulta || consulta_bypass){
							//se for o primeiro da lista para apagar
							lista[i].inicio = j->prox;
							fprintf(stderr,"\nA remover utente com o pid: %d\n",pid);
							kill(pid,SIGUSR1);
							free(j);
							break;
						}
						else{
							fprintf(stderr,RED "O utente nao pode ser eliminado pois esta numa consulta" WHITE);
						}
						
					}
				for(struct Cliente* k = lista[i].inicio;k != NULL;k = k->prox){	// se estiver no meio
					struct Cliente* temp = k->prox;
					if(temp != NULL){ 
						if(temp->pid == pid){
							if(!temp->em_consulta || consulta_bypass){
								fprintf(stderr,"\nA remover utente com o pid: %d\n",pid);
								kill(pid,SIGUSR1);
								rcv_t rcv;
								k->prox = temp->prox;
								if(temp->medico != NULL){
									temp->medico->em_consulta =false;
								}
								unlink(temp->fifo_nome);
								free(temp);
								break;
							}
								fprintf(stderr,RED "O utente nao pode ser eliminado pois esta numa consulta" WHITE);
						}
					}
					else{
						fprintf(stderr,RED "o PID introduzido nao foi encontrado." WHITE);
						break;
					}
				}
			}
		}
}
void* check_vida(void* param){
	Tsinal_vida* dados_vida = (Tsinal_vida*) param;
	while(1){
		sleep(20);
		fprintf(stderr,"Ha procura de sinal de vida\n");
		for(int i=0;i<5;i++){
			for(struct Medico* j =dados_vida->listaespecialidades[i].inicioMed;j!=NULL;j = j->prox){
				int fd = open(j->fifo_nome,O_RDONLY);
				if(fd == -1){
					fprintf(stderr,"Medico %s(%d) removido por faltar ao sinal de vida\n",j->nome,j->pid);
					apaga_lista_med(j->pid,dados_vida->listaespecialidades,1);
					continue;
				}
				else{
					close(fd);
				}
			}
		}
	}
}
void teclado(char *buf,struct Especialidades lista[],int stdinclassi){
	
	
	char *comando = strtok(buf," ");
	char* numero_s = strtok(NULL," ");
	//memset(buf,0,sizeof(buf));
	int pid = 0;
	if(numero_s != NULL){
		pid = atoi(numero_s);
		numero_s = "";
	}else{
		pid = 0;
		numero_s="";
	}
	fprintf(stderr,BLUE "ADMIN ESCREVEU PO TECLADO:  ");
	fprintf(stderr, WHITE "%s %s\n",comando,numero_s);
	//fprintf(stderr,"PID: %d",pid);
	if(!strcmp(comando,"utentes\n")){
		for(int i=0;i<5;i++){
			if(lista[i].inicio != NULL){
				for(struct Cliente* j = lista[i].inicio;j != NULL;j = j->prox){
					if(j->em_consulta){
						fprintf(stderr,"%s(%d) encontra se em consulta na unidade de %s com o medico %s \n",j->nome,j->pid,j->especialidade,j->medico->nome);
						continue;
					}
					fprintf(stderr,"%s(%d) encontra-se em espera para ser atendido na unidade de %s com prioridade %d\n",j->nome,j->pid,j->especialidade,j->prioridade);

				}
			}
		}

	}
	else if(!strcmp(comando,"especialistas\n")){
		for(int i=0;i<5;i++){
			if(lista[i].inicioMed != NULL){
				for(struct Medico* j = lista[i].inicioMed;j != NULL;j = j->prox){
					if(j->em_consulta){
						fprintf(stderr,"O medico %s(%d) encontra se a atender o utente %s na unidade de %s \n",j->nome,j->pid,j->cliente->nome,j->especialidade);
						continue;
					}
					fprintf(stderr,"O medico %s(%d) encontra-se em espera na unidade de %s\n",j->nome,j->pid,j->especialidade);
				}
			}
		}
	}
	else if(!strcmp(comando,"delesp") && pid != 0){
		apaga_lista_med(pid,lista,0);
	}
	else if(!strcmp(comando,"delut") && pid != 0){
		apaga_lista_cliente(pid,lista,0);
	}
	else if(!strcmp(comando,"encerra\n")){
		for(int i=0;i<5;i++){
			if(lista[i].inicio != NULL){
				for(struct Cliente* j=lista[i].inicio;j!=NULL;j=j->prox){
					kill(j->pid,SIGUSR2);
					unlink(j->fifo_nome);
					free(j);

				}
			}
			if(lista[i].inicioMed != NULL){
				for(struct Medico* j=lista[i].inicioMed;j!=NULL;j=j->prox){
					kill(j->pid,SIGUSR2);
					unlink(j->fifo_nome);
					free(j);

				}
			}
		}
		write(stdinclassi,"#fim",4);
		unlink(SERVER_FIFO_CLIENTE);
		unlink(SERVER_FIFO_MEDICO);
		exit(EXIT_SUCCESS);
	}
	else if(!strcmp(comando, "freq") && pid > 0){ // pid aqui e serve para setar os N segundos e n um pid
		repeat_time = pid;
		fprintf(stderr,"Repetindo print de lista de medicos e clientes presentes de %d em %d segundos\n",repeat_time,repeat_time);
		
	}
}
void printlista(struct Especialidades lista[]){
	    // Print the array of linked list
	fprintf(stderr,BLUE "\nLista de Clientes: \n");
    for (int i = 0; i < 5; i++) {
		struct Cliente* temp = lista[i].inicio;
		// Linked list number
		fprintf(stderr,"%s-->\t",lista[i].especialidade );
	
		// Print the Linked List
		while (temp != NULL) {
			fprintf(stderr,WHITE "%s" BLUE,temp->nome);
			temp = temp->prox;
		}
        fprintf(stderr,"\n");
    }

	fprintf(stderr,BLUE "\nLista de Medicos: \n");		
	for (int i = 0; i < 5; i++) {
		struct Medico* temp = lista[i].inicioMed;
	
		// Linked list number
		fprintf(stderr,"%s-->\t",lista[i].especialidade );
	
		// Print the Linked List
		while (temp != NULL) {
			fprintf(stderr,WHITE "%s" BLUE,temp->nome);
			temp = temp->prox;
		}
        fprintf(stderr,"\n");
    }
}
void* chat_watcher(void* param){

	Tchat* dados = (Tchat*) param;
	char fifo_name_cl[50];
	sprintf(fifo_name_cl,CLIENT_FIFO,dados->j->pid);
	fprintf(stderr,"\nAbrindo: %s",fifo_name_cl);
	int fd_cli = open(fifo_name_cl,O_WRONLY | O_NONBLOCK);
	if(fd_cli == -1){
		perror("erro");
		exit(EXIT_FAILURE);
	}
	sprintf(dados->ptrRcv->msg,"A ser atendido por: %s",dados->k->nome); // tem que ter \n
	sprintf(dados->ptrRcv->fifo_name,MEDICO_FIFO,dados->k->pid);
	fprintf(stderr, "FD:%d, e mensagem: %s",fd_cli,dados->ptrRcv->msg);
	if(write(fd_cli,dados->ptrRcv,sizeof(*dados->ptrRcv)) == -1){
		perror("erro");
		exit(EXIT_FAILURE);
	}
	close(fd_cli);
	dados->j->em_consulta = true;
	char fifo_name_med[50];
	sprintf(fifo_name_med,MEDICO_FIFO,dados->k->pid);
	int fd_med = open(fifo_name_med,O_WRONLY | O_NONBLOCK);
	if(fd_med == -1){
		perror("erro");
		exit(EXIT_FAILURE);
	}
	memset(dados->ptrRcv,0,sizeof(*dados->ptrRcv));
	sprintf(dados->ptrRcv->msg,"A atender: %s\n",dados->j->nome);
	sprintf(dados->ptrRcv->fifo_name,CLIENT_FIFO,dados->j->pid);
	fprintf(stderr, "FD:%d, e mensagem: %s",fd_med,dados->ptrRcv->msg);
	int byte;
	if((byte = write(fd_med,dados->ptrRcv,sizeof(*dados->ptrRcv))) == -1){
		perror("erro");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,"ESCRITO %d bytes para %s",byte,fifo_name_med);
	close(fd_med);
	dados->k->em_consulta = true;
	
	//MITM TIME
	sleep(2);
	strcat(fifo_name_cl,"b");
	int cli = open(fifo_name_cl,O_RDWR | O_NONBLOCK);
	if(cli == -1){
		fprintf(stderr,RED "Falha ao abrir fifo do cliente/cliente saiu");
		exit(EXIT_FAILURE);
	}
	strcat(fifo_name_med,"b");
	int med = open(fifo_name_med,O_RDWR | O_NONBLOCK);
	if(med == -1){
		fprintf(stderr,RED "Falha ao abrir fifo do medico/medico saiu");
		exit(EXIT_FAILURE);
	}
	
	bool consulta_over=false;
	fd_set read_fds;
	int nfd;
	send_t msg;
	while(!consulta_over){
		// select para ver se vem do medico ou do cliente a mensagem de acabar a consulta
		FD_ZERO(&read_fds);
		FD_SET(cli,&read_fds);
		FD_SET(med,&read_fds);
		nfd = select(max(med,cli)+1,&read_fds,NULL,NULL,NULL);
		
		if(nfd == -1){
			perror(RED "\nErro na criacao do select");
			break; //break ou close dos pipes ?
		}
		if(FD_ISSET(cli,&read_fds)){
			memset(&msg,0,sizeof(msg));
			read(cli,&msg,sizeof(msg));
			//se for o cliente a dzr adeus
			if(!strcmp(msg.msg,"adeus\n")){
				dados->j->em_consulta = false;
				dados->k->em_consulta = false;
				apaga_lista_cliente(dados->j->pid,dados->listaespecialidades,1);
				pthread_cond_signal(&cond);
			}
			
		}
		
		if(FD_ISSET(med,&read_fds)){
			memset(&msg,0,sizeof(msg));
			read(med,&msg,sizeof(msg));
			if(!strcmp(msg.msg,"adeus\n")){
				dados->k->em_consulta = false;
				dados->j->em_consulta = false;
				apaga_lista_cliente(dados->j->pid,dados->listaespecialidades,1);
				pthread_cond_signal(&cond);
			}
		}
		
	}
}
void *timer(void *arg){
	Ttimer* dados = (Ttimer*) arg;
    while(1){
        sleep(repeat_time);
        printlista(dados->listaespecialidades);
    }
    
}
void* threadconsulta(void* param){
	Tconsulta* dados = (Tconsulta*) param;
	pthread_mutex_lock(&mutex);
	while(1){
		pthread_cond_wait(&cond,&mutex);
		for(int i=0;i<5;i++){
			if(dados->listaespecialidades[i].inicio != NULL && dados->listaespecialidades[i].inicioMed != NULL){
				for(struct Cliente* j = dados->listaespecialidades[i].inicio;j!=NULL;j = j->prox){
					if(!j->em_consulta){
						for(struct Medico* k = dados->listaespecialidades[i].inicioMed;k!=NULL;k = k->prox){
							if(!k->em_consulta){
								//pthread_cond_wait(&cond,&mutex);
								pthread_t chat;
								pthread_attr_t detach;
								pthread_attr_init(&detach);
								pthread_attr_setdetachstate(&detach,PTHREAD_CREATE_DETACHED);
								Tchat Tchat;
								Tchat.ptrRcv = dados->ptrRcv;
								Tchat.j = j;
								Tchat.k = k;
								Tchat.listaespecialidades = dados->listaespecialidades;
								pthread_create(&chat,&detach,chat_watcher,&Tchat);
								k->em_consulta = true;
								j->em_consulta = true;
								j->medico = k;
								k->cliente = j;
								//pthread_detach(chat);
								
							}
						}
					}
				}


			}
			

		}
		//pthread_mutex_unlock(&mutex);
	}

	pthread_mutex_unlock(&mutex);


}
void inserir_cliente_ordem(struct Especialidades lista[],struct Cliente cliente_inserir){

fprintf(stderr,BLUE "Nome do cliente na func lista ligada: %s",cliente_inserir.nome);

fprintf(stderr,BLUE "PRIORIDADE: %d \t ESPECIALIDADE: %s\n",cliente_inserir.prioridade, cliente_inserir.especialidade);



//Listas Desligadas

for(int i=0;i< 5;i++){

	if(!strcmp(cliente_inserir.especialidade,lista[i].especialidade)){
		//inserir no

		if(lista[i].inicio == NULL || lista[i].inicio->prioridade > cliente_inserir.prioridade){ // se n tem nos inserir na primeira posicao ou se o no a inserir tiver menor prioridade que o primeiro no

			struct Cliente* novo_no = (struct Cliente*) malloc(sizeof(struct Cliente));

			strcpy(novo_no->especialidade,cliente_inserir.especialidade);

			novo_no->prioridade = cliente_inserir.prioridade;

			strcpy(novo_no->nome,cliente_inserir.nome);

			strcpy(novo_no->fifo_nome,cliente_inserir.fifo_nome);
			novo_no->file_descriptor = cliente_inserir.file_descriptor;

			novo_no->pid = cliente_inserir.pid;

			novo_no->prox = lista[i].inicio;

			lista[i].inicio = novo_no;

			fprintf(stderr,"LISTA VAZIA ADICIONADO A CABECA\n");

			fprintf(stderr,"ADICIONANDO CLIENTE COM O NOME: %s\n",(lista[i].inicio)->nome);

			break;

		}

		// se nao, vamos percorrer a lista e ver ate onde o prox tiver menos prioridade e inserir antes

		struct Cliente* j;

		for(j=lista[i].inicio;j->prox != NULL; j=j->prox){ // enquanto nao o prox tiver null nao tamos no fim logo a proxima iteracao o j fica igual ao prox

			if(cliente_inserir.prioridade < (j->prox)->prioridade){ // se j prox tiver uma prioridade maior encontramos onde inserir o cliente

				//alocar memoria para novo cliente e copiar para la a informacao

				struct Cliente* novo_no = (struct Cliente*) malloc(sizeof(struct Cliente));

				strcpy(novo_no->especialidade,cliente_inserir.especialidade);

				novo_no->prioridade = cliente_inserir.prioridade;

				strcpy(novo_no->nome,cliente_inserir.nome);

				novo_no->pid = cliente_inserir.pid;

				strcpy(novo_no->fifo_nome,cliente_inserir.fifo_nome);

				novo_no->file_descriptor = cliente_inserir.file_descriptor;
				

				////////////

				novo_no->prox = j->prox;

				j->prox = novo_no;

				fprintf(stderr,"LISTA VAZIA ADICIONADO NO MEIO");

				break;

			}

		}

		// se nenhuma se verifica inserir no fim

		if(j->prox == NULL){

			fprintf(stderr,"LISTA VAZIA ADICIONADO NO FIM\n");

			struct Cliente* novo_no = (struct Cliente*) malloc(sizeof(struct Cliente));

			strcpy(novo_no->especialidade,cliente_inserir.especialidade);

			novo_no->prioridade = cliente_inserir.prioridade;

			strcpy(novo_no->nome,cliente_inserir.nome);

			novo_no->pid = cliente_inserir.pid;

			strcpy(novo_no->fifo_nome,cliente_inserir.fifo_nome);

			novo_no->file_descriptor = cliente_inserir.file_descriptor;

			j->prox = novo_no;

				//fprintf(stderr,"LISTA VAZIA ADICIONADO NO FIM");

		}

	}

}
	printlista(lista);
}
void insere_med(struct Especialidades lista[],struct Medico medico_inserir){
	sprintf(medico_inserir.fifo_nome,MEDICO_FIFO,medico_inserir.pid);
	for(int i=0;i< 5;i++){

		if(!strcmp(medico_inserir.especialidade,lista[i].especialidade)){ // inserir a cabeca
			if(lista[i].inicioMed == NULL){
				struct Medico* novo_no = (struct Medico*) malloc(sizeof(struct Medico));
				strcpy(novo_no->nome,medico_inserir.nome);
				strcpy(novo_no->especialidade,medico_inserir.especialidade);
				strcpy(novo_no->fifo_nome,medico_inserir.fifo_nome);
				novo_no->pid = medico_inserir.pid;
				novo_no->fd = medico_inserir.fd;
				

				novo_no->prox = lista[i].inicioMed;

				lista[i].inicioMed = novo_no;

				fprintf(stderr,"LISTA VAZIA ADICIONADO A CABECA\n");

				fprintf(stderr,"ADICIONANDO CLIENTE COM O NOME: %s\n",(lista[i].inicioMed)->nome);
				break;
			}

			struct Medico* j;

			for(j=lista[i].inicioMed;j->prox != NULL; j=j->prox); // inserir no fim
			if(j->prox == NULL){

			fprintf(stderr,"LISTA VAZIA ADICIONADO NO FIM\n");

			struct Medico* novo_no = (struct Medico*) malloc(sizeof(struct Medico));

			strcpy(novo_no->especialidade,medico_inserir.especialidade);

			strcpy(novo_no->nome,medico_inserir.nome);
			
			strcpy(novo_no->fifo_nome,medico_inserir.fifo_nome);

			novo_no->pid = medico_inserir.pid;

			novo_no->fd = medico_inserir.fd;

			j->prox = novo_no;

		}

		}
	}
	printlista(lista);
}
void* threadmed(void* param){
	TDadosMedico* dados = (TDadosMedico*) param;
	fprintf(stderr,"THREAD[%d]: ",dados->id);
	pthread_mutex_lock(&mutex);
	insere_med(dados->listaespecialidades,*dados->ptrmedico);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);


}
// ####### THREAD #######
void* threadle(void* param){
	Tdados* dados = (Tdados*) param;

	fprintf(stderr,"THREAD[%d]: ",dados->id);
	//Ypthread_mutex_lock(&mutex);
	//int res = read(s_fifo_fd, dados->ptrsendmsg,sizeof(*dados->ptrsendmsg));

	pthread_mutex_lock(&mutex);
	memset(dados->ptrRcv,0,sizeof(*dados->ptrRcv));
	if(write(*dados->stdinclassi,dados->ptrsendmsg->msg,strlen(dados->ptrsendmsg->msg)) == -1){	
		fprintf(stderr,RED "\nErro...\n");
	}

	//memset(dados->ptrRcv->msg,'\0',PIPE_BUF);
	if(read(*dados->stdoutclassi,dados->ptrRcv->msg,sizeof(dados->ptrRcv->msg)) == -1){								
		//printf(RED "Erro na leitura do pipe\n");		
		perror(RED "erro a ler");
	}
	
	memset(dados->ptrcliente->especialidade,'\0',TAM_MAX); // limpar restos que possa estar na struct
	
	//divide a string vinda do classificador
	char *especialidade = strtok(dados->ptrRcv->msg," ");
	int prioridade = atoi(strtok(NULL," "));
	
	//inserir dados na struct cliente

	//##### LISTA LIGADA #####
	fprintf(stderr,"ESPECIALIDADE 1: %s",dados->listaespecialidades[0].especialidade);
	
	int i;	
	int contador = 1;
	
	sprintf(dados->cli_fifo_name,CLIENT_FIFO,dados->ptrsendmsg->pid_client);
	strcpy(dados->ptrcliente->fifo_nome,dados->cli_fifo_name);
	strcpy(dados->ptrcliente->especialidade,especialidade);
	strcpy(dados->ptrcliente->nome,dados->ptrsendmsg->nome);
	dados->ptrcliente->file_descriptor = *dados->c_fifo_fd;
	dados->ptrcliente->prioridade = prioridade;
	dados->ptrcliente->pid = dados->ptrsendmsg->pid_client;
	inserir_cliente_ordem(dados->listaespecialidades,*dados->ptrcliente);
	for(i=0;i<5;i++){
		if(!strcmp(dados->listaespecialidades[i].especialidade, dados->ptrcliente->especialidade)){
			break;
		}
	}
	for(struct Cliente* j = dados->listaespecialidades[i].inicio; j != NULL;j=j->prox){
		if(j->pid == dados->ptrsendmsg->pid_client){
			for(struct Cliente* k = j->prox; k != NULL; k=k->prox){
				fprintf(stderr, "\nestamos aqui [pid:%d]-> %s",k->pid,k->nome);
				fprintf(stderr,"\nPosicao: %d",k->posicao_lista);
				k->posicao_lista = contador + 1;
				fprintf(stderr,"\nPosicao: %d",k->posicao_lista);
				sprintf(dados->ptrRcv->msg,"\nNome:%s\nEspecialidade:%s\nPrioridade:%d\nPosicao na fila de espera:%d\n",k->nome,k->especialidade,k->prioridade,k->posicao_lista);
				int fd = open(k->fifo_nome,O_WRONLY | O_NONBLOCK);
				if(fd == -1){
					fprintf(stderr,"Utente ja nao se encontra na lista\n");
				}
				if(write(fd,dados->ptrRcv,sizeof(*dados->ptrRcv))==-1){
					perror(RED "\nErro na escrita para o FIFO\n");
					exit(0);
				}
				close(fd);
			}
			break;
		}
		else{
			contador++;
		}
	}
	dados->ptrcliente->posicao_lista = contador;
	//so para debugging :D 
    fprintf(stderr,BLUE "\n\nNome: %s\n",dados->ptrcliente->nome);
    fprintf(stderr,BLUE "Especialidade: %s\n",dados->ptrcliente->especialidade);
    fprintf(stderr,BLUE "Prioridade: %d\n",dados->ptrcliente->prioridade);
    fprintf(stderr,BLUE "Posicao na lista de espera: %d\n",dados->ptrcliente->posicao_lista);
	fprintf(stderr,"TAMANHO STRUCT: %d",sizeof(*dados->ptrcliente));
	/////
	sprintf(dados->cli_fifo_name,CLIENT_FIFO,dados->ptrsendmsg->pid_client);

	*dados->c_fifo_fd = open(dados->cli_fifo_name,O_WRONLY | O_NONBLOCK);
	memset(dados->ptrRcv->msg,'\0',sizeof(dados->ptrRcv->msg));
	sprintf(dados->ptrRcv->msg,"\nNome:%s\nEspecialidade:%s\nPrioridade:%d\nPosicao na fila de espera:%d\n",dados->ptrcliente->nome,dados->ptrcliente->especialidade,dados->ptrcliente->prioridade,dados->ptrcliente->posicao_lista);

	if(c_fifo_fd == -1){
		perror(RED "\nErro na abertura do ficheiro/cliente fechou\n");
		exit(0);
	}
	if(write(*dados->c_fifo_fd,dados->ptrRcv,sizeof(*dados->ptrRcv))==-1){
		perror(RED "\nErro na escrita para o FIFO\n");
		exit(0);
	}
	close(c_fifo_fd);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
	//unlink(dados->cli_fifo_name);

	pthread_exit(NULL);

}


int main(int argc ,char *argv[]){
	int stdinPipe[2];
	int stdoutPipe[2];
	send_t send_msg;
    rcv_t recv_msg;
	cliente cliente;
	medico medico;
	int res = 0;
	char cli_fifo_name[50];
	memset(&send_msg,0,sizeof(send_msg));
	memset(&recv_msg,0,sizeof(recv_msg));
	memset(&cliente,0,sizeof(cliente));
	memset(&medico,0,sizeof(medico));
	//para select
	int nfd; 
	fd_set read_fds;
	//para thread
	Tdados dados;
	TDadosMedico dados_med;
	Tconsulta dados_consulta;
	Ttimer dados_timer;
	Tsinal_vida vida;
	memset(&dados,0,sizeof(dados));
	memset(&dados_med,0,sizeof(dados_med));
	memset(&dados_consulta,0,sizeof(dados_consulta));
	pthread_mutex_init(&mutex,NULL);
	pthread_t t_timer;
	pthread_t t_cliente;
	pthread_t t_medico;
	pthread_t consultas;
	pthread_t sinal_vida;

	// ##################### CLASSIFICADOR ########################
	if (pipe(stdinPipe) == -1){
		printf("An error occured");
		return 1;

	}
	if (pipe(stdoutPipe) == -1){
		close(stdinPipe[0]);
		close(stdinPipe[1]);
		printf("An error occured");
		return 1;

	}

	char* maxclientes_env = getenv("MAXCLIENTES");
	if(maxclientes_env == NULL){
		fprintf(stderr,"Nao encontrou variavel de ambiente MAXCLIENTES\n");
		exit(EXIT_FAILURE);
		
	}
	char* maxmedicos_env = getenv("MAXMEDICOS");
	if(maxmedicos_env == NULL){
		fprintf(stderr,"Nao encontrou variavel de ambiente MAXMEDICOS\n");
		exit(EXIT_FAILURE);
	}
	int maxclientes;
	int maxmedicos;
	char buffer; // so usado para error checking

	//error checking
	if(sscanf(maxclientes_env," %d %*c",&maxclientes,&buffer) == 2){
		fprintf(stderr,"Variaveis de ambiente mal definidas.\nMAXMEDICOS/MAXCLIENTES tem de ser um inteiro maior que 0\n");
		exit(EXIT_FAILURE);
	}
	if(sscanf(maxmedicos_env," %d %*c",&maxmedicos,&buffer) == 2){
		fprintf(stderr,"Variaveis de ambiente mal definidas.\nMAXMEDICOS/MAXCLIENTES tem de ser um inteiro maior que 0\n");
		exit(EXIT_FAILURE);
	}
	printf("INICIALIZANDO BALCAO\nNum de clientes maximo:%d\nNum de medicos maximo:%d\n",maxclientes,maxmedicos);
	if(maxclientes < 0 || maxmedicos < 0){
		fprintf(stderr,"Variaveis de ambiente mal definidas.\nMAXMEDICOS/MAXCLIENTES tem de ser um inteiro maior que 0\n");
		exit(EXIT_FAILURE);
	}
	/////
	
	

	char sintomas[PIPE_BUF];
	char output[PIPE_BUF];
	//printf(GREEN "\naqui vamos pro PID\n");
	if(signal(SIGINT, trataSinal) == SIG_ERR){
		perror(RED "\nNao foi possivel configurar o sinal SIGINT :C \n");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,GREEN "\nSinal SIGINT configurado com sucesso!\n");

	res = mkfifo(SERVER_FIFO_CLIENTE,0777);
	if(res == -1){
		perror(RED "\nNao foi possivel criar o FIFO do servidor :( \n");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,GREEN "\nFIFO do servidor cliente criado!\n");
	res = mkfifo(SERVER_FIFO_MEDICO,0777);
	if(res == -1){
		perror(RED "\nNao foi possivel criar o FIFO do servidor :( \n");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,GREEN "\nFIFO do servidor medico criado!\n");

	s_fifo_fd_cliente = open(SERVER_FIFO_CLIENTE, O_RDWR | O_NONBLOCK);
	if(s_fifo_fd_cliente == -1){
		perror(RED "\n Erro ao abrir o FIFO do servidor");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,GREEN "\nFIFO aberto para leitura e escrita bloqueante\n");

	s_fifo_fd_medico = open(SERVER_FIFO_MEDICO, O_RDWR | O_NONBLOCK);
	if(s_fifo_fd_medico == -1){
		perror(RED "\n Erro ao abrir o FIFO do servidor");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,GREEN "\nFIFO aberto para leitura e escrita bloqueante\n");


	
	int pid = fork();
	//printf(GREEN "\npid: %d",pid);
	if(pid == 0){
		//printf("CLASSIFICADOR");
		if(dup2(stdinPipe[0],STDIN_FILENO) == -1){
			fprintf(stderr,RED"Impossivel criar pipe");
			exit(EXIT_FAILURE);
		}
		if(dup2(stdoutPipe[1],STDOUT_FILENO) == -1){
			fprintf(stderr,RED "Impossivel criar pipe");
			exit(EXIT_FAILURE);
		}
			

		close(stdinPipe[1]);
		close(stdinPipe[0]);
		close(stdoutPipe[1]);
		close(stdoutPipe[0]);
		execl("./classificador","./classificador",NULL);
		

	}
	else{	
		close(stdinPipe[0]);
		close(stdoutPipe[1]);
		//array de especialidades com ponteiros para listas ligadas de clientes e medicos
		especialidades listaEspecialidades[5]={
			{0,0,"oftalmologia",NULL},
			{0,0,"neurologia",NULL},
			{0,0,"estomatologia",NULL},
			{0,0,"ortopedia",NULL},
			{0,0,"geral",NULL}
		};

		char buf[PIPE_BUF];
		int num_thread_em_uso=0;

		dados_consulta.listaespecialidades = listaEspecialidades;
		dados_consulta.ptrRcv = &recv_msg;

		pthread_create(&consultas,NULL,threadconsulta,&dados_consulta);
		dados_timer.listaespecialidades = listaEspecialidades;
		pthread_create(&t_timer,NULL,timer,&dados_timer);
		vida.listaespecialidades = listaEspecialidades;
		pthread_create(&sinal_vida,NULL,check_vida,&vida);

		while(1){
			FD_ZERO(&read_fds);
			FD_SET(0,&read_fds);
			FD_SET(s_fifo_fd_cliente,&read_fds);
			FD_SET(s_fifo_fd_medico,&read_fds);
			nfd = select(max(s_fifo_fd_medico,s_fifo_fd_cliente)+1,&read_fds,NULL,NULL,NULL);

			if(nfd == -1){
				perror(RED "\nErro na criacao do select");
				break; //break ou close dos pipes ?
			}

			if(FD_ISSET(0,&read_fds)){ // caso tenha alguma coisa no teclado
				memset(buf,0,sizeof(buf));
				read(STDIN_FILENO,buf,PIPE_BUF);
				teclado(buf,listaEspecialidades,stdinPipe[1]);
			}
			
			
			if(FD_ISSET(s_fifo_fd_cliente,&read_fds)){	// o pipe do server recebeu alguma coisa ?
			
				//fprintf(stderr,"NUM DE THREADS: %d",num_thread_em_uso);
				fprintf(stderr,"CRIANDO THREAD");
					
				int res = read(s_fifo_fd_cliente, &send_msg,sizeof(send_msg));

				if(res < sizeof(send_msg)){
					fprintf(stderr,RED "\nMensagem recebida esta incompleta\n");
					fprintf(stderr,"mensagem:[%s]",send_msg.msg);

				}
				if(!strcmp(send_msg.msg,"desisti")){
					apaga_lista_cliente(send_msg.pid_client,listaEspecialidades,1);
				}
				else{
					if(n_clientes < maxclientes){
					fprintf(stderr, GREEN "\nRecebido: ");
					fprintf(stderr, WHITE "%s \n",send_msg.msg);
					
					dados.id = num_thread_em_uso;
					dados.ptrsendmsg = &send_msg;
					dados.ptrRcv = &recv_msg;
					dados.s_fifo_fd = &s_fifo_fd_cliente;
					dados.c_fifo_fd = &c_fifo_fd;
					dados.stdinclassi = &stdinPipe[1];
					dados.stdoutclassi = &stdoutPipe[0];
					dados.ptrcliente = &cliente;
					dados.cli_fifo_name = cli_fifo_name;
					dados.listaespecialidades = listaEspecialidades;
					pthread_create(&t_cliente,NULL,threadle,&dados);
					num_thread_em_uso++;
					n_clientes++;
					//pthread_join(t[num_thread_em_uso],NULL);
				}
				else{
					kill(send_msg.pid_client,SIGUSR2);
				}
				}
			}
			if(FD_ISSET(s_fifo_fd_medico,&read_fds)){
				int res = read(s_fifo_fd_medico, &medico,sizeof(medico));

				if(res < sizeof(medico)){
					fprintf(stderr,RED "\nMensagem recebida esta incompleta\n");
					//fprintf(stderr,"mensagem:[%s]",send_msg.msg);

				}
				if(n_medicos < maxmedicos){
					fprintf(stderr,GREEN "\nAUTENTICANDO MEDICO COM O NOME: %s e especialidade %s",medico.nome,medico.especialidade);
					dados_med.ptrmedico = &medico;
					dados_med.listaespecialidades=listaEspecialidades;
					dados_med.id=num_thread_em_uso;
					dados_med.fd=&m_fifo_fd;
					pthread_create(&t_medico,NULL,threadmed,&dados_med);	
					n_medicos++;
				}
				else{
					kill(medico.pid,SIGUSR1);
				}


			}
		}

		close(stdinPipe[1]);
		close(stdinPipe[0]);
		close(stdoutPipe[1]);
		close(stdoutPipe[0]);
	}
	return 0;
								
}
// #duraesforever
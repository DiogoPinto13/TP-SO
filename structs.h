#ifndef STRUCTS_H
#define STRUCTS_H
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#define WHITE "\e[1;37m"
#define RED "\e[0;31m"
#define GREEN "\e[0;32m"
#define BLUE "\e[0;36m"
// nome do Fifo do server
#define SERVER_FIFO_CLIENTE "fifos/fifosvcli"
#define SERVER_FIFO_MEDICO "fifos/fifosvmed"

// nome do Fifo pra cada client 
#define CLIENT_FIFO "fifos/fifocli%d"

#define MEDICO_FIFO "fifos/fifomed%d"
#define TAM_MAX 64
#define MSG_SIZE 400
#define NUMTHREADS 256




typedef struct {
	pid_t pid_client;
	char nome[TAM_MAX];
	char msg[MSG_SIZE]; 

} send_t;

typedef struct{
	char fifo_name[TAM_MAX];
	char msg[MSG_SIZE];
} rcv_t;

struct Medico;
typedef struct Cliente{
	int pid,prioridade,posicao_lista,file_descriptor;
	char fifo_nome[TAM_MAX];
	bool em_consulta;
	char nome[TAM_MAX];
	char especialidade[TAM_MAX];
	struct Medico* medico;
	struct Cliente* prox;
	//struct Medico* medico;

}cliente;


typedef struct Especialidades{
	int n_medicos;
	int n_clientes;
	char especialidade[TAM_MAX];
	struct Cliente* inicio;
	
	struct Medico* inicioMed;

}especialidades;

typedef struct Medico{
	int pid;
	int fd;
	char fifo_nome[TAM_MAX];
	bool em_consulta;
	char nome[TAM_MAX];
	char especialidade[TAM_MAX];
	struct Cliente* cliente;
	struct Medico* prox;

}medico;

typedef struct{
	int id;
	int* fd;
	medico* ptrmedico;
	struct Especialidades* listaespecialidades;


}TDadosMedico;
typedef struct{
	int* s_fifo_fd; // precisa do fifo do server para ler ?
	int* c_fifo_fd;	// precisa do fifo do cli para escrever ?
	int* stdinclassi;
	int* stdoutclassi;
	char * cli_fifo_name;
	struct Especialidades* listaespecialidades;
	int id;
	rcv_t * ptrRcv; // precisa da estrutura para alguma coisa ?
	send_t * ptrsendmsg;
	cliente *  ptrcliente;
	//mutexs i guess
	pthread_mutex_t mutex;

	//por enquanto kekw
}Tdados;
typedef struct{
	struct Especialidades* listaespecialidades;
	rcv_t * ptrRcv;

}Tconsulta;
typedef struct{
	medico * k;
	cliente * j;
	rcv_t * ptrRcv;
	struct Especialidades* listaespecialidades;
}Tchat;
typedef struct{
	struct Especialidades* listaespecialidades;
}Ttimer;
typedef struct{
	struct Especialidades* listaespecialidades;
}Tsinal_vida;

#endif

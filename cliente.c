#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include "structs.h"


int balcao_fifo;
int cliente_fifo;
int md_fd;

void trataSinal(){
	fprintf(stderr,RED "\nVoce foi expulso do hospital\n");
    char fifo_nome[500];
    sprintf(fifo_nome,CLIENT_FIFO,getpid());
    unlink(fifo_nome);
    close (cliente_fifo);
    close(balcao_fifo);
	exit(EXIT_SUCCESS);
}

void trataSinal2(){
    fprintf(stderr,RED "\nO hospital fecho/hospital cheio\n");
    char fifo_nome[500];
    sprintf(fifo_nome,CLIENT_FIFO,getpid());
    unlink(fifo_nome);
    close (cliente_fifo);
    close(balcao_fifo);
	exit(EXIT_SUCCESS);
}

void trataSinal3(){
    balcao_fifo = open(SERVER_FIFO_CLIENTE,O_WRONLY|O_NONBLOCK );
    char fifo_nome[500];
    sprintf(fifo_nome,CLIENT_FIFO,getpid());
    unlink(fifo_nome);
    strcat(fifo_nome,"b");

    send_t msg;
    memset(&msg,0,sizeof(msg));
    strcpy(msg.msg,"desisti");
    msg.pid_client = getpid();
    write(balcao_fifo,&msg,sizeof(msg));

    memset(&msg,0,sizeof(msg));
    strcpy(msg.msg,"adeus\n");
    write(md_fd,&msg,sizeof(msg));

    unlink(fifo_nome);
    close(md_fd);
    close (cliente_fifo);
    close(balcao_fifo);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv){
    char fifo_nome[25];
    int leitura_res;
    cliente info;
    send_t send_msg;
    rcv_t recv_msg;
    fd_set read_fds;
    int nfd;
    char nome[TAM_MAX];
    memset(&send_msg,0,sizeof(send_msg));
    if(signal(SIGUSR1, trataSinal) == SIG_ERR){
		perror(RED "\nNao foi possivel configurar o sinal SIGUSR1 \n");
		exit(EXIT_FAILURE);
	}
    if(signal(SIGUSR2, trataSinal2) == SIG_ERR){
		perror(RED "\nNao foi possivel configurar o sinal SIGUSR2 \n");
		exit(EXIT_FAILURE);
	}
    if(signal(SIGHUP, trataSinal3) == SIG_ERR){
		perror(RED "\nNao foi possivel configurar o sinal SIGHUP \n");
		exit(EXIT_FAILURE);
	}
    if(signal(SIGINT, trataSinal3) == SIG_ERR){
		perror(RED "\nNao foi possivel configurar o sinal SIGINT \n");
		exit(EXIT_FAILURE);
	}
	if(argc >= 2){
        
		for(int i = 1;i< argc;i++){
			strcat(send_msg.nome,argv[i]);
			strcat(send_msg.nome," ");

		}
		printf(GREEN "AUTENTICADO COM O NOME:	");
        printf(BLUE "%s \n" GREEN ,send_msg.nome);
	}
	else{
		printf(RED "Error, missing name.\nMust be called like this: ./cliente <nome>\n");
        exit(EXIT_FAILURE);

	}
    send_msg.pid_client = getpid();
    sprintf(fifo_nome,CLIENT_FIFO,send_msg.pid_client);
    if(mkfifo(fifo_nome,0777) == -1){
        fprintf(stderr,RED "Impossivel criar pipe\n");
		exit(EXIT_FAILURE);
    }
    printf(GREEN "FIFO do cliente criado com sucesso!\n");



    

    printf(GREEN "FIFO do server aberto!\n");

    cliente_fifo = open(fifo_nome,O_RDWR);
    if(cliente_fifo == -1){
        fprintf(stderr,RED "Falha ao abrir FIFO do cliente\n");
        unlink(fifo_nome);
        exit(EXIT_FAILURE);
    }
    printf(GREEN "FIFO do cliente aberto com sucesso!\n");

    memset(send_msg.msg,'\0',sizeof(send_msg.msg));

	printf(WHITE "Quais os seus sintomas?: ");
    fgets(send_msg.msg,sizeof(send_msg.msg),stdin);
    
    balcao_fifo = open(SERVER_FIFO_CLIENTE,O_WRONLY|O_NONBLOCK );
    if(balcao_fifo == -1){
        fprintf(stderr,RED "Falha ao ligar ao servidor/servidor nao esta a correr\n");
        unlink(fifo_nome);
        exit(EXIT_FAILURE);
    }
    
    write(balcao_fifo,&send_msg,sizeof(send_msg));
    close(balcao_fifo);
    while(1){
        memset(fifo_nome,0,sizeof(fifo_nome));
        sprintf(fifo_nome,CLIENT_FIFO,send_msg.pid_client);
        leitura_res = read(cliente_fifo,&recv_msg,sizeof(recv_msg));
        if(leitura_res == sizeof(recv_msg)){
            fprintf(stderr,"\n%s\n\n",recv_msg.msg);
            if(strcmp(recv_msg.fifo_name,"")){
                fprintf(stderr,"VOU ABRIR O FIFO :%s\n",recv_msg.fifo_name);
                md_fd = open(recv_msg.fifo_name,O_WRONLY | O_NONBLOCK);
                if(md_fd == -1){
                    fprintf(stderr,"Erro ao abrir fifo do medico\n");
                    exit(EXIT_FAILURE); // sair secalhar e extremo
                }
                fflush(stdin);
                strcat(fifo_nome,"b");
                if(mkfifo(fifo_nome,0777) == -1 ){
                    fprintf(stderr,RED "Falha ao criar FIFO para consulta\n");
                }
                int fifo_back = open(fifo_nome, O_WRONLY);
                if(fifo_back == -1){
                    fprintf(stderr,RED "Falha ao abrir FIFO para consulta\n");
                }
                while(1){
                    FD_ZERO(&read_fds);
                    FD_SET(0,&read_fds);
                    FD_SET(cliente_fifo,&read_fds);
                    nfd = select(cliente_fifo+1,&read_fds,NULL,NULL,NULL);

                    if(nfd == -1){
                        perror(RED "\nErro na criacao do select");
                        break; //break ou close dos pipes ?
                    }

                    if(FD_ISSET(0,&read_fds)){ // caso tenha alguma coisa no teclado
                        char buf[MSG_SIZE];
                        memset(buf,0,sizeof(buf));
                        read(STDIN_FILENO,buf,MSG_SIZE);
                        memset(send_msg.msg,0,sizeof(send_msg.msg));
                        strcpy(send_msg.msg,buf);
                        fprintf(stderr,BLUE "Utente: ");
                        fprintf(stderr,WHITE "%s\n",send_msg.msg);
                        write(md_fd,&send_msg,sizeof(send_msg));
                        if(!strcmp(send_msg.msg,"adeus\n")){
                            write(fifo_back,&send_msg,sizeof(send_msg));
                            unlink(fifo_nome);
                            close(fifo_back);
                        }
                        
                    }
                    if(FD_ISSET(cliente_fifo,&read_fds)){
                        int res = read(cliente_fifo, &send_msg,sizeof(send_msg));

                        if(res < sizeof(send_msg)){
                            fprintf(stderr,RED "\nMensagem recebida esta incompleta\n");
                            fprintf(stderr,RED "mensagem:[%s]",send_msg.msg);

                        }
                        fprintf(stderr,BLUE "Medico: ");
                        fprintf(stderr,WHITE "%s",send_msg.msg);
                        if(!strcmp(send_msg.msg,"adeus\n")){
                            write(fifo_back,&send_msg,sizeof(send_msg));
                            unlink(fifo_nome);
                            char close_fi[100];
                            sprintf(close_fi,CLIENT_FIFO,getpid());
                            unlink(close_fi);
                            close(fifo_back);
                            break;
                        }
                    }
                }
            }
        }
        else{
            printf(RED "Mensagem corrompida tamanho struct : %d",leitura_res);
        }
    }

    close(cliente_fifo);
    //close(balcao_fifo);
    unlink(fifo_nome);

}

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
int medico_fifo;
int md_fd;

void trataSinal(){
	fprintf(stderr,RED "\nO hospital fecho/hospital cheio\n");
    char fifo_nome[500];
    sprintf(fifo_nome,MEDICO_FIFO,getpid());
    unlink(fifo_nome);
    close (medico_fifo);
    close(balcao_fifo);
	exit(EXIT_SUCCESS);
}
void trataSinal3(){
    char fifo_nome[500];
    sprintf(fifo_nome,MEDICO_FIFO,getpid());
    unlink(fifo_nome);
    strcat(fifo_nome,"b");
    send_t msg;
    memset(&msg,0,sizeof(msg));
    strcpy(msg.msg,"adeus\n");
    write(md_fd,&msg,sizeof(msg));
    
    close(md_fd);
    close (medico_fifo);
    close(balcao_fifo);
	exit(EXIT_SUCCESS);
}
void trataSinal2(){
    fprintf(stderr,RED "\nVoce foi expulso do hospital\n");
    char fifo_nome[500];
    sprintf(fifo_nome,MEDICO_FIFO,getpid());
    unlink(fifo_nome);
    close (medico_fifo);
    close(balcao_fifo);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv){
    char fifo_nome[25];
    int leitura_res;
    int nfd;
    int nfd_desis;
    medico info;
    fd_set read_fds;
    fd_set read_fds_desis;
    cliente info_cliente;
    send_t send_msg;
    rcv_t recv_msg;
    if(signal(SIGHUP, trataSinal3) == SIG_ERR){
        perror(RED "\nNao foi possivel configurar o sinal SIGHUP \n");
        exit(EXIT_FAILURE);
    }
    if(signal(SIGINT, trataSinal3) == SIG_ERR){
        perror(RED "\nNao foi possivel configurar o sinal SIGINT \n");
        exit(EXIT_FAILURE);
    }
    if(signal(SIGUSR1, trataSinal) == SIG_ERR){
		perror(RED "\nNao foi possivel configurar o sinal SIGUSR1 \n");
		exit(EXIT_FAILURE);
	}
    if(signal(SIGUSR2, trataSinal2) == SIG_ERR){
		perror(RED "\nNao foi possivel configurar o sinal SIGUSR2 \n");
		exit(EXIT_FAILURE);
	}
    if(argc >= 3){
        strcpy(info.nome,argv[1]);
        strcpy(info.especialidade,argv[2]);

    }
    else{
        fprintf(stderr,RED "Falta de parametros, ./medico <nome> <especialidade>");
        exit(EXIT_FAILURE);
    }
    info.pid = getpid();
    sprintf(fifo_nome,MEDICO_FIFO,info.pid);
    if(mkfifo(fifo_nome,0777)==-1){
        fprintf(stderr,RED "Erro ao criar fifo");
        exit(EXIT_FAILURE);
    }

    medico_fifo = open(fifo_nome,O_RDWR);
    if(medico_fifo == -1){
        fprintf(stderr,RED "Falha ao abrir FIFO do cliente\n");
        unlink(fifo_nome);
        exit(EXIT_FAILURE);
    }
    printf(GREEN "FIFO do cliente aberto com sucesso!\n");

    balcao_fifo = open(SERVER_FIFO_MEDICO,O_WRONLY|O_NONBLOCK );
    if(balcao_fifo == -1){
        fprintf(stderr,RED "Falha ao ligar ao servidor/servidor nao esta a correr\n");
        unlink(fifo_nome);
        exit(EXIT_FAILURE);
    }
    write(balcao_fifo,&info,sizeof(info));
    close(balcao_fifo);
    
    while(1){
        memset(fifo_nome,0,sizeof(fifo_nome));
        sprintf(fifo_nome,MEDICO_FIFO,info.pid);
        FD_ZERO(&read_fds_desis);
        FD_SET(0,&read_fds_desis);
        FD_SET(medico_fifo,&read_fds_desis);
        nfd = select(medico_fifo+1,&read_fds_desis,NULL,NULL,NULL);
        if(nfd == -1){
            perror(RED "\nErro na criacao do select");
            break; //break ou close dos pipes ?
        }
        if(FD_ISSET(0,&read_fds_desis)){ // caso tenha alguma coisa no teclado
            char buf[MSG_SIZE];
            memset(buf,0,sizeof(buf));
            read(STDIN_FILENO,buf,MSG_SIZE);
            if(strcmp(buf,"sair")){
                unlink(fifo_nome);
                close (medico_fifo);
                close(balcao_fifo);
                exit(EXIT_SUCCESS);
            }
        }

        if(FD_ISSET(medico_fifo,&read_fds_desis)){
           leitura_res = read(medico_fifo,&recv_msg,sizeof(recv_msg));
        if(leitura_res == sizeof(recv_msg)){
            fprintf(stderr,"%s\n",recv_msg.msg);
            if(strcmp(recv_msg.fifo_name,"")){
                md_fd = open(recv_msg.fifo_name,O_RDWR);
                if(md_fd == -1){
                    fprintf(stderr,"Erro ao abrir fifo do medico");
                    exit(EXIT_FAILURE); // sair secalhar e extremo
                }
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
                    FD_SET(medico_fifo,&read_fds);
                    nfd = select(medico_fifo+1,&read_fds,NULL,NULL,NULL);

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
                        fprintf(stderr,BLUE "Medico: ");
                        fprintf(stderr,WHITE "%s\n",send_msg.msg);
                        write(md_fd,&send_msg,sizeof(send_msg));
                        if(!strcmp(send_msg.msg,"adeus\n")){
                            write(fifo_back,&send_msg,sizeof(send_msg));
                            unlink(fifo_nome);
                            close(fifo_back);
                            break;
                        }
                    }

                    if(FD_ISSET(medico_fifo,&read_fds)){
                        //fprintf(stderr,"\nRECEBI UMA COISA");
                        int res = read(medico_fifo, &send_msg,sizeof(send_msg));

                        if(res < sizeof(send_msg)){
                            fprintf(stderr,RED "\nMensagem recebida esta incompleta\n" WHITE);
                            fprintf(stderr,"\nmensagem:[%s]",send_msg.msg);

                        }
                        fprintf(stderr,BLUE "Utente: ");
                        fprintf(stderr,WHITE "%s",send_msg.msg);
                        if(!strcmp(send_msg.msg,"adeus\n")){
                            fprintf(stderr,"Utente saiu. Acabou a consulta");
                            write(fifo_back,&send_msg,sizeof(send_msg));
                            unlink(fifo_nome);
                            close(fifo_back);
                            break;
                        }
                    }
                }
            }
        }
        }
        
        else{
            printf(RED "Mensagem corrompida tamanho struct : %d",leitura_res);
        }
    }

    close(medico_fifo);
    //close(balcao_fifo);
    unlink(fifo_nome);

}
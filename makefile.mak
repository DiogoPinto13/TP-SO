all:
	@echo "A compilar o projeto..."
	@gcc -o balcao balcao.c -pthread
	@gcc -o medico medico.c
	@gcc -o cliente cliente.c

balcao:
	@gcc -o balcao balcao.c -pthread

cliente:
	@gcc -o cliente cliente.c

cleanFifos:
	@echo "A apagar os fifos..."
	@rm fifos/*

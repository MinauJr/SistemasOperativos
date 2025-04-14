CC=gcc
CFLAGS= -Wall -g -pthread

SRC=authEngine.c authReqManager.c backoffice.c functions.c functionsBackoffice.c functionsMobileUser.c mobile.c monitorEngine.c systemManagerMAIN.c
OBJ=$(SRC:.c=.o)

# executáveis
EXECUTABLE=5g_auth_platform mobile_user backoffice_user


all: 5g_auth_platform mobile_user backoffice_user


5g_auth_platform: systemManagerMAIN.o functions.o monitorEngine.o authReqManager.o authEngine.o
	$(CC) -o $@ $^ $(CFLAGS)

mobile_user: mobile.o functionsMobileUser.o
	$(CC) -o $@ $^ $(CFLAGS)

backoffice_user: backoffice.o functionsBackoffice.o
	$(CC) -o $@ $^ $(CFLAGS)


%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

#make clean
.PHONY: clean

clean:
	rm -f $(OBJ) $(EXECUTABLE)


//don't forgot to delate the debuger
//find this
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h> 
#include <semaphore.h>
#include <errno.h>


#define sizeofBuf 14000

//#define PORT_NUMBER 8888
#define statue404 "HTTP/1.1 404 Not Found\r\n"
#define statueOK  "HTTP/1.1 200 OK\r\n"
#define statueNETERROR "HTTP/1.1 500 Internal Server Error\r\n"
#define statueBADREQUEST "HTTP/1.1 400 Bad Request\r\n"
#define statueFORBID "HTTP/1.1 403 Forbidden\r\n"
#define statueCREATED "HTTP/1.1 201 Created\r\n"

int parseMessage(int cl, char* operation, char* fileID, char* buffer, int* head);
int clientGetFile(int cl, char* fileID, char* buffer);
int clientPutFile(int cl, char* fileID, int length, char* buffer, int* head);
int getFileHeader(int cl, char* buffer);
void moveDataForward(char* buffer, int* head);
void getHeaderPointer(char* buffer, int* head);
void *handlingOneRequest(void* ThreadID);

sem_t* sem;
sem_t* mainSem;
sem_t* lock;
//sem_t* logLock;
int* statue;
int* clBox;
//int lock = 0;

int logFilePointer = 0;
sem_t logSem;

//creat a struct
typedef struct packetObj{
	int ID;
	int* clBox;
	int* statue;
	sem_t* sem;
	sem_t* mainSem;
	sem_t* lock;
	//sem_t* logLock;
}packetObj;

typedef struct packetObj* packet;

int main(int argc, char* argv[]){
	dprintf(1, "number of argument: %d\n", argc);
	int portNumber = 0;
	char* hostName;
	int numThread = 4;
	//handing some argument error
	if(argc == 5){
		portNumber = atoi(argv[4]);
		hostName = argv[3];
		numThread = atoi(argv[2]);
	}
	else if(argc == 3){
		portNumber = atoi(argv[2]);
		hostName = argv[1];
	}
	else if(argc == 7){
		portNumber = atoi(argv[6]);
		hostName = argv[5];
		numThread = atoi(argv[2]);
	}

	
	//char sevMes[300];
	//client use the command to connect with server
	//connect(sock, (struct sockaddr *)&addr, sizeof(sddr));

	//this build a standart of the sockey in address

	struct hostent *hent = gethostbyname(hostName);//"localhost" take the first token of the argument
	struct sockaddr_in addr;
	//copy the information into the 
	//printf("line 53\n");
	memcpy(&addr.sin_addr.s_addr, hent->h_addr, hent->h_length);

	//printf("%s\n", strtok(argv[1], ":"));

	addr.sin_port = htons(portNumber); //8888
	addr.sin_family = AF_INET;


	//creat a socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);

	//socket setup for server
	int enable = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	
	listen(sock, 0);

	write(1, "./start Listening\n", strlen("./start Listening\n"));

	//cl is the file descripter for receiverd and get
	//int count = 0;

	
	
	//allocate space for an array
	sem = (sem_t*) calloc(numThread, sizeof(sem_t));
	lock = (sem_t*) calloc(numThread, sizeof(sem_t));
	mainSem = (sem_t*) calloc(1, sizeof(sem_t));
	//logLock = (sem_t*) calloc(1,sizeof(sem_t));
	statue = (int*) calloc(numThread, sizeof(int));
	clBox = (int*) calloc(numThread, sizeof(int));

//initialize all the semphore to 0
	for(int i = 0; i < numThread; i++){
		int s = sem_init(&sem[i], 0, 0);
		int	k =	sem_init(&lock[i], 0, 1);
		if(s != 0){
			//printf("sem initial ERROR\n");
			exit(0);
		}
		if(k != 0){
			//printf("sem initial ERROR\n");
			exit(0);
		}
	}

	//printf("thread created now !!\n");

	
	//threadIDArray = (threadID*)malloc(numThread*sizeof(threadID));
	packet* p = (packet *) calloc(numThread, sizeof(packet));
	for(int i = 0; i < numThread; i++){
	    p[i] = (packet) malloc(sizeof(packetObj));
		p[i]->ID = i;
		p[i]->sem = &(sem[i]);
		//p[i]->lock = &(lock[i]);
		p[i]->mainSem = mainSem;
		p[i]->clBox = &(clBox[i]);
		p[i]->statue = &(statue[i]);
		//p[i]->logLock = logLock;
	}

	//creat n thread
	//declear n thread
	pthread_t* threadID = (pthread_t*) calloc(numThread, sizeof(pthread_t));
	for(int i = 0; i < numThread ; i++){
		int error = pthread_create(&threadID[i], NULL, &handlingOneRequest, (void*) p[i]);
        if (error != 0){
            //printf("\nThread can't be created :[%s]\n", strerror(error)); 
        } else {
	    //printf("\nThread created\n");
        }
	}

	//initializa a main thread sem initializa as n - 1

	sem_init(mainSem, 0, numThread -1);
	//sem_init(logLock, 0, 1);

	//put all the client into a queue


	//use a while loop to handling job to all thread
	//use another while loop to find which one is empty
	int i = 0;
	while(1){
		//accet a client and put cl into a queue

		//find a free thread
		//printf("loop running\n");
		//sem_wait(&lock[i]);
		if(statue[i] == 0){//if we find a free thread
		//sem_post(&lock[i]);
			//printf("%d, will accept client\n", i);
			//sem_wait(&lock[i]);
			clBox[i] = accept(sock, NULL, NULL); //function can cause it pause
			//printf("%d, accepted clent\n", i);

			//sem_wait(&lock[i]);
			//int* statueCP = &statue[i];
			//dprintf(1,"before the statue pointer is:                                               %p\n", &statue[i]);
			statue[i] = 1;
			//printf("after statue pointer:                                                          %p\n", &statue[i]);
			//sem_post(&lock[i]);

			//printf("%d, set statue to 1 cpmplete\n", i);
			sem_post(&sem[i]);
			//printf("%d, sem sem_post\n", i);
			//main thread sem down
			sem_wait(mainSem);
			//printf("main thread working\n");
			//sem_post(&lock[i]);
			//sem_wait(mainSem);	//try to stop the loop
		}
	
		i++;
		i = i % numThread;
		
	}

	//test
	//clBox[0] = accept(sock, NULL, NULL);
	//sem_post(&sem[0]);

}















void *handlingOneRequest(void* pack){

	char buffer[sizeofBuf+1];
	//put null into last position
	int head = 0;

	//cast to packet
	packet p = (packet) pack;

	//open the packet
	//printf("%d\n", ID);

	int IDT = p->ID;
	int IDTcp = IDT;
	//dprintf(1, "IDT pointer:                                                          %p\n", &IDT);
	sem_t* semPT = p->sem;
	//sem_t* semPTcp = semPT;
	sem_t* mainSemT = p->mainSem;
	//sem_t* logLockT = p->logLock;
	//sem_t* lockT = p->lock;
	//int* clBoxT = p->clBox;
	//int* statueT = p->statue;

	//memcpy(&semP,&&sem[ID], sizeof(sem_t*));

	while(1){
		//printf("before statue pointer: %p\n", statueT);
		//sem_wait for the crosponding value
		//printf("%d semphore wait to go down\n", IDT);
		//printf("before sem pointer: %p\n", semPT);
		sem_wait(semPT);
		//printf("after sem pointer: %p\n", semPT);

		//sem_wait(&lock[ID]);
		IDT = IDTcp;

		//printf("%d semiphore go down\n", IDT);

		int cl = clBox[IDT];

		dprintf(1, "--------------------------------- %d handling request ---------------------------------\n", IDT);
		//write(1, sevMes, strlen(sevMes));

		//printf("/.--- %d request\n", count);

		//reset the buffer
		memset(buffer, 0, sizeofBuf+1);
		IDT = IDTcp;
		//strcpy(buffer, "");
		//int end = 0;
		//printf("  wipe buffer: the end of buffer is%p\n", buffer + strlen(buffer));
		//printf("the buffer should end with         %p\n", buffer + sizeofBuf + 1);
		int i = 0;

		while(1){
			/*if(i == 1){
				break;
			}
			i++;*/
			i++;
			//only get more data when buffer is not full

			//if the socket is empty skip this step
			//if(end != -2){
			
			if(strstr(buffer, "\r\n\r\n") == NULL){
				getFileHeader(cl, buffer);
				//printf("getFileHeader: the end of buffer is%p\n", buffer + strlen(buffer));
				//printf("the buffer should end with         %p\n", buffer + sizeofBuf + 1);
				IDT = IDTcp;
			}
			//}

			//handle error message
			//if the header is empty
		
			//printf("print buffer:\n%s\n", buffer);

			//parse the message

			//get the offset of header
			//dprintf(1, "%d: the buffer is:%sEND\n\n\n\n", i, buffer);
			


			getHeaderPointer(buffer, &head);
			IDT = IDTcp;
			//dprintf(1, "the head is :%d\n", head);
			//printf("getHeaderPointer: the end of buffer is%p\n", buffer + strlen(buffer));
			//printf("the buffer should end with            %p\n", buffer + sizeofBuf + 1);

			//dprintf(1, "after getHeaderPointer\n");
			if(head == -1){
				break;
			}
		
			char operation[3];
			char fileID[27];

			int length = parseMessage(cl, operation, fileID, buffer, &head);
			IDT = IDTcp;
			//printf("\noperation: %s  fileID: %s \n", operation, fileID);
			//printf("length is %d\n", length);
			//dprintf(1, "after parseMessage\n");
			//printf("parseMessage: the end of buffer is%p\n", buffer + strlen(buffer));
			//printf("the buffer should end with        %p\n", buffer + sizeofBuf + 1);

			//remove the header
			moveDataForward(buffer, &head);
			IDT = IDTcp;

			//dprintf(1, "after move date forward\n");
			//printf("moveDataForward: the end of buffer is%p\n", buffer + strlen(buffer));
			//printf("the buffer should end with           %p\n", buffer + sizeofBuf + 1);
			
			
			//dprintf(1, "operation:  %s\nfileID:  %s\ncontent-length:  %d\n\n", operation, fileID, length);
			//write(1, sevMes, strlen(sevMes));

			if(length == -1){
				continue;
			} 

			//??????????????????????????????????????????????/when should I send OK
		
			//printf("cl is %d\n", cl);
			//if the operation is get
			//printf("start handling request\n\n");

			if(strcmp(operation, "GET") == 0){
				//int clientGetFile(int cl, char* fileID, char* str){
				//printf("Get File:\n");
				//dprintf(1, "Get File:\n");
				//write(1, sevMes, strlen(sevMes));

				clientGetFile(cl, fileID, buffer);
				IDT = IDTcp;
				//printf("getFile: the end of buffer is%p\n", buffer + strlen(buffer));
				//printf("the buffer should end with   %p\n", buffer + sizeofBuf + 1);
				//dprintf(cl, "Content-Length: %d\r\n", length);
			}

			//if the the operation is PUT
			else if(strcmp(operation, "PUT") == 0){
				//int clientPutFile(int cl, char* fileID, char* str, int length)
				//printf("PUT File:\n");
				//dprintf(1,"PUT File:\n");
				//write(1, sevMes, strlen(sevMes));

				clientPutFile(cl, fileID, length, buffer, &head);
				IDT = IDTcp;
				//printf("putFile: the end of buffer is%p\n", buffer + strlen(buffer));
				//printf("the buffer should end with   %p\n", buffer + sizeofBuf + 1);
			}
		
		}
		dprintf(1, "---------------------------------%d: finish handling request ---------------------------------\n", IDT);

		//*statueT = 0;
		
		//main thread sem up
		//printf("after sem pointer: %p\n", semP);
		IDT = IDTcp;
		sem_post(mainSemT);
		IDT = IDTcp;
		//printf("%d called mainSem up\n", IDT);
		//dprintf(1,"before the statue pointer is:%p\n", &statueT);
		dprintf(1,"before the statue pointer is:                                               %p\n",(void*)&statue[IDT]);
		//printf("the buffer should end with      %p\n", buffer + sizeofBuf + 1);
		//int* statueTCP = statueT;
		//sem_wait(lockT);
		statue[IDT] = 0;
		IDT = IDTcp;
		//sem_post(lockT);
		//printf("%d set statue to 0\n", IDT);
		dprintf(1, "after statue pointer:                                                          %p\n",(void*)&statue[IDT]);
		//IDT = IDTcp;
		//printf("the buffer should end with      %p\n", buffer + sizeofBuf + 1);

		//semP = &sem[ID];
		//sem_post(&lock[ID]);
	}
}



//this function take message as input, then pass the operation name and fileID by refereance
//then return the length of required, if there is no requirement of length, return -1
//use function sscanf(): sscanf (buffer,"%s %d",name,&age);
int parseMessage(int cl, char* operation, char* fileID, char* buffer, int* head){
	//printf("%s", buffer);
	//parse the first line of message
	//get the operation, file ID
	char lengthStr[100];
	char length[100];

	char headerLine[400];

	/*
	while sscanf is not NULL
		if(i == 1)
			get the first line and parse the message
			print some error message 
		else{
			get next line and pasre the information
		}

	*/
	int i = 0, index = 0;
	char* linePointer = buffer;
	int n;
	while(sscanf(linePointer, "%401[^\r\n]%n", headerLine, &n) == 1){
		//printf("%d\n", n);
		//printf("%s\n", headerLine);
		if(i == 0){
			sscanf(headerLine, "%s %s %*s", operation, fileID);

			//check if the header is valid
			//neighter GET or PUT respond badrequest (invalide request)
			if( !(strcmp(operation, "GET") == 0 || strcmp(operation, "PUT") == 0) )
			{
			write(cl, statueBADREQUEST, strlen(statueBADREQUEST));
			dprintf(cl, "Content-Length: %d\r\n", 0);
			dprintf(cl, "\r\n");
			//printf("parseMessage: not get or put\n");
			return -1;
			}
			else if(strlen(fileID) != 27){
			write(cl, statueBADREQUEST, strlen(statueBADREQUEST));
			dprintf(cl, "Content-Length: %d\r\n", 0);
			dprintf(cl, "\r\n");
			//printf("%lu\n", strlen(fileID));
			//printf("parseMessage: length is not 27\n");
			return -1;
			}
			//???????????????????????????????????????????????????????? how about / or some other char in file ID
			else if(strstr(fileID, "/") != NULL){
			write(cl, statueBADREQUEST, strlen(statueBADREQUEST));
			dprintf(cl, "Content-Length: %d\r\n", 0);
			dprintf(cl, "\r\n");
			//printf("%lu\n", strlen(fileID));
			//printf("parseMessage: length is not 27\n");
			return -1;
			}
			i++;
		}


		else{
			sscanf(headerLine, "%s %s", lengthStr, length);

			if(strcmp(lengthStr, "Content-Length:") == 0){
				return atoi(length);
			}
		}

		//modify the pointer
		index = index + n + 2;
		if(index >= (*head)-2){
			break;
		}
		linePointer = &buffer[index];
	}
	return 0;
}

//the function handle the GET request
int clientGetFile(int cl, char* fileID, char* buffer){

	//let's get use the rest of space in buffer
	int sizeofStr = sizeofBuf - strlen(buffer) + 1 - 1;
	char* str = &buffer[strlen(buffer) + 1];
	memset(str, 0, sizeofStr); 

	//open the file client request
	//chmod(fileID, S_IRWXU);
	int inputFild = open(fileID ,O_RDONLY);
    //printf("%d\n", inputFild);
	//201 (Created)!!
	
	//if the file doesn't exist
	if(inputFild < 0){
		
		write(cl, statue404, strlen(statue404));
		return -1;
	}

	write(cl, statueOK, strlen(statueOK));

	//count the content length we need to send
	int size = -1, sumSize = 0;
	while(size != 0){
		size = read(inputFild, str, sizeofStr);
		sumSize = sumSize + size;
	}
	dprintf(cl, "Content-Length: %d\r\n\r\n", sumSize);

	//close and reopen the file
	close(inputFild);
	inputFild = open(fileID ,O_RDONLY);

	//raed 28KB and write into client socket
	int readSize = 0, writeSize = 0;
	int sumOfWrite = 0, sendLength = 0;
	
	do{ //read from local directory
		readSize = read(inputFild, str, sizeofStr);

		//printf("readFile size: %d\n", readSize);
		//if we send everything we can, then the size we send is the size we read
		sendLength = sendLength + readSize;
		if(readSize == 0){
			break;
		}
		//write how much we just read, multiple times if necessary
		do{//repeat to write until it finish
			writeSize = write(cl, str, readSize);

			//printf("sendFile size: %d\n", writeSize);
			//printf("%s\n", str);
			//if the write size == -1, which means the Internet goes down
			if(writeSize == -1){
				write(cl, statueNETERROR, strlen(statueNETERROR));
				//printf("clientGetFile\n");
			}
			//find how much we write
			sumOfWrite = sumOfWrite + writeSize;
		}
		while(readSize > sumOfWrite);//repeat if the sum of read != sum of write
		sumOfWrite = 0;
	}
	while(1);


	//send the length of message
	//dprintf(1, "Content-Length: %d\r\n", sendLength);

	//close the file
	close(inputFild);

	memset(str, 0, sizeofStr); 
	str = NULL;

	return 0;
}

//recive the file from the client and write the file into local directory
int clientPutFile(int cl, char* fileID, int length, char* buffer, int* head){
	//printf("here\n");

	int readSize = 0, writeSize = 0;
	int lengthMod = 0, ONOFF = 1;
	int lengthSize = length;
	int lengthRead = 0;

	if(length > 0){
		lengthMod = 1;
	}
	//overwrite the file if it does exist, else creat one

	int outField = open(fileID, O_WRONLY | O_CREAT | O_TRUNC);
	write(cl, statueCREATED, strlen(statueCREATED));

	if(length == 0){
		write(cl, statueOK, strlen(statueOK));
		
		return 0;
	}
	//printf("here\n");
	//printf("out field is %d\n", outField);

	//if the buffer have all we need to put
	//int t = strlen(buffer);
	//printf("%d\n", length);
	//printf("\nhere\n");
	if((int)strlen(buffer) > length){
		int l = write(outField, buffer, length);
		//move the memory forward
		//printf("\nhere is the seg fault\n");
		*head = l;
		moveDataForward(buffer, head);
		//printf("in the put block\n");
		//write(cl, statueOK, strlen(statueOK));
		close(outField);
		chmod(fileID, S_IRWXU);
		dprintf(cl, "Content-Length: %d\r\n", 0);
		dprintf(cl, "\r\n");

		return 0;
	}
	//printf("\nhere\n");
	//if buffer doesn't have all we need

	//write the left pure data into buffer
	else if(strlen(buffer) != 0){
		int t = write(outField, buffer, strlen(buffer));
		lengthSize = lengthSize - t;
		*head = t;
		moveDataForward(buffer, head);
	}


	do{
		//printf("\nhere\n");
		readSize = read(cl, buffer, sizeofBuf);//read from client
		//printf("Read size: %d\n", readSize);
		//printf("%s", str);
		lengthRead = lengthRead + readSize;

		//if the clent close the server
		if(readSize == -1){
			write(cl, statueNETERROR, strlen(statueNETERROR));
			//printf("clientPutFile\n");
			return -1;
		}

		if(readSize == 0) break;

		lengthSize = lengthSize - readSize;//check how many bytes left

		//if the requirent lenth is less than what we read and lenth mode is on
		if(lengthSize <= 0 && lengthMod == 1){
			readSize = lengthSize + readSize;//calculate how much byte in the last turn we need to write
			ONOFF = 0; //turn off the read loop
		}

		writeSize = write(outField, buffer, readSize);//write to local directory readSize Byte
		//printf("read: %d\n", writeSize);
		*head = writeSize;
		moveDataForward(buffer, head);
		//printf("write size: %d\n", writeSize);
	}
	//if the client is not done
	while(readSize != 0 && ONOFF == 1);

	//if the actual length is shorter than length client want to put 
	if(lengthRead < length){
		write(cl, statueBADREQUEST,strlen(statueBADREQUEST));
		return -1;
	}

	//send the clinet file created
	//printf("done\n");
	//write(cl, statueOK, strlen(statueOK));
	close(outField);
	chmod(fileID, S_IRWXU);
	dprintf(cl, "Content-Length: %d\r\n", 0);
	dprintf(cl, "\r\n");
	return 0;


}

//this function can read the whole file header peace by peace
//return 0 if header can't be read

int getFileHeader(int cl, char* buffer){

	int size = 0;
	char* c = NULL;

	do{

		size = read(cl, &buffer[strlen(buffer)], sizeofBuf-strlen(buffer));
		dprintf(1, "readHeader: %d\n", size);
		c = strstr(buffer, "\r\n\r\n");
        if(c != NULL){
        	size = 0;
        }

	}
	while(size != 0);

	return 0;

}

void moveDataForward(char* buffer, int* head){
	if((*head) == 0){
		return;
	}
	else if((int)strlen(buffer) == (*head)){
		memset(buffer, 0, sizeofBuf);
		*head = 0;
		return;
	}
	memmove(buffer, &buffer[*head], strlen(buffer)-(*head));
	memset(&buffer[strlen(buffer)-(*head)], 0, sizeofBuf - (strlen(buffer)-(*head)) + 1);
	return;
}

void getHeaderPointer(char* buffer, int* head){
	char* c = strstr(buffer, "\r\n\r\n");
	
	//get the off set of buffer
	*head = c - buffer;
	*head = *head + 4;
	//printf("%s",buffer);
	if(c == NULL){
		*head = -1;
	}
}
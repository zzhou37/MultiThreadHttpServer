
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

#define sizeofStr 20000

//#define PORT_NUMBER 8888
#define statue404 "HTTP/1.1 404 Not Found\r\n"
#define statueOK  "HTTP/1.1 200 OK\r\n"
#define statueNETERROR "HTTP/1.1 500 Internal Server Error\r\n"
#define statueBADREQUEST "HTTP/1.1 400 Bad Request\r\n"
#define statueFORBID "HTTP/1.1 403 Forbidden\r\n"
#define statueCREATED "HTTP/1.1 201 Created\r\n"

int parseMessage(int cl, char* message, char* operation, char* fileID);
int clientGetFile(int cl, char* fileID);
int clientPutFile(int cl, char* fileID, int length);
int getFileHeader(int cl, char* mes);

int main(int argc, char* argv[]){
	//handing some argument error
	if(argc != 3){
		write(1, "invalide argument\n", strlen("invalide argument\n"));
		exit(0);
	}

	
	char mes[4096];
	char sevMes[300];
	//client use the command to connect with server
	//connect(sock, (struct sockaddr *)&addr, sizeof(sddr));

	//this build a standart of the sockey in address

	struct hostent *hent = gethostbyname( argv[1] );//"localhost" take the first token of the argument
	struct sockaddr_in addr;
	//copy the information into the 
	//printf("line 53\n");
	memcpy(&addr.sin_addr.s_addr, hent->h_addr, hent->h_length);

	//printf("%s\n", strtok(argv[1], ":"));

	addr.sin_port = htons( atoi( argv[2] ) ); //8888
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











	while(1){
		int cl = accept(sock, NULL, NULL);

		sprintf(sevMes, "--------------------------------- handling request ---------------------------------\n");
		write(1, sevMes, strlen(sevMes));

		//printf("/.--- %d request\n", count);

		//reseieve message from the client
		strcpy(mes, "");

		getFileHeader(cl, mes);

		//handle error message
		//if the header is empty
		
		//printf("%s\n", mes);

		//parse the message
		
		char operation[3];
		char fileID[27];

		int length = parseMessage(cl, mes, operation, fileID);
		//printf("\noperation: %s  fileID: %s \n", operation, fileID);
		//printf("length is %d\n", length);
		sprintf(sevMes, "operation:  %s\nfileID:  %s\ncontent-length:  %d\n\n", operation, fileID, length);
		write(1, sevMes, strlen(sevMes));

		if(length == -1){
			break;
		} 

		//??????????????????????????????????????????????/when should I send OK
		
		//printf("cl is %d\n", cl);
		//if the operation is get
		//printf("start handling request\n\n");

		if(strcmp(operation, "GET") == 0){
			//int clientGetFile(int cl, char* fileID, char* str){
			//printf("Get File:\n");
			sprintf(sevMes, "Get File:\n");
			write(1, sevMes, strlen(sevMes));

			clientGetFile(cl, fileID);
			dprintf(cl, "Content-Length: %d\r\n", length);
		}

		//if the the operation is PUT
		else if(strcmp(operation, "PUT") == 0){
			//int clientPutFile(int cl, char* fileID, char* str, int length)
			//printf("PUT File:\n");
			sprintf(sevMes, "PUT File:\n");
			write(1, sevMes, strlen(sevMes));

			clientPutFile(cl, fileID, length);
		}
		
	}

}



















//this function take message as input, then pass the operation name and fileID by refereance
//then return the length of required, if there is no requirement of length, return -1
//use function sscanf(): sscanf (buffer,"%s %d",name,&age);
int parseMessage(int cl, char* message, char* operation, char* fileID){
	char sevMes[300];
	
	//parse the first line of message
	//get the operation, file ID
	char* headerLine = NULL;
	char lengthStr[100];
	char length[100];


	headerLine = strtok(message, "\r\n");
	sscanf(headerLine, "%s %s %*s", operation, fileID);

	//check if the header is valid
	//neighter GET or PUT respond badrequest (invalide request)
	if( !(strcmp(operation, "GET") == 0 || strcmp(operation, "PUT") == 0) )
	{
		write(cl, statueFORBID, strlen(statueFORBID));
		//printf("parseMessage: not get or put\n");
		return -1;
	}
	else if(strlen(fileID) != 27){
		write(cl, statueBADREQUEST, strlen(statueBADREQUEST));
		//printf("%lu\n", strlen(fileID));
		//printf("parseMessage: length is not 27\n");
		return -1;
	}
	//???????????????????????????????????????????????????????? how about / or some other char in file ID
	else if(strstr(fileID, "/") != NULL){
		write(cl, statueBADREQUEST, strlen(statueBADREQUEST));
		//printf("%lu\n", strlen(fileID));
		//printf("parseMessage: length is not 27\n");
		return -1;
	}


	//find the length of the PUT
	do{
		headerLine = strtok(NULL, "\r\n");
		if(headerLine == NULL || headerLine[0] == '\0'){
			break;
		}
		//strcat(headerLine, "\0");
		//printf("while loop in parse header\n");
		//printf("%s\n", headerLine);

		sprintf(sevMes, "%s\n", headerLine);
		write(1, sevMes, strlen(sevMes));

		//printf("191\n");
		sscanf(headerLine, "%s %s", lengthStr, length);
		//printf("%s %s\n", lengthStr, length);
		if(strcmp(lengthStr, "Content-Length:") == 0){
			return atoi(length);
		}
	}
	while(1);

	return -2; //MEANS no lengh input
}

//the function handle the GET request
int clientGetFile(int cl, char* fileID){
	
	char str[sizeofStr];
	//open the file client request
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
	dprintf(1, "Content-Length: %d\r\n", sendLength);

	//close the file
	close(inputFild); 

	return 0;
}

//recive the file from the client and write the file into local directory
int clientPutFile(int cl, char* fileID, int length){

	char str[sizeofStr];

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
	//printf("out field is %d\n", outField);


	do{
		readSize = read(cl, str, sizeofStr);//read from client
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

		writeSize = write(outField, str, readSize);//write to local directory readSize Byte
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
	write(cl, statueOK, strlen(statueOK));
	dprintf(cl, "Content-Length: %d\r\n", 0);
	dprintf(cl, "\r\n");
	return 0;


}

//this function can read the whole file header peace by peace
//return 0 if header can't be read

int getFileHeader(int cl, char* mes){

	ssize_t size = 0;
	char tempt[1000];

	do{

		size = read(cl, tempt, strlen(tempt));
		if(size == -1){
			write(cl, statueNETERROR, strlen(statueNETERROR));
			write(cl, "getFileHeader", strlen("getFileHeader"));
			return -1;
		}
		strcat(mes, tempt);
        if(strstr(mes, "\r\n\r\n") != NULL){
        	size = 0;
        } 

	}
	while(size != 0);

	return 0;

}
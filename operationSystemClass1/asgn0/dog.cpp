//name: zhihang zhou
//ID:   zzhou37
//CSE130 Miller

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

int main(int argc, char* argv[]){

	char str[32000];
	size_t nbyte;
	int inputFild;

	//handling the clase no input
	if(argc == 1){
		nbyte = sizeof(str);
		inputFild = 0;

		ssize_t buffersize = -1;
		while(buffersize != 0){
			buffersize = read(inputFild, str, nbyte);
			write(1, str, buffersize);
		}

		//close the file
		close(inputFild); 
	}

	
    //iterate all the argument in the command line from the first argument which is index 1
	for(int i = 1; i < argc; i++){
		nbyte = sizeof(str);
		//if the argument is -, the switch input resourse to stdin
		if(strcmp(argv[i], "-") == 0){
			inputFild = 0; //0 is stdin
		}
		//if the input is not "-"
		else{
			inputFild = open(argv[i], O_RDONLY); //open the file
			//if the file doesn't exist
			if(inputFild == -1){
				perror("the input file doesn't exist.");
				break;
			}
		}

		//using a while loop to read 32KB of data into a buffer and print it out 
		//then repeat it
		ssize_t buffersize = -1;
		while(buffersize != 0){
			buffersize = read(inputFild, str, nbyte);
			write(1, str, buffersize);
		}

		//close the file
		close(inputFild); 
		
	}
}
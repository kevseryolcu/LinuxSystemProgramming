#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

#define LOG_FILE "log.txt"
#define MAX_PATH_LENGTH 512
#define MAX_DIR_NUM 512
#define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int findString(char* chPtrBuffer, char* str, char* fileName);
int countLine(FILE* fpInp);
void bufferReader(FILE* fpInp, int countCh, char * chPtrBuffer);
int isDirectory(char * pathName);
int addPathName(char* newPath, char* path, char * dfName);
int list(char* fileName, char* str, int pipeArr[]);
int listDir(char* dirName, char* str);
pid_t r_wait(int* stat_loc);

pid_t mainProcess;

int main(int argc, char* argv[])
{
	FILE *fpLog;
	pid_t tmpid;
	int intWordCount = 0;
	
	mainProcess = getpid();
	remove(LOG_FILE);
	
	
	/*Usage*/
	if(argc != 3){
		printf("Usage: ./exe \'string\' <directory>!");
		return 0;
	}
	
	intWordCount = listDir(argv[2], argv[1]);
	
	if(intWordCount == -1){
		fscanf(stderr, "String couldn\'t be searced in directory!");
		return -1;
	}
	
	tmpid = getpid();
	
	fprintf(stderr, "%d %s found in total.\n", intWordCount, argv[1]);
	if(tmpid == mainProcess){
		
		/*printf("Count main : %d", intWordCount);*/
		
		if((fpLog = fopen(LOG_FILE, "a")) == NULL){
			fprintf(stderr, "error\n");
			return -1;
		}
		
		fprintf(fpLog, "%d %s found in total.\n", intWordCount, argv[1]);
		
		/*Closing file and error check*/
		if(fclose(fpLog) == EOF){
			perror("Failed to close log file");
			return -1;
		}
				
		/*remove(COUNTER_FILE);*/
	}
	
	return 0;
}

/*Count how many file and creates fifo for directories*/
void makeFifoDir(int* numFile, int* numDir, char* dirName, char fifoNameArr[MAX_DIR_NUM][MAX_PATH_LENGTH]){
	struct dirent* direntPtr;
	DIR * dirPtr;
	char newPathName[MAX_PATH_LENGTH];
	char fifoName[MAX_PATH_LENGTH];
	int i = 0;
	
	if((dirPtr = opendir(dirName)) == NULL){
		perror("Failed to open directory!");
		return;		
	}
	
	/*Referenced from class book(Unix system programming) page 193*/
	while((direntPtr = readdir(dirPtr)) != NULL){
			
		if(strcmp(direntPtr->d_name, "..") != 0 && strcmp(direntPtr->d_name, ".") != 0){
			if(addPathName(newPathName, dirName, direntPtr->d_name) == -1){
				perror("Couldnot create new path!");
				return;
			}
				
			if(isDirectory(newPathName) != 0){
				*numDir += 1;
				/**/
				sprintf(fifoName, "%d-%s", getpid(), direntPtr->d_name);
                strcpy(fifoNameArr[i++], fifoName);
                    
				if (mkfifo(fifoName, FIFO_PERMS) == -1) {
					perror("Failed to create FIFO!");
					return;
				}
                    /**/
			}
			
			else	
				*numFile += 1;
		}
	}
	
	/*Referenced from class book(Unix system programming) page 193*/
	while((closedir(dirPtr) == -1) && (errno == EINTR));	
}


int list(char* fileName, char* str, int pipeArr[]){
	FILE* fpInp;
	int intCountCh;
	char* chPtrBuffer;
	int intCountFound = 0;
	int tmp = 0;
	
	
	/*Opening input File*/
	fpInp = fopen(fileName, "r");
	
	/*File opening error check*/
	if(fpInp == NULL){
		perror("Failed to open input file");
		return -1;
	}
	
	/*Counting number of characters*/
	intCountCh = countLine(fpInp);
	
	/*Closing file and error check*/
	if(fclose(fpInp) == EOF){
		perror("Failed to close input file");
		return -1;
	}
	
	/*Opening input File for reading*/
	if((fpInp = fopen(fileName, "r")) == NULL){
		fprintf(stderr, "error\n");
		return -1;
	}
	
	/*File opening error check*/
	if(fpInp == NULL){
		perror("Failed to open input file");
		return -1;
	}	
	
	chPtrBuffer = (char*) malloc((intCountCh+1) * sizeof(char));
	
	/*Takes memory space according to number of character for chPtrBuffer
	 * Reades file content in chPtrBuffer*/
	bufferReader(fpInp, intCountCh, chPtrBuffer);
	
	/*Finds counts and prints string in chPtrBuffer*/
	intCountFound = findString(chPtrBuffer, str, fileName);
	
	/*Frees allocating memory*/
	free(chPtrBuffer);
	
	
	close(pipeArr[1]); 
	read(pipeArr[0], &tmp, sizeof(tmp));
	close(pipeArr[0]); 
	
	fprintf(stderr, "int count found: %d , tmp: %d", intCountFound, tmp);
	
	intCountFound += tmp;
	
	close(pipeArr[0]); 
	write(pipeArr[1], &intCountFound, sizeof(intCountFound));/**/
	close(pipeArr[1]); 
	
	fprintf(stderr, "int count found: %d , tmp: %d", intCountFound, tmp);
	
	/*Closing file and error check*/
	if(fclose(fpInp) == EOF){
		perror("Failed to close input file");
		return -1;
	}
	
	return 0;
}

int listDir(char* dirName, char* str){
	struct dirent* direntPtr;
	DIR * dirPtr;
	char newPathName[MAX_PATH_LENGTH];
	int listRes;
	pid_t childPid;
	int intNumFile = 0;
	int intNumDir = 0;
	int pipeArr[2];
	int countWord = 0;
	int tmpPipeContent = 0;
	int tmpFifoContent = 0;
	char fifoNameArr[MAX_DIR_NUM][MAX_PATH_LENGTH];
	char fifoName[MAX_PATH_LENGTH];
	char* token;
	const char s[2] = "/";
    char direc_name[20];
	int fdFifoR;
	int fdFifoW;
	int i;
    
    
	makeFifoDir(&intNumFile, &intNumDir, dirName, fifoNameArr);
	
	/*Creating pipe for file*/
	if(0 < intNumFile){
		if(pipe(pipeArr) == -1){
			perror("Failed to create pipe!");
			return -1;
		}
		close(pipeArr[0]); 
		write(pipeArr[1], &countWord, sizeof(countWord));
		close(pipeArr[1]); 
			
	}
	
	/*Referenced from class book(Unix system programming) page 192*/
	if((dirPtr = opendir(dirName)) == NULL){
		perror("Failed to open directory!");
		return -1;		
	}
	
	/*Referenced from class book(Unix system programming) page 193*/
	while((direntPtr = readdir(dirPtr)) != NULL){
			
		if(strcmp(direntPtr->d_name, "..") != 0 && strcmp(direntPtr->d_name, ".") != 0){
			if((childPid = fork())	== -1){
				perror("Child Couldnot be create!");
				return -1;		
			}
			if(childPid == 0){
				if(addPathName(newPathName, dirName, direntPtr->d_name) == -1){
					perror("Couldnot create new path!");
					return -1;
				}
				
				if(isDirectory(newPathName) != 0){
					if(listDir(newPathName, str) == -1)
						return -1;				
				}
				
				else{							
					listRes = list(newPathName, str, pipeArr);
					
					if(listRes == -1)
						return -1;
					break;			
				}
				exit(0);
			}			
		}		
	}
	
	/*Referenced from class book(Unix system programming) page 193*/
	while((closedir(dirPtr) == -1) && (errno == EINTR));
	
	for (i = 0; i < intNumDir; ++i) {

        sprintf(fifoName, "%s", fifoNameArr[i]);

        if ((fdFifoR = open(fifoName, O_RDONLY)) != -1) {
            read(fdFifoR, &tmpFifoContent, sizeof (tmpFifoContent));
            close(fdFifoR);

            unlink(fifoName);

            countWord += tmpFifoContent;

        }
    }
	
	/*Waits for children*/
	while(r_wait(NULL) > 0);
	
	if(0 < intNumFile){
		close(pipeArr[1]); 
		read(pipeArr[0], &tmpPipeContent, sizeof(tmpPipeContent));
		countWord += tmpPipeContent;
		close(pipeArr[0]); 
	}
	
	/*/////////*/
	if (getpid() != mainProcess) {

        token = strtok(dirName, s);

        while (token != NULL) {
            strcpy(direc_name, token);

            token = strtok(NULL, s);

        }

        sprintf(fifoName, "%d-%s", getppid(), direc_name);
        fdFifoW = open(fifoName, O_WRONLY);
        write(fdFifoW, &countWord, sizeof (countWord));
        close(fdFifoW);

    }
    
    fprintf(stderr, "\n countword %d \n", countWord);
    
	/*//////////////*/
	return countWord;
}

/*Referenced from class book(Unix system programming) page 98*/
pid_t r_wait(int* stat_loc){
	int retVal = 0;
	
	while(((retVal = wait(stat_loc)) == -1) && (errno == EINTR));
	
	return retVal;	
}

/*Referenced from class book(Unix system programming) page 196*/
int isDirectory(char * pathName){
	struct stat statBuf;
	if(stat(pathName, &statBuf) == -1)
		return 0;
	return S_ISDIR(statBuf.st_mode);
	
}

int addPathName(char* newPath, char* path, char * dfName){
	if(path == NULL){
		perror("Adding path name unsuccesfull!");
		return -1;
	}
	
	if(dfName == NULL){
		strncpy(newPath, path, MAX_PATH_LENGTH);
		return 0;
	}
	
	strncpy(newPath, path, MAX_PATH_LENGTH);
	strncat(newPath, "/", MAX_PATH_LENGTH);
	strncat(newPath, dfName, MAX_PATH_LENGTH);
	
	return 0;
}

/*Counts how many characters are in file*/
int countLine(FILE* fpInp){
	int intCountCh;
	char chTmp;
	for(intCountCh = 0; fscanf(fpInp, "%c", &chTmp) != EOF; ++intCountCh);	
	return intCountCh;
}

/*Copies content of a file to string*/
void bufferReader(FILE* fpInp, int countCh, char * chPtrBuffer){
	int intI;	
	for(intI = 0; fscanf(fpInp, "%c", &chPtrBuffer[intI]) != EOF; ++intI);
	chPtrBuffer[intI] = '\0';
}

/*searches str in chPtrBuffer, prints found coordiantion and 
 *count total number of repetiton*/
int findString(char* chPtrBuffer, char* str, char* fileName){
	int intI, intJ, intCountRow = 1, intCountCol = 1, 
		intFoundRow = 0, intFoundCol = 0, intCountFound = 0;
	int intTmp;
		
	FILE *fpLog;
	
	
	fpLog = fopen(LOG_FILE, "a");
	
	if(fpLog == NULL){
		perror("Failed to open log file");
		return -1;
	}	
		
	for(intI = 0; chPtrBuffer[intI] != '\0'; ++intI)
	{		
		if(chPtrBuffer[intI] == str[0])
		{			
			intFoundCol = intCountCol;
			intFoundRow = intCountRow;
			intTmp = intI;
			for(intJ = 1; str[intJ] != '\0' ; ++intJ)
			{
				intTmp++;
				if(chPtrBuffer[intTmp] == '\n' || 
				   chPtrBuffer[intTmp] == '\t' || 
				   chPtrBuffer[intTmp] == ' '){
				   intJ--;				   
			   }				
				else if(chPtrBuffer[intTmp] != str[intJ]) {
					intFoundCol = 0;
					intFoundRow = 0;
					break;					
				}
			}
		}				
		
		if(intFoundCol != 0){
			
			fprintf(fpLog, "%s:[%d, %d] %s first character is found. \n", fileName, intFoundCol, intFoundRow, str);
			intCountFound++;
			intFoundCol = 0;
			intFoundRow = 0;
		}	
		if(chPtrBuffer[intI] == '\n'){
			intCountRow++;
			intCountCol = 0;
		}		
		intCountCol++;
	}
	
	/*Closing file and error check*/
	if(fclose(fpLog) == EOF){
		perror("Failed to close input file");
		return -1;
	}
	
	return intCountFound;
}



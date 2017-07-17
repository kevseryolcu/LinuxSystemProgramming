#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

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
void findString(char* chPtrBuffer, char* str){
	int intI, intJ, intCountRow = 1, intCountCol = 1, 
		intFoundRow = 0, intFoundCol = 0, intCountFound = 0;	
	for(intI = 0; chPtrBuffer[intI] != '\0'; ++intI)
	{		
		if(chPtrBuffer[intI] == str[0])
		{			
			intFoundCol = intCountCol;
			intFoundRow = intCountRow;
			int intTmp = intI;
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
			printf("%s found in col%d row%d\n", str, intFoundCol, intFoundRow);
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
	printf("-------------------------\n");
	printf("Total %d %s found.\n", intCountFound, str);
}
int main(int argc, char* argv[])
{
	FILE* fpInp;
	int intCountCh;
	char* chPtrBuffer;
	int intI, intJ, intCountRow = 1, intCountCol = 1, 
		intFoundRow = 0, intFoundCol = 0, intCountFound = 0;
	int i;	
	/*Usage*/
	if(argc != 3){
		printf("Usage: ./list \'string\' <file>!");
		return 0;
	}
	/*Opening input File*/
	fpInp = fopen(argv[2], "r");
	
	/*File opening error check*/
	if(fpInp == NULL){
		perror("Failed to open input file");
		return 1;
	}	
	/*Counting number of characters*/
	intCountCh = countLine(fpInp);
	/*Closing file and error check*/
	if(fclose(fpInp) == EOF){
		perror("Failed to close input file");
		return 1;
	}
	/*Opening input File for reading*/
	fpInp = fopen(argv[2], "r");
	/*File opening error check*/
	if(fpInp == NULL){
		perror("Failed to open input file");
		return 1;
	}	
	chPtrBuffer = (char*) malloc((intCountCh+1) * sizeof(char));
	/*Takes memory space according to number of character for chPtrBuffer
	 * Reades file content in chPtrBuffer*/
	bufferReader(fpInp, intCountCh, chPtrBuffer);
	/*Finds counts and prints string in chPtrBuffer*/
	findString(chPtrBuffer, argv[1]);
	/*Frees allocating memory*/
	free(chPtrBuffer);
	/*Closing file and error check*/
	if(fclose(fpInp) == EOF){
		perror("Failed to close input file");
		return 1;
	}
	return 0;
}

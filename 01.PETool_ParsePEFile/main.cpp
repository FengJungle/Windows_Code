#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>

#define IN_FILE_NAME "Test.exe"

//#define DEBUG


int Get_File_Length(const char* fileName, int* fileLength) {
	if (fileName == NULL || fileLength == NULL) {
		printf("[ERROR] %d: Wrong parameters!\n", __LINE__);
		return -1;
	}
	FILE* fp = fopen(fileName, "rb");
	if (NULL == fp) {
		printf("[ERROR] %d: fopen failed!\n", __LINE__);
		return -1;
	}

	fseek(fp, 0, SEEK_END);

	*fileLength = ftell(fp);
	fclose(fp);

	return 0;
}

VOID Read_PE_File_To_FileBuffer(void** fileBuffer, int* pFileLength) {
	FILE*   fp = NULL;
	size_t ret = 0;

	// 1. Get file length
	Get_File_Length(IN_FILE_NAME, pFileLength);

	// 2. Malloc
	*fileBuffer = (char*)malloc(sizeof(char)*(*pFileLength));
	if (NULL == *fileBuffer) {
		printf("[ERROR] %d: malloc failed!\n", __LINE__);
		return;
	}

	// 3. Open initial exe file
	fp = fopen(IN_FILE_NAME, "rb");
	if (NULL == fp) {
		printf("[ERROR] %d: fopen failed!\n", __LINE__);
		return;
	}

	ret = fread(*fileBuffer, *pFileLength, 1, fp);
	if (ret != 1) {
		printf("[ERROR] %d: fread failed! Actual read:%d\n", __LINE__, ret);
		return;
	}
	fclose(fp);
}

void Parse_PE_File(char* fileBuffer) {

	if (NULL == fileBuffer) {
		printf("[ERROR] %d: fileBuffer is NULL!\n", __LINE__);
		return;
	}

	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	PIMAGE_SECTION_HEADER     pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	// 1. Dos头
	printf("************       DOS     *************\n");
	printf("sizeof DOS:\t%d\n", sizeof(IMAGE_DOS_HEADER));
	printf("e_magic:\t0x%x\n", pDosHeader->e_magic);
	printf("e_lfanew:\t0x%x\n", pDosHeader->e_lfanew);
	printf("****************************************\n\n");
	 
	// 2. PE头
	printf("*************     PE_HEADER    *********\n");
	printf("sizeof PE_HEADER:\t%d\n", sizeof(IMAGE_FILE_HEADER));
	printf("Machine:\t\t0x%x\n", pImageFileHeader->Machine);
	printf("NumberOfSections:\t0x%x\n", pImageFileHeader->NumberOfSections);
	printf("SizeOfOptionalHeader:\t0x%x\n", pImageFileHeader->SizeOfOptionalHeader);
	printf("Characteristics:\t0x%x\n", pImageFileHeader->Characteristics);
	printf("****************************************\n\n");

	// 3. 可选PE头
	printf("**********  OPTIONAL PE HEADER    ***********\n");
	printf("sizeof IMAGE_OPTIONAL_HEADER32:\t%d\n", sizeof(IMAGE_OPTIONAL_HEADER32));
	printf("Magic:\t\t\t0x%x\n", pOptionalHeader->Magic);
	printf("SizeOfCode:\t\t0x%x\n", pOptionalHeader->SizeOfCode);
	printf("SizeOfInitializedData:\t0x%x\n", pOptionalHeader->SizeOfInitializedData);
	printf("AddressOfEntryPoint:\t0x%x\n", pOptionalHeader->AddressOfEntryPoint);
	printf("BaseOfCode:\t\t0x%x\n", pOptionalHeader->BaseOfCode);
	printf("BaseOfData:\t\t0x%x\n", pOptionalHeader->BaseOfData);
	printf("ImageBase:\t\t0x%x\n", pOptionalHeader->ImageBase);
	printf("SectionAlignment:\t0x%x\n", pOptionalHeader->SectionAlignment);
	printf("FileAlignment:\t\t0x%x\n", pOptionalHeader->FileAlignment);
	printf("SizeOfImage:\t\t0x%x\n", pOptionalHeader->SizeOfImage);
	printf("SizeOfHeaders:\t\t0x%x\n", pOptionalHeader->SizeOfHeaders);
	printf("NumberOfRvaAndSizes:\t0x%x\n", pOptionalHeader->NumberOfRvaAndSizes);
	printf("****************************************\n\n");

	
	// 4. 节表
	printf("************     SECTION   *************\n");
	printf("sizeof SECTION:\t\t%d\n", sizeof(IMAGE_SECTION_HEADER));
	printf("Number Of SECTIONS:\t%d\n", pImageFileHeader->NumberOfSections);

	for (int i = 0; i < pImageFileHeader->NumberOfSections; i++) {
		printf("\n++++++     ");
		for (int j = 0; j < 8; j++) {
			printf("%c", pSectionHeader[i].Name[j]);
		}
		printf("     ++++++\n");
		printf("Misc:\t\t\t0x%x\n", pSectionHeader[i].Misc);
		printf("VirtualAddress:\t\t0x%x\n", pSectionHeader[i].VirtualAddress);
		printf("SizeOfRawData:\t\t0x%x\n", pSectionHeader[i].SizeOfRawData);
		printf("PointerToRawData:\t0x%x\n", pSectionHeader[i].PointerToRawData);
		printf("Characteristics:\t0x%x\n", pSectionHeader[i].Characteristics);
	}
	printf("****************************************\n\n");
}

int main()
{
	char* fileBuffer = NULL;
	int fileBufferLength = 0;

	Read_PE_File_To_FileBuffer((void**)&fileBuffer, &fileBufferLength);

	Parse_PE_File(fileBuffer);

	if (fileBuffer) {
		memset(fileBuffer, 0, fileBufferLength);
		free(fileBuffer);
		fileBuffer = NULL;
	}
	return 0;
}
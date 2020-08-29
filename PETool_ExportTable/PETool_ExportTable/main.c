#include <stdio.h>
#include <Windows.h>

#define FILE_NAME "MathSample.dll"

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

void Read_PE_File_To_FileBuffer(void** fileBuffer) {
	FILE*   fp = NULL;
	size_t ret = 0;
	int fileLength = 0;

	// 1. Get file length
	Get_File_Length(FILE_NAME, &fileLength);

	// 2. Malloc
	*fileBuffer = (char*)malloc(sizeof(char)*fileLength);
	if (NULL == *fileBuffer) {
		printf("[ERROR] %d: malloc failed!\n", __LINE__);
		return;
	}

	// 3. Open initial exe file
	fp = fopen(FILE_NAME, "rb");
	if (NULL == fp) {
		printf("[ERROR] %d: fopen failed!\n", __LINE__);
		return;
	}

	ret = fread(*fileBuffer, fileLength, 1, fp);
	if (ret != 1) {
		printf("[ERROR] %d: fread failed! Actual read:%d\n", __LINE__, ret);
		return;
	}
	fclose(fp);
}

DWORD RVA_To_FOA(char* fileBuffer, DWORD RvaAddress)
{
	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	PIMAGE_SECTION_HEADER     pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	DWORD FoaAddress = 0x00;
	int numOfSections = pImageFileHeader->NumberOfSections;
	int sectionIndex = -1;
	for (int i = 0; i < numOfSections; i++) {
		if (RvaAddress >= pSectionHeader[i].VirtualAddress
			&& RvaAddress < pSectionHeader[i].VirtualAddress + pSectionHeader[i].SizeOfRawData) {
			sectionIndex = i;
			break;
		}
	}
	int offset = RvaAddress - pSectionHeader[sectionIndex].VirtualAddress;
	FoaAddress = pSectionHeader[sectionIndex].PointerToRawData + offset;
	return FoaAddress;
}

void Print_ExportTable(char* fileBuffer)
{
	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	PIMAGE_SECTION_HEADER     pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	DWORD virtualAddress_of_ExportTable = pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	DWORD FOA_of_ExportTable = RVA_To_FOA(fileBuffer, virtualAddress_of_ExportTable);
	PIMAGE_EXPORT_DIRECTORY pExportTable = (PIMAGE_EXPORT_DIRECTORY)((char*)fileBuffer + FOA_of_ExportTable);

	/*
	typedef struct _IMAGE_EXPORT_DIRECTORY {
		DWORD   Characteristics;
		DWORD   TimeDateStamp;
		WORD    MajorVersion;
		WORD    MinorVersion;
		DWORD   Name;
		DWORD   Base;
		DWORD   NumberOfFunctions;
		DWORD   NumberOfNames;          // 以名字导出的函数的数量
		DWORD   AddressOfFunctions;     // RVA from base of image
		DWORD   AddressOfNames;         // RVA from base of image
		DWORD   AddressOfNameOrdinals;  // RVA from base of image  导出函数的序号表
	} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;
	*/

	printf("EXPORT Table info:\n");
	printf("Base:\t%d\n", pExportTable->Base);
	printf("NumberOfFunctions:\t%d\n", pExportTable->NumberOfFunctions);
	printf("NumberOfNames:\t%d\n", pExportTable->NumberOfNames);
	printf("AddressOfFunctions:\t0x%x\n", pExportTable->AddressOfFunctions);
	printf("AddressOfNames:\t0x%x\n", pExportTable->AddressOfNames);
	printf("AddressOfNameOrdinals:\t0x%x\n", pExportTable->AddressOfNameOrdinals);

	DWORD FOA_AddressOfFunctions = RVA_To_FOA(fileBuffer, pExportTable->AddressOfFunctions);
	DWORD FOA_AddressOfNames = RVA_To_FOA(fileBuffer, pExportTable->AddressOfNames);
	DWORD FOA_AddressOfNameOrdinals = RVA_To_FOA(fileBuffer, pExportTable->AddressOfNameOrdinals);

	// print address of functions
	PDWORD FunctionAddressArr = (PDWORD)((char*)fileBuffer + FOA_AddressOfFunctions);
	for (int i = 0; i < pExportTable->NumberOfFunctions; i++) {
		printf("Address: 0x%x\n", FunctionAddressArr[i]);
	}

	// print name of functions
	PDWORD FunctionNameArr = (PDWORD)((PBYTE)fileBuffer + FOA_AddressOfNames);
	// print ordinal of functions
	PWORD FunctionOrdinalArr = (PWORD)((PBYTE)fileBuffer + FOA_AddressOfNameOrdinals);
	for (int i = 0; i < pExportTable->NumberOfNames; i++) {
		DWORD FOA_of_FunctionName = fileBuffer + RVA_To_FOA(fileBuffer, FunctionNameArr[i]);
		printf("%s\t%d\n", (char*)FOA_of_FunctionName, FunctionOrdinalArr[i]);
	}
}

DWORD Get_FunctionAddress_by_Name(char* fileBuffer, char* functionName)
{
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_OPTIONAL_HEADER pOptionalHeader = (PIMAGE_OPTIONAL_HEADER)((PBYTE)fileBuffer + pDosHeader->e_lfanew + 0x14 + 0x04);
	DWORD RVA_of_ExportDirectory = pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	DWORD FOA_of_ExportDirectory = RVA_To_FOA(fileBuffer, RVA_of_ExportDirectory);
	PIMAGE_EXPORT_DIRECTORY pExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((PBYTE)fileBuffer + FOA_of_ExportDirectory);

	// Find the index of functionName 
	int Index_of_FunctionName = -1;
	PDWORD FunctionNameArr = (PDWORD)((PBYTE)fileBuffer + RVA_To_FOA(fileBuffer, pExportDirectory->AddressOfNames));
	for (int i = 0; i < pExportDirectory->NumberOfNames; i++) {
		char* FunctionName = (char*)(fileBuffer + RVA_To_FOA(fileBuffer, FunctionNameArr[i]));
		if (memcmp(functionName, FunctionName, strlen(functionName)) == 0) {
			Index_of_FunctionName = i;
			break;
		}
	}
	if (Index_of_FunctionName == -1) {
		printf("[ERROR] %d: No function named %s\n", __LINE__, functionName);
		return -1;
	}

	PWORD FunctionOrdinalArr = (PWORD)((PBYTE)fileBuffer + RVA_To_FOA(fileBuffer, pExportDirectory->AddressOfNameOrdinals));
	int Index_of_FunctionAddress = FunctionOrdinalArr[Index_of_FunctionName];

	PDWORD FunctionAddressArr = (PDWORD)((PBYTE)fileBuffer + RVA_To_FOA(fileBuffer, pExportDirectory->AddressOfFunctions));
	DWORD Address_of_Function = FunctionAddressArr[Index_of_FunctionAddress];
	printf("Address of function %s is 0x%x\n", functionName, Address_of_Function);

	return Address_of_Function;
}

int main()
{
	char* fileBuffer = NULL;
	Read_PE_File_To_FileBuffer(&fileBuffer);

	Print_ExportTable(fileBuffer);

	Get_FunctionAddress_by_Name(fileBuffer, "sum");

	free(fileBuffer);
	system("pause");
	return 0;
}
#include <Windows.h>
#include <stdio.h>

#define FILE_NAME "test.exe"

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

void printRelocationDirectory(char* fileBuffer)
{
	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	PIMAGE_SECTION_HEADER     pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	IMAGE_DATA_DIRECTORY RelocationDirectory = pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]; // 5
	DWORD FOA_of_RelocationDirectory = RVA_To_FOA(fileBuffer, RelocationDirectory.VirtualAddress);

	printf("重定位表的相对虚拟地址：%x\n", RelocationDirectory.VirtualAddress);
	printf("重定位表的大小：%x\n", RelocationDirectory.Size);

	/*
	*   typedef struct _IMAGE_BASE_RELOCATION {
	*		DWORD   VirtualAddress;
	*		DWORD   SizeOfBlock;
	*	} IMAGE_BASE_RELOCATION;
	* 
	*    VirtualAddress | SizeOfBlock
	*    data  |  data  | data  |  data
	*    data  |  data  | data  |  data
	*    ....  |  ....  | ....  |  .... 
	*    
	*    VirtualAddress | SizeOfBlock
	*    data  |  data  | data  |  data
	*    data  |  data  | data  |  data
	*    ....  |  ....  | ....  |  ....
	*
	*    Note: each data occupy 2 bytes
	*    |---- 1 1 1 1 1 1 9 8 7 6 5 4 3 2 1 0 ----|
	*    |---- 5 4 3 2 1 0                     ----|
	*          
	*/
	PIMAGE_BASE_RELOCATION pImage_Base_Relocation;
	pImage_Base_Relocation = (PIMAGE_BASE_RELOCATION)(fileBuffer + FOA_of_RelocationDirectory);
	while (pImage_Base_Relocation->VirtualAddress != 0 && pImage_Base_Relocation->SizeOfBlock != 0) {
		size_t dataSize = pImage_Base_Relocation->SizeOfBlock - 8;
		printf("\nVirtualAddress: %p", pImage_Base_Relocation->VirtualAddress);
		
		DWORD foa = RVA_To_FOA(fileBuffer, pImage_Base_Relocation->VirtualAddress);
		CHAR sectionName[9] = { 0 };
		int sectionIndex = 0;
		for (int i = 0; i < pImageFileHeader->NumberOfSections; i++) {
			DWORD begin = RVA_To_FOA(fileBuffer, pSectionHeader[i].VirtualAddress);
			DWORD end   = RVA_To_FOA(fileBuffer, pSectionHeader[i].VirtualAddress + pSectionHeader[i].Misc.VirtualSize);
			if (foa >= begin && foa <= end) {
				sectionIndex = i;
				memcpy(sectionName, (PIMAGE_SECTION_HEADER*)(pSectionHeader + i)->Name, 8);
				printf("\nsection: %s\n", sectionName);
				break;
			}
		}

		PWORD dataAddr = (PWORD)((PBYTE)pImage_Base_Relocation + 8);

		for (int i = 0; i < dataSize / 2; i++) {
			dataAddr += 1;
			WORD flag = (*dataAddr & 0xF000) >> 12;
			WORD tmpDataAddr = 0;
			if (flag == 0x03) {
				tmpDataAddr = pImage_Base_Relocation->VirtualAddress + (foa + (*dataAddr & 0x0FFF));
				printf("type: %d, VirtualAddress: 0x%x\n", flag, tmpDataAddr);
			}
		}
		pImage_Base_Relocation = (PIMAGE_BASE_RELOCATION)((PBYTE)pImage_Base_Relocation + pImage_Base_Relocation->SizeOfBlock);
	}
}

int main()
{
	char* fileBuffer = NULL;
	Read_PE_File_To_FileBuffer(&fileBuffer);
	printRelocationDirectory(fileBuffer);
	free(fileBuffer);

	system("pause");
	return 0;
}
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>

#define IN_FILE_NAME   "Test.exe"
#define OUT_FILE_NAME  "test-new.exe"

VOID Save_Buffer_To_File(char* Buffer, int BufferLength) {
	if (Buffer == NULL || BufferLength <= 0) {
		printf("[ERROR] %d: Wrong parameters!\n", __LINE__);
		return;
	}
	FILE*   fp = NULL;
	size_t ret = 0;
	fp = fopen(OUT_FILE_NAME, "wb");
	if (NULL == fp) {
		printf("[ERROR] %d: fopen failed!\n", __LINE__);
		return;
	}

	ret = fwrite(Buffer, BufferLength, 1, fp);
	if (ret != 1) {
		printf("[ERROR] %d: fwrite failed! Actual write:%d\n", __LINE__, ret);
		return;
	}
	fclose(fp);
}

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
	if (sectionIndex == -1) {
		printf("[ERROR] %d: Wrong RVA!\n", __LINE__);
		return -1;
	}
	int offset = RvaAddress - pSectionHeader[sectionIndex].VirtualAddress;
	FoaAddress = pSectionHeader[sectionIndex].PointerToRawData + offset;
	return FoaAddress;
}

DWORD FOA_To_RVA(char* fileBuffer, DWORD FoaAddress)
{
	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	PIMAGE_SECTION_HEADER     pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	DWORD RvaAddress = 0x00;
	int numOfSections = pImageFileHeader->NumberOfSections;
	int sectionIndex = -1;
	for (int i = 0; i < numOfSections; i++) {
		if (FoaAddress >= pSectionHeader[i].PointerToRawData
			&& FoaAddress < pSectionHeader[i].PointerToRawData + pSectionHeader[i].SizeOfRawData) {
			sectionIndex = i;
			break;
		}
	}
	if (sectionIndex == -1) {
		printf("[ERROR] %d: Wrong RVA!\n", __LINE__);
		return -1;
	}
	int offset = FoaAddress - pSectionHeader[sectionIndex].PointerToRawData;
	RvaAddress = pSectionHeader[sectionIndex].VirtualAddress + offset;
	return RvaAddress;
}

VOID Print_Import_Directory(char* fileBuffer) {

	if (fileBuffer == NULL) {
		printf("[ERROR] %d: Wrong parameters!\n", __LINE__);
		return;
	}

	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	PIMAGE_SECTION_HEADER     pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	// 1. 定位导入表
	DWORD RVA_Of_ImportDirectory = pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress; // 1
	DWORD FOA_Of_ImportDirectory = RVA_To_FOA(fileBuffer, RVA_Of_ImportDirectory);
	
	// 2. 解析导入表
	// 导入表可能有多个
	PIMAGE_IMPORT_DESCRIPTOR pImageImportDirectory = (PIMAGE_IMPORT_DESCRIPTOR)((PBYTE)fileBuffer + FOA_Of_ImportDirectory);
	IMAGE_IMPORT_DESCRIPTOR NullImportDirectory = { 0 };

	while (1) {
		size_t isEnd = memcmp(pImageImportDirectory, &NullImportDirectory, sizeof(IMAGE_IMPORT_DESCRIPTOR));
		if (!isEnd) {
			break;
		}

		/*
		typedef struct _IMAGE_IMPORT_DESCRIPTOR {
			union {
				DWORD   Characteristics;            // 0 for terminating null import descriptor
				DWORD   OriginalFirstThunk;         // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
			} DUMMYUNIONNAME;
			DWORD   TimeDateStamp;                  // 0 if not bound,
													// -1 if bound, and real date\time stamp
													//     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
													// O.W. date/time stamp of DLL bound to (Old BIND)

			DWORD   ForwarderChain;                 // -1 if no forwarders
			DWORD   Name;
			DWORD   FirstThunk;                     // RVA to IAT (if bound this IAT has actual addresses)
		} IMAGE_IMPORT_DESCRIPTOR;
		typedef IMAGE_IMPORT_DESCRIPTOR UNALIGNED *PIMAGE_IMPORT_DESCRIPTOR;
		*/

		// 2.1 Dll名称
		DWORD FOA_Of_DllName = RVA_To_FOA(fileBuffer, pImageImportDirectory->Name);
		printf("Dll Name: %s\n", (char*)(fileBuffer + FOA_Of_DllName));

		// 2.2. INT表
		printf("\nINT表\n");
		DWORD FOA_Of_ImportNameTable = RVA_To_FOA(fileBuffer, pImageImportDirectory->OriginalFirstThunk);
		PIMAGE_THUNK_DATA32 pImageThunkData32Arr = (PIMAGE_THUNK_DATA32)((PBYTE)fileBuffer + FOA_Of_ImportNameTable);
		while (pImageThunkData32Arr->u1.ForwarderString != 0x0000) {
			// 最高位为1： 导出函数的序号
			if (pImageThunkData32Arr->u1.Ordinal & 0x8000 == 1) {
				printf("函数按序号导出， 序号： %d\n", pImageThunkData32Arr->u1.Ordinal);
			}
			// 最高位不为1： PIMAGE_IMPORT_BY_NAME的RVA, PIMAGE_IMPORT_BY_NAME中包含函数名
			else { 
				DWORD FOA_Of_ImageImportByName = RVA_To_FOA(fileBuffer, pImageThunkData32Arr->u1.AddressOfData);
				PIMAGE_IMPORT_BY_NAME pImageImportByName = (PIMAGE_IMPORT_BY_NAME)((PBYTE)fileBuffer + FOA_Of_ImageImportByName);
				printf("函数按名称导出， 函数名： %s\n", pImageImportByName->Name);
			}
			pImageThunkData32Arr += 1;
		}

		// 2.3 IAT表
		printf("\nIAT表\n");
		DWORD FOA_Of_ImportAddressTable = RVA_To_FOA(fileBuffer, pImageImportDirectory->FirstThunk);
		PIMAGE_THUNK_DATA32 pImageThunkData32Arr_IAT = (PIMAGE_THUNK_DATA32)((PBYTE)fileBuffer + FOA_Of_ImportAddressTable);
		while (pImageThunkData32Arr_IAT->u1.ForwarderString != 0x0000) {
			// 最高位为1： 导出函数的序号
			if (pImageThunkData32Arr_IAT->u1.Ordinal & 0x8000 == 1) {
				printf("函数按序号导出， 序号： %d\n", pImageThunkData32Arr_IAT->u1.Ordinal);
			}
			// 最高位不为1： PIMAGE_IMPORT_BY_NAME的RVA, PIMAGE_IMPORT_BY_NAME中包含函数名
			else {
				DWORD FOA_Of_ImageImportByName = RVA_To_FOA(fileBuffer, pImageThunkData32Arr_IAT->u1.AddressOfData);
				PIMAGE_IMPORT_BY_NAME pImageImportByName = (PIMAGE_IMPORT_BY_NAME)((PBYTE)fileBuffer + FOA_Of_ImageImportByName);
				printf("函数按名称导出， 函数名： %s\n", pImageImportByName->Name);
			}
			pImageThunkData32Arr_IAT += 1;
		}

		printf("\n********************************\n");
		pImageImportDirectory += 1;
	}
}

int main()
{
	char* fileBuffer = NULL;
	int fileBufferLength = 0;

	Read_PE_File_To_FileBuffer(&fileBuffer, &fileBufferLength);
	Print_Import_Directory(fileBuffer);

	if (fileBuffer) {
		free(fileBuffer);
		memset(fileBuffer, 0, fileBufferLength);
	}

	system("pause");
	return 0;
}
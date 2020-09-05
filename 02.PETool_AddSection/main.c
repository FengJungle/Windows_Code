#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>

#define IN_FILE_NAME   "Test.exe"
#define OUT_FILE_NAME  "Out.exe"

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

DWORD FileDataSize_To_ImageDataSize(char* fileBuffer, DWORD fileDataSize) {

	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);

	if (pOptionalHeader->FileAlignment == pOptionalHeader->SectionAlignment) {
		return fileDataSize;
	}
	return (1 + fileDataSize / pOptionalHeader->SectionAlignment)*pOptionalHeader->SectionAlignment;
}

/*
* Function name: Move_NtHeader
* Description  : Move NT headers to Dos stub, that is the end of the Dos header
* Parameters   : fileBuffer       - Original fileBuffer
*                fileBufferLength - Length of the original fileBuffer
* Return       : None
*/
VOID Move_NtHeader(char* fileBuffer, DWORD fileBufferLength) {

	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4); 
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	PIMAGE_SECTION_HEADER     pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	char* Destination = (char*)((PBYTE)pDosHeader + sizeof(IMAGE_DOS_HEADER));
	DWORD size = 4 + sizeof(IMAGE_FILE_HEADER) + pImageFileHeader->SizeOfOptionalHeader 
		+ pImageFileHeader->NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
	memcpy(Destination, (char*)pNtHeader, size);

	// Update the e_lfanew
	pDosHeader->e_lfanew = sizeof(IMAGE_DOS_HEADER);
	
	//Save_Buffer_To_File(fileBuffer, fileBufferLength);
}

VOID Add_Section(char* fileBuffer, DWORD fileBufferLength) {

	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	PIMAGE_SECTION_HEADER     pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	// 0. 新增节
	IMAGE_SECTION_HEADER NewSectionHeader = { 0 }; 

	// 1. 判断节表后面是否有80个字节的空间
	DWORD Offset = pOptionalHeader->SizeOfHeaders - (sizeof(IMAGE_DOS_HEADER) + 4 + sizeof(IMAGE_FILE_HEADER) + pImageFileHeader->SizeOfOptionalHeader +
		pImageFileHeader->NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
	if (Offset < 80) {
		printf("[WARN] %d: No enough space! Move NtHeader to Dos stub!\n", __LINE__);
		Move_NtHeader(fileBuffer, fileBufferLength);

		// 2. 重新获取
		pDosHeader        = (PIMAGE_DOS_HEADER)fileBuffer;
		pNtHeader         = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
		pImageFileHeader  = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
		pOptionalHeader   = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
		pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);
	}

	// 3. 设置新增节的属性

	// 3.1 获取原fileBuffer的最后一个节的指针
	PIMAGE_SECTION_HEADER pLastSection = (PIMAGE_SECTION_HEADER)(pSectionHeader + pImageFileHeader->NumberOfSections - 1);

	// 3.2 设置新增节的属性
	CHAR SectionName[8] = ".NewSec";
	memcpy(NewSectionHeader.Name, SectionName, sizeof(SectionName));
	NewSectionHeader.Misc.VirtualSize = 0x1000;
	NewSectionHeader.SizeOfRawData = 0x1000;
	NewSectionHeader.PointerToRawData = (DWORD)(pLastSection->PointerToRawData + (DWORD)pLastSection->SizeOfRawData);
	NewSectionHeader.VirtualAddress = (DWORD)(pLastSection->VirtualAddress + FileDataSize_To_ImageDataSize(fileBuffer, (DWORD)pLastSection->SizeOfRawData));
	NewSectionHeader.Characteristics = 0x6000020;
	NewSectionHeader.PointerToRelocations = 0;
	NewSectionHeader.PointerToLinenumbers = 0;
	NewSectionHeader.NumberOfRelocations = 0;
	NewSectionHeader.NumberOfLinenumbers = 0;

	// 4. 分配新buffer
	char* newFileBuffer = (char*)malloc(fileBufferLength + NewSectionHeader.SizeOfRawData);
	if (NULL == newFileBuffer) {
		printf("[ERROR] %d: malloc failed!\n", __LINE__);
		return;
	}

	// 4.1 复制fileBuffer到newFileBuffer
	memcpy(newFileBuffer, fileBuffer, fileBufferLength);

	// 4.2 设置新增节的内容为0
	memset(newFileBuffer + fileBufferLength, 0, NewSectionHeader.SizeOfRawData);

	// 5. 获取newFileBuffer属性
	pDosHeader = (PIMAGE_DOS_HEADER)newFileBuffer;
	pNtHeader = (PIMAGE_NT_HEADERS32)(newFileBuffer + pDosHeader->e_lfanew);
	pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);

	// 6. 修改newFileBuffer属性
	pImageFileHeader->NumberOfSections += 1;
	pOptionalHeader->SizeOfImage += (newFileBuffer, 0x1000);
	pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	// 7. 复制新增节到节表中
	memcpy((char*)(pSectionHeader + pImageFileHeader->NumberOfSections - 1), &NewSectionHeader, sizeof(IMAGE_SECTION_HEADER));

	// 8. 保存
	Save_Buffer_To_File(newFileBuffer, fileBufferLength + NewSectionHeader.SizeOfRawData);

	// 9. 释放资源
	free(newFileBuffer);
	newFileBuffer = NULL;
}

int main()
{
	char* fileBuffer = NULL;
	int fileBufferLength = 0;

	Read_PE_File_To_FileBuffer(&fileBuffer, &fileBufferLength);
	
	Add_Section(fileBuffer, fileBufferLength);

	if (fileBuffer) {
		memset(fileBuffer, 0, fileBufferLength);
		free(fileBuffer);
		fileBuffer = NULL;
	}

	return 0;
}
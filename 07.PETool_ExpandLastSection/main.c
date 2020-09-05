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
* Function name: Expand_Last_Section
* Description  : Expand the last section, then save as a new file
* Parameters   : fileBuffer       - Original fileBuffer
*                fileBufferLength - Length of the original fileBuffer
* Return       : None
*/
VOID Expand_Last_Section(char* fileBuffer, DWORD fileBufferLength) {

	// 1. Malloc for new buffer and 
	char* newBuffer = (char*)malloc(fileBufferLength + 0x1000);
	if (NULL == newBuffer) {
		printf("[ERROR] %d: malloc failed!\n", __LINE__);
		return;
	}
	memset(newBuffer, 0, fileBufferLength + 0x1000);

	// 2. Copy fileBuffer to new buffer and parse it
	memcpy(newBuffer, fileBuffer, fileBufferLength);
	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)newBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(newBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	PIMAGE_SECTION_HEADER     pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	// 3. Modify attributes
	// 3.1 Modify SizeOfImage
	pOptionalHeader->SizeOfImage += FileDataSize_To_ImageDataSize(newBuffer, 0x1000);

	// 3.2 Modify the attribute of the last section
	PIMAGE_SECTION_HEADER pLastSection = (PIMAGE_SECTION_HEADER)(pSectionHeader + pImageFileHeader->NumberOfSections - 1);
	pLastSection->SizeOfRawData += 0x1000;
	pLastSection->Misc.VirtualSize += FileDataSize_To_ImageDataSize(newBuffer, 0x1000);

	// 4. Save as a new file
	Save_Buffer_To_File(newBuffer, fileBufferLength + 0x1000);

	// 5. Free
	memset(newBuffer, 0, fileBufferLength + 0x1000);
	free(newBuffer);
	newBuffer = NULL;
}

int main()
{

	char* fileBuffer = NULL;
	int fileBufferLength = 0;

	Read_PE_File_To_FileBuffer(&fileBuffer, &fileBufferLength);

	Expand_Last_Section(fileBuffer, fileBufferLength);

	if (fileBuffer) {
		memset(fileBuffer, 0, fileBufferLength);
		free(fileBuffer);
		fileBuffer = NULL;
	}

	return 0;
}
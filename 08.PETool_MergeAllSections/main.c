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

DWORD ImageDataSize_To_FileDataSize(char* fileBuffer, DWORD ImageDataSize) {

	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);

	if (pOptionalHeader->FileAlignment == pOptionalHeader->SectionAlignment) {
		return ImageDataSize;
	}
	return (1 + ImageDataSize / pOptionalHeader->FileAlignment)*pOptionalHeader->FileAlignment;
}

VOID FileBuffer_To_ImageBuffer(char* fileBuffer, char* imageBuffer) {

	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	PIMAGE_SECTION_HEADER     pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	// 1. Copy headers
	memcpy(imageBuffer, fileBuffer, pOptionalHeader->SizeOfHeaders);

	// 2. Copy sections
	for (int i = 0; i < pImageFileHeader->NumberOfSections; i++) {
		memcpy(imageBuffer + pSectionHeader[i].VirtualAddress, fileBuffer + pSectionHeader[i].PointerToRawData, pSectionHeader[i].SizeOfRawData);
	}
}

/*
* Function name: Merge_All_Sections
* Description  : Merge all sections to one section
* Parameters   : fileBuffer       - Original fileBuffer
*                fileBufferLength - Length of the original fileBuffer
* Return       : None
*/
VOID Merge_All_Sections(char* fileBuffer, DWORD fileBufferLength) {

	// 1. fileBuffer --> ImageBuffer

	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	PIMAGE_SECTION_HEADER     pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	char* imageBuffer = (char*)malloc(pOptionalHeader->SizeOfImage);
	if (NULL == imageBuffer) {
		printf("[ERROR] %d: malloc failed!\n", __LINE__);
		return;
	}
	memset(imageBuffer, 0, pOptionalHeader->SizeOfImage);

	FileBuffer_To_ImageBuffer(fileBuffer, imageBuffer);

	// 2. Modify the attribute of the first section
	pDosHeader       = (PIMAGE_DOS_HEADER)imageBuffer;
	pNtHeader        = (PIMAGE_NT_HEADERS32)(imageBuffer + pDosHeader->e_lfanew);
	pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	pOptionalHeader  = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	pSectionHeader   = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	// 2.1 Calculate SizeOfRawData and VirtualSize
	DWORD VirtualSize = pOptionalHeader->SizeOfImage - pSectionHeader->VirtualAddress;
	DWORD SizeOfRawData = ImageDataSize_To_FileDataSize(fileBuffer, VirtualSize);

	// 2.2 Modify the attribute of the first section
	pSectionHeader->SizeOfRawData = SizeOfRawData;
	pSectionHeader->Misc.VirtualSize = VirtualSize;
	for (int i = 1; i < pImageFileHeader->NumberOfSections; i++) {
		pSectionHeader[0].Characteristics |= pSectionHeader[i].Characteristics;
	}
	memset(pSectionHeader + 1, 0, sizeof(IMAGE_SECTION_HEADER) * (pImageFileHeader->NumberOfSections - 1));

	// 3. Modify the NumberOfSecitons to 1
	pImageFileHeader->NumberOfSections = 1;

	// 5. Save as a new file
	Save_Buffer_To_File(imageBuffer, pOptionalHeader->SizeOfImage);

	// 6. Free
	memset(imageBuffer, 0, pOptionalHeader->SizeOfImage);
	free(imageBuffer);
	imageBuffer = NULL;
}

int main()
{

	char* fileBuffer = NULL;
	int fileBufferLength = 0;

	Read_PE_File_To_FileBuffer(&fileBuffer, &fileBufferLength);

	Merge_All_Sections(fileBuffer, fileBufferLength);

	if (fileBuffer) {
		memset(fileBuffer, 0, fileBufferLength);
		free(fileBuffer);
		fileBuffer = NULL;
	}

	return 0;
}
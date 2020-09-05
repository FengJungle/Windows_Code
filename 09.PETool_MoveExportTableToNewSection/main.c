#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>

#define IN_FILE_NAME   "MathSample.dll"
#define OUT_FILE_NAME  "NewFile.dll"

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

	if (RvaAddress < pOptionalHeader->SizeOfHeaders) {
		return RvaAddress;
	}

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

DWORD FOA_To_RVA(char* fileBuffer, DWORD FoaAddress)
{
	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	PIMAGE_SECTION_HEADER     pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	if (FoaAddress < pOptionalHeader->SizeOfHeaders) {
		return FoaAddress;
	}

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
	int offset = FoaAddress - pSectionHeader[sectionIndex].PointerToRawData;
	RvaAddress = pSectionHeader[sectionIndex].VirtualAddress + offset;
	return RvaAddress;
}

DWORD FileDataSize_To_ImageDataSize(char* fileBuffer, DWORD fileDataSize) {

	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);

	if (pOptionalHeader->FileAlignment == pOptionalHeader->SectionAlignment) {
		return fileDataSize;
	}
	if (fileDataSize % pOptionalHeader->SectionAlignment == 0) {
		return (fileDataSize / pOptionalHeader->SectionAlignment)*pOptionalHeader->SectionAlignment;
	}
	return (1 + fileDataSize / pOptionalHeader->SectionAlignment)*pOptionalHeader->SectionAlignment;
}

DWORD ImageDataSize_To_FileDataSize(char* fileBuffer, DWORD imageDataSize) {

	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);

	if (pOptionalHeader->FileAlignment == pOptionalHeader->SectionAlignment) {
		return imageDataSize;
	}
	if (imageDataSize % pOptionalHeader->FileAlignment == 0) {
		return (imageDataSize / pOptionalHeader->FileAlignment)*pOptionalHeader->FileAlignment;
	}
	return (1 + imageDataSize / pOptionalHeader->FileAlignment)*pOptionalHeader->FileAlignment;
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

VOID Add_Section(
	char* fileBuffer,
	DWORD fileBufferLength,
	char** newFileBuffer,
	DWORD* newFileBufferLength,
	DWORD  addSectionLength)
{

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
		pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
		pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
		pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
		pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
		pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);
	}

	// 3. 设置新增节的属性

	// 3.1 获取原fileBuffer的最后一个节的指针
	PIMAGE_SECTION_HEADER pLastSection = (PIMAGE_SECTION_HEADER)(pSectionHeader + pImageFileHeader->NumberOfSections - 1);

	// 3.2 设置新增节的属性
	CHAR SectionName[8] = ".NewSec";
	memcpy(NewSectionHeader.Name, SectionName, sizeof(SectionName));
	NewSectionHeader.Misc.VirtualSize = addSectionLength;
	NewSectionHeader.SizeOfRawData = addSectionLength;
	NewSectionHeader.PointerToRawData = (DWORD)(pLastSection->PointerToRawData + (DWORD)pLastSection->SizeOfRawData);
	NewSectionHeader.VirtualAddress = (DWORD)(pLastSection->VirtualAddress + FileDataSize_To_ImageDataSize(fileBuffer, (DWORD)pLastSection->SizeOfRawData));
	NewSectionHeader.Characteristics = 0x6000020;
	NewSectionHeader.PointerToRelocations = 0;
	NewSectionHeader.PointerToLinenumbers = 0;
	NewSectionHeader.NumberOfRelocations = 0;
	NewSectionHeader.NumberOfLinenumbers = 0;

	// 4. 分配新buffer
	*newFileBuffer = (char*)malloc(fileBufferLength + NewSectionHeader.SizeOfRawData);
	if (NULL == *newFileBuffer) {
		printf("[ERROR] %d: malloc failed!\n", __LINE__);
		return;
	}

	// 4.1 复制fileBuffer到newFileBuffer
	memcpy(*newFileBuffer, fileBuffer, fileBufferLength);

	// 4.2 设置新增节的内容为0
	memset(*newFileBuffer + fileBufferLength, 0, NewSectionHeader.SizeOfRawData);

	// 5. 获取newFileBuffer属性
	pDosHeader = (PIMAGE_DOS_HEADER)*newFileBuffer;
	pNtHeader = (PIMAGE_NT_HEADERS32)(*newFileBuffer + pDosHeader->e_lfanew);
	pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);

	// 6. 修改newFileBuffer属性
	pImageFileHeader->NumberOfSections += 1;
	pOptionalHeader->SizeOfImage += FileDataSize_To_ImageDataSize(*newFileBuffer, 0x1000);
	pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	// 7. 复制新增节到节表中
	memcpy((char*)(pSectionHeader + pImageFileHeader->NumberOfSections - 1), &NewSectionHeader, sizeof(IMAGE_SECTION_HEADER));

	*newFileBufferLength = fileBufferLength + NewSectionHeader.SizeOfRawData;
}

/*
* Function name: Move_ExportDirectory_To_NewSection
* Description  : Move ExportDirectory to a new section
* Parameters   : fileBuffer          - Original fileBuffer
*                fileBufferLength    - Length of the original fileBuffer
*                newFileBuffer       - New fileBuffer
*                newFileBufferLength - Length of the new fileBuffer
* Return       : None
*/
VOID Move_ExportDirectory_To_NewSection(
	char* fileBuffer, 
	DWORD fileBufferLength, 
	char** newFileBuffer, 
	DWORD* newFileBufferLength)
{

	PIMAGE_DOS_HEADER             pDosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS32            pNtHeader = (PIMAGE_NT_HEADERS32)(fileBuffer + pDosHeader->e_lfanew);
	PIMAGE_FILE_HEADER      pImageFileHeader = (PIMAGE_FILE_HEADER)((char*)pNtHeader + 4);
	PIMAGE_OPTIONAL_HEADER32 pOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)((char*)pImageFileHeader + 0x14);
	PIMAGE_SECTION_HEADER     pSectionHeader = (PIMAGE_SECTION_HEADER)((char*)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	/*
	*	typedef struct _IMAGE_EXPORT_DIRECTORY {
			DWORD   Characteristics;
			DWORD   TimeDateStamp;
			WORD    MajorVersion;
			WORD    MinorVersion;
			DWORD   Name;
			DWORD   Base;
			DWORD   NumberOfFunctions;
			DWORD   NumberOfNames;
			DWORD   AddressOfFunctions;     // RVA from base of image
			DWORD   AddressOfNames;         // RVA from base of image
			DWORD   AddressOfNameOrdinals;  // RVA from base of image
		} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;
	*
	*/

	// 1. 定位导出表
	PIMAGE_EXPORT_DIRECTORY pExportDirectory = (PIMAGE_EXPORT_DIRECTORY)(fileBuffer + RVA_To_FOA(fileBuffer, pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress));
	PDWORD pAddressOfFunctions = (PWORD)((DWORD)fileBuffer + (DWORD)(RVA_To_FOA(fileBuffer, pExportDirectory->AddressOfFunctions)));
	PDWORD pAddressOfNames = (PWORD)((DWORD)fileBuffer + (DWORD)(RVA_To_FOA(fileBuffer, pExportDirectory->AddressOfNames)));
	PDWORD pAddressOfOrdinals = (PWORD)((DWORD)fileBuffer + (DWORD)(RVA_To_FOA(fileBuffer, pExportDirectory->AddressOfNameOrdinals)));

	// 2. 计算新增节的大小
	// = NumberOfFunctions * 4 + NumberOfNames * (2 + 4) + 所有函数名的字节 + sizeof(IMAGE_EXPORT_DIRECTORY),然后文件对齐
	DWORD NewSectionSize = 0;
	
	// 2.1 AddressOfFunctions 的空间
	NewSectionSize += pExportDirectory->NumberOfFunctions * 4;  
	
	// 2.2 AddressOfNames + AddressOfNameOrdinals 的空间
	NewSectionSize += pExportDirectory->NumberOfNames * (2 + 4); 
	
	// 2.3 所有导出函数名所占空间
	for (size_t i = 0; i < pExportDirectory->NumberOfNames; i++)
	{
		LPCSTR lpszFuncName = (LPCSTR)((DWORD)fileBuffer + RVA_To_FOA(fileBuffer, pAddressOfNames[i]));
		printf("\nFunction name: %s\n", (char*)lpszFuncName);
		NewSectionSize += strlen(lpszFuncName) + 1;
	}
	// 2.4 导出表大小
	NewSectionSize += sizeof(IMAGE_EXPORT_DIRECTORY);

	// 2.5 文件对齐
	SHORT AddOne = NewSectionSize % pOptionalHeader->FileAlignment != 0;
	NewSectionSize = (AddOne + NewSectionSize / pOptionalHeader->FileAlignment)*pOptionalHeader->FileAlignment;
	printf("\n新增节的大小 = %x\n", NewSectionSize);

	// 3. 添加节，并存在NewFileBuffer中
	Add_Section(fileBuffer, fileBufferLength, newFileBuffer, newFileBufferLength, NewSectionSize);

	pDosHeader       = (PIMAGE_DOS_HEADER)*newFileBuffer;
	pImageFileHeader = (PIMAGE_FILE_HEADER)(pDosHeader->e_lfanew + (DWORD)pDosHeader + 4);
	pOptionalHeader  = (PIMAGE_OPTIONAL_HEADER32)((DWORD)pImageFileHeader + sizeof(IMAGE_FILE_HEADER));
	pSectionHeader   = (PIMAGE_SECTION_HEADER)((DWORD)pOptionalHeader + pImageFileHeader->SizeOfOptionalHeader);

	// 4. 修改新增节属性为可读、含已初始化数据
	pSectionHeader[pImageFileHeader->NumberOfSections - 1].Characteristics = 0x40000040;

	// 5. 在NewFileBuffer中定位导出表
	pExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((DWORD)*newFileBuffer + RVA_To_FOA(*newFileBuffer, pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress));

	pAddressOfFunctions = (PDWORD)((DWORD)*newFileBuffer + RVA_To_FOA(*newFileBuffer, pExportDirectory->AddressOfFunctions));
	pAddressOfOrdinals  = (PWORD)((DWORD)*newFileBuffer + RVA_To_FOA(*newFileBuffer, pExportDirectory->AddressOfNameOrdinals));
	pAddressOfNames     = (PDWORD)((DWORD)*newFileBuffer + RVA_To_FOA(*newFileBuffer, pExportDirectory->AddressOfNames));

	// 6. 把3张子表拷贝到新节，更新指针
	LPVOID pInsert = (LPVOID)((DWORD)*newFileBuffer + pSectionHeader[pImageFileHeader->NumberOfSections - 1].PointerToRawData);

	// 6.1 函数地址表
	memcpy(pInsert, pAddressOfFunctions, 4 * pExportDirectory->NumberOfFunctions);
	pAddressOfFunctions = (PDWORD)pInsert;

	// 6.2 序号表
	pInsert = (LPVOID)((DWORD)pInsert + 4 * pExportDirectory->NumberOfFunctions);
	memcpy(pInsert, pAddressOfOrdinals, 2 * pExportDirectory->NumberOfNames);
	pAddressOfOrdinals = (PWORD)pInsert;

	// 6.3 函数名字表
	pInsert = (LPVOID)((DWORD)pInsert + 2 * pExportDirectory->NumberOfNames);
	memcpy(pInsert, pAddressOfNames, 4 * pExportDirectory->NumberOfNames);
	pAddressOfNames = (PDWORD)pInsert;

	// 6.4 拷贝函数名
	pInsert = (LPVOID)((DWORD)pInsert + 4 * pExportDirectory->NumberOfNames);
	for (size_t i = 0; i < pExportDirectory->NumberOfNames; i++)
	{
		LPCSTR lpszFuncName = (LPCSTR)((DWORD)*newFileBuffer + RVA_To_FOA(*newFileBuffer, pAddressOfNames[i]));
		memcpy(pInsert, lpszFuncName, strlen(lpszFuncName) + 1);
		// 更新函数名的RVA地址
		pAddressOfNames[i] = FOA_To_RVA(*newFileBuffer, (DWORD)pInsert - (DWORD)*newFileBuffer);
		pInsert = (LPVOID)((DWORD)pInsert + strlen(lpszFuncName) + 1);
	}

	// 6.5 拷贝导出表
	memcpy(pInsert, pExportDirectory, sizeof(IMAGE_EXPORT_DIRECTORY));

	// 7. 修改导出表的地址表
	pExportDirectory = (PIMAGE_EXPORT_DIRECTORY)pInsert;
	pExportDirectory->AddressOfFunctions = FOA_To_RVA(*newFileBuffer, (DWORD)pAddressOfFunctions - (DWORD)*newFileBuffer);
	pExportDirectory->AddressOfNameOrdinals = FOA_To_RVA(*newFileBuffer, (DWORD)pAddressOfOrdinals - (DWORD)*newFileBuffer);
	pExportDirectory->AddressOfNames = FOA_To_RVA(*newFileBuffer, (DWORD)pAddressOfNames - (DWORD)*newFileBuffer);

	// 8. 修改目录项，指向新的导出表
	pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = FOA_To_RVA(*newFileBuffer, (DWORD)pExportDirectory - (DWORD)*newFileBuffer);

	// 9. 保存到新的Dll文件中
	Save_Buffer_To_File(*newFileBuffer, *newFileBufferLength);
}

int main()
{

	char* fileBuffer = NULL;
	int fileBufferLength = 0;
	char* newFileBuffer = NULL;
	int newFileBufferLength = 0;

	Read_PE_File_To_FileBuffer(&fileBuffer, &fileBufferLength);

	Move_ExportDirectory_To_NewSection(fileBuffer, fileBufferLength, &newFileBuffer, &newFileBufferLength);

	if (fileBuffer) {
		memset(fileBuffer, 0, fileBufferLength);
		free(fileBuffer);
		fileBuffer = NULL;
	}
	if (newFileBuffer) {
		memset(newFileBuffer, 0, newFileBufferLength);
		free(newFileBuffer);
		newFileBuffer = NULL;
	}

	return 0;
}
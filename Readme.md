# Bug Explanation

**NtCreateFile** can create and access directories with names such as `" ."` (dot+space), whereas **CreateFile** cannot. In these cases, **CreateFile** silently normalizes the name to `" "` (a space), which refers to an entirely different directory.  
This behavior is caused by `KERNELBASE!_imp__RtlDosPathNameToRelativeNtPathName_U_WithStatus`, which—as described in the *"Types of DOS Path"* section of the following article—*“Removes any trailing spaces or dots from the last path element, assuming it isn’t a single- or double-dot name.”*  
The screenshot below demonstrates this behavior when accessing `" ."` using **explorer.exe**.

![alt text](screenshots/screenshot1.png "Accessing ' .' from explorer.exe")

# Implications

1. Malware can be hidden from antivirus software that uses **CreateFile** to enumerate or access files and directories.  
2. Malware can also remain hidden from users relying on tools such as **explorer.exe** and **cmd.exe**, as these ultimately depend on **CreateFile**.

# Disclosure

Microsoft will not fix this issue, citing the following reason:  
> “This is a known portion of the file structure and is detailed online. Beyond that, an attacker would already need to have compromised a machine to make use of this.”

Although this behavior is documented online, it resembles rootkit techniques and can be abused without installing anything—e.g., by simply plugging in a USB stick containing a maliciously crafted file structure that exploits this behavior.  
**I have decided to publish the code.**

**UPDATE:** I asked Microsoft for the relevant documentation and they kindly replied with [this link](https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file?redirectedfrom=MSDN), referencing the following statement:  
> “Do not end a file or directory name with a space or a period. Although the underlying file system may support such names, the Windows shell and user interface do not.”

However, I do not believe that a problem ceases to be a problem simply because it is documented. Moreover, an attacker does not need prior access to the machine to exploit this behavior, as it can be triggered using an external device such as a USB drive.

# PoC

[PoC file](./PoC.c)  
[Mini DDK dependency](./miniddk.h)  

**Note:** If you test the PoC and want to remove the resulting `C:\ .\` directory, an easy way to do so is by using the Git Bash console: <https://git-scm.com/download/win>

# Debugging Session

### 1. `Kernel32!CreateFileW` calls `KERNELBASE!CreateFileInternal`
```
771de463 8364240c00      and     dword ptr [esp+0Ch],0
771de468 8b4514          mov     eax,dword ptr [ebp+14h]
771de46b 8b550c          mov     edx,dword ptr [ebp+0Ch]
771de46e 8b4d08          mov     ecx,dword ptr [ebp+8]      ; u"C:\ .\my_hidden_malware.exe"
771de471 89442410        mov     dword ptr [esp+10h],eax
771de475 8b4520          mov     eax,dword ptr [ebp+20h]
771de478 6a00            push    0
771de47a 89442418        mov     dword ptr [esp+18h],eax
771de47e 8d442404        lea     eax,[esp+4]
771de482 50              push    eax
771de483 ff7518          push    dword ptr [ebp+18h]
771de486 ff7510          push    dword ptr [ebp+10h]
771de489 e812000000      call    KERNELBASE!CreateFileInternal (771de4a0)
```

### 2. Debugging inside `KERNELBASE!CreateFileInternal`
```
KERNELBASE!CreateFileInternal:
771de4a0 8bff            mov     edi,edi
771de4a2 55              push    ebp
771de4a3 8bec            mov     ebp,esp
771de4a5 83e4f8          and     esp,0FFFFFFF8h
771de4a8 81ec84000000    sub     esp,84h
771de4ae a1307b2a77      mov     eax,dword ptr [KERNELBASE!__security_cookie (772a7b30)]
771de4b3 33c4            xor     eax,esp
771de4b5 89842480000000  mov     dword ptr [esp+80h],eax
771de4bc 53              push    ebx
771de4bd 56              push    esi
771de4be 8b7510          mov     esi,dword ptr [ebp+10h]
771de4c1 8bd9            mov     ebx,ecx                 ; u"C:\ .\my_hidden_malware.exe"
```

### 3. `RtlInitUnicodeStringEx` correctly initializes the string into `[esp+34]`
```
771de4fe c744241401000000 mov     dword ptr [esp+14h],1
771de506 53              push    ebx                    ; Source
771de507 8d442434        lea     eax,[esp+34h]
771de50b 50              push    eax                    ; <-- Destination
771de50c ff1570b12a77    call    dword ptr [KERNELBASE!_imp__RtlInitUnicodeStringEx
```

```
0:000:x86> du poi(esp+34)
0040422c  "C:\ .\my_hidden_malware.exe"
```

### 4. The string is then passed to  
`KERNELBASE!_imp__RtlDosPathNameToRelativeNtPathName_U_WithStatus`
```
771de549 50              push    eax
771de54a 6a00            push    0
771de54c 8d442438        lea     eax,[esp+38h]
771de550 50              push    eax
771de551 53              push    ebx    ; "C:\ .\my_hidden_malware.exe"
771de552 ff1538b92a77    call    dword ptr [KERNELBASE!_imp__RtlDosPathNameToRelativeNtPathName_U_WithStatus]
```

### 5. And this is where the vulnerability occurs
```
771de552 ff1538b92a77    call    dword ptr [...]
771de558 85c0            test    eax,eax
771de55a 0f88f61e0300    js      KERNELBASE!CreateFileInternal+0x31fb6
771de560 8b442434        mov     eax,dword ptr [esp+34h] ; "\??\C:\ \my_hidden_malware.exe"
```


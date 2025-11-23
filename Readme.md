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

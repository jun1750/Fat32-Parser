
 # FAT32-Parser
A Library console that reads and parses data from a FAT32 file system (disk). Includes a shell to traverse through folders and download individual files. 
 
 # Usage
 
 $ make
 
 *After make has run successfully, run the exe using a formatted disk image:*
 
 $ ./fat32 diskimage

 ## Shell Supported commands:
  >info
   - provides device, geometry and file system information of the provided fat32 disk.
   
  >dir
   - lists the current directory
   
  >cd <directory_name>
   - switches to the specified directory
   
  >get <file_name>
   - downloads the file from the fat32 disk to the local machine

![image](https://user-images.githubusercontent.com/50674368/217143134-f01ffff9-deac-479b-a743-d75db751426b.png)
![image](https://user-images.githubusercontent.com/50674368/217144746-5fa761a8-0bee-4b3a-8fbe-7bd7cbaa6b92.png)

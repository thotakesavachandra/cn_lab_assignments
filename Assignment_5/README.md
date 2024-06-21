```
Read the assignment problem statement for more details.
Some clarifications and updates are made to the problem statement. 
Those are provided in the update.pdf file.
Extra details are provided in the documentation.txt
```
## Implementation 
This assignment contains the implentation of a custom protocol named **MTP** (My Transfer Protocol) by using UDP as underlying protocol. 
<br>
Reliability and flow control are achieved, even though those are not provided by UDP by using **Selective Repeat ARQ**.
<br>
**Threads** are used to do different tasks like sending and receiving data, managment of sockets and garbage collection.
<br>
Usage and other implementation details are provided in the documentation.txt file.

## Files
- `msocket.h` : Contains the headers required to use the MTP sockets.
- `msocket.c` : Contains the implementation of MTP socket functions like m_sendto and m_recvfrom.
- `initmsocket.c` : Contains the implentaion of the intermediary functions which convert the MTP socket functions to UDP functions and the functions that are required to provide reliability and flow control.
- `makefile` : Contains the commands to compile the code.
- `update.pdf` : Contains the updates and clarifications made to the problem statement.
- `documentation.txt` : Contains the testing, and detailed implementation details.
- `other_files` : Sample files for testing and output files.
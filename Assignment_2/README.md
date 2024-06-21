```
Read the assignment problem statement for more details.
```
## Implementation Details
This assignment contains implentation of a TCP concurrent server and client. The server can handle multiple clients concurrently. 
<br> 
The client sends a file and a encryption-key(k) to the server. The server encrypts the file using Caeser Cipher with key k and sends the encrypted file back to the client. The client saves the encrypted file in the same directory as the original file with the name "{$FILENAME}.enc".
<br> 
Some sample test cases are provided in the materials directory.

## Files
- `client.c` : Contains the implementation of the client
- `server.c` : Contains the implementation of the server
- `makefile` : Contains the commands for compiling the client and server
- `Test_Cases(dir)` : Contains the sample test cases for the client and server

<br>
Note : Some of the testcases provided do not adhere to the constraints of the problem statement.
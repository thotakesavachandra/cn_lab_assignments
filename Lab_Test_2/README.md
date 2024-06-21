```
Read the problem statement for more details.
```
## Implementation
Implementation of online voting portal using TCP client server. Concurrency is implemented using **select** call.
<br>
The server initiates the voting process and waits for clients to connect.
<br>
The client connects to the server and receives the list of candidates. The client can choose to vote for a candidate, or can refrain from voting. The client then receives the acknowledgement from the server.
<br>
The server receives the vote from the client and updates the vote count of the corresponding candidate.
<br>
The server admin can view the vote count of each candidate at any time. Admin can edit the candidate list by adding new candidates or deleting existing candidates.

## Files
- `server.c`: Contains the implementation of the server.
- `client.c`: Contains the implementation of the client.
- `headers.h`: Contains the necessary headers and global variables common to both server and client.
- `makefile`: Contains the commands to compile the server and client.
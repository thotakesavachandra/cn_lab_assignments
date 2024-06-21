```
Read assignment problem statement for more details.
```
## Implementation and usage
This assignment contains the implentation of **SMTP** and **POP3** protocols. The only supported domain is '_iitkgp.edu_'.
<br>
The smtp server runs on port 20000, and the pop3 server runs on port 30000. The client should be run with arguments \<server-IP\> \<smtp-port\> \<pop3-port\>. 
<br>
The main menu has options to read mail, send mail, and quit. An option can be selected by entering the corresponding number.
<br>
For reading mail, intially the list of mails is displayed. Upon selection of a mail by entering the corresponding number, the mail is displayed. Entering -1 as mail number will return to main menu. Inorder to delete the mail after reading, character 'd' should be entered. To avoid deletion any other character can be entered.
<br>
For sending the mail proper SMTP format has to followed.

## Files
- `mailclient.c` : Contains the implementation of the client.
- `smtpserver.c` : Contains the implementation of the SMTP server resposible for writing the received mails to the mailbox.
- `popserver.c` : Contains the implementation of the POP3 server resposible for delivering the received mails to the client.
- `user.txt` : Contains the list of users and their passwords.
- `directory_editor.py` : Contains the code for clearing the old mailbox and creating a new mailbox for the users in user.txt
- `makefile` : Contains the commands for compiling the code.
- `other_dir/mailbox` : Contains the mails intended for the corresponding user.
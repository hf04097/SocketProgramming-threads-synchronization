# SocketProgramming-threads-synchronization
A rudimentary chatting application is implemented which runs on a client/server model

![alt text](https://raw.githubusercontent.com/hf04097/SocketProgramming-threads-synchronization/master/client%20server.png)

## What does the program do?

* You basically run the server passing it a port number.The server program creates a socket and starts listening on the port passed to it at command line.

* You run a client passing it the server programs computer IP, the port it’s listening on, and a name for your client.
The client makes a socket and try to make a connection to server program using the server’s PC’s IP and the port number the server is listening on. Once the connection is established, the client sends its identifier (“client1” in this case) to the server. In case there is no server running, the client terminates with an appropriate message.

* The server program should accept connections from multiple clients. For each connection established the server should store the client’s identifier and keep a record of which port is being used for communication with this client.

* The server and client programs can be on the same PC or different ones.

*  A separate thread is created for each connection with the client.

* The server maintains a linked-list of all the clients connected to it and each node of the linked list would store the information about a client, i.e., it’s name, it’s file descriptor, etc, any other thing that you find useful. This linked-list is accessible via multiple threads in a thread-safe manner.

* When the server creates a thread for a client, this client’s node is added to the linked list.

* When a client exits, its node is removed from the linked list.

* If a new connection request comes from a a client with a client identifier which is already in use, the server informs the client with an appropriate error message and close that connection.

* If you make the client multithreaded as well, you will be able to receive messages from other clients, even when your own client program is blocked waiting for an input from user.

* The client can send “commands” to the server. Each command starts with a ‘/’ character. When the server program receives a command from a client, it should take the appropriate action and send a response to the client. Few commands:
    * /list - if the client sends a “/list” command to the server, it means the client is asking for a list of all the connected clients. Upon receiving this command, the server  puts the names of all the clients connected to itself in string and send it back to the client. The client shall receive this string and display all these names to the user.
  
    * /msg – users can type this command to send messages to other clients. The general syntax is “/msg clientname message”. i.e. if the user at client-1 types “/msg client-2 hi there”, the server receives this message from client-1 and send it to client-2. The threaded the server created for client-1 accesses the linked-list, searchs for the node containing client-2 info, and sends the message to client-2 by writing to its descriptor. The user at client-2 receives the message “hi there” along with the information that it came from client-1. You should do proper error handling for this command at server and client end i.e. missing destination (client) identifier or incorrect destination identifier generates appropriate response.
  
    * /quit – users can type this command to disconnect from the server and quit the client. Upon reception of this command from a client, that particular server thread closes the connection with that client, removes it’s data from the linked-list, frees any resources, and terminates itself.

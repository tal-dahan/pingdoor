PINGDOOR README
===============


ABOUT
-----
This simple C&C server uses icmp messages to signal remote agent the desired command to execute on the trarget machine.
The following commands to be executed on the target machine are supported:

a. ip
b. ps
c. uname
d. shell


HOW TO USE
----------
To use this program, take the following steps:

1. compile the agent on the target machine with the following command:
gcc -g agent.c -o agent core/source/*.c

2. run the program as sudo on the target machine:
sudo ./agent

3. compile the C&C server on the server machine with the following command:
gcc -g server.c -o server core/source/*.c

4. run the program on the server machine (sudo is not reuqired):
./server

5. start using the C&C cli!


WRITTEN BY
----------
Gilad Shevach
Ofir Shen
Tal Dahan

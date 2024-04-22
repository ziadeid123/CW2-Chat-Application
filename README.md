# CW2-Chat-Application
# Important notes for running apllication:
- This application was created and tested on Linux/Ubuntu 64 (Since file creation, reading, and writing permissions are required).
- To compile the server and client files, in the command terminal run the following:
-  "g++ server.cpp -o server -lssl -lcrypto" (server).
-  "g++ client.cpp -o client -lssl -lcrypto" (client).
-  After compiling, run this in the command terminal to execute the files:
-  "./server" (server).
-  "./client" (client).
-  Upon running the application, you will be prompted to enter a username and password of your choice, the inputs will be saved in a txt file named "credentials.txt".
-  when sending messages, all message logs will be saved in a txt file named "message_log.txt" and will be saved hashed for security implementaion.
-  !IMPORTANT NOTE! : you should compile and execute the files in the same path directory on your computer (preferred to all be on the desktop).

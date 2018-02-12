/*****************************************************************************************************/
/**************   Server will transfer a file to the client upon a request the client 	**************/
/*****************************************************************************************************/
/****************			 File Transfer  - Server Side	 	 	   *******************/
/****************														*****************/
/****************			1) Initialize TCP socket					****************/
/****************    		2) wait for a connection, and connect 		***************/
/****************  			3) wait and listen for income message    	***************/
/**************** 		    4) send file upon a request from client    	***************/
/****************														***************/
/*************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512
#define IP_ADDRESS "192.168.125.116"

// Prepare to transfer a file-dish_out.csv 
void getFileContent(char *value_ptr, char *space_ptr)
{
 	ifstream inFile;
	int i =0;
 	
	cout <<"reading file thread is processing...." <<endl;	
	
	inFile.open("dish_out.csv");
	    
	do{
		*value_ptr = inFile.get();
					
			if( !strcmp(value_ptr, space_ptr) )
			{
				i++;
			}
		cout <<  *value_ptr ;
	 	value_ptr++;

	} while(i< 11);
	
	inFile.close();
}

/*************************************************************************/

int main (void)
{
	WSADATA wsaData;
	SOCKET ListenSocket, ClientSocket;
	int iResult, iSendResult, recvbuflen = DEFAULT_BUFLEN, port, len_addr;
	struct addrinfo hints, *res=NULL, *p;
	struct in_addr *ip_addr;
	struct sockaddr_storage local_addr;

	char *ipstr, *sendbuf, ipv_num[] = "ip4";
	char str_dest[64], in_msg[64], *out_msg="welcome to the server!";	
	char recvbuf[DEFAULT_BUFLEN], hostname[64], hostclient[64], serv_name[64];
	bool opt_v = TRUE;
	
	//set of socket descriptors for select and master
    fd_set read_fds;
    fd_set master_fds;
    int fdmax; // maximum file descriptor number
    
    /************************************************************************
    **** send file to client variable declaration ***************************
    ************************************************************************/   		  			   

    char value_str[256], *value_ptr, space_str[]="\n", *space_ptr;

	/***********************************************************************/
	
	
	//initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if(iResult !=0)
	{
		cout << "WSAStartup failed! Error code: " << iResult <<endl;
		WSACleanup();
		return 1; 
		
	}
	
	//successful on initialize window socket
	cout <<"Win socket startup successful...." <<endl;
	
	//clear addrinfo structure, store information
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo("10.0.0.150", DEFAULT_PORT, &hints, &res);

	if(iResult !=0)
	{
		cout << "getaddrinfo failed! Error code: " << iResult <<endl;
		WSACleanup();
		return 1; 
	}
	
	//successful on initialize window socket
	cout <<"sucessful on calling getaddrinfo()...." <<endl;
	
	p=res;
	struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr; //cast to sockaddr_in structure
	
 	ipstr = inet_ntoa(ipv4->sin_addr);
 	if (ipstr ==NULL)
 	{
 		cout <<"Error calling inet_ntoa function " <<endl;
 		WSACleanup();
 		return 1;
	}
	
	cout << "successful on calling 1st inet_ntoa function " <<endl;
	memset(str_dest, 0, sizeof(str_dest));
	 
	strcpy(str_dest, ipstr);
	cout << ipv_num << "address is " << str_dest <<endl;
	
	//call a socket function on the server side
	ListenSocket = INVALID_SOCKET;
	
	ListenSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	cout <<"Server socket descriptor is " << ListenSocket <<endl;
	if(ListenSocket ==INVALID_SOCKET)
	{
		cout <<"Error when creating a server socket! ";
		cout <<"Error code: " << WSAGetLastError() <<endl;
		freeaddrinfo(res);
		WSACleanup();
		return 1;
		
	}
	
	cout << "successful on creating a server socket " << endl;
	
	//set server socket to allow multiple connections , this is just a good habit, it will work without this
    iResult = setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt_v, sizeof(opt_v) );
	if (iResult == SOCKET_ERROR) 
    {
       cout <<"Error code: " << WSAGetLastError() <<endl;
       freeaddrinfo(res);
       closesocket(ListenSocket);
	   WSACleanup();
	   return 1;
    }
	cout << "successful on setting server socket option " << endl;

    //binding a socket
    iResult = bind(ListenSocket, res->ai_addr, (int)res->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
    	cout <<"Error binding a socket! ";
    	cout <<"Error code is " <<WSAGetLastError() <<endl;
    	freeaddrinfo(res);
    	closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
    
    cout << "successful on binding a server socket " << endl;
	freeaddrinfo(res); //free addrinfo after binding, no longer needed
	
    //listen for incoming connection
	iResult = listen(ListenSocket, 2);
	if(iResult == SOCKET_ERROR)
	{
		cout <<"Error listening! ";
    	cout <<"Error code is " <<WSAGetLastError() <<endl;
    	closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	cout << "listening for an incoming connection..... " << endl;
	
	FD_ZERO (&read_fds);  // clear the sets
	FD_ZERO (&master_fds); //clear master sets
	
	// add the server socket ID to the set
	FD_SET(ListenSocket, &master_fds);
	fdmax = ListenSocket;
	
	int a =1;
	//start the select loop
	for(;;)
	{
		cout << "loop #" <<a<<endl;
		read_fds = master_fds; //copy the file descriptor sets, so far just the listening socket
		iResult = select(fdmax+1, &read_fds, NULL, NULL, NULL );
		if(iResult == SOCKET_ERROR )
		{
			cout <<"Error code calling select() " <<WSAGetLastError() <<endl;
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		//then there's some incoming activities
		// run through the existing connections looking for data to read
		for(int i = 0; i <= fdmax; i++)
		{
			if ( FD_ISSET(i, &read_fds) )
			{
				//found one on listening socket
				if (i == ListenSocket) 
				{
					// Accept a client socket
					ClientSocket=INVALID_SOCKET;
					len_addr = sizeof(local_addr);	
    				ClientSocket = accept(ListenSocket, (struct sockaddr *)&local_addr,
					                     &len_addr );
					                     
					//check errors
					if (ClientSocket == INVALID_SOCKET) 
				 	{
        				printf("accept failed with error: %d\n", WSAGetLastError());
        				closesocket(ListenSocket);
        				WSACleanup();
        				return 1;
    				}
					else{
		
						cout << "Client Connected !! " << endl;
						FD_SET(ClientSocket, &master_fds); //add to the master set , so now has listening socket+connected clinet socket
						cout << "client socket ID-" <<ClientSocket<<'\t'<<"listener socket ID-" <<ListenSocket <<endl;
						if (ClientSocket > fdmax )
						{
							fdmax = ClientSocket; //keep track of max descriptor
						}
						
						//send a welcome message
						send(ClientSocket, out_msg, (int)strlen(out_msg), 0)	;
						
						//find out who is on the other side
						iResult = getpeername(ClientSocket, (struct sockaddr*)&local_addr, &len_addr );  
	
						if(iResult == SOCKET_ERROR )
						{
							cout <<"Error getpeername()! ";
    						cout <<"Error code is " <<WSAGetLastError() <<endl;
    						closesocket(ClientSocket);
							WSACleanup();
							return 1;		
						} 
						
						// print out the peername information
						ipv4 = (struct sockaddr_in *) &local_addr;  //cast to sockaddr_in struct type
						port = ipv4->sin_port;   // extract port number		
    					ipstr = inet_ntoa(ipv4->sin_addr);
 						if (ipstr ==NULL)
 						{
 							cout <<"Error calling inet_ntoa function " <<endl;
 							WSACleanup();
 							return 1;
						}
	
						cout << "successful on calling 2nd inet_ntoa function " <<endl;
						memset(str_dest, 0, sizeof(str_dest));
	 
						strcpy(str_dest, ipstr);
						cout << ipv_num << "peer IP address is " << str_dest <<endl;
						cout << ipv_num << "peer Port is " << port <<endl;	

    					//get hostname of the client
						getnameinfo((struct sockaddr*)&local_addr, sizeof(local_addr), 
									hostclient, sizeof(hostclient), serv_name, sizeof(serv_name), 0 );
						cout << "Peer hostname is " <<hostclient<<endl;
						cout << "Peer service name is " <<serv_name<<endl;

    					//get hostname
    					gethostname(hostname, sizeof(hostname) );
    					cout << "Hostname for server machine is  " << hostname <<endl;
												
					} //end else within if listener
					
				} //end if listner
				else //handle data from a client
				{
					memset(recvbuf, 0, sizeof(recvbuf));  //clear recieve buffer before reading it
					
					iResult = recv(i, recvbuf, recvbuflen, 0);
					if(iResult >0)  //if recieve something echo back
					{
		  				cout<< iResult << "Bytes Received! " <<'\n';
		  				cout <<"Content Received on server is: " << recvbuf <<'\n';
		  				cout <<"Content Received on server from socket: " << i <<'\n';
		  				cout <<"*************************************************"<<endl;
		  			
		  			   	memset(value_str, 0, sizeof(value_str) );
						value_ptr = &value_str[0];
						space_ptr = &space_str[0];
		  			   	
							 // create thread
						std::thread t1(getFileContent, &value_str[0], &space_str[0] ); //spwan a new thread-waising task
						std::thread::id threadID = t1.get_id();
						cout << "read file thread ID is " << threadID <<endl;
						
						t1.join();
						
					/**********************************************************************/
					    
					    cout << "fdmax: " <<fdmax<<endl;
					    //send something back
					    for(int j = 0; j <= fdmax; j++) 
						{
							if (FD_ISSET(j, &master_fds) )
							{
							//	if (j != ListenSocket && j != i) 
							cout <<"j is set, j= " <<j<<endl;
							
							if ( (j != ListenSocket) && (j == i) )
								{
									cout << "send data to socket " << j <<endl;
									if(recvbuf[0] =='q')
									{
										cout <<"You are Exiting the program" <<endl;
										iSendResult = send(j, recvbuf, (int)strlen(recvbuf), 0);
										break;
					 				    
									}
									else
									{
										cout <<"send data=" << value_str <<endl;
										iSendResult = send(j, value_str, (int)strlen(value_str), 0);
										
									}

									if (iSendResult == SOCKET_ERROR)
		  							{	  	
		  								cout <<"sending failed! Error code: "<< WSAGetLastError() << endl;
    									closesocket(j);
										WSACleanup();
										return 1;
		  							}
		  
		  							cout<< iSendResult << "Bytes Sent! " <<'\n';
								}//end if j not listener or itself
					
							}//end if socket set in the list

						} //end for j
					} //end if recieve somehting
					else if (iResult == 0)
					{
						cout <<"client-server connection closed! - pause loop" <<endl;
						closesocket(i);
						FD_CLR(i,&master_fds); // remove from master set
					//	WSACleanup();
					    system("pause");
						return 1;			
			
					}
					else{
						cout <<"Receive Failed! Error code: "<< WSAGetLastError() << endl;	
						closesocket(i);
						FD_CLR(i,&master_fds); // remove from master set
					//	WSACleanup();
						return 1;			
					}
				
				} //end handle data from a client
			
			} //end if set i
		
		} //end of for i<=fdmax
		
		a++;
	}  //end of forever loop

//	closesocket(ClientSocket);
	WSACleanup();	
	
	system("pause");
	return 0;
	
}

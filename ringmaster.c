#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/times.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include "potato.h"

int main(int argc, char *argv[])
{
  potato mypotato;

  if(argc < 4){
    printf("Few args than expected\n");
    return 1;
  }
  else if(argc > 4){
    printf("More args than expected\n");
    return 1;
  }
  
  mypotato.player_num = atoi(argv[2]);
  mypotato.hop_num = atoi(argv[3]);
  mypotato.game_ended = 0;
  //mypotato.trace = {0};
  
  if(mypotato.player_num <= 1){
    printf("Player num must be greater than 1\n");
    return 1;
  }
  
  if(mypotato.hop_num < 0 || mypotato.hop_num > 512){
    printf("Invalid hop_num\n");
    return 1;
  }
  
  int status;
  int socket_fd;
  
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = NULL;
  const char *port     = argv[1];

  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  
  if (status != 0) {
    printf("Error: cannot get address info for host\n");
    return -1;
  } 

  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);

  if (socket_fd == -1) {
    printf("Error: cannot create socket\n");
    return -1;
  } 
  //************** common part ends **************//  

  
  printf("Potato Ringmaster\n");
  printf("Player = %d\n", mypotato.player_num);
  printf("Hops = %d\n", mypotato.hop_num);
 
  //******* bind and listen server socket **********//
  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    printf("Error: cannot bind socket\n");
    return -1;
  } 

  status = listen(socket_fd, 100);
  if (status == -1) {
    printf("Error: cannot listen on socket\n");
    return -1;
  }

  //************** init for message recv *****************//
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  int client_connection_fd[mypotato.player_num];
  char player_hostname[mypotato.player_num][512];

  
  for(int i = 0; i < mypotato.player_num; i++){
    //********* accept socket connection ***********//
    //    printf("Waiting for connection of player[%d]...\n", i);

    client_connection_fd[i] = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_connection_fd[i] == -1) {
      printf("Error: cannot accept connection on client_connection_fd[%d]\n", i);
      return -1;
    } 

    //*********** send and receive message with player *********//
    int player_n = client_connection_fd[i] - 4;
    send(client_connection_fd[i], &player_n, sizeof(int), 0);
    send(client_connection_fd[i], &mypotato.player_num, sizeof(int), 0);
    send(client_connection_fd[i], &mypotato.hop_num, sizeof(int), 0);
    if(i != 0){
      send(client_connection_fd[i], &player_hostname[i-1], sizeof(player_hostname[i-1]), 0);
      //printf("Send player %d hostname: %s TO player %d\n\n", i-1, player_hostname[i-1], i);
    }


    //    printf("client_connection_fd[%d]: %d\n", i, client_connection_fd[i]);
    char buffer[512];
    int tmp;
    if(i < 10) tmp = 25;
    else if(i >= 10 && i <= 99) tmp = 26;
    else if(i >= 100 && i <= 999) tmp = 27;
    int ready_len = recv(client_connection_fd[i], buffer, tmp, 0);
    buffer[tmp] = 0;
    //    printf("ready_len = %d\n", ready_len);
    //    printf("Message from player[%d]: %s\n", i, buffer);
    
    
    //    int hostlen = strlen(player_hostname[i]);
    recv(client_connection_fd[i], &player_hostname[i], 20, 0);
    player_hostname[i][20] = 0;
    //    printf("Receive player %d hostname: %s\n", i, player_hostname[i]);
    //    printf("hostname%d: %ld\n",i , strlen(player_hostname[i]) );
  }


  //***** send last connection to player 0 *******//
  //printf("waiting for last connection signal\n");
  int last_connection = 0;
  recv(client_connection_fd[mypotato.player_num - 1], &last_connection, 1, 0);
  if(last_connection == 0){
     printf("last player connection not established\n");
    return -1;
  }
  else if(last_connection == 1){
    //  printf("last player connection set\n\n");
	//    send(client_connection_fd[0], &last_connection, 1, 0);
    send(client_connection_fd[0], &player_hostname[mypotato.player_num-1], 20, 0);
    //      printf("Send player %d hostname: %s TO player 0\n\n", mypotato.player_num-1, player_hostname[mypotato.player_num-1]);
  }
  
  if(mypotato.hop_num == 0){
    //printf("Hop number is zero, ganme is over\n");
    freeaddrinfo(host_info_list);
    close(socket_fd);
    return 0;
  }

  //************ start the game and send potato *********//
  int start_p = 0;
  srand((unsigned int)time(NULL) + start_p);
  start_p = rand() % mypotato.player_num;
  mypotato.start_player = start_p;
  //printf("Ready to start the game, sending potato to player %d\n", start_p);
  mypotato.hop_num -= 1;
  if(mypotato.hop_num == 0)
    mypotato.start_player = 0;
  
  for(int i = 0; i < mypotato.player_num; ++i){
    send(client_connection_fd[i], &mypotato, sizeof(potato), 0);
  }
  /*  if(mypotato.hop_num == 0){
    printf("Hop number is 1, cannot send to start player and game over\n");
    freeaddrinfo(host_info_list);
    close(socket_fd);
    return 0;
    }*/
  
  //*********** wait for potato back **********//
  fd_set read_fds;
  int fdmax = 0;
  potato backpotato;
  int exitwhile = 0;
  int endgame = 4;
  //  printf("waiting for the potato back...\n");
  while(1){
    FD_ZERO(&read_fds);
    for(int i = 0; i < mypotato.player_num; i++){
      FD_SET(client_connection_fd[i], &read_fds);
      if(client_connection_fd[i] > fdmax)
	fdmax = client_connection_fd[i];
    }
    if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
      printf("Error ringmaster select!\n");
      return -1;
    }
    
    for(int i = 0; i < mypotato.player_num; i++){
      if(FD_ISSET(client_connection_fd[i], &read_fds)){
	int finalback = recv(client_connection_fd[i], &backpotato, sizeof(potato), 0);
	if(backpotato.hop_num != 0 || backpotato.next_player != i
	   || backpotato.player_num != mypotato.player_num){
	  continue;
	}
	//	printf("receive potato from player %d\n", i);
	//	printf("backpotato.hop_num = %d\n", backpotato.hop_num);
	//	printf("backpotato.player_num = %d\n", backpotato.player_num);
	//	printf("backpotato.next_player = %d\n\n", backpotato.next_player);
	printf("Trace of potato:\n");
	for(int i = 0; i < mypotato.hop_num; i++){
	  printf("%d,", backpotato.trace[i]);	  
	}
	printf("%d", backpotato.trace[mypotato.hop_num]);
	printf("\n");
	
	if(finalback == -1){
	  printf("final recv potato error\n");
	  return -1;
	}
	else if(finalback == 0){
	  printf("final recv potato shutdown\n");
	  return -1;
	}
	else if(backpotato.hop_num == 0 && backpotato.next_player == i
		&& backpotato.player_num == mypotato.player_num){
	  //printf("Final potato back with %d hop\n", backpotato.hop_num);
	  
	  for(int j = 0; j < mypotato.player_num; j++){  
	    if(send(client_connection_fd[j], &endgame, sizeof(int), 0) == -1){
	      printf("fail to send endgame to player %d\n", j);
	      return -1;
	    }
	    else{
	      //	      printf("send endgame to player %d successfully\n", j);
	    }
	  }
	  exitwhile = 1;
	  break;
	}
      }
    }
    if(exitwhile)
      break;
  }
    
  //  printf("Game ended successfully!\n");
  
  
  //************** freeaddrinfo and close ***********//
  freeaddrinfo(host_info_list);
  close(socket_fd);
  
  return 0;
}

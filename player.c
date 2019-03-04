#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include "potato.h"


int send_potato(int player_n, potato player_potato, int socket_fd_p, int nextplayer_fd){
  player_potato.hop_num -= 1;
  int next = rand();
  srand((unsigned)time(NULL) + next);
  next = rand() % 2;
  //printf("next = %d\n", next);
  if(next == 0){  //send to prev player
    if(player_n == 0){
      player_potato.next_player = player_n - 1 + player_potato.player_num;
      //  printf("player_potato.next_player = %d\n" ,player_potato.next_player);
    }
    else{
      player_potato.next_player = player_n - 1; 
      //  printf("player_potato.next_player = %d\n" ,player_potato.next_player);
    }
    if(send(socket_fd_p, &player_potato, sizeof(potato), 0) == -1){
      printf("send potato to prev player ERROR!\n ");
      return -1;
    }
    else{
      printf("Sending potato to player %d\n", player_potato.next_player);
      player_potato.hop_num = 1000;
      return 0;
    }
  }
  else if(next == 1){  //send to next player
    if(player_n == player_potato.player_num - 1){
      player_potato.next_player = 0;
      //printf("player_potato.next_player = %d\n" ,player_potato.next_player);
    }
    else{
      player_potato.next_player = player_n + 1;
      //printf("player_potato.next_player = %d\n" ,player_potato.next_player);
    }
    if(send(nextplayer_fd, &player_potato, sizeof(potato), 0) == -1){
      printf("send potato to next player ERROR\n");
      return -1;
    }
    else{
      player_potato.hop_num = 1000;
      printf("Sending potato to player %d\n", player_potato.next_player);
      return 0;
    }
  } 
}


int main(int argc, char *argv[])
{
  if (argc < 3) {
    printf("Invalid input!\n");
    return 1;
  }

  //*********** start to connect with ringmaster ********//
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = argv[1];
  const char *port     = argv[2];
  
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

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
  
  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    printf("Error: cannot connect to socket\n");
    return -1;
  }
  //********** connection with ringmaster completed ********//

  
  //*********** from ringmaster: receive player_no **********//
  int player_n = 0;
  int player_total = 0;
  int total_hop = 0;
  char prev_hostname[512];
  
  recv(socket_fd, &player_n, sizeof(int), 0);
  recv(socket_fd, &player_total, sizeof(int), 0);
  recv(socket_fd, &total_hop, sizeof(int), 0);
  printf("Connected as player %d out of %d players\n", player_n, player_total);
  //  printf("Total hop number is %d\n", total_hop);
  if(player_n != 0){
    int  recv_len = recv(socket_fd, &prev_hostname, sizeof(prev_hostname), 0);
    prev_hostname[20] = 0;
    //printf("Receive prev player hostname: %s\n", prev_hostname);
    //printf("recv_len = %d\n", recv_len);
  }

  
  //************ to ringmaster: send ready message *********//
  char ready_message[30];
  sprintf(ready_message, "Player %d is ready to play", player_n);
  send(socket_fd, ready_message, strlen(ready_message), 0);
  //printf("Message sent: %s\n", ready_message);

  char my_hostname[512];
  if(gethostname(my_hostname, 512) == -1){
    printf("Cannot get the hostname of player %d\n", player_n);
      return -1;
  }
  //printf("player %d hostname: %s\n", player_n, my_hostname);
  //send my hostname to ringmaster
  int my_hostname_len = strlen(my_hostname);
  my_hostname[my_hostname_len] = 0;
  if(send(socket_fd, my_hostname, my_hostname_len, 0) == -1){
    printf("send my hostname to ringmaster FAIL\n");
    return -1;
  }
  else{
    //printf("send my hostname to ringmaster: %s\n\n", my_hostname);
  }
  
  

  //******** init connection with next player ******//
  int status_n;
  int socket_fd_n;

  struct addrinfo host_info_n;
  struct addrinfo *host_info_list_n;
  const char *hostname_n = NULL;
  int port_next = 12345 + player_n;
  char port_n[5];
  sprintf(port_n, "%d", port_next);

  memset(&host_info_n, 0, sizeof(host_info_n));
  host_info_n.ai_family   = AF_UNSPEC;
  host_info_n.ai_socktype = SOCK_STREAM;
  host_info_n.ai_flags    = AI_PASSIVE;

  status_n = getaddrinfo(hostname_n, port_n, &host_info_n, &host_info_list_n);

  if (status_n != 0) {
    printf("Error: cannot get address info for next player\n");
    return -1;
  }

  socket_fd_n = socket(host_info_list_n->ai_family,
                     host_info_list_n->ai_socktype,
                     host_info_list_n->ai_protocol);

  if (socket_fd_n == -1) {
    printf("Error: cannot create next player socket\n");
    return -1;
  }
  
  int yes = 1;
  status_n = setsockopt(socket_fd_n, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status_n = bind(socket_fd_n, host_info_list_n->ai_addr, host_info_list_n->ai_addrlen);
  if (status_n == -1) {
    printf("Error: cannot bind next player socket\n");
    return -1;
  }

  status_n = listen(socket_fd_n, 100);
  if (status_n == -1) {
    printf("Error: cannot listen on next player socket\n");
    return -1;
  }


  //********* init connection with prev player *******//
  int status_p;
  int socket_fd_p;
  struct addrinfo host_info_p;
  struct addrinfo *host_info_list_p;
  int port_prev;
  const char * prev_player_hostname = prev_hostname;
  
  if(player_n == 0)
    port_prev = 12345 + player_n - 1 + player_total; 
  else
    port_prev = 12345 + player_n - 1;
  char port_p[5];
  sprintf(port_p, "%d", port_prev);
  
  memset(&host_info_p, 0, sizeof(host_info_p));
  host_info_p.ai_family   = AF_UNSPEC;
  host_info_p.ai_socktype = SOCK_STREAM;
  

  struct sockaddr_storage socket_addr_n;
  socklen_t socket_addr_len_n = sizeof(socket_addr_n);
  int nextplayer_fd;
  
  if(player_n == 0){
    //****** player0, first wait for next, then connect with prev ******//
    //******** waiting connection for next player *******//
    //printf("Waiting for connection of next player [%d]...\n", player_n + 1);
  
    nextplayer_fd = accept(socket_fd_n, (struct sockaddr *)&socket_addr_n, &socket_addr_len_n);
    if (nextplayer_fd == -1) {
      printf("Error: cannot accept connection on next player\n");
      return -1;
    }
    else if(nextplayer_fd){
      //printf("Successfully connected with next player\n");
    }
    
    //********* from next player: recv ready message *********//
    char next_ready[40];
    int isrecv_n = recv(nextplayer_fd, next_ready, 35, 0);
    next_ready[35] = 0;
    if(isrecv_n == 0 || isrecv_n == -1){
      printf("I dont receive any from next player\n");
    }
    else if(isrecv_n){
      //printf("Message received from next player: %s\n", next_ready);
    }

    //****** waiting for all_ready from ringmaster *******//
    //printf("waiting last_connection from ringmaster\n");
    int last_connection = 0;
    recv(socket_fd, &last_connection, 1, 0);
    //printf("receive last_connection: %d\n", last_connection);

    int recv_len = recv(socket_fd, &prev_hostname, sizeof(prev_hostname), 0);
    prev_hostname[20] = 0;
    //printf("Receive prev player hostname: %s\n", prev_hostname);
    //printf("recv_len = %d\n", recv_len);
    
    //******* connecte with prev player ********//
    status_p = getaddrinfo(prev_hostname, port_p, &host_info_p, &host_info_list_p);
    if (status_p != 0) {
      printf("Error: cannot get address info for prev player\n");
      return -1;
    }
    
    socket_fd_p = socket(host_info_list_p->ai_family, 
			 host_info_list_p->ai_socktype, 
			 host_info_list_p->ai_protocol);
    
    if (socket_fd_p == -1) {
      printf("Error: cannot create socket\n");
      return -1;
    } 
    
    status_p = connect(socket_fd_p, host_info_list_p->ai_addr, host_info_list_p->ai_addrlen);
    if (status_p == -1) {
      printf("Error: cannot connect to prev player socket\n");
      return -1;
    }
    else if(status_p == 0){
      //printf("Successfully connected with prev player\n");
    }

    //********* to prev player: send ready message ********//
    char ready_to_prev[40];
    if(player_n == 0){
      sprintf(ready_to_prev, "Player %d is connected with player %d", player_n, player_n - 1 + player_total);
    }
    else{
      sprintf(ready_to_prev, "Player %d is connected with player %d", player_n, player_n - 1);
    }
    send(socket_fd_p, ready_to_prev, strlen(ready_to_prev), 0);
    //printf("Send ready to prev player: %s\n", ready_to_prev);
  }

  else{   //other players, first connect with prev, then wait for next
    //******* connecte with prev player ********//    
    status_p = getaddrinfo(prev_hostname, port_p, &host_info_p, &host_info_list_p);
    if (status_p != 0) {
      printf("Error: cannot get address info for prev player\n");
      return -1;
    }
    socket_fd_p = socket(host_info_list_p->ai_family, 
			 host_info_list_p->ai_socktype, 
			 host_info_list_p->ai_protocol);
    
    if (socket_fd_p == -1) {
      printf("Error: cannot create socket\n");
      return -1;
    } 
    
    status_p = connect(socket_fd_p, host_info_list_p->ai_addr, host_info_list_p->ai_addrlen);
    if (status_p == -1) {
      printf("Error: cannot connect to prev player socket\n");
      return -1;
    }
    else if(status_p == 0){
      //printf("Successfully connected with prev player\n");
    }
    
    //********* to prev player: send ready message ********//
    char ready_to_prev[40];
    if(player_n == 0){
      sprintf(ready_to_prev, "Player %d is connected with player %d", player_n, player_n - 1 + player_total);
    }
    else{
      sprintf(ready_to_prev, "Player %d is connected with player %d", player_n, player_n - 1);
    }
    send(socket_fd_p, ready_to_prev, strlen(ready_to_prev), 0);
    //printf("Send ready to prev player: %s\n", ready_to_prev);
    
    //******** waiting connection for next player *******//    
    //printf("Waiting for connection of next player [%d]...\n", player_n + 1);
    
    int connection_next = 1;
    send(socket_fd, &connection_next, 1, 0);
    
    nextplayer_fd = accept(socket_fd_n, (struct sockaddr *)&socket_addr_n, &socket_addr_len_n);

        
    if (nextplayer_fd == -1) {
      printf("Error: cannot accept connection on next player\n");
      return -1;
    }
    else if(nextplayer_fd){
      //printf("Successfully connected with next player\n"); 
    }
  
    //********* from next player: recv ready message *********//
    char next_ready[40];
    int isrecv_n = recv(nextplayer_fd, next_ready, 35, 0);
    next_ready[35] = 0;
    if(isrecv_n == 0 || isrecv_n == -1){
      printf("I dont receive any from next player\n");
    }
    else if(isrecv_n){
      //printf("Message received from next player: %s\n", next_ready);
    }
    
  }

  
  //************ from ringmaster: wait to receive potato *******//
  //printf("\n waiting the ringmaster starts the game\n");
  potato player_potato;
  int isrecv_p = recv(socket_fd, &player_potato, sizeof(potato), 0);
  //  printf("player_potato.start_player = %d\n", player_potato.start_player);
  player_potato.player_num = player_total;
  if(player_potato.start_player == player_n){
    //printf("I am the starter and got potato with %d hops\n", player_potato.hop_num);
    player_potato.trace[0] = player_n;
    //    printf("player_potato.trace[0] = %d\n", player_potato.trace[0]);
    if(send_potato(player_n, player_potato, socket_fd_p, nextplayer_fd) == -1){
      printf("send_potato ERROR\n");
      return -1;
    }
  }
  else{
    //printf("I am not the starter player\n");
  }



  while(1){
    //    printf("wait to receive the potato\n");
    fd_set read_fds;
    int fdmax = 0;
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);
    FD_SET(socket_fd_p, &read_fds);
    FD_SET(nextplayer_fd, &read_fds);
    if(socket_fd > fdmax)
      fdmax = socket_fd;
    if(socket_fd_p > fdmax)
      fdmax = socket_fd_p;
    if(nextplayer_fd > fdmax)
      fdmax = nextplayer_fd;
    if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
      printf("Error select!\n");
      return -1;
    }

    
    if(FD_ISSET(socket_fd, &read_fds)){ // endgame massage from ringmaster
      int endgame = 0;
      recv(socket_fd, &endgame, sizeof(int), 0);
      if(endgame == 4){
	//	printf("Game Ended from ringmaster!\n");
	freeaddrinfo(host_info_list);
	close(socket_fd);
	break;
      }
    }
    else if(FD_ISSET(socket_fd_p, &read_fds)){  //potato from prev player
      player_potato.hop_num = 1000;
      int recvpotato = recv(socket_fd_p, &player_potato, sizeof(potato), 0);  
      if(recvpotato == 0){
	//	printf("recv potato peer shutdown\n");
	break;
      }
      else if(recvpotato == -1){
	//	printf("recv ERROR\n");
	break;
      }
      if(player_potato.hop_num == 1000){
	continue;
      }
      if(player_potato.next_player == player_n){
	//	printf("I got the potato with %d hops left\n", player_potato.hop_num);
	if(player_potato.hop_num > 0){
	  //判断前一个的trace是否正确
	  if(player_n != 0 && player_potato.trace[total_hop - player_potato.hop_num - 2] != player_n - 1){
	    player_potato.trace[total_hop - player_potato.hop_num - 2] = player_n - 1;
	  }
	  else if(player_n == 0 && player_potato.trace[total_hop - player_potato.hop_num - 2] != player_total - 1){
	    player_potato.trace[total_hop - player_potato.hop_num - 2] = player_total - 1;
	  }
	  // 随机送给下一个player， 修改next_player
	  player_potato.trace[total_hop - player_potato.hop_num - 1] = player_n;
	  if(send_potato(player_n, player_potato, socket_fd_p, nextplayer_fd) == -1){
	    printf("ERROR send_potato\n");
	    return -1;
	  }
	  player_potato.hop_num = 1000;
	}
	else if(player_potato.hop_num == 0){
	  printf("I am it!!!\n");
	  player_potato.trace[total_hop - player_potato.hop_num - 1] = player_n;
	  // 把土豆儿送回给ringmaster
	  send(socket_fd, &player_potato, sizeof(potato), 0);
	  player_potato.hop_num = 1000;
	  send(socket_fd_p,&player_potato, sizeof(potato),0);	 
	  continue;
	}
      }
    }
    else if(FD_ISSET(nextplayer_fd, &read_fds)){ //potato from next player
      player_potato.hop_num = 1000;
      int recvpotato = recv(nextplayer_fd, &player_potato, sizeof(potato), 0);
      if(recvpotato == 0){
	//        printf("recv potato peer shutdown\n");
	break;
      }
      else if(recvpotato == -1){
        printf("recv ERROR\n");
	break;
      }
      
      if(player_potato.hop_num == 1000){
	continue;
      }
      if(player_potato.next_player == player_n){
	//	printf("I got the potato with %d hops left\n", player_potato.hop_num);
	if(player_potato.hop_num > 0){
	  //判断前一个trace是否正确
	  if(player_n != player_total -1 && player_potato.trace[total_hop - player_potato.hop_num - 2] != player_n + 1){
	    player_potato.trace[total_hop - player_potato.hop_num - 2] = player_n + 1;
	  }
	  else if(player_n == player_total -1 && player_potato.trace[total_hop - player_potato.hop_num - 2] != 0){
	      player_potato.trace[total_hop - player_potato.hop_num - 2] = 0;
	  }	  
	  // 随机送给下一个player，修改next_player
	  player_potato.trace[total_hop - player_potato.hop_num - 1] = player_n;
	  if(send_potato(player_n, player_potato, socket_fd_p, nextplayer_fd) == -1){
	    printf("ERROR send_potato\n");
	    return -1;
	  }
	  player_potato.hop_num = 1000;
	}
	else if(player_potato.hop_num == 0){
	  printf("I am it!!!\n");
	  player_potato.trace[total_hop - player_potato.hop_num - 1] = player_n;
	  /// potato trace
	  /*printf("Potato trace: ");
	  for(int i = 0; i < total_hop; i++){
	    printf("%d ", player_potato.trace[i]);
	  }
	  printf("\n");
	  */
	  // 把土豆儿送回给ringmaster
	  send(socket_fd, &player_potato, sizeof(potato), 0);
	  player_potato.hop_num = 1000;
	  send(socket_fd_p,&player_potato, sizeof(potato),0);
	  continue;
	  }
      }
    }    
  }

  freeaddrinfo(host_info_list);
  close(socket_fd);

  return 0;
}



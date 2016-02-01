/*
Program: Implementation of Distance Vector routing protocol for a fixed topology
INPUT: Configuration file in .txt format
OUTPUT: 1.Prints Initial Routing Table
		2.Sends Distance vectors to neighbors
		3.Keeps receiving distance vectors from neighbors
		4.Periodically(after each timeout) sends distance vector to neighbors
		5.Prints Received distance vector
		6.Prints series of Updated Routing Tables
ASSUMPTION: 1. Topology is fixed with nodes A,B,C,D,E,F,G
			2. For unreachable node: distance is MAX_DIST and next_hop is UNKNOWN_HOP
*/

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() and alarm() */
#include <errno.h>      /* for errno and EINTR */
#include <signal.h>     /* for sigaction() */

#define TIMEOUT_SECS    5       /* Seconds between retransmits */
#define MAX_NEIGHBORS   6       /* Assuming topology is fixed*/
#define MAX_DIST        1000    /* when no route exist; dist is considered 1000*/
#define UNKNOWN_HOP		'X'		/*when node is unreachable; next hop is not known*/
#define ECHOMAX         255     /* Longest string to echo */
#define MAXTRIES        2       /* Tries before giving up sending */

int tries=0;
char nodes[7]={'A','B','C','D','E','F','G'};

struct Neighbors{
        char dest;
        int dist;
        char ip[16];
        int port_no;
};
struct Element_Dist_Vector{
        char dest;
        int dist;
};
struct Distance_Vector{
        char sender;
        int no_of_neighbors;
        struct Element_Dist_Vector element_Dist_Vector[MAX_NEIGHBORS];
};
struct Routing_Table{
        char dest;
        int dist;
        char next_hop;
};
struct Info_From_Config{
        char node_name;
        int no_of_neighbors;
        int port_no;
        struct Neighbors neighbors[MAX_NEIGHBORS];
        struct Routing_Table routing_table[MAX_NEIGHBORS];
};
typedef struct Info_From_Config Info_Config;
typedef struct Distance_Vector Dist_Vect;

void Populate_From_Config(FILE *fp,Info_Config *info_config);	/*populate info_config structure from config file*/
int Update_Routing_Table(Info_Config *info_config,Dist_Vect *dist_vect);	/*Updates Routing table after receiving distance vector from neighbors*/
void SendDistVect(Dist_Vect send_dist_vect,Info_Config *info_config);	/*sends the distance vector to all neighbors*/
void Build_Distance_Vector(Dist_Vect *dist_vect, Info_Config *info_config);	/*construct a distance vector*/
void Print_Routing_Table(Info_Config *info_config);		/*Prints Routing table*/
void Print_Neighbor_Table(Info_Config *info_Config);	/*Prints Neighbor table*/
void Print_Distance_Vector(Dist_Vect *dist_vector);	/*Prints Distance Vector*/
void DieWithError(char *errorMessage);   /* Error handling function */
void CatchAlarm(int ignored);            /* Handler for SIGALRM */

int main(int argc, char *argv[])
{
        FILE *fp;
        Dist_Vect send_dist_vect;
        Info_Config info_config;
        int i=0,dist=0,j=0;

        int sock_send,sock_rx;                        /* Socket descriptor */
        struct sockaddr_in echoServAddr; /* Local address */
        struct sockaddr_in echoClntAddr; /* Client address */
        unsigned int cliAddrLen;         /* Length of incoming message */
        char echoBuffer[sizeof(ECHOMAX)+1];        /* Buffer for echo string */
        unsigned short echoServPort;     /* Server port for listening to clients */
        int recvMsgSize,Update_Flag;                 /* Size of received message */
        struct sigaction myAction;       /* For setting signal handler */
        int rxSize,flag=0;
		Dist_Vect *rx_dist_vect=malloc(sizeof(struct Distance_Vector));
		
        if ((argc < 2) || (argc > 3))    /* Test for correct number of arguments */
        {
                fprintf(stderr,"Usage: %s <Config_File>\n", argv[1]);
                exit(1);
        }
        fp=fopen(argv[1],"r");
        if(fp==NULL){
                fprintf(stderr, "Can't open configuration file!\n");
                exit(1);
        }
        /*Read from config file and populate a neighbor table*/
        Populate_From_Config(fp,&info_config);
		/*Printing initial Routing Table*/
        printf("/******************************************************Initial Routing Table:***************************************************************/");
        Print_Routing_Table(&info_config);

        /*Build initial distance vector*/
        Build_Distance_Vector(&send_dist_vect,&info_config);
  
		printf("\n/**********************************************INITIAL SEND******************************************************************************/");
        /*send initial dist_vect to all neighbors*/
		SendDistVect(send_dist_vect,&info_config);
	
		/*Receive distance vectors*/
        echoServPort = info_config.port_no;
		if ((sock_rx = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
			DieWithError("socket() failed");
		
		/* Construct local address structure */
		memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
		echoServAddr.sin_family = AF_INET;                /* Internet address family */
		echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
		echoServAddr.sin_port = htons(echoServPort);      /* Local port */
		
		/* Bind to the local address */
		if (bind(sock_rx, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
			DieWithError("bind() failed");
		
		/* Set signal handler for alarm signal */
		myAction.sa_handler = CatchAlarm;
		if (sigfillset(&myAction.sa_mask) < 0) /* block everything in handler */
			DieWithError("sigfillset() failed");
		myAction.sa_flags = 0;

		/* Set the size of the in-out parameter */
		cliAddrLen = sizeof(echoClntAddr);
		/*
		Continue Accepting distance vectors; if received distance vector->update routing table->send routing table to neighbors
		If no receive within timeout; send distance vector to neighbors
		*/
		for(;;){
			flag=0;
			alarm(TIMEOUT_SECS);        /* Set the timeout */
			/* Block until receive message from a client */
			while ((recvMsgSize = recvfrom(sock_rx, rx_dist_vect, sizeof(*rx_dist_vect) , 0,(struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0){
					if (errno == EINTR){     /* Alarm went off then send the distance vector to neighbors */
							printf("\n/**********************************************PERIODIC SEND TO NEIGHBORS******************************************************************************/");
							SendDistVect(send_dist_vect,&info_config);
							alarm(TIMEOUT_SECS);
					}
			}
			/* recvfrom() got something --  cancel the timeout */
			if(recvMsgSize<0){
					printf("didn't receive");
					continue;
			}
			alarm(0);
			printf("\n/**********************************************RECEIVED DISTANCE VECTOR******************************************************************************/");
			printf("\nReceived distance vector from: %c\n", rx_dist_vect->sender);    /* Print the received data */
			Print_Distance_Vector(rx_dist_vect);
			flag=Update_Routing_Table(&info_config,rx_dist_vect);
			if(flag==1){
				printf("\n/**********************************************UPDATED ROUTING TABLE******************************************************************************/");
				Build_Distance_Vector(&send_dist_vect,&info_config);
				Print_Routing_Table(&info_config);
				SendDistVect(send_dist_vect,&info_config);
			}
				
		}
		/* NOT REACHED */
}
void SendDistVect(Dist_Vect send_dist_vect,Info_Config *info_config){
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    struct sockaddr_in fromAddr;     /* Source address of echo */
    unsigned short echoServPort;     /* Echo server port */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    struct sigaction myAction;       /* For setting signal handler */
    char *servIP;                    /* IP address of server */
    char *echoString;                /* String to send to echo server */
    char echoBuffer[ECHOMAX+1];      /* Buffer for echo string */
    int echoStringLen;               /* Length of string to echo */
    int respStringLen;               /* Size of received datagram */
    int j=0;
    echoStringLen = sizeof(send_dist_vect);

    /* Create a best-effort datagram socket using UDP */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Set signal handler for alarm signal */
    myAction.sa_handler = CatchAlarm;
    if (sigfillset(&myAction.sa_mask) < 0) /* block everything in handler */
        DieWithError("sigfillset() failed");
    myAction.sa_flags = 0;

    if (sigaction(SIGALRM, &myAction, 0) < 0)
        DieWithError("sigaction() failed for SIGALRM");

 for(j=0;j<info_config->no_of_neighbors;j++){
                char sender, dest,next_hop;
                int no_of_neighbors, dist;
				printf("\nSending distance vector to neighbor %c",info_config->neighbors[j].dest);
                servIP = info_config->neighbors[j].ip;
                echoServPort=info_config->neighbors[j].port_no;
                /* Construct the server address structure */
                memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
                echoServAddr.sin_family = AF_INET;
                echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
                echoServAddr.sin_port = htons(echoServPort);       /* Server port */

                fromSize = sizeof(fromAddr);
                /* Send the string to the server */
                if (sendto(sock, (void *)&send_dist_vect, (1024+sizeof(send_dist_vect)), 0, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) == -1)
                        DieWithError("sendto() sent a different number of bytes than expected");

                /* Get a response */
                alarm(TIMEOUT_SECS);        /* Set the timeout */
                while ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &fromAddr, &fromSize)) < 0){
					if (errno == EINTR)     /* Alarm went off  */
					{
							if (tries < MAXTRIES)      /* incremented by signal handler */
							{
								printf("while sending, timed out, %d more tries...\n",MAXTRIES-tries);

								if (sendto(sock, (void *)&send_dist_vect, (1024+sizeof(send_dist_vect)), 0, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) == -1)
										DieWithError("sendto() failed");
								alarm(TIMEOUT_SECS);
							}
							break;
					}
					printf("waiting to receive");
				}
				/* recvfrom() got something --  cancel the timeout */
				alarm(0);
				/*if send is not successful even after timeout; then go for sending to next neighbor*/
				if(respStringLen<0)
						continue;
        }
		close(sock);
        return;
}
int Update_Routing_Table(Info_Config *info_config,Dist_Vect *dist_vect){
		int j=0,dist_to_nei=0,i=0,Update_Flag=0;
        for(j=0;j<info_config->no_of_neighbors;j++){                    //check for link cost to this neighbor
                if(info_config->neighbors[j].dest==dist_vect->sender){
                        dist_to_nei=info_config->neighbors[j].dist;
                }
        }
        for(i=0;i<MAX_NEIGHBORS;i++){                                           //check for each node if there is a new value
        for(j=0;j<MAX_NEIGHBORS;j++){
               if(info_config->routing_table[i].dest==dist_vect->element_Dist_Vector[j].dest){
						/*Update if next_hop is same as sender but dist is changed to dest 
							OR Currently node is unreachable but reachable from sender
							OR	Current distance dest node is greater than (dist to dest from sender+dist to sender)
						*/
                        if((info_config->routing_table[i].dist==MAX_DIST && dist_vect->element_Dist_Vector[j].dist!=MAX_DIST) 
							|| (info_config->routing_table[i].dist>(dist_vect->element_Dist_Vector[j].dist+dist_to_nei))
//						|| (info_config->routing_table[i].next_hop==dist_vect->sender && info_config->routing_table[i].dist!=dist_vect->element_Dist_Vector[j].dist)
							){
                                info_config->routing_table[i].dist=dist_vect->element_Dist_Vector[j].dist + dist_to_nei;
                                info_config->routing_table[i].next_hop=dist_vect->sender;
                                Update_Flag=1;
                       }

                }
        }
        }
return Update_Flag;
}
void Build_Distance_Vector(Dist_Vect *dist_vect,Info_Config *info_config){
		int j=0;
		dist_vect->sender=info_config->node_name;
		dist_vect->no_of_neighbors=info_config->no_of_neighbors;
		for(j=0;j<MAX_NEIGHBORS;j++){
				dist_vect->element_Dist_Vector[j].dest=info_config->routing_table[j].dest;
				dist_vect->element_Dist_Vector[j].dist=info_config->routing_table[j].dist;
		}
}
void Print_Distance_Vector(Dist_Vect *dist_vect){
        int j=0;
        printf("\nSender:%c", dist_vect->sender);
        printf("\nNo_of_neighbors:%d", dist_vect->no_of_neighbors);
        printf("\ndest\tdist");
        for(j=0;j<MAX_NEIGHBORS;j++){
                printf("\n%c\t%d",dist_vect->element_Dist_Vector[j].dest,dist_vect->element_Dist_Vector[j].dist);
        }
}
void Populate_From_Config(FILE *fp,Info_Config *info_config){
        char value1[20],value2[20],value3[20],value4[20];
        int j=0,i=0,no_of_neighbors=0;
		/*Get node_name and port_no from config files first two entries*/
        if(fgets(value1,255,fp)!=NULL && fgets(value2,255,fp)!=NULL){
                info_config->node_name=value1[0];
                info_config->port_no=atoi(value2);
        }else{
                fprintf(stderr, "Error while reading file!\n");
                exit(2);
        }
		/*Get dest, dist, ip, port_no fields for each neighbor*/
        while(fgets(value1,255,fp) && fgets(value2,255,fp) && fgets(value3,255,fp) && fgets(value4,255,fp) !=NULL){
           i=0;
           for(j=0;j<MAX_NEIGHBORS;j++){
                if(nodes[i]==info_config->node_name){   /*if information is about this node itself; don't store it in routing/neighbor table*/
                        i++;
                        j--;
                        continue;
                }else if(nodes[i]==value1[0]){	/*If node is there in config file; populate routing/ neighbor table*/
                        //add to neighbors if valid distance for destination
						if(atoi(value2) > 0 && atoi(value2) < MAX_DIST){
							info_config->neighbors[no_of_neighbors].dest=value1[0];
							info_config->neighbors[no_of_neighbors].dist=atoi(value2);
							strcpy(info_config->neighbors[no_of_neighbors].ip,value3);
							info_config->neighbors[no_of_neighbors].port_no=atoi(value4);
							no_of_neighbors++;
							//add to initial routing table
							info_config->routing_table[j].dest=value1[0];
							info_config->routing_table[j].dist=atoi(value2);
							info_config->routing_table[j].next_hop=nodes[i];
						}else{
							info_config->routing_table[j].next_hop=nodes[i];
							info_config->routing_table[j].dist=MAX_DIST;
                            info_config->routing_table[j].next_hop=UNKNOWN_HOP;
                        }
                        i++;
                }else{							/*If node is not in config file; assign dest with max_dist and 'X'(unknown) next_hop*/
                        info_config->routing_table[j].dest=nodes[i];
                        if(info_config->routing_table[j].dist>MAX_DIST || info_config->routing_table[j].dist==0 || info_config->routing_table[j].dist<0){
                                info_config->routing_table[j].dist=MAX_DIST;
                                info_config->routing_table[j].next_hop=UNKNOWN_HOP;
                        }
                        i++;
                }
        }
        info_config->no_of_neighbors=no_of_neighbors;
        }
        fclose(fp);
}
void Print_Routing_Table(Info_Config *info_config){
        int j=0;
        printf("\ndest\tdist\tnext_hop");
        for(j=0;j<MAX_NEIGHBORS;j++){
                printf("\n%c\t%d\t%c",info_config->routing_table[j].dest,info_config->routing_table[j].dist,info_config->routing_table[j].next_hop);
        }
}
void Print_Neighbor_Table(Info_Config *info_config){
                printf("\nNeighbor Table:");
                int j=0;
                printf("\nNeighbor\tdist\tip\tport_no");
                for(j=0;j<info_config->no_of_neighbors;j++){
                        printf("\n%c\t\t%d\t%s\t%d",info_config->neighbors[j].dest,info_config->neighbors[j].dist,info_config->neighbors[j].ip,info_config->neighbors[j].port_no);
                }
}
void CatchAlarm(int ignored){
    tries += 1;
}
void DieWithError(char *errorMessage){
    perror(errorMessage);
    exit(1);
}

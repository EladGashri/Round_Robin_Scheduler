#include "scheduler.h"

int main(int argc, char *argv[]){
    if (argc!=5){
        printf("must enter 4 command line inputs\n");
        exit(1);
    }
    FILE* input_file = fopen(argv[3], "r");
    if (input_file==NULL){
        printf("file %s does not exist\n", argv[3]);
        exit(1); 
    }
    FILE* output_file = fopen(argv[4], "w");
    connection *connections_by_priority[MAX_NUMBER_OF_CONNECTIONS]={NULL};
    connection_hash_table_item *connections_hash_table[MAX_NUMBER_OF_CONNECTIONS]={NULL};
    char buffer[LINE_LENGTH];
    char buffer_copy[LINE_LENGTH];
    packet *current_packet;
    packet* (*chosen_scheduler)(connection**);
    if (strcmp(argv[1],"RR")==0){
        chosen_scheduler=rr_scheduler;
    }else if (strcmp(argv[1],"WRR")==0){
        chosen_scheduler=wrr_scheduler;
    }else{
        quantum=atoi(argv[2]);
        chosen_scheduler=drr_scheduler;
    }
    unsigned int packet_number=0;
    unsigned long current_packet_time;
    char *fgets_output=fgets(buffer, LINE_LENGTH, input_file);
    while (fgets_output!=NULL){
        strcpy(buffer_copy,buffer);
        time=get_current_package_time(buffer_copy);
        initialize_packet(buffer,connections_by_priority,connections_hash_table);
        //this while loop initializes all packets that share a spesific time
        //once the packet time is diffrente from the rest there is a break statement
        //1 packet is sent according to the scheduler and the time is updated in the write_packet function
        //in the next iteration of the while loop the time is set again by the current packet
        while ((fgets_output=fgets(buffer, LINE_LENGTH, input_file)!=NULL)) {
            strcpy(buffer_copy,buffer);
            if (time==get_current_package_time(buffer_copy)){
                initialize_packet(buffer,connections_by_priority,connections_hash_table);
            }
            else{
                break;
            }
        }
        current_packet=chosen_scheduler(connections_by_priority);
        write_packet(output_file, current_packet);
    }
    //after all the packets until the lastest time have been sent this while loop goes all over the remaining packets and sends them according to the scheduler
    current_packet=chosen_scheduler(connections_by_priority);
    while(current_packet!=NULL){
        write_packet(output_file, current_packet);
        current_packet=chosen_scheduler(connections_by_priority);
    }
    free_connections(connections_by_priority,connections_hash_table);
    return 0;
}


unsigned long get_current_package_time(char* buffer){
    const char delim[2] = " ";
    return (unsigned long) atoi(strtok(buffer, delim));
}


void write_packet(FILE* output_file, packet *current_packet){
    if (current_packet->weight==NULL){
        printf("%lu: %lu %s %hu %s %hu %hu\n\n",time,current_packet->arrival_time, current_packet->identifier->sender_ip, current_packet->identifier->sender_port,
        current_packet->identifier->destiny_ip, current_packet->identifier->destiny_port, current_packet->length);
        fprintf(output_file,"%lu: %lu %s %hu %s %hu %hu\n",time,current_packet->arrival_time, current_packet->identifier->sender_ip, current_packet->identifier->sender_port,
        current_packet->identifier->destiny_ip, current_packet->identifier->destiny_port, current_packet->length);
    }else{
        printf("%lu: %lu %s %hu %s %hu %hu\n\n",time,current_packet->arrival_time, current_packet->identifier->sender_ip, current_packet->identifier->sender_port,
        current_packet->identifier->destiny_ip, current_packet->identifier->destiny_port, current_packet->length);
        fprintf(output_file,"%lu: %lu %s %hu %s %hu %hu %hu\n",time,current_packet->arrival_time, current_packet->identifier->sender_ip, current_packet->identifier->sender_port,
        current_packet->identifier->destiny_ip, current_packet->identifier->destiny_port, current_packet->length, current_packet->weight);
    }
    time+=current_packet->length;
    free(current_packet);
}

//this function implements round robin scheduling
//a while loop that goes over all the connections until a packet is found and then returns it to be written to the output file
//if a packet has been found the global variable current_connection_priority is incremented by 1 so that the next time this function is called the the connection with the next priority will be the first to searched
packet *rr_scheduler(connection **connections_by_priority){
    packet *current_packet=get_packet_from_connection(connections_by_priority[current_connection_priority]);
    unsigned short connections_passed=0;
    while (current_packet==NULL && connections_passed<number_of_connections){
        increment_current_connection_priority();
        current_packet=get_packet_from_connection(connections_by_priority[current_connection_priority]);
        connections_passed++;
    }
    if (current_packet!=NULL){
        increment_current_connection_priority();
    }
    return current_packet;
}


//this function implements weighted round robin scheduling
//a while loop the goes over all the connections until a packet is found
//if the packet has a weight, then the connection current_weight and remainder_to_send_by_weight variables are updated according to it
//whether or not the connection weight has been updated, the remainder is decremented by 1.
//if after that the remainder is 0, meaning this connection has no more packets to send according to its weight the global variable current_connection_priority is incremented by 1 so that the next time this function is called the the connection with the next priority will be the first to searched
packet *wrr_scheduler(connection **connections_by_priority){
    packet *current_packet=get_packet_from_connection(connections_by_priority[current_connection_priority]);
    unsigned short connections_passed=0;
    while (current_packet==NULL && connections_passed<number_of_connections){
        increment_current_connection_priority();
        current_packet=get_packet_from_connection(connections_by_priority[current_connection_priority]);
        connections_passed++;
    }
    if(current_packet!=NULL){
        if(current_packet->weight!=NULL){
            connections_by_priority[current_connection_priority]->current_weight=connections_by_priority[current_connection_priority]->remainder_to_send_by_weight=current_packet->weight;
        }
        connections_by_priority[current_connection_priority]->remainder_to_send_by_weight--;  
        if (connections_by_priority[current_connection_priority]->remainder_to_send_by_weight==0){
            connections_by_priority[current_connection_priority]->remainder_to_send_by_weight=connections_by_priority[current_connection_priority]->current_weight;
            increment_current_connection_priority();
        }
    }
    return current_packet;
}

//this function implements deficit round robin scheduling
//a while loop the goes over all the connections until a packet is found 
//if the packet has been found in a new connection the flag current_connection_not_yet_credited is set to true, so the new connection will receive credit according to the quantum
//if the connection does not have enough credit to send the current packet, the packet is inserted again at the head of the connection packets queue, current_connection_priority is incremented by 1 and the functions runs again
//if the connection has enough credit to send the current packet, the packet's length is reduced from the connection's credit and the packet returns to be written to the output file
packet *drr_scheduler(connection **connections_by_priority){
    packet *current_packet=get_packet_from_connection(connections_by_priority[current_connection_priority]);
    if (current_packet==NULL){
        increment_current_connection_priority();
        current_packet=get_packet_from_connection(connections_by_priority[current_connection_priority]);
        current_connection_not_yet_credited=true;
        unsigned short connections_passed=0;
        while (current_packet==NULL && connections_passed<number_of_connections){
            connections_by_priority[current_connection_priority]->credit=0;
            increment_current_connection_priority();
            current_packet=get_packet_from_connection(connections_by_priority[current_connection_priority]);
            connections_passed++;
        }
    }
    if(current_packet!=NULL){
        if(current_packet->weight!=NULL){
            connections_by_priority[current_connection_priority]->current_weight=current_packet->weight;
        }
        if(current_connection_not_yet_credited){
            connections_by_priority[current_connection_priority]->credit+=(quantum*connections_by_priority[current_connection_priority]->current_weight);  
            current_connection_not_yet_credited=false;
        }
        if(connections_by_priority[current_connection_priority]->credit<current_packet->length){
            insert_packet_in_head(current_packet,current_connection_priority, connections_by_priority);
            current_connection_not_yet_credited=true;
            increment_current_connection_priority();
            return drr_scheduler(connections_by_priority);
        }else{
            connections_by_priority[current_connection_priority]->credit-=current_packet->length;
        }
    }else{
        connections_by_priority[current_connection_priority]->credit=0;
    }
    return current_packet;
}


void increment_current_connection_priority(){
    current_connection_priority++;
    current_connection_priority%=number_of_connections;
}


void decrement_current_connection_priority(){
    current_connection_priority--;
}

//this function gets a packet from the connection's packets queue
packet *get_packet_from_connection(connection *current_connection){
    packet *return_packet=current_connection->packets_head;
    if (return_packet!=NULL){
        current_connection->packets_head=current_connection->packets_head->next;
        if (current_connection->packets_head==NULL){
            current_connection->packets_tail=NULL;
        }
    }
    return return_packet;
}

//this function initializes the data structure packet according to the values in the buffer
//if the packet's connection is not found in the hash table a new connection is created
void initialize_packet(char *buffer, connection **connections_by_priority, connection_hash_table_item **connections_hash_table){
    packet *new_packet=(packet*) malloc(sizeof(packet));
    const char delim[2] = " ";
    new_packet->arrival_time = atoi(strtok(buffer, delim));
    connection_identifier *identifier=(connection_identifier*)malloc(sizeof(connection_identifier));
    strcpy(identifier->sender_ip, strtok(NULL, delim)); 
    identifier->sender_port=atoi(strtok(NULL, delim));
    strcpy(identifier->destiny_ip,strtok(NULL, delim));
    identifier->destiny_port=atoi(strtok(NULL, delim));
    new_packet->length=atoi(strtok(NULL, delim));
    new_packet->next=NULL;
    char* weight=strtok(NULL, delim);
    weight==NULL?(new_packet->weight=NULL):(new_packet->weight=atoi(weight));
    unsigned short hash_index=hash_function(identifier);
    connection_hash_table_item *searched_connection_item=search_connections_hash_table(hash_index, identifier, connections_hash_table);
    if (searched_connection_item==NULL){
        create_a_new_connection(identifier,new_packet,hash_index,connections_by_priority,connections_hash_table);
    }else{
        insert_packet_in_tail(new_packet,searched_connection_item->priority_index,connections_by_priority);
        free(identifier);
    }
}


//this function creates a new connection according to its first packet
//the creation of a new connection includes:
//1. creating a new value in the hash table.the hash table is designed for a O(1) search of a connection when a new packet is initialized and needs to find the connection it belongs to
//   if the index returned by the hash function already has a connection in the hash table a linked list is initiated in that hash table index according to the chaining method
//2. creating a connection data structure that holds the ideitifying values of the connection and the packet queue
void create_a_new_connection(connection_identifier *identifier, packet *new_packet, unsigned short hash_index, connection **connections_by_priority, connection_hash_table_item **connections_hash_table){
    connection_hash_table_item *new_item=(connection_hash_table_item*)malloc(sizeof(connection_hash_table_item));
    new_item->priority_index=number_of_connections++;
    new_item->next=NULL;
    connection_hash_table_item *current_item=connections_hash_table[hash_index];
    if (current_item==NULL){
        *(connections_hash_table+hash_index)=new_item;
    }
    else{
        while(current_item->next!=NULL){
            current_item=current_item->next;
        }
        current_item->next=new_item;
    }
    connection *new_connection=(connection*)malloc(sizeof(connection));
    new_connection->identifier=new_item->identifier=new_packet->identifier=identifier;
    new_connection->hash_index=hash_index;
    new_connection->credit=0;
    new_connection->packets_head=new_connection->packets_tail=NULL;
    *(connections_by_priority+new_item->priority_index)=new_connection;
    initialize_connection(connections_by_priority[new_item->priority_index],new_packet);
}


//initializing a new connection includes setting the first packet at the queue's head and tail and setting the connection's weight according to the packet's weight
void initialize_connection(connection *current_connection, packet *new_packet){
    current_connection->packets_head=current_connection->packets_tail=new_packet;
    (new_packet->weight)==NULL?(current_connection->current_weight=current_connection->remainder_to_send_by_weight=1):(current_connection->current_weight=current_connection->remainder_to_send_by_weight=new_packet->weight);
}


void insert_packet_in_tail(packet *new_packet, unsigned short priority_index, connection **connections_by_priority){
    connection *current_connection=connections_by_priority[priority_index];
    new_packet->identifier=current_connection->identifier;
    if (current_connection->packets_tail==NULL){
        initialize_connection(current_connection,new_packet);
    }else{
        current_connection->packets_tail->next=new_packet;
        current_connection->packets_tail=new_packet;
    }
}


void insert_packet_in_head(packet *new_packet, unsigned short priority_index, connection **connections_by_priority){
    connection *current_connection=connections_by_priority[priority_index];
    if (current_connection->packets_tail==NULL){
        initialize_connection(current_connection,new_packet);
    }else{
        new_packet->next=current_connection->packets_head;
        current_connection->packets_head=new_packet;
    }
}


unsigned short hash_function(connection_identifier *identifier){
    float hash_index=3*identifier->sender_port+(-2)*identifier->destiny_port;
    for (int i=0;i<strlen(identifier->sender_ip);i++){
        if (identifier->sender_ip[i]!='.'){
            hash_index+=(2.5*identifier->sender_ip[i]);
        }
    }
    for (int i=0;i<strlen(identifier->destiny_ip);i++){
        if (identifier->destiny_ip[i]!='.'){
            hash_index+=(-1.5*identifier->destiny_ip[i]);
        }
    }
    unsigned short rounded_hash_index=(unsigned short)round(hash_index);
    rounded_hash_index%=MAX_NUMBER_OF_CONNECTIONS;
    return rounded_hash_index;
}


connection_hash_table_item *search_connections_hash_table(unsigned short hash_index, connection_identifier *identifier, connection_hash_table_item **connections_hash_table){
    connection_hash_table_item *searched_connection=connections_hash_table[hash_index];
    while(searched_connection!=NULL){
        if (compare_identifiers(searched_connection->identifier, identifier)){
            return searched_connection;
        }else{
            searched_connection=searched_connection->next;
        }
    }
    return searched_connection;
}


bool compare_identifiers(connection_identifier *first_identifier, connection_identifier *second_identifier){
    return strcmp(first_identifier->sender_ip,second_identifier->sender_ip)==0 &&
            first_identifier->sender_port==second_identifier->sender_port &&
            strcmp(first_identifier->destiny_ip,second_identifier->destiny_ip)==0 &&
            first_identifier->destiny_port==second_identifier->destiny_port;
}


void free_connections(connection **connections_by_priority, connection_hash_table_item **connections_hash_table){
    unsigned short hash_index;
    connection_hash_table_item *current_item;
    for(int i=0;i<number_of_connections;i++){
        hash_index=connections_by_priority[i]->hash_index;
        free(connections_by_priority[i]->identifier);
        free(connections_by_priority[i]);
        current_item=connections_hash_table[hash_index];
        while(current_item!=NULL){
            current_item=current_item->next;
            free(current_item);
        }
    }
}
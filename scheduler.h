#ifndef SCHEDULER_H
#define SCHEDULER_H


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>


#define LINE_LENGTH 255
#define IP_ADDRESS_LENGTH 15
#define MAX_NUMBER_OF_CONNECTIONS 5000


typedef struct{
    char sender_ip[IP_ADDRESS_LENGTH];
    unsigned short sender_port;
    char destiny_ip[IP_ADDRESS_LENGTH]; 
    unsigned short destiny_port;
} connection_identifier;


typedef struct packet packet;
struct packet{
    unsigned long arrival_time;
    unsigned short length;
    unsigned short weight;
    connection_identifier *identifier;
    packet *next;
};


typedef struct connection connection;
struct connection{
    unsigned short hash_index;
    connection_identifier *identifier;
    packet *packets_head;
    packet *packets_tail;
    unsigned short current_weight;
    unsigned short remainder_to_send_by_weight;
    unsigned short credit;
};


typedef struct connection_hash_table_item connection_hash_table_item;
struct connection_hash_table_item {
    unsigned short priority_index;
    connection_identifier *identifier;
    connection_hash_table_item *next;
};


void initialize_packet(char *buffer, connection **connections_by_priority, connection_hash_table_item **connections_hash_table);
void write_packet(FILE* output_file, packet *current_packet);
packet *rr_scheduler(connection **connections_by_priority);
packet *wrr_scheduler(connection **connections_by_priority);
packet *drr_scheduler(connection **connections_by_priority);
unsigned long get_current_package_time(char* buffer);
void increment_current_connection_priority();
void decrement_current_connection_priority();
packet *get_packet_from_connection(connection *current_connection);
connection_hash_table_item *search_connections_hash_table(unsigned short hash_index, connection_identifier *identifier, connection_hash_table_item **connections_hash_table);
unsigned short hash_function(connection_identifier *identifier);
void create_a_new_connection(connection_identifier *identifier, packet *new_packet, unsigned short hash_index, connection **connections_by_priority, connection_hash_table_item **connections_hash_table);
void initialize_connection(connection *current_connection, packet *new_packet);
void insert_packet_in_tail(packet *new_packet, unsigned short priority_index, connection **connections_by_priority);
void insert_packet_in_head(packet *new_packet, unsigned short priority_index, connection **connections_by_priority);
bool compare_identifiers(connection_identifier *first_identifier, connection_identifier *second_identifier);
void free_connections(connection **connections_by_priority, connection_hash_table_item **connections_hash_table);


unsigned long time=0;
unsigned short number_of_connections=0;
unsigned short current_connection_priority=0;
unsigned short quantum=0;
bool current_connection_not_yet_credited=true;

#endif
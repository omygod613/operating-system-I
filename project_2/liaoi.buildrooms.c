#include <dirent.h>
#include <pwd.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>


#define MIN_CONN 3
#define MAX_CONN 6
#define NUM_ROOMS 7
#define NUM_ROOM_NAMES 10
#define NAME_BUFFER_LEN 12

/*********************************************************************
 *         Room List
 *********************************************************************/
const char *room_names[NUM_ROOM_NAMES] = {
    "aaa",
    "bbb",
    "ccc",
    "ddd",
    "eee",
    "fff",
    "ggg",
    "hhh",
    "iii",
    "jjj"
};

/*********************************************************************
 *          Type of Room
 *********************************************************************/
enum room_type {
    START_ROOM,
    END_ROOM,
    MID_ROOM
};


/*********************************************************************
 *          Room Structure
 *********************************************************************/
struct room {
    enum room_type type;
    const char *name;
    unsigned int cap_conns;
    unsigned int num_conns;
    struct room *connections[NUM_ROOMS];
};

/*********************************************************************
 *          Declaration Function
 *********************************************************************/
struct room rooms_list[NUM_ROOMS];
struct room *generate_rooms();
void print_room(unsigned int room);
int create_rooms_dir(char *dir_name);
void serialize_rooms(struct room rooms[NUM_ROOMS]);
bool connected(unsigned int room1, unsigned int room2, struct room rooms_list[NUM_ROOMS]);
bool already_connected(unsigned int room1, unsigned int room2);
struct room* deserialize_rooms();
const char *pick_right_name(char *in);
struct room *pick_right_room(char *in);
void destroy_rooms(struct room *rooms);


/*********************************************************************
 *          Main Function
 *********************************************************************/
int main() {
    // Seed of random time based on current time.
    srand((unsigned) time(0));
    // Generate the rooms.
    generate_rooms();
    // Creat the files and a directory for those rooms.
    serialize_rooms(rooms_list);
    
    return 0;
}


/*********************************************************************
 *          Generate rooms
 *********************************************************************/
struct room *generate_rooms() {
    int i, j;
    bool taken_names[NUM_ROOMS];
    memset(&taken_names, 0, NUM_ROOMS * sizeof(bool));
    //Each loop genrete one room
    for (i = 0; i < NUM_ROOMS; i++) {
        rooms_list[i].num_conns = 0;
        unsigned int cap_conns = rand() % (MAX_CONN - MIN_CONN);
        cap_conns += MIN_CONN-2;
        rooms_list[i].cap_conns = cap_conns;
        
        // Choose one room and give one name.
        while (true) {
            // Choose one name randomly.
            unsigned int room_index = rand() % NUM_ROOMS;
            
            // if it's not taken.
            if (!taken_names[room_index]) {
                taken_names[room_index] = true;
                rooms_list[i].name = room_names[room_index];
                break;
            }
            
        }
        // Set the middle rooms type
        rooms_list[i].type = MID_ROOM;
    }
    
    // Connect each room to other connections randomly.
    for (i = 0; i < NUM_ROOMS; i++) {
        // for each connection,
        for (j = 0; j < rooms_list[i].cap_conns; j++) {
            
            unsigned int random_room = rand() % NUM_ROOMS;
            while (!connected(i, random_room, rooms_list)) {
                random_room = rand() % NUM_ROOMS;
            }
        }
    }
    // Set the first room as start room.
    // Set the last room as end room.
    rooms_list[0].type = START_ROOM;
    rooms_list[NUM_ROOMS - 1].type = END_ROOM;
    return rooms_list;
}
/*********************************************************************
 *          Connect two rooms.
 *********************************************************************/
bool connected(unsigned int room1, unsigned int room2, struct room rooms_list[NUM_ROOMS]) {
    struct room *r1 = &rooms_list[room1];
    struct room *r2 = &rooms_list[room2];
    // if the rooms reach the maxmium number, then return false
    if (r1->num_conns == MAX_CONN) {
        return true;
    }
    // if the rooms are already connected, then return false
    if (already_connected(room1, room2)) {
        return false;
    }
    // if either the rooms reach the maxmium number, then return false
    if (r1->num_conns >= MAX_CONN || r2->num_conns >= MAX_CONN) {
        return false;
    }
    assert(r1 != NULL);
    assert(r2 != NULL);
    // Connect the rooms
    r1->connections[r1->num_conns] = r2;
    r2->connections[r2->num_conns] = r1;
    // Update the number of connections
    r1->num_conns++;
    r2->num_conns++;
    assert(r1->connections[r1->num_conns-1] != NULL);
    assert(r2->connections[r2->num_conns-1] != NULL);
    return true;
}
/*********************************************************************
 *          Determine if the rooms are connected.
 *********************************************************************/
bool already_connected(unsigned int room1, unsigned int room2) {
    
    int i;
    // if the rooms are already connected, then return true.
    if (room1 == room2) {
        return true;
    }
    // Determine if it is connected to the current room.
    for (i = 0; i < rooms_list[room1].num_conns; i++) {
        if (rooms_list[room1].connections[i] == &rooms_list[room2] &&
            rooms_list[room1].connections[i] != NULL) {
            return true;
        }
    }
    return false;
}

/*********************************************************************
 *          Get the directory name.
 *********************************************************************/
char *get_dir_name() {
    // get the current process id.
    pid_t pid = getpid();
    // get the current user id
    uid_t uid = getuid();
    struct passwd *user_info = getpwuid(uid);
    // define the maximum length for the directory name.
    unsigned long buffer_max_len = strlen(".rooms.") + strlen(user_info->pw_name) + 10;
    // allocate space for the name
    char *dir_name = malloc(buffer_max_len * sizeof(char));
    assert(dir_name != NULL);
    // Create the name
    sprintf(dir_name, "%s.rooms.%d", user_info->pw_name, pid);
    return dir_name;
}
/*********************************************************************
 *          Serialize the rooms to memory.
 *********************************************************************/
void serialize_rooms(struct room rooms[NUM_ROOMS]) {
    
    int i,j;
    // get the directory name
    char *dir_name = get_dir_name();
    // make the directory with all of the permissions
    mkdir(dir_name, 0777);
    // cd/enter the directory
    chdir(dir_name);
    // For each room make a file
    for (i = 0; i < NUM_ROOMS; i++) {
        // open the file
        FILE *fp = fopen(rooms[i].name, "w");
        
        // Write the room name
        fprintf(fp, "ROOM NAME: %s\n", rooms[i].name);
        
        // Write each of the connections
        for (j = 0; j < rooms[i].num_conns; j++) {
            fprintf(fp, "CONNECTION %d: %s\n", j + 1, rooms[j].name);
        }
        
        // Write the right room type
        switch (rooms[i].type) {
            case END_ROOM:
                fprintf(fp, "ROOM TYPE: END_ROOM");
                break;
            case MID_ROOM:
                fprintf(fp, "ROOM TYPE: MID_ROOM");
                break;
            case START_ROOM:
                fprintf(fp, "ROOM TYPE: START_ROOM");
                break;
        }
        // Close the file
        fclose(fp);
    }
    // Return to the original directory.
    chdir("..");
    // Freedom! Free the directory name
    free(dir_name);
}

/*********************************************************************
 *          Print all connections for the user.
 *********************************************************************/
void print_all_connections(struct room *r) {
    int i;
    printf("POSSIBLE CONNECTIONS: ");
    // Print all the connection, then specially for last one giving period.
    for (i = 0; i < r->num_conns-1; i++) {
        printf("%s, ", r->connections[i]->name);
    }
    if (r->num_conns > 0) {
        printf("%s.\n", r->connections[r->num_conns-1]->name);
    }
}
/*********************************************************************
 *          Deserialize a single room.
 *********************************************************************/
struct room deserialize_single_room(char *name) {
    struct room r;
    FILE *file = fopen(name, "r");
    char received_name[NAME_BUFFER_LEN];
    fscanf(file, "ROOM NAME: %s\n", name);
    r.name = pick_right_name(name);
    
    int read;
    int conn_number;
    while ((read =
            fscanf(file, "CONNECTION %d: %s\b", &conn_number, received_name)) != 0
           && read != EOF) {
        r.connections[conn_number-1] = pick_right_room(received_name);
    }
    r.num_conns = conn_number - 1;
    fscanf(file, "ROOM TYPE: %s\n", received_name);
    if (strcmp(name, "START_ROOM") == 0) {
        r.type = START_ROOM;
    }
    else if (strcmp(name, "END_ROOM") == 0) {
        r.type = END_ROOM;
    }
    else
        r.type = MID_ROOM;
        
    fclose(file);
    return r;
}
/*********************************************************************
 *          Check if the name is correct.
 *********************************************************************/
const char *pick_right_name(char *in) {
    
    int i;
    
    for (i = 0; i < MAX_CONN; i++) {
        if (strcmp(in, room_names[i]) == 0) {
            return room_names[i];
        }
    }
    return NULL;
}
/*********************************************************************
 *          Check if the room is correct.
 *********************************************************************/
struct room *pick_right_room(char *in) {
    
    int i;
    
    for (i = 0; i < NUM_ROOMS; i++) {
        if (strcmp(in, rooms_list[i].name) == 0) {
            return &rooms_list[i];
        }
    }
    return NULL;
}

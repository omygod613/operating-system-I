
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

struct room rooms_list[NUM_ROOMS];

void repl();

struct room *generate_rooms();
void print_room(unsigned int room);
int create_rooms_dir(char *dir_name);
void serialize_rooms(struct room rooms[NUM_ROOMS]);
bool connected(unsigned int room1, unsigned int room2,
             struct room rooms_list[NUM_ROOMS]);

void print_all_connections(struct room *r);
// Returns a pointer to the starting room which forms a graph.
struct room* deserialize_rooms();
void destroy_rooms(struct room *rooms);
int remove_directory(const char *path);
char *get_lastmodify_file_name();

/*********************************************************************
 *          Main Function
 *********************************************************************/
int main() {
    char *dir_name = get_lastmodify_file_name();
    // Seed the random number generator with the current time.
    // Don't do this for anything serious like crypto.
    srand((unsigned) time(0));
    remove_directory(dir_name);
    // Generate the list of rooms.
    generate_rooms();
    // Serialize the list of rooms to a file and directory structure
    serialize_rooms(rooms_list);
    // Read them back
    struct room *read_rum = deserialize_rooms();
    // Enter the game loop
    repl();
    destroy_rooms(read_rum);
    return 0;
}
/*********************************************************************
 *          Congratulate the poor sop on success
 *********************************************************************/
void ending_message(unsigned int num_steps, struct room **visited,
                    unsigned int visited_index) {
    int i = 0;
    
    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", num_steps);
    for (i = 0; i < visited_index; i++) {
        printf("%s\n", visited[i]->name);
    }
}

/*********************************************************************
 *          The game loop(tm) where all the fun happens
 *********************************************************************/
void repl() {
    // Set the current room to the starting room, which we define to be
    // the first room.
    struct room *current_room = &rooms_list[0];
    // Allocate an array to hold the list of visited functions
    struct room **visited = malloc(sizeof(struct room*) * NUM_ROOMS);
    unsigned int visited_index = 0;
    unsigned int visited_cap = NUM_ROOMS;
    // Create a buffer for holding the name the user will type. Obviously
    // shouldn't be longer than the longest room's name
    char buffer[NAME_BUFFER_LEN];
    // The number of steps taken so far
    unsigned int num_steps = 0;
    while (true) {
        // I've written a fair bit of C and so far I have listened to
        // Dijkstra's advice on gotos.
        // relevant XKCD:       https://www.xkcd.com/292/
    Top:
        // If we're at the end, say so and return
        if (current_room->type == END_ROOM) {
            ending_message(num_steps, visited, visited_index);
            free(visited);
            return;
        }
        
        // Print all those UGLY MESSAGES!
        printf("\nCURRENT LOCATION: %s\n", current_room->name);
        print_all_connections(current_room);
        printf("WHERE TO? >");
        char *ret = fgets(buffer, NAME_BUFFER_LEN, stdin);
        int i = 0;
        assert(ret != NULL);
        // Strip the newline from the end. In other languages this
        // would be buffer.comp
        buffer[strlen(buffer)-1] = '\0';
        
        // Compare the user's input with all of the connections
        for (i = 0; i < current_room->num_conns; i++) {
            // If a connection matches the user's input
            if (strncmp(buffer, current_room->connections[i]->name,
                        NAME_BUFFER_LEN) == 0) {
                // update the current room
                current_room = current_room->connections[i];
                
                // If we have reached the capacity of the
                // array of visited rooms reallocate
                if (visited_index >= visited_cap) {
                    visited_cap += NUM_ROOMS;
                    visited = realloc(visited,
                                      visited_cap *
                                      sizeof(struct room*)
                                      );
                    
                    assert(visited != NULL);
                }
                // push the room onto the list of visited rooms
                visited[visited_index] = current_room;
                visited_index++;
                num_steps++;
                // Loop again
                goto Top;
            }
        }
        // If none of the user's input didn't match any of the
        // connections print a message and continue.
        printf("\nHUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
    }
}

/*********************************************************************
 *          get lastmoidify file name
 *********************************************************************/
char *get_lastmodify_file_name() {
    
    // get the current user id and their entry in /etc/passwd
    uid_t uid = getuid();
    struct passwd *user_info = getpwuid(uid);
    // define the maximum length for the directory name.
    unsigned long buffer_max_len = strlen(".rooms.") + strlen(user_info->pw_name) + 10;
    // actually allocate space for the name
    char *lastmodify_file_name = malloc(buffer_max_len * sizeof(char));
    assert(lastmodify_file_name != NULL);
    char *fd = ".rooms.";
    char currentDir[100];
    memset(currentDir, '\0', sizeof(currentDir));
    getcwd(currentDir, sizeof(currentDir));
    printf("%s\n", currentDir);
    DIR *d;
    struct dirent *dp;
    struct stat *buffer;
    buffer = malloc(sizeof(struct stat));
    dp = malloc(sizeof(struct dirent));
    time_t lastModified;  // <-THIS
    d = opendir(".");
    
    char *dir_name = malloc(buffer_max_len * sizeof(char));
    
    char *temp_name[10] = {};
    long *temp_time[10] = {};
    int i = 0;
    int j = 0;
    
    if (d) {
        
        while ((dp = readdir(d)) != NULL){
            
            if (strstr(dp->d_name,fd) != NULL){
                stat(dp->d_name, buffer);
                lastModified = buffer->st_mtime;
                
                temp_name[i] = dp->d_name;
                temp_time[i] = &lastModified;
                
                i++;
                
                
                //printf("%s: %s", dp->d_name, ctime(&lastModified));
                //printf("%s: %ld\n", dp->d_name, lastModified);
            }
        }
        
        for(j = 0; j<10; j++){
            if (temp_time[j] > temp_time[j-1]){
                dir_name = temp_name[j];
            }
        }
        
    }
    return dir_name;
}
int remove_directory(const char *path){
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    
    int r = -1;
    
    if (d){
        struct dirent *p;
        r = 0;
        while (!r && (p=readdir(d))){
            int r2 = -1;
            char *buf;
            size_t len;
            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")){
                continue;
            }
            
            len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);
            
            if (buf){
                struct stat statbuf;
                snprintf(buf, len, "%s/%s", path, p->d_name);
                if (!stat(buf, &statbuf)){
                    if (S_ISDIR(statbuf.st_mode)){
                        r2 = remove_directory(buf);
                    }
                    else{
                        r2 = unlink(buf);
                    }
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }
    if (!r){
        r = rmdir(path);
    }
    return r;
}
/*********************************************************************
 *          Generate a graph of rooms
 *********************************************************************/
struct room *generate_rooms() {
    // Create a list of names which have been taken and set them all to
    // false to indicate they are available.
    int i = 0;
    int j = 0;
    bool taken_names[NUM_ROOMS];
    memset(&taken_names, 0, NUM_ROOMS * sizeof(bool));
    // For each room,
    for (i = 0; i < NUM_ROOMS; i++) {
        // Initialize the number of connections to 0.
        rooms_list[i].num_conns = 0;
        // Set the capacity to a random number between MIN_CONN and
        // MAX_CONN minus 2. If the minus 2 is not there the process
        // of connecting the rooms will tend to make all of the rooms
        // have the maximum number of connections.
        unsigned int cap_conns = rand() % (MAX_CONN - MIN_CONN);
        cap_conns += MIN_CONN-2;
        rooms_list[i].cap_conns = cap_conns;
        
        // Pick a name for the room
        while (true) {
            // generate a random name
            unsigned int room_index = rand() % NUM_ROOMS;
            // if it's not taken,
            if (!taken_names[room_index]) {
                // set it as taken and give it to the room.
                taken_names[room_index] = true;
                rooms_list[i].name = room_names[room_index];
                break;
            }
            
        }
        // Set the room type
        rooms_list[i].type = MID_ROOM;
    }
    
    // Now that all of the rooms have been properly initialized, connect
    // them
    // for each room,
    for (i = 0; i < NUM_ROOMS; i++) {
        // for each connection,
        for (j = 0; j < rooms_list[i].cap_conns; j++) {
            // generate a random room to connect to
            unsigned int random_room = rand() % NUM_ROOMS;
            // Attempt to connect the rooms. If that fails,
            // generate a new random room.
            while (!connected(i, random_room, rooms_list)) {
                random_room = rand() % NUM_ROOMS;
            }
        }
    }
    // Set the start and end rooms.
    rooms_list[0].type = START_ROOM;
    rooms_list[NUM_ROOMS - 1].type = END_ROOM;
    return rooms_list;
}

/*********************************************************************
 *          Determine if the rooms are connected.
 *********************************************************************/
bool already_connected(unsigned int room1, unsigned int room2) {
    // rooms are  connected to themselves.
    int i = 0;
    if (room1 == room2) {
        return true;
    }
    // For each room, check whether it is connected to the current room.
    for (i = 0; i < rooms_list[room1].num_conns; i++) {
        if (rooms_list[room1].connections[i] == &rooms_list[room2] &&
            rooms_list[room1].connections[i] != NULL) {
            return true;
        }
    }
    return false;
}

/*********************************************************************
 *          Connect two rooms.
 *********************************************************************/
bool connected(unsigned int room1, unsigned int room2,
             struct room rooms_list[NUM_ROOMS]) {
    struct room *r1 = &rooms_list[room1];
    struct room *r2 = &rooms_list[room2];
    // If the number of connections is maxed out, we're done
    if (r1->num_conns == MAX_CONN) {
        return true;
    }
    // if the rooms are already connected, don't connect them again
    if (already_connected(room1, room2)) {
        return false;
    }
    // if either of the rooms maximum number of connections has been
    // reached don't connect them.
    if (r1->num_conns >= MAX_CONN || r2->num_conns >= MAX_CONN) {
        return false;
    }
    assert(r1 != NULL);
    assert(r2 != NULL);
    // Connect the rooms
    r1->connections[r1->num_conns] = r2;
    r2->connections[r2->num_conns] = r1;
    // update the number of connections
    r1->num_conns++;
    r2->num_conns++;
    assert(r1->connections[r1->num_conns-1] != NULL);
    assert(r2->connections[r2->num_conns-1] != NULL);
    return true;
}

// prints info about the room. Useful for debugging.
void print_room(unsigned int room) {
    int i = 0;
    printf("name: %s,\nnum_conns %d,\ncap_conns %d,\n",
           rooms_list[room].name,
           rooms_list[room].num_conns,
           rooms_list[room].cap_conns
           );
    for (i = 0; i < rooms_list[room].num_conns; i++) {
        printf("connection: %s\n", rooms_list[room].connections[i]->name);
    }
}

/*********************************************************************
 *          Get the directory name.
 *********************************************************************/
char *get_dir_name() {
    // get the current process id.
    pid_t pid = getpid();
    
    // get the current user id and their entry in /etc/passwd
    uid_t uid = getuid();
    struct passwd *user_info = getpwuid(uid);
    // define the maximum length for the directory name.
    // It must be big enough to hold the constant string ".rooms."
    // and the user name, and finally the base 10 representation of
    // the process id calculated using the change of base formula:
    // https://en.wikipedia.org/wiki/Logarithm#Change_of_base
    // Add a pinch to grow an inch -- err rather an extra 1 to make
    // valgrind happy for that \0 at the end of the string.
    // Note -- we don't actually do this because that would require
    // linking against the math library. Instead I inspected the maximum
    // number of processes on the os-class server with
    // $ cat /proc/sys/kernel/pid_max
    // I then decided to double the number of characters in there because
    // if we're adding arbitrary constants they might as well be round.
    unsigned long buffer_max_len = strlen(".rooms.") +
    strlen(user_info->pw_name) +
    10;
    //log(pid)/log(10) + 5;
    // actually allocate space for the name
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
    // get the directory name
    char *dir_name = get_dir_name();
    // make the directory with all of the permissions
    mkdir(dir_name, 0777);
    // cd/enter the directory
    chdir(dir_name);
    // For each room make a file
    int i = 0;
    int j = 0;
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
 *          Check if the name is correct.
 *********************************************************************/
const char *pick_right_name(char *in) {
    int i = 0;
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
    int i = 0;
    for (i = 0; i < NUM_ROOMS; i++) {
        if (strcmp(in, rooms_list[i].name) == 0) {
            return &rooms_list[i];
        }
    }
    return NULL;
}

void iter_rooms(struct room *r, void (*func)(struct room *)) {
    int i = 0;
    for (i = 0; i < r->num_conns; i++) {
        func((r->connections[i]));
    }
}

// Print all adjacent connections for the user
void print_all_connections(struct room *r) {
    int i = 0;
    printf("POSSIBLE CONNECTIONS: ");
    // Print first couple
    for (i = 0; i < r->num_conns-1; i++) {
        printf("%s, ", r->connections[i]->name);
    }
    // Print the last one with a period at the end.
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
    
    // I control the names so I know how long to make this buffer
    // should be as long as the longest name or START_ROOM + 1 more
    // character for \0.
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
    } else if (strcmp(name, "END_ROOM") == 0) {
        r.type = END_ROOM;
    } else if (strcmp(name, "MID_ROOM") == 0) {
        r.type = MID_ROOM;
    } else {
        // What the hell are you doing?
    }
    fclose(file);
    return r;
}

// Recreate the rooms structure by reading the files
struct room* deserialize_rooms() {
    struct room *rooms = malloc(NUM_ROOMS * sizeof(struct room));
    unsigned int room_count = 0;
    char *dir_name = get_dir_name();
    // FIXME: Make sure that directory exists with stat
    chdir(dir_name);
    DIR *dp;
    struct dirent *dir;
    dp = opendir (".");
    assert(dp != NULL);
    
    while ((dir = readdir (dp))) {
        rooms[room_count] = deserialize_single_room(dir->d_name);
    }
    
    closedir (dp);
    
    free(dir_name);
    chdir("..");
    return rooms;
}

// Free the deserialized rooms data structure
void destroy_rooms(struct room *rooms) {
    // Take advantage of the fact that the rooms are allocated as a
    // contiguous array.
    free(rooms);
}

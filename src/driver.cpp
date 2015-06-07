#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

extern "C" {
#include <lirc/lirc_client.h>
}

static const std::size_t UNIX_PATH_MAX = 108;

void ir_received(char* raw_code){
    std::string full_code(raw_code);

    auto code_end = full_code.find(' ');
    std::string code(full_code.begin(), full_code.begin() + code_end);

    auto repeat_end = full_code.find(' ', code_end + 1);
    std::string repeat(full_code.begin() + code_end + 1, full_code.begin() + repeat_end);

    auto key_end = full_code.find(' ', repeat_end + 1);
    std::string key(full_code.begin() + repeat_end + 1, full_code.begin() + key_end);

    std::cout << "Code: " << code << std::endl;
    std::cout << "Repeat: " << repeat << std::endl;
    std::cout << "Key: " << key << std::endl;
}

int main(){
    //Initiate LIRC. Exit on failure
    char lirc_name[] = "lirc";
    if(lirc_init(lirc_name, 1) == -1){
        std::cout << "asgard:ir: Failed to init LIRC" << std::endl;
        return 1;
    }

    //Open the socket
    auto socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(socket_fd < 0){
        std::cout << "asgard:ir: socket() failed" << std::endl;
        return 1;
    }

    //Init the address
    struct sockaddr_un address;
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, UNIX_PATH_MAX, "/tmp/asgard_socket");

    //Connect to the server
    if(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0){
        std::cout << "asgard:ir: connect() failed" << std::endl;
        return 1;
    }

    char buffer[4096];

    auto nbytes = snprintf(buffer, 4096, "hello from a client");
    write(socket_fd, buffer, nbytes);

    nbytes = read(socket_fd, buffer, 4096);
    buffer[nbytes] = 0;

    printf("MESSAGE FROM SERVER: %s\n", buffer);

    //Read the default LIRC config
    struct lirc_config* config;
    if(lirc_readconfig(NULL,&config,NULL)==0){
        char* code;

        //Do stuff while LIRC socket is open  0=open  -1=closed.
        while(lirc_nextcode(&code)==0){
            //If code = NULL, nothing was returned from LIRC socket
            if(code){
                //Send code further
                ir_received(code);

                //Need to free up code before the next loop
                free(code);
            }
        }

        //Frees the data structures associated with config.
        lirc_freeconfig(config);
    } else {
        std::cout << "asgard:ir: Failed to read LIRC config" << std::endl;
    }

    //Closes LIRC
    lirc_deinit();
 
    //Close the socket
    close(socket_fd);

    return 0;
}

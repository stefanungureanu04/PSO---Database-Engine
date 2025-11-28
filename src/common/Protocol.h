#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>

// Maximum buffer size for messages
#define BUFFER_SIZE 4096

// Command definitions
#define CMD_EXIT_UPPERCASE "EXIT"
#define CMD_EXIT_LOWERCASE "exit"
#define CMD_BACKUP "BACKUP"

// Response codes
#define RESP_OK "OK"
#define RESP_ERR "ERROR"

// Helper struct for messages
struct Message
{
	char data[BUFFER_SIZE];
};

#endif // PROTOCOL_H
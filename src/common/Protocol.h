#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>

#define BUFFER_SIZE 4096

#define CMD_EXIT "EXIT"
#define CMD_BACKUP "BACKUP"
#define CMD_CREATE_DB "CREATE DATABASE"
#define CMD_DROP_DB "DROP DATABASE"
#define CMD_USE "USE"
#define CMD_PWDB "PWDB"
#define CMD_SHOW "SHOW"
#define CMD_LOG "LOG"
#define CMD_CREATE_TABLE "CREATE TABLE"
#define CMD_DROP_TABLE "DROP TABLE"
#define CMD_INSERT "INSERT INTO"
#define CMD_SELECT "SELECT"
#define CMD_UPDATE "UPDATE"
#define CMD_DELETE "DELETE FROM"
#define CMD_SAVE "SAVE"

#define KEYWORD_VALUES "VALUES"
#define KEYWORD_FROM "FROM"
#define KEYWORD_WHERE "WHERE"
#define KEYWORD_ORDER_BY "ORDER BY"
#define KEYWORD_SET "SET"
#define KEYWORD_AND " AND "
#define KEYWORD_OR " OR "

#define TYPE_INT_STR "INT"
#define TYPE_VARCHAR_STR "VARCHAR"
#define TYPE_DATE_STR "DATE"

#define OPT_CONFIG "-CONFIGURATION"
#define OPT_LOGS "-LOGS"
#define OPT_TABLE "-TABLE"
#define OPT_DBS "-DATABASES"
#define OPT_TABLES "-TABLES"

#define RESP_OK "OK"
#define RESP_ERR "ERROR"
#define MSG_WELCOME "Welcome\n"
#define MSG_SERVER_LISTENING "Server listening on port "
#define MSG_NEW_CONNECTION "New connection: "
#define MSG_USER_CONNECTED "User connected: "
#define MSG_CLIENT_DISCONNECTED "Client disconnected"
#define MSG_CONNECT_SUCCESS "Connected to server. Type 'EXIT' to quit.\n"
#define MSG_CONNECT_FAIL "Connection failed!\n"
#define MSG_SOCK_ERR "Socket creation error\n"
#define MSG_ADDR_ERR "Invalid address/Address not supported\n"
#define MSG_SERVER_DISC "Server disconnected!\n"
#define MSG_SEND_FAIL "Send fail."
#define MSG_HELP_NOT_FOUND "ERROR: Help topic not found."
#define MSG_NO_DB_SELECTED "ERROR: No database selected. Use USE <dbname> first."
#define MSG_TABLE_EXISTS "ERROR: Table already exists"
#define MSG_TABLE_NOT_FOUND "ERROR: Table not found"
#define MSG_SYNTAX_ERROR "ERROR: Syntax error"
#define MSG_UNKNOWN_CMD "ERROR: Unknown command"

struct Message {
  char data[BUFFER_SIZE];
};

#endif

#include "Saver.h"
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

std::string Saver::saveConfig(std::string filename, std::string currentDB, std::string owner) {
  if (currentDB.empty())
    return "ERROR: No database selected";

  std::string user = owner;

  std::string path = ".data/" + user + "/" + currentDB;
  // List files in directory using ls
  int pipefd[2];
  if (pipe(pipefd) == -1)
    return "ERROR: Pipe failed";

  pid_t pid = fork();
  if (pid == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);
    execlp("ls", "ls", path.c_str(), NULL);
    exit(EXIT_FAILURE);
  } else {
    close(pipefd[1]);
    char buffer[1024];
    std::string result;
    int bytesRead;
    while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
      buffer[bytesRead] = '\0';
      result += buffer;
    }
    close(pipefd[0]);
    wait(NULL);

    std::ofstream outFile(filename);
    if (outFile.is_open()) {
      outFile << "Tables in " << currentDB << ":\n" << result;
      outFile.close();
      return "OK: Configuration saved to " + filename;
    }
    return "ERROR: Could not open file " + filename;
  }
}

std::string Saver::saveLogs(std::string filename) {
  std::string logFile = ".logs/server.log";

  // Copy log file to filename
  pid_t pid = fork();
  if (pid == 0) {
    execlp("cp", "cp", logFile.c_str(), filename.c_str(), NULL);
    exit(EXIT_FAILURE);
  } else {
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
      return "OK: Logs saved to " + filename;
    }
    return "ERROR: Failed to save logs";
  }
}

std::string Saver::saveTable(std::string tableName, std::string filename,
                             std::string currentDB, Database &db) {
  std::string content = db.getTableContent(tableName, currentDB);
  if (content.empty())
    return "ERROR: Table not found or empty";

  std::ofstream outFile(filename);
  if (outFile.is_open()) {
    outFile << content;
    outFile.close();
    return "OK: Table saved to " + filename;
  }
  return "ERROR: Could not open file " + filename;
}

/**
 * @file parser.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loader.h"
#include "parser.h"
#include "syntax.h"

#define READING 0
#define END_OF_FILE 2

FILE *open_process_file(char *filename);
void read_processes(FILE *fptr, char *line);
void read_resources(FILE *fptr, char *line);
void read_mailboxes(FILE *fptr, char *line);
int read_process(FILE *fptr, char *line);
void read_req_resource(FILE *fptr, char *line);
void read_rel_resource(FILE *fptr, char *line);
char *read_comms_send(FILE *fptr, char *line);
char *read_comms_recv(FILE *fptr, char *line);
int read_string(FILE *fptr, char *line);

/**
 * @brief Reads in a specified file, parse it and store it in the associated
 *        data-structure.
 *
 * Reads the process.list file and parse it. It reads the processes and
 * continues by reading the resources. Next the function looks for the process
 * setup and reads the request and release statements. At each stage of the
 * parsing each element is stored in the specific datastructure.
 *
 * @param filename A string with the location of the process.list file for
 * reading.
 */
void parse_process_file(char *filename) {
  FILE *fptr = NULL;
  char line[1024];
  int status;

  fptr = open_process_file(filename);

  if (fptr == NULL) {
    printf("File is NULL: this is bad");
  }

  read_string(fptr, line);

  read_processes(fptr, line);

  read_string(fptr, line);

  read_resources(fptr, line);

  read_string(fptr, line);

  read_mailboxes(fptr, line);

  read_string(fptr, line);
  /* If the mailboxes exist an extra readline is needed
   * to skip all the whitespaces of the next line */
  if (strcmp(line, "") == 0) {
    read_string(fptr, line);
  }

  status = READING;
  while (status != END_OF_FILE) {
    status = read_process(fptr, line);
  }

  fclose(fptr);
}

/**
 * @brief Opens the file with filename and return a pointer to the file.
 *
 * Opens a file, with filename, for read-only. If the file could not be
 * open return NULL else return the pointer to the file.
 *
 * @param filename The name of the file to open
 *
 * @return A file pointer
 */
FILE *open_process_file(char *filename) {
  FILE *file = fopen(filename, "r");

  if (file == NULL) {
    file = NULL;
  }

  return file;
}

/**
 * @brief Reads the list of processes and loads it with functions defined in
 *        loader.h
 *
 * Reads a line from the file referenced by fptr and expects it to be the
 * PROCESSES keyword. If true continue to reserve space for the process name
 * and repeatedly read all the processes and load it.
 *
 * @param fptr A pointer to the file from which to read.
 * @param line A pointer to a string read from the file.
 */
void read_processes(FILE *fptr, char *line) {
  char *processName;

  if (strcmp(line, PROCESSES) == 0) {
    processName = malloc(3 * sizeof(char));

    while (read_string(fptr, processName) != 0) {
      load_process(processName);
      processName = malloc(3 * sizeof(char));
    }
    load_process(processName);
  }
}

/**
 * @brief Reads the list of resources and loads it with functions defined in
 *        loader.h
 *
 * Reads the list of resources and loads it with the load_resource function
 * defined in loader.h
 *
 * @param fptr A pointer to the file from which to read.
 * @param line A pointer to a string read from the file.
 */
void read_resources(FILE *fptr, char *line) {
  char *resourceName;

  if (strcmp(line, RESOURCES) == 0) {
    resourceName = malloc(3 * sizeof(char));
    while (read_string(fptr, resourceName) != 0) {
      load_resource(resourceName);
      resourceName = malloc(3 * sizeof(char));
    }
    load_resource(resourceName);
  }
}

/**
 * @brief Reads the list of mailboxes and loads it.
 *
 * Reads the list of mailboxes and loads it with the load_mailbox function
 * defined in loader.h
 *
 * @param fptr A pointer to the file from which to read.
 * @param line A pointer to a string read from file.
 */
void read_mailboxes(FILE *fptr, char *line) {
  char *mailboxName;

  if (strcmp(line, MAILBOXES) == 0) {
    mailboxName = malloc(strlen(line) * sizeof(char));
    while (read_string(fptr, mailboxName) != 0) {
      load_mailbox(mailboxName);
      mailboxName = malloc(strlen(line) * sizeof(char));
    }
    load_mailbox(mailboxName);
  }
}

/**
 * @brief Reads the defined instruction for a process and loads it.
 *
 * Reads the list of instructions for each process and loads it in the
 * appropriate datastructure using the functions defined in loader.h
 *
 * @param fptr A pointer to the file from which to read.
 * @param line A pointer to a string read from file.
 *
 * @return s Indicates the current status of reading the process.
 */
int read_process(FILE *fptr, char *line) {
  char *resource_name;
  char *process_name;
  char *msg;
  int s;

  s = 0; /* Must test this assignment */

  if (strcmp(line, PROCESS) == 0) {
    /* reads the process name */
    process_name = malloc(sizeof(char) * 3);
    read_string(fptr, process_name);
    /* 1. Use the resource_name to find the relevant pcb */
#ifdef DEBUG
    printf("Process %s\n", process_name);
#endif
    resource_name = malloc(sizeof(char) * 64);
    while ((s = read_string(fptr, resource_name)) != 0 && s != 2) {
      if (strcmp(resource_name, REQ) == 0) {
        /* Read the REQ resource */
        read_req_resource(fptr, resource_name);
        load_process_instruction(process_name, REQ, resource_name, NULL);
        /* 2. Store instruction using the pcb pointer */
      } else if (strcmp(resource_name, REL) == 0) {
        /* Read the REL resource */
        read_rel_resource(fptr, resource_name);
        load_process_instruction(process_name, REL, resource_name, NULL);
      } else if (strcmp(resource_name, SEND) == 0) {
        /* Read the COMMS resource */
        msg = read_comms_send(fptr, resource_name);
        load_process_instruction(process_name, SEND, resource_name, msg);
      } else if (strcmp(resource_name, RECV) == 0) {
        /* Read the COMMS resource */
        msg = read_comms_recv(fptr, resource_name);
        load_process_instruction(process_name, RECV, resource_name, msg);
      } else {
        /* Execute on white spaces */
        /* Execute the while loop when encoutering new lines and white spaces,
         * exit the loop when encountering the instructions for the next
         * Process */
        if (strcmp(resource_name, "") != 0) {
          break;
        }
      }
      resource_name = malloc(sizeof(char) * 64);
    }
  }
  return s;
}

/**
 * @brief Reads the resource name in a request instruction.
 *
 * Uses the read_string function to read the name of the resource specified in
 * this request instruction.
 *
 * @param fptr A pointer to the file from which to read.
 * @param line A pointer to a string read from file.
 */
void read_req_resource(FILE *fptr, char *line) {
  read_string(fptr, line);
#ifdef DEBUG
  printf("req %s\n", line);
#endif
}

/**
 * @brief Reads the resource name in a release instruction.
 *
 * Uses the read_string function to read the name of the resource specified in
 * this release instruction.
 *
 * @param fptr A pointer to the file from which to read.
 * @param line A pointer to a string read from file.
 */
void read_rel_resource(FILE *fptr, char *line) {
  read_string(fptr, line);
#ifdef DEBUG
  printf("rel %s\n", line);
#endif
}

/**
 * @brief Reads the send instruction and the data.
 *
 * Reads all the elements from the send instruction and stores the mailbox as
 * well as the message to be loaded.
 *
 * @param fptr A pointer to the file from which to read.
 * @param line A pointer to a string read from file.
 *
 * @return message The message which the instruction will send.
 */
char *read_comms_send(FILE *fptr, char *line) {
  int ch;
  char *message;
  int index;

  message = malloc(sizeof(char) * 128);

  while ((ch = fgetc(fptr)) != '\n') {
    /* Check to make sure that the character is in the
     * range of all the ascii letters */
    if ((ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122)) {
      index = 1;
      line[0] = ch;
      while ((ch = fgetc(fptr)) != COMMA) {
        if (ch != WHITESPACE) {
          line[index] = ch;
          index++;
        }
      }
      line[index] = '\0'; /* Adds the termination character at the string end */
      index = 0;
      while ((ch = fgetc(fptr)) != RIGHTBRACKET) {
        message[index] = ch;
        index++;
      }
    }
  }
#ifdef DEBUG
  printf("send (%s, %s)\n", line, message);
#endif
  return message;
}

/**
 * @brief Reads the receive instruction and the data.
 *
 * Reads all the elements from the receive instruction and stores the mailbox as
 * well as the message to be loaded. In terms of the receive instruction, the
 * message is the variable in which to receive a message from the specified
 * mailbox. In order to conform to the specification, it has been included as a
 * read message.
 *
 * @param fptr A pointer to the file from which to read.
 * @param line A pointer to a string read from file.
 *
 * @return message A placeholder for the variable which receives the message.
 */
char *read_comms_recv(FILE *fptr, char *line) {
  int ch;
  char *message;
  int index;

  message = malloc(sizeof(char) * 128);

  while ((ch = fgetc(fptr)) != '\n') {
    /* Check to make sure that the character is in the
     * range of all the ascii letters */
    if ((ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122)) {
      index = 1;
      line[0] = ch;
      while ((ch = fgetc(fptr)) != COMMA) {
        if (ch != WHITESPACE) {
          line[index] = ch;
          index++;
        }
      }
      line[index] = '\0';
      index = 0;
      while ((ch = fgetc(fptr)) != RIGHTBRACKET) {
        message[index] = ch;
        index++;
      }
    }
  }
#ifdef DEBUG
  printf("recv (%s, %s)\n", line, message);
#endif
  return message;
}

/**
 * @brief Reads the next string.
 *
 * Reads the file character for character and constructs a string until a white
 * space or termination character is matched.
 *
 * @param fptr A pointer to the file from which to read.
 * @param line A pointer to space where the string can be stored.
 *
 * return status The status indicates when the END_OF_FILE or NEW LINE has
 * been reached.
 */
int read_string(FILE *fptr, char *line) {
  int index = 0;
  int ch = 0;
  int status = 1;

  ch = fgetc(fptr);
  while (ch != '\n' && ch != ' ') {
    if (ch == EOF) {
      status = END_OF_FILE;
      break;
    }
    line[index] = ch;
    index++;
    ch = fgetc(fptr);
    status = (ch == '\n' ? 0 : 1);
  }
  line[index] = '\0';

  return status;
}

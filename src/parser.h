/**
  * @file parser.h
  * @description A definition of the structures and functions necessary to read
  *              in the process.list file, parse it and load it into the
  *              necessary datastructures.
  */

#ifndef _PARSER_H
#define _PARSER_H

/**
 * @brief Reads in a specified file, parse it and store it in the associated
 *        data-structure.
 *
 * Reads the process.list file and parse it. It reads the processes and
 * continues by reading the resources. Next the function looks for the process
 * setup and reads the request and release statements. At each stage of the
 * parsing each element is stored in the specific datastructure.
 *
 * @param filename A string with the location of the process.list file for reading.
 */
void parse_process_file(char* filename);

#endif

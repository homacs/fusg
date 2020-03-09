/*
 * search.h
 *
 *  Created on: 6 Mar 2020
 *      Author: homac
 */




int search_files(const char* conf_file, int num_execs, char** execs);

int search_execs(const char* conf_file, int num_files, char** files);

int search_dump_all(const char* conf_file);

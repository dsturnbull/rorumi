#pragma once

#include <stdlib.h>
#include <ruby/ruby.h>

#define DEV_DIR ".rorumi"

extern VALUE ruby_top_self;

typedef struct {
	double offset;
	double size;
	double latitude;
	double longitude;
	double timestamp;
	//double horizontal_accuracy;
} reading_t;

typedef struct {
	int count;
	char *filename;
	char *db_filename;
	char *name;
	int reading_count;
	reading_t **readings;
} device_t;

char * find_rorumi_path();
device_t * find_devices(char *rorumi_path, char *ext);
unsigned char * decoded_name(char *b64);
reading_t * decode_raw_data(char *data, size_t sz);
void print_reading(reading_t *reading);

// vim: ts=4:sw=4

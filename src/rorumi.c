#include "src/reader.h"
#include "src/writer.h"

#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <glib.h>
#include <getopt.h>
#include <stdbool.h>

int
main(int argc, char *argv[])
{
	ruby_init();
	ruby_init_loadpath();

	static struct option long_options[] = {
		{ "create",		no_argument,	NULL,	'c'	},
		{ "rebuild",	no_argument,	NULL,	'r'	},
		{ "dump",		no_argument,	NULL, 	'd'	},
		{ NULL,			0,				NULL,	 0	}
	};

	bool show_help = true;

	char *base_path = find_rorumi_path();
	char *config_file_path = "config";
	int  config_path_len = strlen(find_rorumi_path()) + strlen(config_file_path) + 2;
	char config_path[config_path_len];

	snprintf(config_path, config_path_len, "%s/%s", base_path, config_file_path);

	FILE *cfg = fopen(config_path, "r");
	if (!cfg) {
		fprintf(stderr, "create ~/.rorumi/config, add username and password in first two lines\n");
		exit(1);
	}

	size_t l;
	char *username = fgetln(cfg, &l);
	username[l - 1] = 0;
	char *password = fgetln(cfg, &l);
	password[l - 1] = 0;

	int ch;
	while ((ch = getopt_long(argc, argv, "crd", long_options, NULL)) != -1) {
		switch (ch) {
			case 'c':
				add_entry(username, password);
				show_help = false;
				break;

			case 'd':
				read_indices();
				show_help = false;
				break;

			case 'r':
				rebuild_indices();
				show_help = false;
				break;
		}
	}

	if (show_help)
		printf("-c to create, -r to rebuild, -d to dump\n");

	ruby_finalize();
	return 0;
}

char *
find_rorumi_path()
{
	char *home = getenv("HOME");
	int pathlen = strlen(home) + strlen(DEV_DIR) + 2;	// newline + slash

	char *rorumi_path = malloc(sizeof(char) * pathlen);
	snprintf(rorumi_path, pathlen, "%s/%s", home, DEV_DIR);

	return rorumi_path;
}

device_t *
find_devices(char *rorumi_path, char *ext)
{
	int buf_len = 16;
	int num_devices = 0;
	int len;

	struct dirent *dp;
	DIR *dirp;

	device_t *devices = malloc(sizeof(device_t) * buf_len);

	dirp = opendir(rorumi_path);
	while ((dp = readdir(dirp)) != NULL) {
		len = dp->d_namlen;

		if (len > 4) {
			char *sep = ".";
			char *brkt;
			char *name = strtok_r(dp->d_name, sep, &brkt);
			char *extok = strtok_r(NULL, sep, &brkt);

			if (strcmp(extok, ext) == 0) {
				if (num_devices == buf_len)
					realloc(devices,
						sizeof(devices) +
						sizeof(device_t) * buf_len);

				device_t *device = malloc(sizeof(device_t));

				device->name = name;
				device->reading_count = 0;

				char *filename = malloc(sizeof(char *) * strlen(rorumi_path) + 1 +
					strlen(dp->d_name) + 4);
				sprintf(filename, "%s/%s.idx", rorumi_path,
					dp->d_name);
				device->filename = filename;

				filename = malloc(sizeof(char *) * strlen(rorumi_path) + 1 +
					strlen(dp->d_name) + 3);
				sprintf(filename, "%s/%s.db", rorumi_path,
					dp->d_name);
				device->db_filename = filename;

				devices[num_devices++] = *device;
				devices[0].count = num_devices;
			}
		}
	}

	return devices;
}

unsigned char *
decoded_name(char *b64)
{
	unsigned long n;
	unsigned char *decoded_name;
	decoded_name = g_base64_decode(b64, &n);
	return decoded_name;
}

reading_t *
decode_raw_data(char *data, size_t sz)
{
	reading_t *reading = calloc(sizeof(reading_t), 1);

	VALUE str = rb_str_new(data, sz);
	VALUE obj = rb_marshal_load(str);

	VALUE keys = rb_funcall(obj, rb_intern("keys"), 0);
	int length = NUM2INT(rb_funcall(keys, rb_intern("length"), 0));

	rb_funcall(0, rb_intern("p"), 1, obj);

	for (int i = 0; i < length; i++) {
		// VALUE key = rb_funcall(keys, rb_intern("fetch"), 1, INT2NUM(i));
		// VALUE val = rb_funcall(obj, rb_intern("fetch"), 1, key);

		// char *k = RSTRING(key)->ptr;

		// rb_funcall(0, rb_intern("p"), 1, key);
		// rb_funcall(0, rb_intern("p"), 1, val);
	}

	VALUE loc_key = rb_str_new2("location");
	VALUE loc = rb_funcall(obj, rb_intern("fetch"), 1, loc_key);
	VALUE loc_keys = rb_funcall(loc, rb_intern("keys"), 0);
	length = NUM2INT(rb_funcall(loc_keys, rb_intern("length"), 0));

	for (int i = 0; i < length; i++) {
		VALUE key = rb_funcall(loc_keys, rb_intern("fetch"), 1, INT2NUM(i));
		VALUE val = rb_funcall(loc, rb_intern("fetch"), 1, key);

		char *k = RSTRING(key)->ptr;

		if ((strcmp(k, "latitude") == 0))
			reading->latitude = NUM2DBL(val);
		else if ((strcmp(k, "longitude") == 0))
			reading->longitude = NUM2DBL(val);
		else if ((strcmp(k, "timeStamp") == 0))
			reading->timestamp = NUM2DBL(val) / 1000.0;
		//else if ((strcmp(k, "horizontalAccuracy") == 0))
		//	reading->horizontal_accuracy = NUM2DBL(val);
	}

	return reading;
}

void
print_reading(reading_t *reading)
{
	double time = reading->timestamp;
	char timestr[20 + 1];
	struct tm tm;

	memset(&tm, 0, sizeof(struct tm));
	memset(&timestr, 0, sizeof(char) * 20 + 1);

	sprintf(timestr, "%f", time);
	strptime(timestr, "%s", &tm);

	char fmttime[24];
	strftime(fmttime, sizeof(fmttime), "%F %r", &tm);

	printf("%s:", fmttime);
	printf("%f, ", reading->latitude);
	printf("%f, ", reading->longitude);
	printf("%f, ", reading->offset);
	printf("%f\n", reading->size);
}

// vim: ts=4:sw=4

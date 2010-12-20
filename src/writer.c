#include "src/writer.h"

#include <ruby/ruby.h>
	
void
rebuild_indices()
{
	device_t *devices = find_devices(find_rorumi_path(), "db");

	if (devices != NULL) {
		for (int i = 0; i < devices[0].count; i++) {
			device_t device = devices[i];

			unsigned char *name = decoded_name(device.name);
			printf("rebuilding for device: %s\n", name);
			free(name);

			FILE *idx = fopen(device.filename, "r");
			FILE *db = fopen(device.db_filename, "r");

			size_t rbld_path_len = strlen(device.filename) + 6;
			char rbld_path[rbld_path_len];
			snprintf(rbld_path, rbld_path_len, "%s_rbld", device.filename);
			FILE *rbld = fopen(rbld_path, "w");

			char buf[sizeof(reading_t)];

			if (idx && db && rbld) {
				while ((fread(buf, sizeof(reading_t), 1, idx) != 0)) {
					reading_t *old = (reading_t *)buf;
					char *buf2 = malloc(sizeof(char *) * (size_t)old->size);

					fseek(db, (size_t)old->offset, SEEK_SET);
					fread(buf2, old->size, 1, db);

					reading_t *new = decode_raw_data(buf2, old->size);
					new->offset = old->offset;
					new->size = old->size;

					print_reading(old);
					print_reading(new);
					fwrite(new, sizeof(reading_t), 1, rbld);

					free(new);
					free(buf2);
				}

				fclose(idx), fclose(db), fclose(rbld);
				rename(rbld_path, device.filename);
			}

		}
		free(devices);
	}
}

void
add_entry(char *username, char *password)
{
	char *base = "LocationFinder::FindMyIphone.find";
	int cmd_len = strlen(base) + strlen(username) + strlen(password) + 8;
	char cmd[cmd_len];

	rb_load_file("lib/location_finder/find_my_iphone.rb");
	ruby_exec();
	snprintf(cmd, cmd_len, "%s(\"%s\",\"%s\")", base, username, password);

	VALUE res = rb_eval_string(cmd);
	rb_funcall(0, rb_intern("p"), 1, res);
}

// vim: ts=4:sw=4

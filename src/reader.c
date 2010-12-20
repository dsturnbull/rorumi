#include "src/rorumi.h"
#include "src/reader.h"

#include <stdio.h>

#include <string.h>
#include <sys/stat.h>
#include <dispatch/dispatch.h>

void fill_records(device_t *device);

void
read_indices()
{
	device_t *devices = find_devices(find_rorumi_path(), "idx");

	if (devices != NULL) {
		dispatch_queue_t queue = dispatch_get_global_queue(0, 0);
		dispatch_apply(devices[0].count, queue, ^(size_t i) {
			fill_records(&devices[i]);
		});

		for (int i = 0; i < devices[0].count; i++) {
			device_t device = devices[i];

			unsigned char *name = decoded_name(device.name);
			printf("data for device: %s\n", name);
			free(name);

			for (int j = 0; j < device.reading_count; j++) {
				reading_t *reading = device.readings[j];
				print_reading(reading);
				free(reading);
			}

			//printf("\n");
		}

		free(devices);
	}
}

void
fill_records(device_t *device)
{
	struct stat st;
	stat(device->filename, &st);
	size_t nitems = st.st_size / sizeof(reading_t);

	FILE *fp = fopen(device->filename, "r");
	char buf[sizeof(reading_t)];

	if (fp) {
		device->readings = malloc(sizeof(reading_t) * nitems);

		while ((fread(buf, sizeof(reading_t), 1, fp) != 0)) {
			reading_t *r = malloc(sizeof(reading_t));
			memcpy(r, (reading_t *)buf, sizeof(reading_t));
			device->readings[device->reading_count++] = r;
		}

		fclose(fp);
	}
}

// vim: ts=4:sw=4

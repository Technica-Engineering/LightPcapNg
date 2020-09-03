
#include "light_io.h"
#include "light_pcapng.h"

int light_file_diff(light_file a, light_file b, FILE* out) {

	light_block block_a = NULL;
	light_block block_b = NULL;

	while (1)
	{
		light_read_block(a, &block_a);
		light_read_block(b, &block_b);

		if (block_a == NULL && block_b == NULL) {
			// EOF
			return 0;
		}

		if (!block_a != !block_b) {
			// One of them is null, the other is not
			return block_a - block_b;
		}
		bool type_match = block_a->type == block_b->type;
		bool length_match = block_a->total_length == block_b->total_length;
		bool body_match = *block_a->body == *block_b->body;

		if (!(type_match && length_match && body_match)) {
			fprintf(out, "type match: %d\n", type_match);
			fprintf(out, "length match: %d\n", length_match);
			fprintf(out, "body match: %d\n", body_match);
			return 1;
		}
	}

	return 0;
}
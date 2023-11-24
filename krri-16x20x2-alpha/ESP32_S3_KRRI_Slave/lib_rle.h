// #include <stdint.h>
// #include <string.h>
// #include <stdio.h>

#define RL (128)  /* run length */
#define ROVER (1) /* encoding only run lengths greater than ROVER + 1 */

void dump_buf(uint8_t *buf, int size);

static int get(uint8_t *in, int *in_idx, int in_size)
{
    if (*in_idx < in_size)
        return in[(*in_idx)++];
    else
        return -1;
}

static int put(int ch, uint8_t *out, int *out_idx, int out_size)
{
    if (*out_idx < out_size)
    {
        out[(*out_idx)++] = ch;
        return ch;
    }
    else
        return -1;
}

static int rle_write_buf(uint8_t *out, int *out_idx, int out_size, uint8_t *buf, int len)
{
    if (len == 0) 
        return 0;
    if (put(len + RL, out, out_idx, out_size) < 0)
        return -1;
    for (int i = 0; i < len; i++) {
        if (put(buf[i], out, out_idx, out_size) < 0)
            return -1;
    }
    return 0;
}

static int rle_write_run(uint8_t *out, int *out_idx, int out_size, int count, int ch) 
{
    if (put(count, out, out_idx, out_size) < 0)
        return -1;
    if (put(ch, out, out_idx, out_size) < 0)
        return -1;
    return 0;
}

int rle_encode(uint8_t *in, int in_size, uint8_t *out, int out_size)
{
    int in_idx = 0;    
    int out_idx = 0;
	uint8_t buf[RL] = { 0 }; /* buffer to store data with no runs */
	int idx = 0, prev = -1;
	for (int c = 0; (c = get(in, &in_idx, in_size)) >= 0; prev = c) {
		if (c == prev) { /* encode runs of data */
			int j = 0, k = 0;  /* count of runs */
			if (idx == 1 && buf[0] == c) {
				k++;
				idx = 0;
			}
again:
			for (j = k; (c = get(in, &in_idx, in_size)) == prev && j < RL + ROVER; j++)
				/*loop does everything*/;
			k = 0;
			if (j > ROVER) { /* run length is worth encoding */
				if (idx >= 1) { /* output any existing data */
					if (rle_write_buf(out, &out_idx, out_size, buf, idx) < 0)
						return -1;
					idx = 0;
				}
				if (rle_write_run(out, &out_idx, out_size, j - ROVER, prev) < 0)
					return -1;
			} else { /* run length is not worth encoding */
				while (j-- >= 0) { /* encode too small run as literal */
					buf[idx++] = prev;
					if (idx == (RL - 1)) {
						if (rle_write_buf(out, &out_idx, out_size, buf, idx) < 0)
							return -1;
						idx = 0;
					}
				}
			}
			if (c < 0)
				goto end;
			if (c == prev && j == RL) /* more in current run */
				goto again;
			/* fall-through */
		}
		buf[idx++] = c;
		if (idx == (RL - 1)) {
			if (rle_write_buf(out, &out_idx, out_size, buf, idx) < 0)
				return -1;
			idx = 0;
		}
	}
end: /* no more input */
	if (rle_write_buf(out, &out_idx, out_size, buf, idx) < 0) /* we might still have something in the buffer though */
		return -1;
	return out_idx;
}

int rle_decode(uint8_t *in, int in_size, uint8_t *out, int out_size)
{
    int in_idx = 0;    
    int out_idx = 0;
	for (int c = 0, count = 0; (c = get(in, &in_idx, in_size)) >= 0;) {
		if (c > RL) { /* process run of literal data */
			count = c - RL;
			for (int i = 0; i < count; i++) {
				if ((c = get(in, &in_idx, in_size)) < 0)
					return -1;
				if (put(c, out, &out_idx, out_size) != c)
					return -1;
			}
			continue;
		}
		/* process repeated byte */
		count = c + 1 + ROVER;
		if ((c = get(in, &in_idx, in_size)) < 0)
			return -1;
		for (int i = 0; i < count; i++)
			if (put(c, out, &out_idx, out_size) != c)
				return -1;
	}
	return out_idx;
}
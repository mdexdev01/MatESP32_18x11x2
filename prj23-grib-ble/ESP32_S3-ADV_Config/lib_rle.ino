#define RL (128)  /* run length */
#define ROVER (1) /* encoding only run lengths greater than ROVER + 1 */

int rle_encode(uint8_t *in, int in_size, uint8_t *out, int out_size) {
  uint8_t buf[RL] = { 0 };
  int idx = 0, prev = -1;
  int in_idx = 0, out_idx = 0;

  for (int c = 0; (in_idx < in_size) && ((c = in[in_idx++]) >= 0); prev = c) {
    if (c == prev) {
      int j = 0, k = 0;
      if (idx == 1 && buf[0] == c) {
        k++;
        idx = 0;
      }
again:
      for (j = k; (in_idx < in_size) && ((c = in[in_idx++]) == prev) && j < RL + ROVER; j++)
        ;
      k = 0;
      if (j > ROVER) {
        if (idx >= 1) {
          if (out_idx + idx + 1 > out_size)
            return -1;
          out[out_idx++] = idx + RL;
          memcpy(&out[out_idx], buf, idx);
          out_idx += idx;
          idx = 0;
        }
        if (out_idx + 2 > out_size)
          return -1;
        out[out_idx++] = j - ROVER;
        out[out_idx++] = prev;
      } else {
        while (j-- >= 0) {
          buf[idx++] = prev;
          if (idx == (RL - 1)) {
            if (out_idx + idx + 1 > out_size)
              return -1;
            out[out_idx++] = idx + RL;
            memcpy(&out[out_idx], buf, idx);
            out_idx += idx;
            idx = 0;
          }
        }
      }
      if (in_idx >= in_size)
        goto end;
      if (c == prev && j == RL)
        goto again;
    }
    buf[idx++] = c;
    if (idx == (RL - 1)) {
      if (out_idx + idx > out_size)
        return -1;
      out[out_idx++] = idx + RL;
      memcpy(&out[out_idx], buf, idx);
      out_idx += idx;
      idx = 0;
    }
  }
end:
  if (out_idx + idx > out_size)
    return -1;

  out[out_idx++] = idx + RL;
  memcpy(&out[out_idx], buf, idx);
  out_idx += idx;

  return out_idx;
}

int rle_decode(uint8_t *in, int in_size, uint8_t *out, int out_size) {
  int in_idx = 0, out_idx = 0;

  for (int c = 0, count = 0; in_idx < in_size;) {
    c = in[in_idx++];
    if (c > RL) {
      count = c - RL;
      for (int i = 0; i < count; i++) {
        if (in_idx < in_size) {
          if (out_idx >= out_size)
            return -1;
          out[out_idx++] = in[in_idx++];
        } else {
          return -1;
        }
      }
      continue;
    }

    count = c + 1 + ROVER;
    if (in_idx < in_size) {
      c = in[in_idx++];
      for (int i = 0; i < count; i++) {
        if (out_idx >= out_size)
          return -1;
        out[out_idx++] = c;
      }
    } else {
      return -1;
    }
  }
  return out_idx;
}
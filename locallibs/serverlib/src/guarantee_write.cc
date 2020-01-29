#if HAVE_CONFIG_H
#endif

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

ssize_t gwrite(const char* nick, const char* file, int line, int fd, const void* data, size_t total_len)
{
    ssize_t written = 0;
    while (1) {
        ssize_t written_chunk = write(fd, static_cast<const uint8_t*>(data) + written, total_len - written);
        if (written_chunk == -1) {
            fprintf(stderr, "%s %s:%d write failed %d %s\n", nick, file, line, errno, strerror(errno));
            written = written_chunk;
            break;
        }
        written += written_chunk;
        if (written != (ssize_t)total_len) {
            fprintf(stderr, "%s %s:%d we were unable to write all the data in one chunk. "
                    "will finish on next loops. %zd out of %zu\n",
                    nick, file, line, written, total_len);
        } else {
            break;
        }
    }
    return written;
}

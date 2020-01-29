#ifndef GUARANTEE_WRITE_H
#define GUARANTEE_WRITE_H

/**
  * from man 2 write:
  * write() writes up to count bytes from the buffer pointed buf to the file referred to by the file descriptor fd.
  *
  * The  number  of  bytes  written  may  be  less  than count if,
  * for example, there is insufficient space on the underlying physical medium, or the
  * RLIMIT_FSIZE resource limit is encountered (see setrlimit(2)),
  * or the call was interrupted by a signal handler after  having  written  less  than
  * count bytes.  (See also pipe(7).)
  *
  * gwrite will handle partitial writes and guarantee all the data will be written if 'write' returned > 0
*/
ssize_t gwrite(const char* nick, const char* file, int line, int fd, const void* data, size_t total_len);

#endif // GUARANTEE_WRITE_H

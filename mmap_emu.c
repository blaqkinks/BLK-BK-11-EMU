
// 'mmap'/'munmap' wrapper functions

#include "defines.h"
#include "config.h"

#if HAVE_MMAP

#include <sys/ioctl.h>
#include <sys/mman.h>

#endif

void *file_map (int fd, int writeable, unsigned length) {
void *buffer;

#if HAVE_MMAP

buffer = mmap (0, length, PROT_READ | (writeable ? PROT_WRITE : 0), MAP_SHARED, fd, 0);
if (buffer == MAP_FAILED)	return 0;

#else

if (lseek (fd, 0, SEEK_SET) != 0)
	return 0;			/* failed */

if (! (buffer = (void *) malloc (length)))
	return 0;

if (read (fd, buffer, length) != length) {
	free (buffer);
	return 0;
	}

#endif

return buffer;
}	// file_map

void file_unmap (void *data, unsigned length, int writeable) {

#if HAVE_MMAP

munmap(data, length);

#else

// TODO: if writeable, write data back!
free (data);

#endif
}	// file_unmap


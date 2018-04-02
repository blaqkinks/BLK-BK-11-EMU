#include "defines.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <libintl.h>
#define _(String) gettext (String)

#include "mmap_emu.h"

/* let's operate on 819200 byte images first */
#define SECSIZE 512
#define SECPERTRACK 10

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define DISK_REG	0177130
#define DISK_SIZE	2

/* Why bother, let's memory-map the files! */
typedef struct {
	unsigned length;
	unsigned short * image;
	unsigned short * ptr;
	unsigned char track;
	unsigned char side;
	unsigned char ro;
	unsigned char motor;
	unsigned char inprogress;
	unsigned char crc;
	unsigned char need_sidetrk;
	unsigned char need_sectsize;
	unsigned char cursec;
} disk_t;

disk_t disks[4];
static int selected = -1;

void disk_open(disk_t * pdt, char * name) {
	int fd = open(name, O_RDWR|O_BINARY);
	if (fd == -1) {
		pdt->ro = 1;
		fd = open(name, O_RDONLY|O_BINARY);
	}
	if (fd == -1) {
		perror(name);
		return;
	}
	pdt->length = lseek(fd, 0, SEEK_END);
	if (pdt->length == -1) perror("seek");
	if (pdt->length % SECSIZE) {
		fprintf(stderr, _("%s is not an integer number of blocks: %d bytes\n"), name, pdt->length);
		close(fd);
		return;
	}

	if (! (pdt->image = file_map (fd, !pdt->ro, pdt->length)))
		perror(name);

	if (pdt->ro) {
		fprintf(stderr, _("%s will be read only\n"), name);
	}
}

/* Are there any interrupts to open or close ? */

void disk_init() {
	static char init_done = 0;
	int i;
	if (!init_done) {
		disk_open(&disks[0], floppyA);	
		disk_open(&disks[1], floppyB);	
		disk_open(&disks[2], floppyC);	
		disk_open(&disks[3], floppyD);	
		init_done = 1;
	}
	for (i = 0; i < 4; i++) {
		disks[i].ptr = NULL;
		disks[i].track = disks[i].side =
		disks[i].motor = disks[i].inprogress = 0;
	}
	selected = -1;
}

void disk_finish() {
	int i;
	for (i = 0; i < 4; i++) {
		if (!disks[i].image)
			continue;

		file_unmap (disks[i].image, disks[i].length, !disks[i].ro);

	}	
}

/* The index hole appears for 1 ms every 100 ms,
 */
int index_flag() {
	unsigned msec = pdp.clock / (TICK_RATE/1000);
	return (msec % 100 == 0);
}

#define FILLER 047116
#define LAST 0120641
#define IDXFLAG 0120776
#define DATAFLAG 0120773
#define ENDFLAG 0120770

unsigned short index_marker[] = {
FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER,  
FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, 
0, 0, 0, 0, 0, 0, LAST, IDXFLAG
};

#define IDXMRKLEN (sizeof(index_marker)/sizeof(*index_marker))

unsigned short data_marker[] = {
FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER,
FILLER, FILLER, FILLER, 0, 0, 0, 0, 0, 0, LAST, DATAFLAG
};
#define DATAMRKLEN (sizeof(data_marker)/sizeof(*data_marker))

unsigned short end_marker[] = {
FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER,
FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER,
FILLER, FILLER, 0, 0, 0, 0, 0, 0, LAST, ENDFLAG
};
#define ENDMARKLEN (sizeof(end_marker)/sizeof(*end_marker))

unsigned short
disk_read_word(disk_t * pdt) {
	unsigned short ret;
	if (pdt->need_sidetrk) {
		ret = (pdt->side << 8) | pdt->track;
		pdt->need_sidetrk = 0;
		pdt->need_sectsize = 1;
	} else if (pdt->need_sectsize) {
		ret = ((pdt->cursec+1) << 8) | 2;
		pdt->need_sectsize = 0;
		pdt->ptr = data_marker;
	} else {
		ret = *pdt->ptr++;
		if (pdt->ptr == index_marker + IDXMRKLEN) {
			pdt->need_sidetrk = 1;
		} else if (pdt->ptr == data_marker + DATAMRKLEN) {
			pdt->ptr = pdt->image +
			(((pdt->track * 2 + pdt->side) * SECPERTRACK) + pdt->cursec) * SECSIZE/2;
		} else if (pdt->ptr ==
			pdt->image + (((pdt->track * 2 + pdt->side) * SECPERTRACK) + pdt->cursec+1) * SECSIZE/2) {
			pdt->ptr = end_marker;
		} else if (pdt->ptr == end_marker + ENDMARKLEN) {
			if (++pdt->cursec == SECPERTRACK) pdt->inprogress = 0;
			pdt->ptr = index_marker;
		}
	}
	return ret;
}

static CPU_RES disk_read(c_addr addr, d_word *word) {
	d_word offset = addr - DISK_REG;
	disk_t * pdt = &disks[selected];
	int index;
        switch(offset) {
        case 0: /* 177130 */
	if (selected == -1) {
		fprintf(stderr, _("Reading 177130, returned 0\n"));
		*word = 0;
		break;
	}
	index = index_flag();
	if (index) {
		pdt->ptr = index_marker;
		pdt->cursec = 0;
		pdt->inprogress = pdt->image != 0;
	}
	*word = (pdt->track == 0) |
		((pdt->image != 0) << 1) |
		(pdt->ro << 2) |
		(pdt->inprogress << 7) |
		(pdt->crc << 14) |
		(index << 15);
#if 0
	fprintf(stderr, "Reading 177130 @%06o, returned %06o\n",
		pdp.regs[PC], *word);
#endif
	break;
	case 2: /* 177132 */
		if (pdt->inprogress) {
			*word = disk_read_word(pdt);
#if 0
			fprintf(stderr, "Reading 177132 @%06o, returned %06o\n",
			pdp.regs[PC], *word);
#endif
		} else {
			static d_word dummy = 0x0;
			fprintf(stderr, "?");
			// fprintf(stderr, _("Reading 177132 when no I/O is progress?\n"));
			*word = dummy = ~dummy;
		}
	break;
	}
	return CPU_OK;
}	// disk_read

static CPU_RES disk_write(c_addr addr, d_word word) {
	d_word offset = addr - DISK_REG;
	disk_t * pdt;
	switch (offset) {
	case 0:         /* control port */
		if (word) {
			/* Print a message if something other than turning
			 * all drives off is requested
			 */
			fprintf(stderr, _("Writing 177130, data %06o\n"), word);
		}
		switch (word & 0xf) {
		/* lowest bit set selects the drive */
		case 0: selected = -1; break;
		case 1: default: selected = 0; break;
		case 2: case 6: case 10: case 14: selected = 1; break;
		case 4: case 12: selected = 2; break;
		case 8: selected = 3; break;
		}
		if (selected >= 0) {
			pdt = &disks[selected];
			pdt->inprogress |= !! (word & 0400);
			fprintf(stderr, "Drive %d i/o %s\n", selected,
				pdt->inprogress ? "ON" : "OFF");
		}
		break;
	case 2:
		fprintf(stderr, _("Writing 177132, data %06o\n"), word);
		break;
	}
	return CPU_OK;
}	// disk_write

static CPU_RES disk_bwrite(c_addr addr, d_byte byte) {
	d_word offset = addr - DISK_REG;
	switch (offset) {
	case 0:
		fprintf(stderr, _("Writing byte 177130, data %03o\n"), byte);
		break;
	case 1: 
		fprintf(stderr, _("Writing byte 177131, data %03o\n"), byte);
		break;
	case 2:
		fprintf(stderr, _("Writing byte 177132, data %03o\n"), byte);
		break;
	case 3:
		fprintf(stderr, _("Writing byte 177133, data %03o\n"), byte);
		break;
	}
	return CPU_OK;

}	// disk_bwrite

pdp_qmap q_disk = {
	"disk", "Floppy controller",
	DISK_REG, DISK_SIZE,
	disk_init, disk_read, disk_write, disk_bwrite
	};


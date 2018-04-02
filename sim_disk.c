
#include "defines.h"
#include <libintl.h>

#define _(String) gettext (String)

#define SECSIZE 512
#define SECPERTRACK 10

#define ERRNO 052

#define ADDR 026
#define WCNT 030
#define SIDE 032
#define TRK 033
#define UNIT 034
#define SECTOR 035

void do_disk_io(int drive, int blkno, int nwords, int ioaddr);

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

extern disk_t disks[4];

void fake_disk_io() {
	d_word blkno = pdp.regs[0];
	d_word nwords = pdp.regs[1];
	c_addr ioaddr = pdp.regs[2];
	c_addr parms = pdp.regs[3];
	d_word drive;
	lc_word(parms+UNIT, &drive);
	drive &= 3;
	do_disk_io(drive, blkno, nwords, ioaddr);
}

void fake_sector_io() {
	c_addr parms = pdp.regs[3];
	d_word drive;
	d_word nwords, ioaddr, side, trk, sector;
	int blkno;
        lc_word(parms+UNIT, &drive);
	lc_word(parms+ADDR, &ioaddr);
	lc_word(parms+WCNT, &nwords);
	sector = drive >> 8;
	drive &= 3;
	lc_word(parms+SIDE, &side);
	trk = side >> 8;
	side &= 1;
	blkno = sector-1 + SECPERTRACK * (side + 2 * trk);
	do_disk_io(drive, blkno, nwords, ioaddr);
}

void do_disk_io(int drive, int blkno, int nwords, int ioaddr) {
	fprintf(stderr, _("%s block %d (%d words) from drive %d @ addr %06o\n"),
		nwords & 0100000 ? _("Writing") : _("Reading"), blkno,
		nwords & 0100000 ? -nwords & 0xffff : nwords, drive, ioaddr);
	pdp.psw |= CC_C;
	sl_byte(&pdp, ERRNO, 0);
	if (disks[drive].image == 0) {
		fprintf(stderr, _("Disk not ready\n"));
		sl_byte(&pdp, ERRNO, 6);
	} else if (blkno >= disks[drive].length / SECSIZE) {
		fprintf(stderr, _("Block number %d too large for image size %d\n"),
			blkno, disks[drive].length);
		sl_byte(&pdp, ERRNO, 5);
	} else if (nwords < 0100000) {
		/* positive nwords means "read" */
		int i;
		for (i = 0;
		i < nwords && 256 * blkno + i < disks[drive].length/2;
		i++, ioaddr += 2) {
			if (CPU_OK != sc_word(ioaddr, disks[drive].image[256 * blkno + i])) {
				fprintf(stderr, _("Read failure @ %06o\n"), ioaddr);
				sl_byte(&pdp, ERRNO, 7);
				break;
			}
		}
		if (i == nwords) pdp.psw &= ~CC_C;
	} else if (!disks[drive].ro) {
		int i;
		/* negative nwords means "write" */
		nwords = -nwords & 0xffff;
		for (i = 0;
		i < nwords && 256 * blkno + i < disks[drive].length/2;
		i++, ioaddr += 2) {
			d_word word;
                        if (CPU_OK != lc_word(ioaddr, &word)) {
                                fprintf(stderr, _("Write failure @ %06o\n"), ioaddr);
				sl_byte(&pdp, ERRNO, 7);
                                break;
                        }
			disks[drive].image[256 * blkno + i] = word; 
                }
                if (i == nwords) pdp.psw &= ~CC_C;

	} else {
		fprintf(stderr, _("Disk is read-only\n"));
		sl_byte(&pdp, ERRNO, 1);
	}
	/* make all disk I/O ops 10 ms long */
	pdp.clock += TICK_RATE/100;

  rts_PC(&pdp);
	fprintf(stderr, _("Done\n"));
}

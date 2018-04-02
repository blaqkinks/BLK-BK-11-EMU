#include "defines.h"
#include <fcntl.h>
#include <stdio.h>

// GDG

#include <libintl.h>
#define _(String) gettext (String)

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef linux
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <sys/types.h>
#include <sys/time.h>

#include <sys/uio.h>

#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#define DEVTAP "/dev/net/tun"

static int fd = -1;
static unsigned char plip_buf[1500];
static unsigned char plip_buf_tx[1500];
static unsigned lasttime;

/* PLIP for BK-0010 uses the ability of the BK parallel port
 * to receive 16 bit; that is, from PC to BK a whole byte can
 * be sent at once.
 */
void bkplip_init() {

  if (fd != -1) return;

  fd = open(DEVTAP, O_RDWR);
  if(fd == -1) {
    perror("tundev: tundev_init: open");
    exit(1);
  }

  {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN|IFF_NO_PI;
    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
      perror("tundev: tundev_init: ioctl");
      exit(1);
    }
  }

  lasttime = 0;
}

static int len_left = 0;
static int curbyte = 0;
static int txlen = 0, txbyte = 0;

/*
 * When no data is present, returns 0.
 * If a packet is present, returns its length (a word) with bit 15 set,
 * then its contents (N bytes). Each read returns a new byte, no strobing yet.
 */

static CPU_RES bkplip_read(c_addr addr, d_word *word) {

  fd_set fdset;
  struct timeval tv;
  int ret;

  if (len_left) {
	*word = plip_buf[curbyte];
	curbyte++;
	len_left--;
	return CPU_OK;
  }

  tv.tv_sec = 0;
  tv.tv_usec = 500000;

  FD_ZERO(&fdset);
  FD_SET(fd, &fdset);
  ret = select(fd + 1, &fdset, NULL, NULL, &tv);
  if(ret == 0) {
    *word = 0;
    lasttime = 0;    
    return CPU_OK;
  } 
  ret = read(fd, plip_buf, 1500);  
  if(ret == -1) {
    perror("tun_dev: tundev_read: read");
  }
  curbyte = 0;
  len_left = ret;
  *word = 1<<15 | ret;

  printf("Got packet of length %d\n", ret);
  return CPU_OK;

}

/*
 * Expects a packet length (a word) with bit 15 set,
 * then N bytes. Each write transmits a byte, no strobing yet.
 */
static CPU_RES bkplip_write(c_addr addr, d_word word) {

	if (word & (1<<15)) {
		if (txlen) {
			printf("Sending new packet when %d bytes left from the old one\n",
			txlen);
			return CPU_BUS_ERROR;
		}
		txlen = word & 0x7fff;
		txbyte = 0;
		if (txlen > 1500) {
			printf("Transmit length %d???\n", txlen);
			return CPU_BUS_ERROR;
		}
		return CPU_OK;
	}
	if (!txlen) return CPU_OK;
	plip_buf_tx[txbyte] = word & 0xff;
	txlen--;
	txbyte++;
	if (!txlen) {
		printf("Sending packet of length %d\n", txbyte);
		write(fd, plip_buf_tx, txbyte);
	}
	return CPU_OK;

}

static CPU_RES bkplip_bwrite(c_addr addr, d_byte byte) {

	d_word offset = addr & 1;
	d_word word;
	bkplip_read(addr & ~1, &word);
	if (offset == 0) {
		word = (word & 0177400) | byte;
	} else {
		word = (byte << 8) | (word & 0377);
	}
	return bkplip_write(addr & ~1, word);

}

#else
void bkplip_init()  { }
static CPU_RES bkplip_read(c_addr addr, d_word *word) {
	return CPU_OK;
}

static CPU_RES bkplip_write(c_addr addr, d_word byte) {
	return CPU_OK;
}

static CPU_RES bkplip_bwrite(c_addr addr, d_byte byte) {
	return CPU_OK;
}
#endif

pdp_qmap q_bkplip = {
	"bkplip", "BK parallel port interface",
	PORT_REG, PORT_SIZE, bkplip_init, bkplip_read, bkplip_write, bkplip_bwrite
};


#ifndef CHECKSUM_HPP_
#define CHECKSUM_HPP_

namespace cppbase {
  namespace net {
  /*
   * Taken from TCP/IP Illustrated Vol. 2(1995) by Gary R. Wright and
   * W. Richard Stevens. Page 236
   */
  unsigned short ip_checksum(const char *ip, int len)
  {
	int sum = 0;

	while (len > 1) {
		sum += *((unsigned short*)ip);
		ip += 2;
		if (sum & 0x80000000) {
			sum = (sum & 0xFFFF) + (sum >> 16);
		}
		len -= 2;
	}

	if (len) {
		sum += (unsigned short)*ip;
	}

	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	return ~sum;
  }

} //namespace net
} //namespace cppbase

#endif


#include "UnixAddress.h"

#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "Common.h"

namespace RAMCloud {

/**
 * Construct an IpAddress from the information in a ServiceLocator.
 * \param serviceLocator
 *      The "host" and "port" options describe the desired address.
 * \throw BadIpAddress
 *      The serviceLocator couldn't be converted to an IpAddress
 *      (e.g. a required option was missing, or the host name
 *      couldn't be parsed).
 */
UnixAddress::UnixAddress(const ServiceLocator* serviceLocator)
    : address()
{
        sockaddr_un* addr = reinterpret_cast<sockaddr_un*>(&address);
        addr->sun_family = AF_UNIX;
	const string locator = serviceLocator->getDriverLocatorString();
	memcpy(&addr->sun_path, locator.c_str(), sizeof(addr->sun_path));
}

/**
 * Construct an IpAddress from the two input arguments ip and port.
 * This only works for ip4 version.
 * \param ip
 *      the ip version 4 address for the host. This must be constructed
 *      by putting to gether the 4 bytes of the ip in host order.
 * \param port
 *      the ip port for the host. This must be constructed
 *      by putting to gether the 2 bytes of the port in host order.
 */
//UnixAddress::UnixAddress(const uint32_t ip, const uint16_t port)
//    :address()
//{
/*    sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(&address);
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(ip);
    addr->sin_port = NTOHS(port);*/
//}
/**
 * Return a string describing the contents of this IpAddress (host
 * address & port).
 */
string
UnixAddress::toString() const {
/*    const sockaddr_in *addr = reinterpret_cast<const sockaddr_in*>(&address);
    uint32_t ip = ntohl(addr->sin_addr.s_addr);
    uint32_t port = NTOHS(addr->sin_port);
    return format("%d.%d.%d.%d:%d", (ip>>24)&0xff,
                  (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff, port);
*/
	return "test";
}

} // namespace RAMCloud

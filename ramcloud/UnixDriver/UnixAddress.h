#ifndef RAMCLOUD_UNIXADDRESS_H
#define RAMCLOUD_UNIXADDRESS_H

#include <netinet/in.h>
#include <sys/un.h>

#include "Common.h"
#include "Driver.h"
#include "ServiceLocator.h"

namespace RAMCloud {

/**
 * This class translates between ServiceLocators and IP sockaddr structs,
 * providing a standard mechanism for use in Transport and Driver classes.
 */
class UnixAddress : public Driver::Address {
  public:
    UnixAddress() : address() {}
    explicit UnixAddress(const ServiceLocator* serviceLocator);
    explicit UnixAddress(const sockaddr_un *address) : address(*address) {}
    //explicit UnixAddress(const uint32_t ip, const uint16_t port);
    UnixAddress(const UnixAddress& other)
        : Address(other), address(other.address) {}
    string toString() const;
    sockaddr_un address;
    private:
    void operator=(UnixAddress&);
};

} // end RAMCloud

#endif

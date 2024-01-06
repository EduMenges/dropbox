#include "primary.hpp"

dropbox::PrimaryReplica::PrimaryReplica(const std::string& ip)  {
    sockaddr_in addr = {kFamily, htons(kAdminPort), {inet_addr(ip.c_str())}};

    if (bind(receiver_, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) == -1) {
        throw Binding();
    }

    if (listen(receiver_, 10) == -1) {
        throw Listening();
    }

    SetTimeout(receiver_, kTimeout);
}

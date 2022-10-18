#ifndef _CMD_AUTOMAP_NET_H_
#define _CMD_AUTOMAP_NET_H_ 1

#include <string>
#include <tuple>

std::tuple<std::string, int> ParseHostPort(const std::string& str,
                                           int default_port);

#endif  // _CMD_AUTOMAP_NET_H_

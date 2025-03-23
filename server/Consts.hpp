#pragma once

#include <map>
#include <vector>
#include <string>
#include <utility>  // for std::pair, std::make_pair

extern const std::string kDefaultConfig;
extern const int kMaxBuff;
extern const std::string kDefaultListen;
extern const std::string kDefaultHost;
extern const std::string kDefaultPort;
extern const std::string kDefaultServerName;
extern const std::string kDefaultRoot;
extern const int kDefaultClientTimeout;
extern const int kDefaultClientHeaderBufferSize;
extern const int kDefaultClientMaxBodySize;
extern const std::map<std::string, bool> kDefaultAllowedMethods;
extern const std::vector<std::string> kDefaultIndex;
extern const std::map<int, std::string> kDefaultErrorPages;
extern const bool kDefaultAutoindex;

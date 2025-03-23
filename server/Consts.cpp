#include "Consts.hpp"

const std::string kDefaultConfig = "default.conf";
const int kMaxBuff = 1000;
const std::string kDefaultHost = "0.0.0.0";
const std::string kDefaultPort = "80";
const std::string kDefaultListen = kDefaultHost + ":" + kDefaultPort;
const std::string kDefaultServerName = "";
const std::string kDefaultRoot = "/var/www/html";
const int kDefaultClientTimeout = 75;
const int kDefaultClientHeaderBufferSize = 1024; // 1k
const int kDefaultClientMaxBodySize = 1048576;	 // 1m
const bool kDefaultAutoindex = false;

static std::pair<const std::string, bool> methodPairsArr[] = {
    std::make_pair("GET", true),
    std::make_pair("POST", true),
    std::make_pair("DELETE", true)
};
const std::map<std::string, bool> kDefaultAllowedMethods(
    methodPairsArr,
    methodPairsArr + sizeof(methodPairsArr) / sizeof(methodPairsArr[0]));

static const char *indexArr[] = {"index.html"};
const std::vector<std::string> kDefaultIndex(
	indexArr,
	indexArr + sizeof(indexArr) / sizeof(indexArr[0]));

static const std::pair<const int, std::string> errorPagesArr[] = {
	std::make_pair(404, "/404.html"),
	std::make_pair(500, "/50x.html"),
	std::make_pair(502, "/50x.html"),
	std::make_pair(503, "/50x.html"),
	std::make_pair(504, "/50x.html")};
const std::map<int, std::string> kDefaultErrorPages(
	errorPagesArr,
	errorPagesArr + sizeof(errorPagesArr) / sizeof(errorPagesArr[0]));

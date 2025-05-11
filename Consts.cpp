#include "Consts.hpp"
#include <unistd.h>
#include <limits.h>

// Helper function to get current directory + /www
std::string getCurrentDirPlusWww()
{
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		return std::string(cwd) + "/www";
	}
	return "/var/www/html"; // Fallback if getcwd fails
}

const std::string kDefaultConfig = "conf/default.conf";
const int kMaxBuff = 65536; // 64k
const std::string kDefaultHost = "0.0.0.0";
const std::string kDefaultPort = "80";
const std::string kDefaultListen = kDefaultHost + ":" + kDefaultPort;
const std::string kDefaultServerName = "";
const std::string kDefaultRoot = getCurrentDirPlusWww();
const int kDefaultClientTimeout = 75;
const int kDefaultClientHeaderBufferSize = 2048;  // 2k
const size_t kDefaultClientMaxBodySize = 1048576; // 1m
const bool kDefaultAutoindex = false;

static std::pair<const std::string, bool> methodPairsArr[] = {
	std::make_pair("GET", true),
	std::make_pair("POST", true),
	std::make_pair("DELETE", true)};
const std::map<std::string, bool> kDefaultAllowedMethods(
	methodPairsArr,
	methodPairsArr + sizeof(methodPairsArr) / sizeof(methodPairsArr[0]));

static const char *indexArr[] = {"index.html"};
const std::vector<std::string> kDefaultIndex(
	indexArr,
	indexArr + sizeof(indexArr) / sizeof(indexArr[0]));

static const std::pair<const int, std::string> errorPagesArr[] = {
	std::make_pair(400, "/error/400.html"),
	std::make_pair(404, "/error/404.html"),
	std::make_pair(500, "/error/500.html"),
	std::make_pair(505, "/error/505.html")};
const std::map<int, std::string> kDefaultErrorPages(
	errorPagesArr,
	errorPagesArr + sizeof(errorPagesArr) / sizeof(errorPagesArr[0]));

static const std::pair<const std::string, std::string> statusCodesArr[] = {
	// 1xx Informational - RFC 9110 Section 15.2
	std::make_pair("100", "Continue"),
	std::make_pair("101", "Switching Protocols"),

	// 2xx Success - RFC 9110 Section 15.3
	std::make_pair("200", "OK"),
	std::make_pair("201", "Created"),
	std::make_pair("202", "Accepted"),
	std::make_pair("203", "Non-Authoritative Information"),
	std::make_pair("204", "No Content"),
	std::make_pair("205", "Reset Content"),
	std::make_pair("206", "Partial Content"),

	// 3xx Redirection - RFC 9110 Section 15.4
	std::make_pair("300", "Multiple Choices"),
	std::make_pair("301", "Moved Permanently"),
	std::make_pair("302", "Found"),
	std::make_pair("303", "See Other"),
	std::make_pair("304", "Not Modified"),
	std::make_pair("305", "Use Proxy"),
	std::make_pair("307", "Temporary Redirect"),
	std::make_pair("308", "Permanent Redirect"),

	// 4xx Client Errors - RFC 9110 Section 15.5
	std::make_pair("400", "Bad Request"),
	std::make_pair("401", "Unauthorized"),
	std::make_pair("402", "Payment Required"),
	std::make_pair("403", "Forbidden"),
	std::make_pair("404", "Not Found"),
	std::make_pair("405", "Method Not Allowed"),
	std::make_pair("406", "Not Acceptable"),
	std::make_pair("407", "Proxy Authentication Required"),
	std::make_pair("408", "Request Timeout"),
	std::make_pair("409", "Conflict"),
	std::make_pair("410", "Gone"),
	std::make_pair("411", "Length Required"),
	std::make_pair("412", "Precondition Failed"),
	std::make_pair("413", "Content Too Large"),
	std::make_pair("414", "URI Too Long"),
	std::make_pair("415", "Unsupported Media Type"),
	std::make_pair("416", "Range Not Satisfiable"),
	std::make_pair("417", "Expectation Failed"),
	std::make_pair("418", "I'm a teapot"),
	std::make_pair("421", "Misdirected Request"),
	std::make_pair("422", "Unprocessable Content"),
	std::make_pair("426", "Upgrade Required"),

	// 5xx Server Errors - RFC 9110 Section 15.6
	std::make_pair("500", "Internal Server Error"),
	std::make_pair("501", "Not Implemented"),
	std::make_pair("502", "Bad Gateway"),
	std::make_pair("503", "Service Unavailable"),
	std::make_pair("504", "Gateway Timeout"),
	std::make_pair("505", "HTTP Version Not Supported")};
const std::map<std::string, std::string> kStatusCodes(
	statusCodesArr,
	statusCodesArr + sizeof(statusCodesArr) / sizeof(statusCodesArr[0]));

const size_t kMaxHexLength = 8; // maximum valid chunk size in hex would be "FFFFFFFF" (4GB in hex)
const bool kDefaultKeepAlive = true;

#ifndef CGI_HPP
# define CGI_HPP

# include <string>
# include <map>
# include <iostream>
# include "HttpRequest.hpp"
# include "HttpResponse.hpp"

class CGI
{
	private:
		std::vector<char*> _env;
		std::string _cgiDirectory;
		WebServer *_webserver;

		void setEnvironmentVariables(const HttpRequest &request, const std::string &scriptPath);

	public:
		CGI(const std::string &cgiDirectory, WebServer* webserver);
		CGI(const CGI &src);
		CGI &operator=(const CGI &src);
		~CGI();

		std::string execute(const std::string &scriptPath, const HttpRequest &request);

};

#endif

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
		std::string _scriptPath;
		std::map<std::string, std::string> _env;
		std::string _postData;

		void prepareEnvironmentVariables();

	public:
		CGI(const std::string &scriptPath, const std::map<std::string, std::string> &env, const std::string &postData);
		CGI(const CGI &src);
		CGI &operator=(const CGI &src);
		~CGI();

		std::string execute(std::string serverRoot);
};

#endif

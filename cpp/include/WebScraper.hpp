#ifndef WEB_SCRAPER_HPP
#define WEB_SCRAPER_HPP

#include <nlohmann/json.hpp>
#include <httplib/httplib.h>
#include <string>


class WebScraper{
private:
	std::unique_ptr<httplib::Client> client;

public:
	WebScraper(std::string endpoint);

	nlohmann::json parsePage(std::string seed, std::string url);

};


#endif

#include "WebScraper.hpp"

#include <iostream>

using json = nlohmann::json;


WebScraper::WebScraper(std::string endpoint)
	: client(std::make_unique<httplib::Client>(endpoint)){
}


nlohmann::json WebScraper::parsePage(std::string seed, std::string link){
	std::cout << "parsing " << seed << link << "..." << std::endl;

	json params = {
		  { "seed", seed },
		  { "link", link }
	};

	auto res = this->client->Post("/parsed-pages/", params.dump().c_str(), "application/json");

	if (res && res->status == 201) {
	    std::cout << "...parsed " << seed << link << std::endl;
	    return json::parse(res->body);
	} else {
	    // Handle potential errors
	    std::cerr << "...failed to parse " << seed << link << std::endl;

	    json error = {
	    		{"Status Code", res->status},
				{"Error Details", res->body}
	    };

	    return error;
	}
}

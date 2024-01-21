#include "httplib.h"
#include <string>
#include <vector>
#include <mutex>

const std::string form = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Oliver's URL Shortener</title>
</head>
<body>
    <form action="/submit" method="post">
        <label for="url">URL:</label>
        <input type="text" id="url" name="url">
        <input type="submit" value="Submit">
    </form>
</body>
</html>
)";

std::vector<std::string> urls;

std::mutex url_mutex; //mutex to protect access to urls

void redirect(const httplib::Request& req, httplib::Response& res){ //redirected directory
    size_t index = std::stoi(req.matches[1]);

    std::lock_guard<std::mutex> guard(url_mutex);
    if (index < urls.size()){ //find url at index
        res.set_redirect(urls[index]);
    } else {
        res.status = 404;
        res.set_content("URL not found", "text/plain");
    }
    //lock_guard unlocked
}

int main(){
    httplib::Server svr;

    svr.Get("/", [](const httplib::Request&, httplib::Response& res){ //root
        res.set_content(form, "text/html");
    });

    svr.Post("/submit", [](const httplib::Request& req, httplib::Response& res){ //submit page
        std::string url = req.get_param_value("url");

        if (url.find("http://") != 0 && url.find("https://") != 0){ //start url with http:// or else will treat like directory
            url = "http://" + url; 
        }

        int index = -1;
        { //add url
            std::lock_guard<std::mutex> guard(url_mutex);
            index = urls.size();
            urls.push_back(url);
            //lock_guard unlocks
        }

        res.set_content("Shortened URL: <a href='/" + std::to_string(index) + "'>/" + std::to_string(index) + "</a>", "text/html");
    });

    svr.Get(R"(/(\d+))", redirect); //get directory at index, redirects to urls[index]

    svr.listen("0.0.0.0", 8080); //start server
}

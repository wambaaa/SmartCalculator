#include <algorithm>
#include <pistache/net.h>
#include <pistache/http.h>
#include <pistache/peer.h>
#include <pistache/http_headers.h>
#include <pistache/cookie.h>
#include <pistache/router.h>
#include <pistache/endpoint.h>
#include <pistache/common.h>
#include <chrono>
#include <ctime> 
#include <signal.h>
#include <json/value.h>
#include <fstream>

using namespace std;
using namespace Pistache;


// General advice: pay atetntion to the namespaces that you use in various contexts. Could prevent headaches.

// This is just a helper function to preety-print the Cookies that one of the enpoints shall receive.
void printCookies(const Http::Request& req) {
    auto cookies = req.cookies();
    std::cout << "Cookies: [" << std::endl;
    const std::string indent(4, ' ');
    for (const auto& c: cookies) {
        std::cout << indent << c.name << " = " << c.value << std::endl;
    }
    std::cout << "]" << std::endl;
}

// Some generic namespace, with a simple function we could use to test the creation of the endpoints.
namespace Generic {

    void handleReady(const Rest::Request&, Http::ResponseWriter response) {
        response.send(Http::Code::Ok, "1");
    }

}

// Definition of the CalculatorEnpoint class 
class CalculatorEndpoint {
public:
    explicit CalculatorEndpoint(Address addr)
        : httpEndpoint(std::make_shared<Http::Endpoint>(addr))
    { }

    // Initialization of the server. Additional options can be provided here
    void init(size_t thr = 2) {
        auto opts = Http::Endpoint::options()
            .threads(static_cast<int>(thr));
        httpEndpoint->init(opts);
        // Server routes are loaded up
        setupRoutes();
    }

    // Server is started threaded.  
    void start() {
        httpEndpoint->setHandler(router.handler());
        httpEndpoint->serveThreaded();
    }

    // When signaled server shuts down
    void stop(){
        httpEndpoint->shutdown();
    }

private:
    void setupRoutes() {
        using namespace Rest;
        // Defining various endpoints
        // Generally say that when http://localhost:9080/ready is called, the handleReady function should be called. 
        Routes::Get(router, "/ready", Routes::bind(&Generic::handleReady));
        Routes::Get(router, "/auth", Routes::bind(&CalculatorEndpoint::doAuth, this));
        Routes::Get(router, "/binary-converter/:formula/", Routes::bind(&CalculatorEndpoint::setCalc, this));
        Routes::Get(router, "/luminosity/:lux", Routes::bind(&CalculatorEndpoint::setLum, this));
        Routes::Get(router, "/battery/:bat", Routes::bind(&CalculatorEndpoint::setBat, this));
        Routes::Get(router, "/temp/:temp", Routes::bind(&CalculatorEndpoint::setTemp, this));
        Routes::Get(router, "/datetime/:an/:luna/:zi", Routes::bind(&CalculatorEndpoint::setDate, this));
        Routes::Get(router, "/datetime/", Routes::bind(&CalculatorEndpoint::setCurDate, this));
       
    }
    void setCalc(const Rest::Request& request, Http::ResponseWriter response){
        
        string formula = request.param(":formula").as<std::string>();

        // This is a guard that prevents editing the same value by two concurent threads. 
        Guard guard(calculatorLock);
 
        // Setting the adidasi's setting to value
        int setResponse = clc.initBinar(formula);

 
        // Sending some confirmation or error response.
        if (setResponse == 1) {
            response.send(Http::Code::Ok, "Numarul scris in binar: " + clc.getBinary(stoi(formula)) + "!");
        }
        else {
            response.send(Http::Code::Not_Found, "inputul nu corespunde cerintelor.");
        }
    }
    void setLum(const Rest::Request& request, Http::ResponseWriter response){

        // This is a guard that prevents editing the same value by two concurent threads. 
        Guard guard(calculatorLock);
        string lum = request.param(":lux").as<std::string>();
        // Setting the adidasi's setting to value
        response.send(Http::Code::Ok, "Luminozitatea ta este de: " + clc.getLuminosity(lum) + "!");
     
    }
     void setBat(const Rest::Request& request, Http::ResponseWriter response){

        // This is a guard that prevents editing the same value by two concurent threads. 
        Guard guard(calculatorLock);
        string battery = request.param(":bat").as<std::string>();
        // Setting the adidasi's setting to value
        response.send(Http::Code::Ok, "Bateria ta este de: " + clc.getBattery(battery) + "!");
     
    }
    void setTemp(const Rest::Request& request, Http::ResponseWriter response){

        // This is a guard that prevents editing the same value by two concurent threads. 
        Guard guard(calculatorLock);
        string t = request.param(":temp").as<std::string>();
        // Setting the adidasi's setting to value
        response.send(Http::Code::Ok, "Temperatura in zona ta este de: " + clc.getTemperature(t) + "!");
     
    }
    void setDate(const Rest::Request& request, Http::ResponseWriter response){

        // This is a guard that prevents editing the same value by two concurent threads. 
        Guard guard(calculatorLock);
        string an = request.param(":an").as<std::string>();
        string luna = request.param(":luna").as<std::string>();
        string  zi = request.param(":zi").as<std::string>();
        // Setting the adidasi's setting to value
        response.send(Http::Code::Ok, "Data: " + clc.getDate(stoi(an),stoi(luna),stoi(zi)) + "!");
     
    }
    void setCurDate(const Rest::Request& request, Http::ResponseWriter response){

        // This is a guard that prevents editing the same value by two concurent threads. 
        Guard guard(calculatorLock);
        // Setting the adidasi's setting to value
        response.send(Http::Code::Ok, "Data: " + clc.getCurrentDate() + "!");
     
    }
    
    void doAuth(const Rest::Request& request, Http::ResponseWriter response) {
        // Function that prints cookies
        printCookies(request);
        // In the response object, it adds a cookie regarding the communications language.
        response.cookies()
            .add(Http::Cookie("lang", "en-US"));
        // Send the response
        response.send(Http::Code::Ok);
    }


    // Defining the class of the Calculator. It should model the entire configuration of the Calculator
    class Calculator {
    public:
        explicit Calculator(){ }
        int initBinar(string formula){
            conversii.formula = stoi(formula);
            return 1;
        }
        int initLuminosity(string lumina){
            luminozitate.lumina = lumina;
            return 1;
        }
        string getCurrentDate()
        {
            time_t     now = time(0);
            struct tm  tstruct;
            char       buf[80];
            tstruct = *localtime(&now);
 
            strftime(buf, sizeof(buf), "%d/%m/%Y", &tstruct);

            return buf;
        
        }
       string getDate(int year, int month, int day)
        {
             std::tm tm {} ;
            tm.tm_year = year - 1900 ;
            tm.tm_mon = month - 1 ;
            tm.tm_mday = day ;


            char buffer[128] ;
            std::strftime( buffer, sizeof(buffer), "%d/%m/%Y", std::addressof(tm) ) ;
            return buffer ;
        }
        string getLuminosity(string natLight)
        {
        int light = stoi(natLight);
	    int screenLightLevel;
	    if (light < 40)

		    screenLightLevel = 50;
	    else if (light <= 90)
        {
            screenLightLevel = light + 10;
        }
        else{
            screenLightLevel = 100;
        }

	    return std::to_string(screenLightLevel);
        }
        string getBinary(int n)
    {
        string s = "";
        // Size of an integer is assumed to be 32 bits
        for (int i = 31; i >= 0; i--) {
        int k = n >> i;
            if (k & 1)
              s = s.append("1");
            else
                s = s.append("0");
        
        }

    return s;

    }       
        string getBattery(string b)
        {   
            return b + "%";
        }
        string getTemperature(string temp)
       { 
            return temp + " °C";
        }

       

       
    private:
        // Defining and instantiating settings.
        struct boolSetting{
            std::string name;
            bool value;
            std::string formula;
        }conversii;
    private:
        // Defining and instantiating settings.
        struct dateSetting{
            std::string name;
            bool value;
        }date;
    private:
        // Defining and instantiating settings.
        struct tempSetting{
            std::string name;
            bool value;
        }temperatura;
     private:
        // Defining and instantiating settings.
        struct batSetting{
            std::string name;
            bool value;
        }baterie;
         struct lumSetting{
            std::string name;
            bool value;
            std::string lumina;
        }luminozitate;
    };

    // Create the lock which prevents concurrent editing of the same variable
    using Lock = std::mutex;
    using Guard = std::lock_guard<Lock>;
    Lock calculatorLock;

    // Instance of the  calculator model
    Calculator clc;

    // Defining the httpEndpoint and a router.
    std::shared_ptr<Http::Endpoint> httpEndpoint;
    Rest::Router router;
};

int main(int argc, char *argv[]) {

    // This code is needed for gracefull shutdown of the server when no longer needed.
    sigset_t signals;
    if (sigemptyset(&signals) != 0
            || sigaddset(&signals, SIGTERM) != 0
            || sigaddset(&signals, SIGINT) != 0
            || sigaddset(&signals, SIGHUP) != 0
            || pthread_sigmask(SIG_BLOCK, &signals, nullptr) != 0) {
        perror("install signal handler failed");
        return 1;
    }

    // Set a port on which your server to communicate
    Port port(9080);

    // Number of threads used by the server
    int thr = 2;

    if (argc >= 2) {
        port = static_cast<uint16_t>(std::stol(argv[1]));

        if (argc == 3)
            thr = std::stoi(argv[2]);
    }

    Address addr(Ipv4::any(), port);

    cout << "Cores = " << hardware_concurrency() << endl;
    cout << "Using " << thr << " threads" << endl;
    // Instance of the class that defines what the server can do.
    CalculatorEndpoint stats(addr);

    // Initialize and start the server
    stats.init(thr);
    stats.start();

    // Code that waits for the shutdown sinal for the server
    int signal = 0;
    int status = sigwait(&signals, &signal);
    if (status == 0)
    {
        std::cout << "received signal " << signal << std::endl;
    }
    else
    {
        std::cerr << "sigwait returns " << status << std::endl;
    }

    stats.stop();
}

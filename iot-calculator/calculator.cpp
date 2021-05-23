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
        Routes::Post(router, "/:value", Routes::bind(&CalculatorEndpoint::setSetting, this));
        Routes::Get(router, "/", Routes::bind(&CalculatorEndpoint::getSetting, this));
        Routes::Get(router, "/setCalc/:formula/", Routes::bind(&CalculatorEndpoint::setCalc, this));
        Routes::Get(router, "/setLum/", Routes::bind(&CalculatorEndpoint::setLum, this));
        Routes::Get(router, "/setBat/", Routes::bind(&CalculatorEndpoint::setBat, this));
        Routes::Get(router, "/setTemp/", Routes::bind(&CalculatorEndpoint::setTemp, this));
        Routes::Get(router, "/setDate/", Routes::bind(&CalculatorEndpoint::setDate, this));
       
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
        // Setting the adidasi's setting to value
        response.send(Http::Code::Ok, "Luminozitatea ta este de: " + clc.getLuminosity() + "!");
     
    }
     void setBat(const Rest::Request& request, Http::ResponseWriter response){

        // This is a guard that prevents editing the same value by two concurent threads. 
        Guard guard(calculatorLock);
        // Setting the adidasi's setting to value
        response.send(Http::Code::Ok, "Bateria ta este de: " + clc.getBattery() + "!");
     
    }
    void setTemp(const Rest::Request& request, Http::ResponseWriter response){

        // This is a guard that prevents editing the same value by two concurent threads. 
        Guard guard(calculatorLock);
        // Setting the adidasi's setting to value
        response.send(Http::Code::Ok, "Temperatura in zona ta este de: " + clc.getTemperature() + "°C!");
     
    }
    void setDate(const Rest::Request& request, Http::ResponseWriter response){

        // This is a guard that prevents editing the same value by two concurent threads. 
        Guard guard(calculatorLock);
        // Setting the adidasi's setting to value
        response.send(Http::Code::Ok, "Este ora si data de: " + clc.getCurrentDate() + "!");
     
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

    // Endpoint to configure one of the Calculator's settings.
    void setSetting(const Rest::Request& request, Http::ResponseWriter response){
        // You don't know what the parameter content that you receive is, but you should
        // try to cast it to some data structure. Here, I cast the settingName to string.
        auto settingName = request.param(":settingName").as<std::string>();

        // This is a guard that prevents editing the same value by two concurent threads. 
        Guard guard(calculatorLock);

        
        string val = "";
        if (request.hasParam(":value")) {
            auto value = request.param(":value");
            val = value.as<string>();
        }

        // Setting the calculator's setting to value
        int setResponse = clc.set(settingName, val);

        // Sending some confirmation or error response.
        if (setResponse == 1) {
            response.send(Http::Code::Ok, settingName + " was set to " + val);
        }
        else {
            response.send(Http::Code::Not_Found, settingName + " was not found and or '" + val + "' was not a valid value ");
        }

    }

    // Setting to get the settings value of one of the configurations of the Calculator
    void getSetting(const Rest::Request& request, Http::ResponseWriter response){
        auto settingName = request.param(":settingName").as<std::string>();

        Guard guard(calculatorLock);

        string valueSetting = clc.get(settingName);

        if (valueSetting != "") {

            // In this response I also add a couple of headers, describing the server that sent this response, and the way the content is formatted.
            using namespace Http;
            response.headers()
                        .add<Header::Server>("pistache/0.1")
                        .add<Header::ContentType>(MIME(Text, Plain));

            response.send(Http::Code::Ok, settingName + " is " + valueSetting);
        }
        else {
            response.send(Http::Code::Not_Found, settingName + " was not found");
        }
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
        auto start = std::chrono::system_clock::now();
    // Some coSmputation here
        auto end = std::chrono::system_clock::now();

        std::chrono::duration<double> elapsed_seconds = end - start;
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
        std::string s = std::ctime(&end_time);
        return  s;

        }
        string getLuminosity()
        {
            srand(time(0));
	    int naturalLightLevel = (rand() % 100) + 1;
	    int screenLightLevel;
	    if (naturalLightLevel <= 90)

		    screenLightLevel = naturalLightLevel + 10;
	    else
		    screenLightLevel = 100;

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
        string getBattery()
        {   srand(time(0));
            int batteryLevel = (rand() % 100) + 1;
            return std::to_string(batteryLevel);
        }
        string getTemperature()
        {   srand(time(0));
            int maxi = 35;
	        int mini = -30;
	        int randNum = rand() % (maxi - mini + 1) + mini;
	
            return std::to_string(randNum);
        }

        // Setting the value for one of the settings. Hardcoded for the defrosting option
        int set(std::string name, std::string value){
            if(name == "conversii"){
                conversii.name = name;
                if(value == "true"){
                    conversii.value = true;
                    
                    return 1;
                }
              if(value == "false"){
                    conversii.value = false;
                    return 1;
                }
            }
            if(name == "date"){
                date.name = name;
                if(value == "true"){
                    date.value = true;
                    
                    return 1;
                }
              if(value == "false"){
                    date.value = false;
                    return 1;
                }
            }
            if(name == "temperatura"){
                temperatura.name = name;
                if(value == "true"){
                    temperatura.value = true;
                    
                    return 1;
                }
              if(value == "false"){
                    temperatura.value = false;
                    return 1;
                }
            }
            if(name == "baterie"){
                baterie.name = name;
                if(value == "true"){
                    baterie.value = true;
                    
                    return 1;
                }
              if(value == "false"){
                    baterie.value = false;
                    return 1;
                }
            }
            if(name == "luminozitate"){
                luminozitate.name = name;
                if(value == "true"){
                    luminozitate.value = true;
                    
                    return 1;
                }
              if(value == "false"){
                    luminozitate.value = false;
                    return 1;
                }
            }
            return 0;
        }

        // Getter
        string get(std::string name){
            if (name == "conversii"){
                return std::to_string(conversii.value);
            }
            else{
                return "";
            }
            if (name == "date"){
                //std::to_string(date.value)
                if(date.value)
                    return getCurrentDate();

                return "Mon Jan 01 00:00:00 1950";
            }
            else{
                return "";
            }
            if (name == "temperatura"){
                //std::to_string(temperatura.value)
                if(temperatura.value)
                    return getTemperature();
                
                return "0°C";
            }
            else{
                return "";
            }
            if (name == "baterie"){
                //return std::to_string(baterie.value);
                if(date.value)
                    return getBattery();

                return "0%";
            
            }
            else{
                return "";
            }
            if (name == "luminozitate"){
                //return std::to_string(luminozitate.value);
                if(luminozitate.value)
                    return getLuminosity();

                return "0";
            
            }
            else{
                return "";
            }
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
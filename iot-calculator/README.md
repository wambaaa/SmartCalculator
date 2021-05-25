# Example IoT Device

## Getting Started

Git clone this project in your machine.

### Prerequisites

Build tested on Ubuntu Server. Pistache doesn't support Windows, but you can use something like [WSL](https://docs.microsoft.com/en-us/windows/wsl/install-win10) or a virtual machine with Linux.

You will need to have a C++ compiler. I used g++ that came preinstalled. Check using `g++ -v`

You will need to install the [Pistache](https://github.com/pistacheio/pistache) library.
On Ubuntu, you can install a pre-built binary as described [here](http://pistache.io/docs/#installing-pistache).

### Building

#### Using Make

You can build the `calculator` executable by running `make`.

### Manually

A step by step series of examples that tell you how to get a development env running

You should open the terminal, navigate into the root folder of this repository, and run\
`g++ calculator.cpp -o calc -lpistache -lcrypto -lssl -lpthread`

This will compile the project using g++, into an executable called `calc` using the libraries `pistache`, `crypto`, `ssl`, `pthread`. You only really want pistache, but the last three are dependencies of the former.
Note that in this compilation process, the order of the libraries is important.

### Running

To start the server run\
`./calc`

Your server should display the number of cores being used and no errors.

To test, open up another terminal, and type\
`curl http://localhost:9080/` followed by the function you wish to use, and its parameters. For example:

`curl http://localhost:9080/temp/25`

`curl http://localhost:9080/battery/13`

`curl http://localhost:9080/binary-converter/20`

`curl http://localhost:9080/luminosity/66`

`curl http://localhost:9080/datetime/2012/06/31` sets the current date with the one given through the parameter.

`curl http://localhost:9080/datetime/`   returns current date.
## Built With

* [Pistache](https://github.com/pistacheio/pistache) - Web server

## License

This project is licensed under the Apache 2.0 Open Source Licence - see the [LICENSE](LICENSE) file for details

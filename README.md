# Authors
- [Francisco Bernad](https://github.com/FrBernad)
- [Nicolás Rampoldi](https://github.com/NicolasRampoldi) 
- [Agustín Manfredi](https://github.com/imanfredi)
- [Joaquín Legammare](https://github.com/JoacoLega)

# PERCY HTTP PROXY
**PERCY HTTP PROXY** is a simple **HTTP** proxy daemon. It provides a **configuration system**, that allows the client
to change the daemon setting in **real time**. By default, a configuration client is provided. This client communicates
with the proxy using de **PERCY protocol** defined here. The proxy provides a garbage collector system which cleans up 
inactive connections, increasing its performance. Moreover, the daemon also logs connections and **HTTP**/**POP3**
**sniffed credentials**.

# Installation
Standing on the root project folder run the command `make all`. This will generate the proxy daemon (**httpd**), and a 
client (**httpctl**) that allows to change the proxy settings in real time.

# Usage
Once the **httpd** file has been created, it can be executed to start the proxy daemon using any
of the options listed below for further customization. 

### Proxy Options
- **--doh-ip <address-doh>**
        establishes the DoH server's address. Default: 127.0.0.1.

- **--doh-port <port>**
        establishes the DoH server's port. Default: 8053.

- **--doh-host <hostname>** 
        establishes the value of header Host. Default: localhost.

- **--doh-path <path>** 
        establishes the DoH request path. Default: /getnsrecord.

- **--doh-query <query>** 
        establishes the string query if the DoH request uses the DoH method. Default: ?dns=.

- **-h** 
        prints help and finish

- **-N** 
        disables password dissectors.

- **-l <address-http>** 
        establishes the address where the HTTP proxy serves. Default: all interfaces.

- **-L <address-de-management>** 
        establishes the address where the management service serves. Default: loopback only.

- **-o <management-port>** 
        establishes the port where management service serves. Default: 9090.

- **-p <local-port>** 
        tcp port which listens for incoming HTTP connections. Default: 8080.

- **-v** 
        prints information about the version and finish.


## Management Service
A management service client is provided by the proxy. The **httpctl** file communicates with the management service on the default port and address. These values can be changed passing the ***-L*** and ***-o*** arguments to change de address and port respectively. To ask for the client's help the ***-h*** argument is available. This client allows you to change the proxy settings in real time and retrieve
useful metrics. The provided functionalities are:
- **Request the number of historical connections.**
- **Request the number of concurrent connections.**
- **Request the number of bytes sent.**
- **Request the number of bytes received.**
- **Request the number of total bytes transferred.**
- **Request I/O buffer sizes.**
- **Request selector timeout.**
- **Request the maximum amount of concurrent.**
- **Request the number of failed connections.**
- **Set I/O buffer size.**
- **Set selector timeout.**
- **Enable or disable dissectors.**

## PERCY Protocol
The **PERCY protocol** used to communicate with the proxy management service is defined in the **percy_protocol.txt**
file. This file describes the protocol standards that allows a client to create a successful self-made implementation.

## Testing
The project can be tested by running `make test` on the source directory. The ***test*** directive will analyze the entire project
using ***cppcheck*** and ***scanbuild*** static analysis tools. Moreover, it will analyze all project functions
complexities using ***GNU complexity***. Once the testing is done a directory named ***test_results***
will be created in the root folder containing the following files:
- ***results.ccpcheck***
- ***results.complexity***
- ***scanbuild_results:*** contains results in a nice html page in case any report was made.
- ***results.sb***

To remove the test results, run `make cleanTest` on the source folder.

# Authors
- [Francisco Bernad](https://github.com/FrBernad)
- [Nicolás Rampoldi](https://github.com/NicolasRampoldi) 
- [Agustín Manfredi](https://github.com/imanfredi)
- [Joaquín Legammare](https://github.com/JoacoLega)

# PERCY HTTP PROXY
**PERCY HTTP PROXY** is a simple *HTTP* proxy daemon. It provides a *configuration system*, that allows the client
to change the daemon setting in *real time*. By default a configuration client is provided. This client communicates
with the proxy using de *PERCY protocol* defined here. Moreover, the daemon also logs connections and *HTTP* and *POP3*
*sniffed credentials*.

# Installation
Standing on the root project folder run the command `make all`. This will generate the proxy daemon (**httpd**) and a 
client (**httpctl**) that allows to change the proxy settings in real time.

# Usage
Once the **httpd** file has been created, it can be executed to start the proxy daemon. 

## Proxy Options
- **--doh-ip <dirección-doh>**
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
        disables password disectors.

- **-l <dirección-http>** 
        establishes the address where the HTTP proxy serves. Default: all interfaces.

- **-L <dirección-de-management>** 
        establishes the address where the management service serves. Default: loopback only.

- **-o <puerto-de-management>** 
        establishes the port where management service serves. Default: 9090.

- **-p <puerto-local>** 
        tcp port which listens for incoming HTTP connections. Default: 8080.

- **-v** 
        prints information about the version and finish.


## Management Service
A management service client is provided by the proxy. The **httpctl** file communicates with the maganement service
on the default port and address. This client allows you to change the proxy settings in real time and retrieve
usefull information. The provided functionalities are:
- **Request the number of historical connections.**
- **Request the number of concurrent connections.**
- **Request the number of bytes sent.**
- **Request the number of bytes received.**
- **Request the number of total bytes transfered.**
- **Request I/O buffer sizes.**
- **Request selector timeout.**
- **Request the maximum amount of concurrent.**
- **Request the number of failed connections.**
- **Set I/O buffer size.**
- **Set selector timeout.**
- **Enable or disable disectors.**

## PERCY Protocol
The **PERCY protocol** used to communicate with the proxy management service is defined in the **percy_protocol.txt**
file. This file describes the protocol standards that allows a client to create a succesful self made implementation.

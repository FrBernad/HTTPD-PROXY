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
  
        establece la dirección del servidor DoH.  Por defecto 127.0.0.1.

- **--doh-port <port>**
  
        establece el puerto del servidor DoH.  Por defecto 8053.

- **--doh-host <hostname>** 

        establece el valor del header Host.  Por defecto localhost.

- **--doh-path <path>** 

        establece el path del request doh.  por defecto /getnsrecord.

- **--doh-query <query>** 

        establece el query string si el request DoH utiliza el método Doh por defecto ?dns=.

- **-h** 

        imprime la ayuda y termina.

- **-N** 

        deshabilita los passwords disectors.

- **-l <dirección-http>** 

        establece la dirección donde servirá el proxy HTTP.  Por defecto escucha en todas las interfaces.

- **-L <dirección-de-management>** 

        establece la dirección donde servirá el servicio de management. Por defecto escucha únicamente en loopback.

- **-o <puerto-de-management>** 

        puerto donde se encuentra el servidor de management.  Por defecto el valor es 9090.

- **-p <puerto-local>** 

        puerto TCP donde escuchará por conexiones entrantes HTTP.  Por defecto el valor es 8080.

- **-v** 

        imprime información sobre la versión versión y termina.


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

## Testing
TODO

## PERCY Protocol
The **PERCY protocol** used to communicate with the proxy management service is defined in the **percy_protocol.txt**
file. This file describes the protocol standards that allows a client to create a succesful self made implementation.

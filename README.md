# fiesta-filter - A packet filter library for Fiesta Online

## Purpose
This library can be used to easily set up "Packet Filters", both on the client side and server side.

A "Packet Filter" allows you to inspect all traffic between a client and a server and react to it.
Possible reactions could be:
	* Drop a packet entirely, so it never reaches the other end
	* Rewrite a packet, changing its contents and/or size
	* Disconnect a client that sends invalid packets
	* Inject additional packets between the client and server

Fiesta servers are incredibly vulnerable to a variety of packet sending attacks. There are a few ways to mitigate these, for example by editing the server binaries to detect malicious packets and handle them properly. This requires significant reverse engineering expertise and isn't a viable option for most people.

A much easier way to handle them is to prevent them from ever reaching the server in the first place, by detecting them with a Packet Filter, dropping the malicious packets and potentially disconnecting or banning offending clients.

Filters can also be used to build custom features beyond the base game, for example a filtered auto-pickup.
To implement this, we just have to react to "Item Drop" packets from the server and inject an "Item Pickup" packet to the server if the dropped item matches our filter criteria.

I hope that by publicly releasing this library, people will contribute with fixes to these attacks, so everyone can make their server more secure and robust.

I'd also like to remind you here that this project is under the GNU GPL 3.0, meaning any public releases based on this project MUST include the same license and their source code MUST be publicly released.

Even for private modifications, I'd appreciate if you open pull requests for all new fixes, protections and additional features you implement, so that the entire community can benefit.

## Building
To build this project, simply install Visual Studio 2022 Community Edition with C++ build support and open the "Filters.sln" solution.

To start a new project that uses the library, simply create a new project (either as part of the solution or separate) and set the corresponding include path ("FilterLib/include"), the library paths ("bin/x64/Release" or "bin/x64/Debug") and link with "FilterLib.lib" and "ws2_32.lib"

## Usage

To use the library, first either build the SampleFilter project or your own custom project. Then start the resulting binary with the correct command line arguments.

### Command Line Arguments

You can automatically log information to the console or to a file. Which types of messages will be logged depends on the "Log Level" you set.
* Info is for common, generic information
* Status is for status information, like a new connection or disconnect
* Warning is for non-critical problems
* Error is for significant erros with the functioning of the program, e.g. that terminate a connection

```
--log-to-console=Info|Status|Warning|Error|None - Default: Status
--log-to-file=Info|Status|Warning|Error|None - Default: Status
```

Example

```
--log-to-console=Info
```

For server-side use, you need to set the path to the ServerInfo.txt used by the server.

```
"--server-config=My Path/To/ServerInfo.txt"
```

For client-side use, you instead need to provide the ip and port to the login server, as well as the local port to bind to (that the client will connect to, usually 9010.

```
--bind-port=9010
--server-ip=123.123.123.123
--server-port=9010
```

### ServerInfo

For server side use, you will need to adjust the ServerInfo.txt to include configuration for the filters.

"ExampleServerInfo.txt" is provided as a guide.

First, define the new tokens at the top, below all other tokens:

```
#DEFINE TUNNEL_BINDING
  <INTEGER>
  <STRING>
#ENDDEFINE
        
#DEFINE TUNNEL_INFO
  <STRING>
  <INTEGER>
  <INTEGER>
  <INTEGER>
  <INTEGER>
  <INTEGER>
#ENDDEFINE
```

Then, add a TUNNEL_BINDING with a specific id and your public IP:

```
; TUNNEL_BINDING <BINDING_ID>,              <IP>
TUNNEL_BINDING              0, "123.123.123.123"
```

Then, for each server that needs to be filtered (currently needs to be EVERY server), add a TUNNEL_INFO, ensuring that you match the BINDING_ID set earlier. Example given for the login server:

```
; TUNNEL_INFO      <NAME>, <TARGET_SERVER_TYPE>, <TARGET_WORLD>, <TARGET_ZONE>, <BINDING_ID>, <PORT>
TUNNEL_INFO    "PG_Login",                    4,              0,             0,            0,   9010
```

Additionally, we need to make sure the login server isn't accessible publicly (setting the ip to 127.0.0.1) and is running on a different port, so it doesn't conflict with the tunnel. Adjust the corresponding SERVER_INFO entry (with source server type = 20)

```
SERVER_INFO  "PG_Login",     4, 0, 0,20,  "127.0.0.1",   9012,  100,   200  ; From Client
```

After these changes, ALL entries for ALL servers should bind to 127.0.0.1, only tunnels should refer to the  TUNNEL_BINDING that has a public IP.

## Known Issues

TODO
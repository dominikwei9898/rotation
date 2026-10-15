# Fiery Path Sample Client

This client implements a subset of Fiery Path API to demonstrate basic usage. The purpose is to provide a starting point and reference when creating a Fiery Path client. Fiery Path client can be written in any language that supports creating a REST client. In this case, TypeScript has been used


## Table of contents

- Requirements
- Installation
- Sources
- Running/Debugging (console)
- Building/Running/Debugging (Visual Studio 2022)
- Building/Running/Debugging (Visual Studio Code)


## Requirements

1. Node.js version 18.16.0 or later
2. NPM version 9.5.1 or later (NPM will be installed with Node.js)


## Installation

1. On a command-line where the source is copied, run "npm install"

This will download all the necessary node_modules required to run the client


## Sources

There are 4 source files for this project.

1. app.ts - this is the file that contains the code to interpret the command-line options, setup logging and start the printer client
2. session.ts - it contains the code to establish connection for Fiery Path including login and setup of websocket connection
3. printer.ts - it implements the necessary code to initiate the session login, send a few configurations and job related printing API's
4. datapath.ts - it implements the websocket server and internal protocol used to communicate between this client and the Data Path FITS Plugin

The corresponding JavaScript files, generated from the TypeScript files (app.js, session.js, printer.js and datapath.js) have been provided with the sources.


## Running/Debugging (console)

1. The client needs a minimum of 4 command-line parameters - ip, username, password and accessrights
2. node app.js "ip=XX.XX.XX.XX" "username=ServerUsername" "password=ServerPassword" "accessrights=AccessRightsProvided". For production, please add encryption/decryption for the username and password being passed.
3. Logs for the client get created under the "logs" directory. However, if you want to see the output on the console as well, add a 5th option on the command-line - "log-console=true"
4. Use Chrome for debugging -> node --inspect-brk=0.0.0.0:9229 app.js "ip=XX.XX.XX.XX" "username=ServerUsername" "password=ServerPassword" "accessrights=AccessRightsProvided"

## Productization

To productize this code, consider the following 2 actions:
1. Encrypt credentials (username, password and accessrights) and decrypt them in app.ts
2. Obfuscate the generated JavaScript files into a single JavaScript file. Webpack can be used for this purpose 


## Building/Running/Debugging (Visual Studio 2022)

1. It is recommended that Visual Studio 2022 be used to build this client
2. Open the .sln file in Visual Studio 2022
3. It is necessary that "npm install" was already run prior to this step as documented above
4. Build the client using -> F7
5. Update launch.json file "args" to update the 4 required arguments
6. Run or Debug from the IDE

## Building/Running/Debugging (Visual Studio Code)

1. Open the directory where the sources are placed in Visual Studio Code
2. It is necessary that "npm install" was already run prior to this step as documented above
3. Build the client from Terminal within the IDE using -> node_modules/.bin/tsc
4. Update launch.json file "args" to update the 4 required arguments
5. Run or Debug from the IDE



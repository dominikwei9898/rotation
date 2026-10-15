"use strict";
/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const winston = __importStar(require("winston"));
const path_1 = require("path");
const fs = __importStar(require("fs"));
const commander_1 = require("commander");
const moment_1 = __importDefault(require("moment"));
///////////////////////////////////////////////////////////////////////////////////
// Command-line Parameters
///////////////////////////////////////////////////////////////////////////////////
const program = new commander_1.Command();
program.parse(process.argv);
// Get all key-value pairs passed via the command line. Command-line parameters need to be passed in as "key1=value1 key2=value2" format
const keyValuePairs = program.args.map((arg) => {
    const key = arg.substring(0, arg.indexOf('='));
    const value = arg.substring(arg.indexOf('=') + 1);
    return { key, value };
});
// These are the required command-line parameters. ip is the Fiery server IP address, username/password/accessrigts are required for login access 
const requiredKeys = ['ip', 'username', 'password', 'accessrights'];
// Check if all required parameters are present
const missingKeys = requiredKeys.filter((key) => !keyValuePairs.some((pair) => pair.key === key));
// If not, display message and exit
if (missingKeys.length > 0 || program.args.length === 0) {
    console.error('Error: Missing required parameters. Please provide values for ip, username, password and accessrights in the format ip=value1 username=value2 password=value3 accessrights=value4.');
    process.exit(1);
}
const m_settings = {
    'log-level': 'info',
    'log-filename': (0, path_1.resolve)(__dirname, "./logs/printer.log"),
    'log-maxsize': 10 << 20,
    'log-maxFiles': 5,
    'log-console': false // Log to console or not
};
// Apply command-line overrides if any are provided
keyValuePairs.forEach(({ key, value }) => {
    m_settings[key] = value;
});
// Do not allow required parameters to be empty
for (const key of requiredKeys) {
    let strValue = m_settings[key];
    if (strValue.length === 0) {
        console.error(`Error: Required parameter ${key} cannot be empty`);
        process.exit(1);
    }
}
///////////////////////////////////////////////////////////////////////////////////
// Winston Logging
///////////////////////////////////////////////////////////////////////////////////
if (!fs.existsSync((0, path_1.resolve)(__dirname, './logs'))) {
    fs.mkdirSync((0, path_1.resolve)(__dirname, './logs'));
}
function addWinstonFile(nameP, levelP, filenameP, maxSizeP, maxFilesP) {
    winston.add(winston.transports.File, {
        name: nameP,
        level: levelP,
        filename: filenameP,
        json: false,
        maxsize: maxSizeP,
        maxFiles: maxFilesP,
        prettyPrint: true,
        tailable: true,
        timestamp: function () {
            return (0, moment_1.default)().format("MMDD HH:mm:ss.SSSSSSS");
        },
    });
}
// Use custom log levels based on npm levels
const log_levels = { error: 0, warn: 1, info: 2, debug: 3 };
winston.cli();
winston.setLevels(log_levels);
if (!m_settings['log-console'])
    winston.remove(winston.transports.Console);
addWinstonFile("main", m_settings['log-level'], m_settings['log-filename'], m_settings['log-maxsize'], m_settings['log-maxFiles']);
winston.info("");
winston.info("-----------------------------------------------------------------------------------------------------------------------------------------------------");
winston.info("------------------------------------------------------" + "Starting Fiery Path Printer Client at " + ((0, moment_1.default)().format("YYYY/MM/DD HH:mm:ss")) + "----------------------------------------------------");
winston.info("-----------------------------------------------------------------------------------------------------------------------------------------------------");
///////////////////////////////////////////////////////////////////////////////////
// Printer Interface
///////////////////////////////////////////////////////////////////////////////////
const printer_1 = require("./printer");
const m_printer_ifc = new printer_1.PrinterIfc(m_settings);
//# sourceMappingURL=app.js.map
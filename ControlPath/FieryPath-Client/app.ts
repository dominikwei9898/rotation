/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/

import * as winston from 'winston';
import { resolve } from 'path';
import * as fs from 'fs';
import { Command } from 'commander';

import moment from 'moment';

///////////////////////////////////////////////////////////////////////////////////
// Command-line Parameters
///////////////////////////////////////////////////////////////////////////////////

const program = new Command();
program.parse(process.argv);

// Get all key-value pairs passed via the command line. Command-line parameters need to be passed in as "key1=value1 key2=value2" format
const keyValuePairs = program.args.map((arg) =>
{
    const key = arg.substring(0, arg.indexOf('='));
    const value = arg.substring(arg.indexOf('=') + 1);
    return { key, value };
});

// These are the required command-line parameters. ip is the Fiery server IP address, username/password/accessrigts are required for login access 
const requiredKeys = ['ip', 'username', 'password', 'accessrights'];

// Check if all required parameters are present
const missingKeys = requiredKeys.filter((key) => !keyValuePairs.some((pair) => pair.key === key));

// If not, display message and exit
if (missingKeys.length > 0 || program.args.length === 0)
{
    console.error('Error: Missing required parameters. Please provide values for ip, username, password and accessrights in the format ip=value1 username=value2 password=value3 accessrights=value4.');
    process.exit(1);
}

// All required parameters are provided, we can continue. Define an object to hold command-line parameters
export interface CommandLineParams
{
    [key: string]: string | number | boolean;
}

const m_settings: CommandLineParams =
{
    'log-level': 'info',                                        // winston log level
    'log-filename': resolve(__dirname, "./logs/printer.log"),   // name of log file
    'log-maxsize': 10 << 20,                                    // 10 MB files
    'log-maxFiles': 5,                                          // 5 files max before it rotates
    'log-console': false                                        // Log to console or not
};

// Apply command-line overrides if any are provided
keyValuePairs.forEach(({ key, value }) =>
{
    m_settings[key] = value as string | number;
});

// Do not allow required parameters to be empty
for (const key of requiredKeys)
{
    let strValue = m_settings[key] as string
    if (strValue.length === 0)
    {
        console.error(`Error: Required parameter ${key} cannot be empty`);
        process.exit(1);
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Winston Logging
///////////////////////////////////////////////////////////////////////////////////

if (!fs.existsSync(resolve(__dirname, './logs'))) {
    fs.mkdirSync(resolve(__dirname, './logs'));
}

function addWinstonFile(nameP: string, levelP: string, filenameP: string, maxSizeP: number, maxFilesP: number)
{
    winston.add(winston.transports.File,
        {
            name: nameP,
            level: levelP,
            filename: filenameP,
            json: false,
            maxsize: maxSizeP,
            maxFiles: maxFilesP,
            prettyPrint: true,
            tailable: true,
            timestamp: function () 
            {
                return moment().format("MMDD HH:mm:ss.SSSSSSS");
            },
        });
}

// Use custom log levels based on npm levels
const log_levels = { error: 0, warn: 1, info: 2, debug: 3 };

winston.cli();
winston.setLevels(log_levels);

if (!m_settings['log-console'])
    winston.remove(winston.transports.Console);

addWinstonFile("main", m_settings['log-level'] as string, m_settings['log-filename'] as string, m_settings['log-maxsize'] as number, m_settings['log-maxFiles'] as number);

winston.info("");
winston.info("-----------------------------------------------------------------------------------------------------------------------------------------------------");
winston.info("------------------------------------------------------" + "Starting Fiery Path Printer Client at " + (moment().format("YYYY/MM/DD HH:mm:ss")) + "----------------------------------------------------");
winston.info("-----------------------------------------------------------------------------------------------------------------------------------------------------");

///////////////////////////////////////////////////////////////////////////////////
// Printer Interface
///////////////////////////////////////////////////////////////////////////////////

import { PrinterIfc } from './printer';
const m_printer_ifc = new PrinterIfc(m_settings);



/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/

import * as winston from 'winston';
import { resolve, dirname, join } from 'path';
import * as fs from 'fs';
import { Command } from 'commander';
import moment from 'moment';
import { spawnSync } from 'child_process';
import { PrinterIfc } from './printer';

///////////////////////////////////////////////////////////////////////////////////
// Command-line Parameters
///////////////////////////////////////////////////////////////////////////////////

const program = new Command();
program.parse(process.argv);

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

///////////////////////////////////////////////////////////////////////////////////
// Winston Logging
///////////////////////////////////////////////////////////////////////////////////

if (!fs.existsSync(resolve(__dirname, './logs'))) 
{
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

// Get all key-value pairs passed via the command line. Command-line parameters need to be passed in as "key1=value1 key2=value2" format
const keyValuePairs = program.args.map((arg) =>
{
    const key = arg.substring(0, arg.indexOf('='));
    const value = arg.substring(arg.indexOf('=') + 1);
    return { key, value };
});

// These are the required command-line parameters. ip is the Fiery server IP address, username/password/accessrigts are required for login access 
const requiredKeys = ['ip', 'username', 'password', 'accessrights'];
const embeddedKey = 'embedded';

// Helper to check for embedded mode
const isEmbedded = keyValuePairs.length === 1 && keyValuePairs[0].key === embeddedKey && keyValuePairs[0].value === 'true';

// Helper to check for all required keys
const hasAllRequired = requiredKeys.every((key) => keyValuePairs.some((pair) => pair.key === key));

// Error handling for invalid parameter combinations
if (!(isEmbedded || hasAllRequired)) 
{
    const errorMsg =
        'Error: Invalid parameters. Provide either:\n' +
        '  1. All of ip, username, password, accessrights in the format ip=value1 username=value2 password=value3 accessrights=value4\n' +
        '  2. Or just embedded=true';
    winston.error(errorMsg);
    console.error(errorMsg);
    process.exit(1);
}

// If embedded, retrieve credentials from CredTool.exe
let continueProcessing = true;
if (isEmbedded) 
{
    // Possible locations for CredTool.exe if installed via DataXPress
    const exeNames = [
        resolve(__dirname, 'CredTool.exe'),
        resolve(__dirname, '../../DataXPress-install/CredTool.exe')
    ];

    let credResult: any = null;
    for (const exePath of exeNames) 
    {
        if (fs.existsSync(exePath)) 
        {
            const result = spawnSync(exePath, ['-get'], { encoding: 'utf8' });
            if (result.status === 0 && result.stdout) 
            {
                try 
                {
                    credResult = JSON.parse(result.stdout);
                    break;
                }
                catch (err)
                {
                    // Try next location
                    winston.error(`Failed to parse credentials from CredTool at ${exePath}: ${err}`);
                }
            }
        }
    }

    if (!credResult || !credResult.ip || !credResult.username || !credResult.password || !credResult.key) 
    {
        const errorMsg = 'Unable to retrieve credentials';
        winston.error(errorMsg);
        console.error(errorMsg);

        continueProcessing = false;
    }
    else
    {
        // Map JSON keys to required keys
        m_settings['ip'] = credResult.ip;
        m_settings['username'] = credResult.username;
        m_settings['password'] = credResult.password;
        m_settings['accessrights'] = credResult.key;
        winston.info('Credentials retrieved');
    }
}
else
{
    // Apply command-line overrides
    keyValuePairs.forEach(({ key, value }) => 
    {
        m_settings[key] = value as string | number;
    });

    // Do not allow required parameters to be empty
    for (const key of requiredKeys) 
    {
        let strValue = m_settings[key] as string;
        if (!strValue || strValue.length === 0) 
        {
            const errorMsg = `Error: Required parameter ${key} cannot be empty`;
            winston.error(errorMsg);
            console.error(errorMsg);

            continueProcessing = false;
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Printer Interface
///////////////////////////////////////////////////////////////////////////////////

if (continueProcessing)
{
    winston.info(`Connecting to Fiery Server ${m_settings['ip']}`);
    const m_printer_ifc = new PrinterIfc(m_settings);
}
else
{
    winston.error('Exiting due to invalid parameters...');
    console.error('Exiting due to invalid parameters...');

    // Flush Winston logs before exiting
    Object.keys(winston.transports).forEach((key) => 
    {
        const transport = (winston as any).transports[key];
        if (transport && transport.close) 
        {
            transport.close();
        }
    });

    // Give Winston a moment to flush
    setTimeout(() => process.exit(1), 100);
}
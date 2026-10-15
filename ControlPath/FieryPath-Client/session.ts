/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/

import * as fs from 'fs';
import * as https from 'https';
import { Agent } from 'https';

import request, { OptionsWithUrl } from 'request';
import WebSocket from 'ws';

///////////////////////////////////////////////////////////////////////////////////
// Interface definitions
///////////////////////////////////////////////////////////////////////////////////

export interface Credentials
{
    username: string;
    password: string;
    accessrights:
    {
        a1: string;
    };
}

///////////////////////////////////////////////////////////////////////////////////
// Class that implements the Fiery Path REST API's
///////////////////////////////////////////////////////////////////////////////////

export class FieryPathSession
{
    private ws: WebSocket | null;
    private m_initialized: boolean = false;
    private m_parent: any;
    private m_info_logger: ((str: string, parent: any) => void) | null;
    private m_warn_logger: ((str: string, parent: any) => void) | null;
    private m_error_logger: ((str: string, parent: any) => void) | null;
    private m_debug_logger: ((str: string, parent: any) => void) | null;
    private m_loginCallback: ((parent: any) => void) | null;
    private m_eventCallback: ((method: string, message: any, parent: any) => void) | null;
    private m_credentials: Credentials;
    private m_counter: number = 0;
    private m_defer_list_map: Map<string, any> = new Map();
    private m_token: string = '';
    private m_agent: https.Agent | null;
    private m_http_url: string;
    private m_ws_url: string;
    private m_ip: string;
    private m_login_in_progress: boolean;

    constructor()
    {
        // Initialize member variables
        this.m_info_logger = null;
        this.m_warn_logger = null;
        this.m_error_logger = null;
        this.m_debug_logger = null;
        this.m_loginCallback = null;
        this.m_eventCallback = null;

        this.m_ip = '';
        this.ws = null;
        this.m_agent = null;
        this.m_http_url = '';
        this.m_ws_url = '';
        this.m_login_in_progress = false;

        this.m_credentials = { username: '', password: '', accessrights: { a1: '' } };

        // Listen for SIGINT event to exit cleanly
        process.on('SIGINT', this.exitHandler.bind(this));
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Interface
    ///////////////////////////////////////////////////////////////////////////////////

    // Initializes the session using the parameters passed, this function expects certain callbacks to be defined in the caller class
    public Initialize(parent: any, ip: string, credentials: Credentials): boolean
    {
        if (parent?.InfoCallback && parent?.WarnCallback && parent?.ErrorCallback && parent?.DebugCallback && parent?.LoginCallback && parent?.EventCallback && ip && credentials) 
        {
            this.m_parent = parent;
            this.m_info_logger = parent.InfoCallback;
            this.m_warn_logger = parent.WarnCallback;
            this.m_error_logger = parent.ErrorCallback;
            this.m_debug_logger = parent.DebugCallback;
            this.m_loginCallback = parent.LoginCallback;
            this.m_eventCallback = parent.EventCallback;
            this.m_ip = ip;
            this.m_credentials = credentials;

            this.m_counter = 0;
            this.m_defer_list_map = new Map();
            this.setupOptions();
            this.m_initialized = true;
        }

        return this.m_initialized;
    }

    // Initiates the login process
    public Login(): void
    {
        setTimeout(() => this.LoginCallback(), 100);
    }

    // Gets a value from the server
    public Get(route: string, message: any): Promise<any>
    {
        return new Promise<any>((resolve, reject) => {
            request.get(this.createRequestOptions(route, message), (err: any, res: any, body: any) =>
            {
                if (err)
                    reject(err);
                else
                    resolve(body);
            });
        });
    }

    // Post a value to the server
    public Post(route: string, message: any): Promise<any>
    {
        return new Promise<any>((resolve, reject) =>
        {
            request.post(this.createRequestOptions(route, message), (err: any, res: any, body: any) =>
            {
                if (err)
                    return reject(err);
                return resolve({ body: body, status: res });
            });
        });
    }

    // Put a value to the server
    public Put(route: string, message: any): Promise<any>
    {
        return new Promise<any>((resolve, reject) =>
        {
            request.put(this.createRequestOptions(route, message), (err: any, res: any, body: any) =>
            {
                if (err)
                    return reject(err);
                return resolve({ body: body, status: res });
            });
        });
    }

    // Delete a value from the server
    public Delete(route: string, message: any): Promise<any>
    {
        return new Promise<any>((resolve, reject) =>
        {
            request.del(this.createRequestOptions(route, message), (err: any, res: any, body: any) =>
            {
                if (err)
                    return reject(err);
                return resolve(body);
            });
        });
    }

    // Post a file to the server
    public PostFile(route: string, fileToSend: string): Promise<any>
    {
        return new Promise<any>((resolve, reject) =>
        {
            if (fileToSend)
            {
                const req = request.post(this.createFileRequestOptions(route), (err: any, res: any, body: any) =>
                {
                    if (err)
                        return reject(err);
                    return resolve(body);
                });

                const form = req.form();
                form.append('file', fs.createReadStream(fileToSend));
            }
            else
            {
                // Handle the case when 'fileToSend' is not provided
                reject(new Error('File path is not provided'));
            }
        });
    }

    // Post multiple files to the server
    public PostMultiFile(route: string, filesToSend: Record<string, string>): Promise<any>
    {
        return new Promise<any>((resolve, reject) =>
        {
            if (filesToSend)
            {
                const req = request.post(this.createFileRequestOptions(route), (err: any, res: any, body: any) =>
                {
                    if (err)
                        return reject(err);
                    return resolve(body);
                });

                const form = req.form();
                for (const name in filesToSend)
                {
                    if (filesToSend.hasOwnProperty(name))
                    {
                        form.append(name, fs.createReadStream(filesToSend[name]));
                    }
                }
            }
            else
            {
                // Handle the case when 'filesToSend' is not provided
                reject(new Error('Files object is not provided'));
            }
        });
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Internal Login Helpers
    ///////////////////////////////////////////////////////////////////////////////////

    // Setup Fiery Path options
    private setupOptions(): void 
    {
        const agent_options = { keepAlive: true, maxSockets: 10, noDelay: false, scheduling: 'fifo' as 'fifo' };
//        const agent_options = { keepAlive: true, maxSockets: 10, noDelay: false, scheduling: 'fifo' as 'fifo', rejectUnauthorized: false };
        this.m_agent = new https.Agent(agent_options);
        this.m_http_url = 'https://' + this.m_ip + '/fierypath/v1';
        this.m_ws_url = 'wss://' + this.m_ip + '/fierypathevents/v1';
    }

    // Common options for all non-file based requests
    private createRequestOptions(route: string, message: any): OptionsWithUrl
    {
        const requestOptions: OptionsWithUrl =
        {
            url: this.m_http_url + route,
            headers:
            {
                Authorization: `Bearer ${this.m_token}`,
                'Content-Type': 'application/json',
            },
            agent: this.m_agent as any | null,
            json: message,
        };

        return requestOptions;
    }

    // Common options for all file based requests
    private createFileRequestOptions(route: string): OptionsWithUrl
    {
        const requestOptions: OptionsWithUrl =
        {
            url: this.m_http_url + route,
            headers:
            {
                Authorization: `Bearer ${this.m_token}`,
            },
            agent: this.m_agent as any | null
        };

        return requestOptions;
    }

    // Sends a login request
    private login(): Promise<string>
    {
        return new Promise<string>((resolve, reject) =>
        {
            request(
                {
                    method: 'POST',
                    url: this.m_http_url + '/session/login',
                    headers: { 'Content-Type': 'application/json' },
                    json: this.m_credentials,
                },
                (err, res, body) =>
                {
                    if (err)
                    {
                        reject(err);
                    } else if (!body.token)
                    {
                        reject('login token is invalid');
                    }
                    else
                    {
                        this.logInfo('Valid login token received');
                        resolve(body.token);
                    }
                }
            );
        });
    }

    // Creates a websocket connection
    private createWebSocket(): Promise<string>
    {
        return new Promise<string>((resolve, reject) =>
        {
            this.ws = new WebSocket(this.m_ws_url, { headers: { 'Content-Type': 'application/json' } });

            this.ws.on('open', () =>
            {
                this.logInfo('Connected ws');
                resolve('connected ws');
            });

            this.ws.on('error', (err) =>
            {
                this.logError('error ws ' + err);
                reject(err);
            });

            this.ws.on('message', (data, flags) =>
            {
                const message = data.toString();
                this.handleMessage(message);
            });

            this.ws.on('close', (code) =>
            {
                this.logError('error close ' + code);

                if (!this.m_login_in_progress)
                {
                    reject(code);
                    this.ws = null;
                    if (code === 1006)
                    {
                        // Server is not available anymore, restart the login process
                        this.logInfo('Restarting login process...');
                        setTimeout(() => this.LoginCallback(), 10);
                    }
                }
                else
                    this.logWarn('Login in progress, ignoring close code...');
            });
        });
    }

    // Sends the mandatory "connect" message to the server once a websocket connection is created
    private connectWebSocket(): void 
    {
        this.sendMessage('connect', { token: this.m_token });
    }

    // Logout from the server
    public logout(): Promise<any>
    {
        return new Promise<any>((resolve, reject) =>
        {
            request.post(this.createRequestOptions('/session/logout', {}), (err: any, res: any, body: any) =>
            {
                this.m_token = '';

                if (err)
                    resolve(err);
                else
                    resolve("Sucessfully logged out");
            });
        });
    }

    // Executes the steps need to perform a login and make a websocket connection
    private async LoginCallback(): Promise<void>
    {
        try
        {
            if (!this.m_login_in_progress)
            {
                this.m_login_in_progress = true
                await this.logout();
                this.m_token = await this.login();
                await this.createWebSocket();
                await this.connectWebSocket();

                this.logInfo('Successfully logged in and opened websocket connection');
                this.m_login_in_progress = false;

                // Inform the client that the login steps have been completed
                if (this.m_loginCallback && this.m_parent)
                    this.m_loginCallback(this.m_parent);
            }
            else
                this.logWarn('Login already in progress, waiting...');
        }
        catch (error)
        {
            this.m_login_in_progress = false;
            this.logInfo('Error during login, trying again...' + error);
            setTimeout(() => this.LoginCallback(), 5000);
        }
    }

    // Sends websocket messages. There is only one "connect" message that is initiated from the client
    private sendMessage(method: string, params: any): Promise<any>
    {
        return new Promise<any>((resolve, reject) =>
        {
            if (this.ws)
            {
                this.m_defer_list_map.set(this.m_counter.toString(), { resolve, reject });

                this.ws.send(
                    JSON.stringify({
                        jsonrpc: '2.0',
                        method: method,
                        params: params,
                        id: this.m_counter.toString(),
                    })
                );
                ++this.m_counter;
            }
            else
            {
                reject(new Error('WebSocket is not initialized.'));
            }
        });
    }

    // Handle messages sent by the server. This includes all Fiery Path events
    private handleMessage(response: string): void 
    {
        try
        {
            const message = JSON.parse(response);
            if (message.hasOwnProperty('method'))
            {
                // We have received an event from the server
                if (this.m_eventCallback)
                    this.m_eventCallback(message.method, message, this.m_parent);
            } else if (this.m_defer_list_map.has(message.id)) 
            {
                const defer = this.m_defer_list_map.get(message.id);
                if (message.hasOwnProperty('error')) 
                {
                    defer.reject(message.error);
                }

                if (message.hasOwnProperty('result')) 
                {
                    defer.resolve(message.result);
                    this.m_defer_list_map.delete(message.id);
                }
            }
        }
        catch (e: any)
        {
            this.logError(e);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Internal Logger Helpers
    ///////////////////////////////////////////////////////////////////////////////////

    private logInfo(str: string): void 
    {
        if (this.m_info_logger)
            this.m_info_logger(str, this.m_parent);
    }

    private logWarn(str: string): void 
    {
        if (this.m_warn_logger)
            this.m_warn_logger(str, this.m_parent);
    }

    private logError(str: string): void 
    {
        if (this.m_error_logger)
            this.m_error_logger(str, this.m_parent);
    }

    private logDebug(str: string): void 
    {
        if (this.m_debug_logger)
            this.m_debug_logger(str, this.m_parent);
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Exit Handler
    ///////////////////////////////////////////////////////////////////////////////////

    private async exitHandler(): Promise<void>
    {
        this.logInfo('Exiting...');
        const message = await this.logout();
        this.logInfo(message);
        process.exit(0);
    }
}


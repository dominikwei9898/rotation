"use strict";
/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
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
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.FieryPathSession = void 0;
const fs = __importStar(require("fs"));
const https = __importStar(require("https"));
const request_1 = __importDefault(require("request"));
const ws_1 = __importDefault(require("ws"));
///////////////////////////////////////////////////////////////////////////////////
// Class that implements the Fiery Path REST API's
///////////////////////////////////////////////////////////////////////////////////
class FieryPathSession {
    constructor() {
        this.m_initialized = false;
        this.m_counter = 0;
        this.m_defer_list_map = new Map();
        this.m_token = '';
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
    Initialize(parent, ip, credentials) {
        if ((parent === null || parent === void 0 ? void 0 : parent.InfoCallback) && (parent === null || parent === void 0 ? void 0 : parent.WarnCallback) && (parent === null || parent === void 0 ? void 0 : parent.ErrorCallback) && (parent === null || parent === void 0 ? void 0 : parent.DebugCallback) && (parent === null || parent === void 0 ? void 0 : parent.LoginCallback) && (parent === null || parent === void 0 ? void 0 : parent.EventCallback) && ip && credentials) {
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
    Login() {
        setTimeout(() => this.LoginCallback(), 100);
    }
    // Gets a value from the server
    Get(route, message) {
        return new Promise((resolve, reject) => {
            request_1.default.get(this.createRequestOptions(route, message), (err, res, body) => {
                if (err)
                    reject(err);
                else
                    resolve(body);
            });
        });
    }
    // Post a value to the server
    Post(route, message) {
        return new Promise((resolve, reject) => {
            request_1.default.post(this.createRequestOptions(route, message), (err, res, body) => {
                if (err)
                    return reject(err);
                return resolve({ body: body, status: res });
            });
        });
    }
    // Put a value to the server
    Put(route, message) {
        return new Promise((resolve, reject) => {
            request_1.default.put(this.createRequestOptions(route, message), (err, res, body) => {
                if (err)
                    return reject(err);
                return resolve({ body: body, status: res });
            });
        });
    }
    // Delete a value from the server
    Delete(route, message) {
        return new Promise((resolve, reject) => {
            request_1.default.del(this.createRequestOptions(route, message), (err, res, body) => {
                if (err)
                    return reject(err);
                return resolve(body);
            });
        });
    }
    // Post a file to the server
    PostFile(route, fileToSend) {
        return new Promise((resolve, reject) => {
            if (fileToSend) {
                const req = request_1.default.post(this.createFileRequestOptions(route), (err, res, body) => {
                    if (err)
                        return reject(err);
                    return resolve(body);
                });
                const form = req.form();
                form.append('file', fs.createReadStream(fileToSend));
            }
            else {
                // Handle the case when 'fileToSend' is not provided
                reject(new Error('File path is not provided'));
            }
        });
    }
    // Post multiple files to the server
    PostMultiFile(route, filesToSend) {
        return new Promise((resolve, reject) => {
            if (filesToSend) {
                const req = request_1.default.post(this.createFileRequestOptions(route), (err, res, body) => {
                    if (err)
                        return reject(err);
                    return resolve(body);
                });
                const form = req.form();
                for (const name in filesToSend) {
                    if (filesToSend.hasOwnProperty(name)) {
                        form.append(name, fs.createReadStream(filesToSend[name]));
                    }
                }
            }
            else {
                // Handle the case when 'filesToSend' is not provided
                reject(new Error('Files object is not provided'));
            }
        });
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // Internal Login Helpers
    ///////////////////////////////////////////////////////////////////////////////////
    // Setup Fiery Path options
    setupOptions() {
        const agent_options = { keepAlive: true, maxSockets: 10, noDelay: false, scheduling: 'fifo' };
        //        const agent_options = { keepAlive: true, maxSockets: 10, noDelay: false, scheduling: 'fifo' as 'fifo', rejectUnauthorized: false };
        this.m_agent = new https.Agent(agent_options);
        this.m_http_url = 'https://' + this.m_ip + '/fierypath/v1';
        this.m_ws_url = 'wss://' + this.m_ip + '/fierypathevents/v1';
    }
    // Common options for all non-file based requests
    createRequestOptions(route, message) {
        const requestOptions = {
            url: this.m_http_url + route,
            headers: {
                Authorization: `Bearer ${this.m_token}`,
                'Content-Type': 'application/json',
            },
            agent: this.m_agent,
            json: message,
        };
        return requestOptions;
    }
    // Common options for all file based requests
    createFileRequestOptions(route) {
        const requestOptions = {
            url: this.m_http_url + route,
            headers: {
                Authorization: `Bearer ${this.m_token}`,
            },
            agent: this.m_agent
        };
        return requestOptions;
    }
    // Sends a login request
    login() {
        return new Promise((resolve, reject) => {
            (0, request_1.default)({
                method: 'POST',
                url: this.m_http_url + '/session/login',
                headers: { 'Content-Type': 'application/json' },
                json: this.m_credentials,
            }, (err, res, body) => {
                if (err) {
                    reject(err);
                }
                else if (!body.token) {
                    const errorMsg = `login token is invalid (HTTP status: ${res && res.statusCode ? res.statusCode : 'unknown'})`;
                    this.logError(errorMsg);
                    reject(errorMsg);
                }
                else {
                    this.logInfo('Valid login token received');
                    resolve(body.token);
                }
            });
        });
    }
    // Creates a websocket connection
    createWebSocket() {
        return new Promise((resolve, reject) => {
            this.ws = new ws_1.default(this.m_ws_url, { headers: { 'Content-Type': 'application/json' } });
            this.ws.on('open', () => {
                this.logInfo('Connected ws');
                this.setupKeepAlive();
                resolve('connected ws');
            });
            this.ws.on('error', (err) => {
                this.logError('error ws ' + err);
                reject(err);
            });
            this.ws.on('message', (data, flags) => {
                const message = data.toString();
                this.handleMessage(message);
            });
            this.ws.on('close', (code) => {
                this.logError('error close ' + code);
                if (!this.m_login_in_progress) {
                    reject(code);
                    this.ws = null;
                    if (code === 1006) {
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
    connectWebSocket() {
        return this.sendMessage('connect', { token: this.m_token });
    }
    // Logout from the server
    logout() {
        return new Promise((resolve, reject) => {
            request_1.default.post(this.createRequestOptions('/session/logout', {}), (err, res, body) => {
                this.m_token = '';
                if (err)
                    resolve(err);
                else
                    resolve("Sucessfully logged out");
            });
        });
    }
    // Executes the steps need to perform a login and make a websocket connection
    LoginCallback() {
        return __awaiter(this, void 0, void 0, function* () {
            try {
                if (!this.m_login_in_progress) {
                    this.m_login_in_progress = true;
                    yield this.logout();
                    this.m_token = yield this.login();
                    yield this.createWebSocket();
                    yield this.connectWebSocket();
                    this.logInfo('Successfully logged in and opened websocket connection');
                    this.m_login_in_progress = false;
                    // Inform the client that the login steps have been completed
                    if (this.m_loginCallback && this.m_parent)
                        this.m_loginCallback(this.m_parent);
                }
                else
                    this.logWarn('Login already in progress, waiting...');
            }
            catch (error) {
                this.m_login_in_progress = false;
                this.logInfo('Error during login, trying again...' + error);
                setTimeout(() => this.LoginCallback(), 5000);
            }
        });
    }
    // Sends websocket messages. There is only one "connect" message that is initiated from the client
    sendMessage(method, params) {
        return new Promise((resolve, reject) => {
            if (this.ws) {
                this.m_defer_list_map.set(this.m_counter.toString(), { resolve, reject });
                this.ws.send(JSON.stringify({
                    jsonrpc: '2.0',
                    method: method,
                    params: params,
                    id: this.m_counter.toString(),
                }));
                ++this.m_counter;
            }
            else {
                reject(new Error('WebSocket is not initialized.'));
            }
        });
    }
    // Handle messages sent by the server. This includes all Fiery Path events
    handleMessage(response) {
        try {
            const message = JSON.parse(response);
            if (message.hasOwnProperty('method')) {
                // We have received an event from the server
                if (this.m_eventCallback)
                    this.m_eventCallback(message.method, message, this.m_parent);
            }
            else if (this.m_defer_list_map.has(message.id)) {
                const defer = this.m_defer_list_map.get(message.id);
                this.m_defer_list_map.delete(message.id);
                if (message.hasOwnProperty('error')) {
                    const errObj = message.error;
                    const err = new Error(typeof (errObj === null || errObj === void 0 ? void 0 : errObj.message) === 'string' ? errObj.message : 'JSON-RPC error');
                    err.code = errObj === null || errObj === void 0 ? void 0 : errObj.code;
                    err.data = errObj === null || errObj === void 0 ? void 0 : errObj.data;
                    defer.reject(err);
                    return;
                }
                if (message.hasOwnProperty('result')) {
                    defer.resolve(message.result);
                    return;
                }
                defer.reject(new Error('Invalid JSON-RPC response: missing result/error'));
            }
        }
        catch (e) {
            this.logError(e);
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // Internal Logger Helpers
    ///////////////////////////////////////////////////////////////////////////////////
    logInfo(str) {
        if (this.m_info_logger)
            this.m_info_logger(str, this.m_parent);
    }
    logWarn(str) {
        if (this.m_warn_logger)
            this.m_warn_logger(str, this.m_parent);
    }
    logError(str) {
        if (this.m_error_logger)
            this.m_error_logger(str, this.m_parent);
    }
    logDebug(str) {
        if (this.m_debug_logger)
            this.m_debug_logger(str, this.m_parent);
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // Exit Handler
    ///////////////////////////////////////////////////////////////////////////////////
    exitHandler() {
        return __awaiter(this, void 0, void 0, function* () {
            this.logInfo('Exiting...');
            const message = yield this.logout();
            this.logInfo(message);
            process.exit(0);
        });
    }
    setupKeepAlive() {
        setInterval(() => {
            if (this.ws && this.ws.readyState === ws_1.default.OPEN)
                this.ws.ping();
        }, 30000);
    }
}
exports.FieryPathSession = FieryPathSession;
//# sourceMappingURL=session.js.map
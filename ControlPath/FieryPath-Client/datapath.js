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
exports.DataPathSession = void 0;
const ws_1 = __importDefault(require("ws"));
///////////////////////////////////////////////////////////////////////////////////
// Class that implements the Data Path internal API's to communicate with FITS Plugin
///////////////////////////////////////////////////////////////////////////////////
class DataPathSession {
    constructor() {
        this.m_port = 0;
        this.m_initialized = false;
        this.m_unique_id_map = new Map();
        // Initialize member variables
        this.m_info_logger = null;
        this.m_warn_logger = null;
        this.m_error_logger = null;
        this.m_debug_logger = null;
        this.m_eventCallback = null;
        this.m_data_path_clients = [];
        this.wss = null;
        this.m_port = 9085;
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // Interface
    ///////////////////////////////////////////////////////////////////////////////////
    // Initializes the session using the parameters passed, this function expects certain callbacks to be defined in the caller class
    Initialize(parent) {
        if ((parent === null || parent === void 0 ? void 0 : parent.InfoCallback) && (parent === null || parent === void 0 ? void 0 : parent.WarnCallback) && (parent === null || parent === void 0 ? void 0 : parent.ErrorCallback) && (parent === null || parent === void 0 ? void 0 : parent.DebugCallback) && (parent === null || parent === void 0 ? void 0 : parent.EventCallback)) {
            this.m_parent = parent;
            this.m_info_logger = parent.InfoCallback;
            this.m_warn_logger = parent.WarnCallback;
            this.m_error_logger = parent.ErrorCallback;
            this.m_debug_logger = parent.DebugCallback;
            this.m_eventCallback = parent.EventCallback;
            this.m_initialized = true;
        }
        return this.m_initialized;
    }
    // Initiates the process to start the webserver
    Start() {
        setTimeout(() => this.startWebServer(), 100);
    }
    ClientsConnected() {
        return this.m_data_path_clients.length != 0;
    }
    // Sends message to all connected clients
    SendMessage(message) {
        if (this.m_data_path_clients.length !== 0) {
            this.logInfo(`Sending message to ${this.m_data_path_clients.length} clients: ${message}`);
            this.m_data_path_clients.forEach((client) => {
                client.socket.send(message);
            });
        }
        else {
            this.logError(`No data path client is connected, not sending message ${message}`);
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // Internal Webserver Helpers
    ///////////////////////////////////////////////////////////////////////////////////
    // Executes the steps need to start webserver
    startWebServer() {
        return __awaiter(this, void 0, void 0, function* () {
            try {
                this.logInfo(`Starting Data Path Web Server on port: ${this.m_port}`);
                this.wss = new ws_1.default.Server({ port: this.m_port });
                this.wss.on('connection', (ws) => {
                    ws.on('message', (message) => {
                        const data = JSON.parse(message.toString());
                        switch (data.type) {
                            case 'connect':
                                const inks = data.inks;
                                this.m_data_path_clients.push({ inks, socket: ws });
                                this.logInfo(`Client connected with inks: ${inks}`);
                                break;
                            case 'printed':
                                const uniqueId = data.jobId + "-" + data.imageId;
                                // Check if we have the required number of responses and if yes, invoke the callback, else update the map that keeps track of response count
                                let count = 0;
                                let deleteMap = false;
                                if (this.m_unique_id_map.has(uniqueId)) {
                                    count = this.m_unique_id_map.get(uniqueId);
                                    deleteMap = true;
                                }
                                // If we have the required number, then send a mesage and clear the map
                                if (count + 1 >= this.m_data_path_clients.length) {
                                    if (this.m_eventCallback) {
                                        const message = { method: "dataPathImagePrinted", jobId: data.jobId, imageId: data.imageId };
                                        this.m_eventCallback(message.method, message, this.m_parent);
                                    }
                                    else
                                        this.logError("Event callback not setup to send dataPathImagePrinted event");
                                    if (deleteMap)
                                        this.m_unique_id_map.delete(uniqueId);
                                }
                                else
                                    this.m_unique_id_map.set(uniqueId, count + 1);
                                break;
                            default:
                                this.logError(`Message type ${data.type} not recognized`);
                                break;
                        }
                    });
                    ws.on('close', () => {
                        const index = this.m_data_path_clients.findIndex((client) => client.socket === ws);
                        if (index !== -1) {
                            const disconnectedClient = this.m_data_path_clients.splice(index, 1)[0];
                            this.logWarn(`Client with inks ${disconnectedClient.inks} disconnected`);
                        }
                    });
                });
            }
            catch (error) {
                this.logError('Error during webserver operation' + error);
            }
        });
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
}
exports.DataPathSession = DataPathSession;
//# sourceMappingURL=datapath.js.map
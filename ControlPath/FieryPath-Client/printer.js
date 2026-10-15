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
Object.defineProperty(exports, "__esModule", { value: true });
exports.PrinterIfc = void 0;
const _ = __importStar(require("underscore"));
const winston = __importStar(require("winston"));
const session_1 = require("./session");
const events_1 = require("events");
const datapath_1 = require("./datapath");
///////////////////////////////////////////////////////////////////////////////////
// Class that implements the Fiery Path Printer API's using FieryPathSession
///////////////////////////////////////////////////////////////////////////////////
class FieryPathIfc extends events_1.EventEmitter {
    constructor() {
        super();
        this.m_jobs_priority_map = new Map();
        this.m_image_starting_normal_list = [];
        this.m_image_completing_normal_list = [];
        this.m_image_completing_pending_normal_list = [];
        this.m_input_slot_list = [];
        this.m_ready_status_set_once = false;
        this.m_defaultInkList = ["cyan", "magenta", "yellow", "black"];
        this.m_unique_job_number = 1;
        this.m_unique_image_number = 1;
        // Use FieryPathSession to send REST API's and receive websocket events
        this.m_fiery_path_session = new session_1.FieryPathSession();
        // Initialize and start the DataPathSession. This is to let Data Path know when to print images after an imageCompleting has been received
        this.m_data_path_session = new datapath_1.DataPathSession();
        if (this.m_data_path_session.Initialize(this))
            this.m_data_path_session.Start();
        else
            winston.info("Unable to initialize Data Path Session");
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // Callbacks required for FieryPathSession
    ///////////////////////////////////////////////////////////////////////////////////
    InfoCallback(str, context) {
        winston.info(str);
    }
    WarnCallback(str, context) {
        winston.warn(str);
    }
    ErrorCallback(str, context) {
        winston.error(str);
    }
    DebugCallback(str, context) {
        winston.debug(str);
    }
    LoginCallback(context) {
        // The login process is successfully completed, initiate the next steps
        context.emit("FieryPath_Session_Connected");
    }
    EventCallback(method, message, context) {
        // We have recevied an event from the server
        context.emit(method, message);
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // Server State
    ///////////////////////////////////////////////////////////////////////////////////
    // Check if the server is ready/connected to determine if subsequent commands can be sent
    CheckServerStatus() {
        return __awaiter(this, void 0, void 0, function* () {
            try {
                const result = yield this.m_fiery_path_session.Get("/system/dfe/status", {});
                if (result != null && result.state != null && (result.state.toLowerCase() === "connected" || result.state.toLowerCase() === "ready")) {
                    winston.info("CheckServerStatus: Server is initialized");
                    // Server is ready to accept commands, the first action should be set the "initializing" state for the printer
                    this.setPrinterInitializingStatus();
                }
                else
                    setTimeout(() => this.CheckServerStatus(), 100);
            }
            catch (err) {
                winston.error("CheckServerStatus: error - " + err);
                setTimeout(() => this.CheckServerStatus(), 100);
            }
        });
    }
    // Set printer status as initializing to let the server know it is in the process of connecting/reconnecting
    setPrinterInitializingStatus() {
        return __awaiter(this, void 0, void 0, function* () {
            try {
                const result = yield this.m_fiery_path_session.Put("/system/printer/status", { "action": "initializing" });
                // Send the configuration
                this.sendConfiguration();
            }
            catch (err) {
                winston.error("setPrinterInitializingStatus: error - " + err);
                setTimeout(() => this.CheckServerStatus(), 100);
            }
        });
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // Configuration
    ///////////////////////////////////////////////////////////////////////////////////
    sendConfiguration() {
        return __awaiter(this, void 0, void 0, function* () {
            winston.info("sendConfiguration: received");
            try {
                const result = yield this.sendDeviceConfiguration();
                if (result) {
                    if (result.status && result.status.statusCode && result.status.statusCode === 201) {
                        // Send all other configurations
                        yield this.sendInkConfiguration();
                        yield this.sendLocalizationConfiguration();
                        // Perform any printer specific initialization. Once all initialization has completed, set the printer status to ready
                        this.setPrinterReadyStatus();
                    }
                    else
                        winston.error('sendConfiguration: not sending any other configurations, statusCode is - ' + result.status.statusCode + ', body is - ' + result.body);
                }
                else
                    winston.error('sendConfiguration: not sending any other configurations');
            }
            catch (err) {
                winston.error('sendConfiguration: not sending any other configurations - ' + err);
            }
        });
    }
    sendDeviceConfiguration() {
        return __awaiter(this, void 0, void 0, function* () {
            winston.info('sendDeviceConfiguration: sending device specific information');
            const engine_info = { EngineInfo: [{ key: 'Model', value: 'MyModel' }, { key: 'Manufacturer', value: 'MyCompany' }, { key: 'Serial Number', value: 'abc123' }] };
            const configurationObj = { configuration: { engine: engine_info } };
            try {
                const result = yield this.m_fiery_path_session.Post('/system/printer/oem/configuration', configurationObj);
                winston.info('sendDeviceConfiguration: device specific information sent');
                return result;
            }
            catch (err) {
                winston.error('sendDeviceConfiguration: sending device specific information - ' + err);
                throw err;
            }
        });
    }
    sendInk(inkObj) {
        return __awaiter(this, void 0, void 0, function* () {
            winston.info('sendInk: sending ink - ' + inkObj.ink.type);
            try {
                const res = yield this.m_fiery_path_session.Post('/system/printer/inks', inkObj);
                if (res) {
                    if (res.status && res.status.statusCode && res.status.statusCode === 201)
                        yield this.m_fiery_path_session.Put('/system/printer/inks/' + res.body.id + '/status', { ink: { level: 100 } });
                    else {
                        // Retry only if the server cannot accept inks at this time
                        if (res.status && res.status.statusCode && res.status.statusCode === 503) {
                            yield new Promise((resolve) => setTimeout(resolve, 2000));
                            yield this.sendInk(inkObj);
                        }
                        else
                            winston.info('sendInk: sending inks - not retrying since status code is not 503');
                    }
                }
                else {
                    yield new Promise((resolve) => setImmediate(resolve));
                    yield this.sendInk(inkObj);
                }
            }
            catch (err) {
                winston.error('sendInk: sending inks error - ' + err);
                yield new Promise((resolve) => setImmediate(resolve));
                yield this.sendInk(inkObj);
            }
            winston.info('sendInk: sending inks success for - ' + inkObj.ink.type);
        });
    }
    sendInkConfiguration() {
        return __awaiter(this, void 0, void 0, function* () {
            winston.info('sendInkConfiguration: sending inks');
            try {
                // Check if we can get inks from the server
                let inkTypeList = _.clone(this.m_defaultInkList);
                const result = yield this.m_fiery_path_session.Get("/system/dfe/info", {});
                if (result != null && result.settings != null && result.settings.inks != null)
                    inkTypeList = result.settings.inks;
                // First delete any current inks if set
                yield this.m_fiery_path_session.Delete('/system/printer/inks', {});
                const inkTypeListLength = inkTypeList.length;
                winston.info('sendInkConfiguration: inkTypeList is ' + JSON.stringify(inkTypeList));
                const dropSizeArray = [1.1, 2.2, 3.3];
                let number = 1;
                for (let s = 0; s < inkTypeListLength; s++) {
                    const inkType = inkTypeList.shift();
                    // const inkObj = { ink: {type: inkType, capacity: 100, number: number++, dropsize: dropSizeArray} }; // This is how you can send additional information like drop size for each ink if required
                    const inkObj = { ink: { type: inkType, capacity: 100, number: number++ } };
                    yield this.sendInk(inkObj);
                }
            }
            catch (err) {
                winston.error('sendInkConfiguration: deleting inks error - ' + err);
            }
        });
    }
    sendLocalizationConfiguration() {
        return __awaiter(this, void 0, void 0, function* () {
            winston.info('sendLocalizationConfiguration: sending localization');
            const localizationObj = {
                localization: [
                    {
                        'This is string one': {
                            fr_FR: "C'est la première chaîne",
                            ja_JP: "これは文字列1です"
                        }
                    },
                    {
                        'This is string two': {
                            fr_FR: "C'est la chaîne deux",
                            ja_JP: "これは文字列2です"
                        }
                    }
                ]
            };
            try {
                yield this.m_fiery_path_session.Post('/system/printer/localization', localizationObj);
                winston.info('sendLocalizationConfiguration: sending localization success');
            }
            catch (err) {
                winston.error('sendLocalizationConfiguration: sending localization error - ' + err);
            }
        });
    }
    setPrinterReadyStatus() {
        return __awaiter(this, void 0, void 0, function* () {
            try {
                // Set printer status to ready
                winston.info("setPrinterReadyStatus: setting printer status to ready");
                const result = yield this.m_fiery_path_session.Put("/system/printer/status/", { "action": "ready" });
                winston.info("setPrinterReadyStatus: setting printer status to ready success");
                // Optional messages. Fiery will let you know if and when these optional messages are needed. These are just examples of how to send them if required
                /*
                const jsonEVData = { "key1": "value1", "key2": "value2" };
                const evdata = { "eVMessage": jsonEVData }
                winston.info("Sending message to EV " + JSON.stringify(evdata))
                await this.m_fiery_path_session.Put('/dfe/external/evmessage', evdata);
    
                const jsonDPData = { "key1": "value1", "key2": "value2" };
                const dpdata = { "eVMessage": jsonDPData }
                winston.info("Sending message to DP " + JSON.stringify(dpdata))
                await this.m_fiery_path_session.Put('/dfe/external/dpmessage', dpdata);
                */
            }
            catch (err) {
                winston.error("setPrinterReadyStatus: setting printer status to ready error - " + err);
            }
        });
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // jobStarting event response handler
    ///////////////////////////////////////////////////////////////////////////////////
    // Sends job "start" for the jobStarting event received
    sendJobStart(jobId, settings) {
        return __awaiter(this, void 0, void 0, function* () {
            winston.info(`sendJobStart: received for jobId = ${jobId}`);
            return new Promise((resolve, reject) => __awaiter(this, void 0, void 0, function* () {
                const sendRequest = () => __awaiter(this, void 0, void 0, function* () {
                    try {
                        const result = yield this.m_fiery_path_session.Put(`/jobs/${jobId}/status`, { status: { settings, state: "start" } });
                        if (this.canRetry(result)) {
                            if (result !== null && result.body !== undefined) {
                                if (result.body.success !== undefined && result.body.success === true) {
                                    winston.info(`sendJobStart: successfully sent for jobId = ${jobId}`);
                                    resolve(result);
                                }
                                else {
                                    winston.warn(`sendJobStart: did not receive a successful result for jobId = ${jobId}`);
                                    setImmediate(sendRequest);
                                }
                            }
                            else {
                                winston.warn(`sendJobStart: did not receive a successful object for jobId = ${jobId}`);
                                setImmediate(sendRequest);
                            }
                        }
                        else
                            reject(result);
                    }
                    catch (error) {
                        winston.warn(`sendJobStart}: received error for jobId = ${jobId}: ${error}`);
                        setImmediate(sendRequest);
                    }
                });
                // Send the initial request and if required retry until successful
                sendRequest();
            }));
        });
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // imageStarting event response handlers
    ///////////////////////////////////////////////////////////////////////////////////
    // Sends image "properties" for an "imageStarting" event received to Data Path Session clients.
    sendImagePropertiesToDataPath(image) {
        return __awaiter(this, void 0, void 0, function* () {
            if (!image || !image.properties)
                return;
            const message = { type: "imageProperties", jobId: image.jobId, imageId: image.imageId, imageProperties: image.properties };
            this.m_data_path_session.SendMessage(JSON.stringify(message));
            // Wait until the current send request is queued before proceeding
            yield new Promise((resolve) => setImmediate(resolve));
        });
    }
    // Sends image "start" for an "imageStarting" event received. The client should initiate this as quickly as possible. Any delay in sending this message can result in performance issues
    sendImageStart(image) {
        return __awaiter(this, void 0, void 0, function* () {
            // First inform the Data Path Session client about any image properties it is interested in
            yield this.sendImagePropertiesToDataPath(image);
            const imageToSend = { id: image.imageId, offset: image.offset };
            winston.info(`sendImageStart: request sent for jobId = ${image.jobId}, imageId = ${image.imageId}`);
            try {
                // Send the image "start" using the Put method
                const result = yield this.m_fiery_path_session.Put(`/jobs/${image.jobId}/status`, { status: { image: imageToSend, settings: image.settings, state: "start" } });
                winston.info(`sendImageStart: response received for jobId = ${image.jobId}, imageId = ${image.imageId}`);
                // If ths response indicates we should not retry sending the message in case of a failure, ignore it.
                if (this.canRetry(result)) {
                    if (result !== null && result.body !== undefined) {
                        if (result.body.success !== undefined && result.body.success === true) {
                            // We have successfully sent the image "start"
                            let imageRemoved = null;
                            {
                                imageRemoved = this.m_image_starting_normal_list.shift();
                                // Add this to the pending list if it is not already part of the completing list.This can only happen if we receive a imageCompleting before we get a response to image start ###
                                if (!this.listContainsImageId(this.m_image_completing_normal_list, imageRemoved.jobId, imageRemoved.imageId))
                                    this.m_image_completing_pending_normal_list.push({ jobId: imageRemoved.jobId, imageId: imageRemoved.imageId });
                                else
                                    winston.warn(`sendImageStart ignoring adding to image completing normal pending list since imageCompleting was already received for jobId ${imageRemoved.jobId} imageId ${imageRemoved.imageId}`);
                            }
                            if (image.imageId !== imageRemoved.imageId)
                                winston.error("sendImageStart image removed does not match the image sent on success");
                            image = null;
                            imageRemoved = null;
                            // Trigger the next image start if available as soon as possible. The printer should not throttle sending image "start"
                            this.processImageStart();
                        }
                        else {
                            // We need to resend the message
                            winston.warn(`sendImageStart did not receive a successful result for jobId = ${image.jobId}, imageId = ${image.imageId}`);
                            image.sending = false;
                            this.processImageStart();
                        }
                    }
                    else {
                        // We need to resend the message
                        winston.warn(`sendImageStart did not receive result object for jobId = ${image.jobId}, imageId = ${image.imageId}`);
                        image.sending = false;
                        this.processImageStart();
                    }
                }
                else {
                    // No need to resend the message, remove it from our list
                    this.m_image_starting_normal_list.shift();
                }
            }
            catch (error) {
                // We need to resend the message
                winston.error(`sendImageStart received error for jobId = ${image.jobId}, imageId = ${image.imageId}: ${error}`);
                image.sending = false;
                this.processImageStart();
            }
        });
    }
    // Process the next image for which a "start" needs to be sent
    processImageStart() {
        let image = null;
        if (this.m_image_starting_normal_list.length !== 0)
            image = this.m_image_starting_normal_list[0];
        if (image !== null) {
            const job = this.m_jobs_priority_map.get(image.jobId);
            if (job !== null && job.started) {
                if (!image.sending) {
                    image.sending = true;
                    this.sendImageStart(image);
                }
            }
        }
    }
    // Add the image received with imageStarting to the internal list
    addToImageStartingList(image) {
        const job = this.m_jobs_priority_map.get(image.jobId);
        if (job !== null) {
            // Add the image for processing
            this.m_image_starting_normal_list.push(image);
            if (job.started) {
                // Trigger sending image "start" as soon as possible. The printer should not throttle sending image "start"
                this.processImageStart();
            }
        }
        else
            winston.debug(`Ignoring image start list request, job ${image.jobId} must have been canceled`);
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // imageCompleting event response handlers
    ///////////////////////////////////////////////////////////////////////////////////
    // Sends image "complete" for an "imageCompleting" event received. The client should initiate this once the image has been printed/stacked/ejected and not before that
    sendImageComplete(image) {
        return __awaiter(this, void 0, void 0, function* () {
            winston.info(`sendImageComplete: request sent for jobId = ${image.jobId}, imageId = ${image.imageId}`);
            try {
                // Send the image "complete" using the Put method
                const result = yield this.m_fiery_path_session.Put(`/jobs/${image.jobId}/status`, { status: { image: { id: image.imageId }, state: "complete" } });
                winston.info(`sendImageComplete: response received for jobId = ${image.jobId}, imageId = ${image.imageId}`);
                // If ths response indicates we should not retry sending the message in case of a failure, ignore it.
                if (this.canRetry(result)) {
                    if (result !== null && result.body !== undefined) {
                        if (result.body.success !== undefined && result.body.success === true) {
                            // We have successfully sent the image "complete"
                            let imageRemoved = null;
                            imageRemoved = this.m_image_completing_normal_list.shift();
                            if (image.imageId !== imageRemoved.imageId)
                                winston.error("sendImageComplete image removed does not match the image sent on success");
                            // Once the last image "complete" has been sent, we need to complete the job by sending a "complete" for the jobCompleting event and cleaning up the job internally
                            if (imageRemoved.last)
                                this.completeJob(imageRemoved.jobId);
                            else
                                image = null;
                            imageRemoved = null;
                            // Trigger the next image "complete". In the real case when not running as a simulator, the next image "complete" should be sent once that image has been printed/stacked/ejected and not before that
                            this.processImageComplete();
                        }
                        else {
                            // We need to resend the message
                            winston.warn(`sendImageComplete did not receive a successful result for jobId = ${image.jobId}, imageId = ${image.imageId}`);
                            image.sending = false;
                            this.processImageComplete();
                        }
                    }
                    else {
                        // We need to resend the message
                        winston.warn(`sendImageComplete did not receive result object for jobId = ${image.jobId}, imageId = ${image.imageId}`);
                        image.sending = false;
                        this.processImageComplete();
                    }
                }
                else {
                    // No need to resend the message, remove it from our list
                    this.m_image_completing_normal_list.shift();
                }
            }
            catch (error) {
                // We need to resend the message
                winston.error(`sendImageComplete received error for jobId = ${image.jobId}, imageId = ${image.imageId}: ${error}`);
                image.sending = false;
                this.processImageComplete();
            }
        });
    }
    // Process the next image for which a "complete" needs to be sent. This means the next image "complete" should be sent once that image has been printed/stacked/ejected
    // and not before that. This is also the point to let Data Path know to print images(s). The "complete" should be sent only once a response is recevied from Data Path
    // that the particular image has been printed/stacked/ejected
    processImageComplete() {
        let image = null;
        if (this.m_image_completing_normal_list.length !== 0)
            image = this.m_image_completing_normal_list[0];
        if (image !== null) {
            const job = this.m_jobs_priority_map.get(image.jobId);
            if (job !== null && job.started) {
                if (!image.sending && image.printed) {
                    image.sending = true;
                    this.sendImageComplete(image);
                    // Cleanup the inks entry from the map
                    job.inksMap.delete(image.imageId);
                }
            }
        }
    }
    // Send the print request to DataPathSession client(s) to print/ink the image. The client(s) will send a "printed" response once the image has been sent to electronics and printed/stacked/ejected
    sendImagePrintToDataPath(image, inks) {
        const inksArray = inks.split(';').map(item => item.toLowerCase());
        const settingsR = image.settings;
        settingsR.jobNumber = image.jobNum;
        settingsR.imageNumber = image.imageNum;
        settingsR.copyNumber = image.copyNum;
        settingsR.jobMode = image.jobMode;
        settingsR.nozzleOffset = image.offset;
        const message = { type: "print", jobId: image.jobId, imageId: image.imageId, jobName: image.jobName, inks: inksArray, settings: settingsR };
        this.m_data_path_session.SendMessage(JSON.stringify(message));
    }
    // Callback when DataPathSession client(s) indicate that the image has been printed/stacked/ejected
    ImagePrintedReceivedFromDataPath(jobId, imageId) {
        // Update the "printed" status for this image and trigger processing it
        const job = this.m_jobs_priority_map.get(jobId);
        if (job !== null) {
            const imageObj = _.find(this.m_image_completing_normal_list, (item) => item.jobId === jobId && item.imageId === imageId);
            if (imageObj)
                imageObj.printed = true;
            // Trigger sending image complete. Keep in mind that this is just indicatin that the image was sent to print electronics
            // The client may need addtional inforation from press to determine when the image is actually printed/stacked/ejected.
            // It is only after that point that the image "complete" should be sent    
            this.processImageComplete();
        }
        else
            winston.error("Unable to update printed status for " + imageId + ", job " + jobId + " is not available");
    }
    // Add the image received with imageCompleting to the internal list
    addToImageCompletingList(image) {
        const job = this.m_jobs_priority_map.get(image.jobId);
        if (job !== null) {
            // We should have client(s) connected for DataPathSession. However,if we don't have any, then just mark the image "printed" as true
            image.printed = !this.m_data_path_session.ClientsConnected();
            // Add the list for processing
            this.m_image_completing_normal_list.push(image);
            this.m_image_completing_pending_normal_list = _.reject(this.m_image_completing_pending_normal_list, (listImage) => image.jobId === listImage.jobId && image.imageId === listImage.imageId);
            if (job.started) {
                // Trigger sending image "complete" if this image has already been marked as printed because there are no DataPathSession client(s). Otherwise send a "print" request to DataPathSession client(s).
                // Once a "printed" response is received from the DataPathSession client(s), image "complete" response will be triggered at that point
                if (image.printed)
                    this.processImageComplete();
                else
                    this.sendImagePrintToDataPath(image, job.inksMap.get(image.imageId));
            }
            else
                winston.error("cannot send image complete for image " + image.imageId + ", job is not started");
        }
        else
            winston.debug(`Ignoring image complete list request, job ${image.jobId} must have been canceled`);
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // jobCompleting event response handlers
    ///////////////////////////////////////////////////////////////////////////////////
    completeJob(jobId) {
        return __awaiter(this, void 0, void 0, function* () {
            this.cleanupJob(jobId);
            try {
                const result = yield this.sendJobComplete(jobId);
                winston.info(`completeJob: successful for jobId = ${jobId}`);
            }
            catch (error) {
                winston.error(`completeJob: received error for jobId = ${jobId}: ${error}`);
            }
        });
    }
    // Sends job "complete" for the jobCompleting event received
    sendJobComplete(jobId) {
        return __awaiter(this, void 0, void 0, function* () {
            winston.info(`sendJobComplete: received for jobId = ${jobId}`);
            return new Promise((resolve, reject) => __awaiter(this, void 0, void 0, function* () {
                const sendRequest = () => __awaiter(this, void 0, void 0, function* () {
                    try {
                        const result = yield this.m_fiery_path_session.Put(`/jobs/${jobId}/status`, { status: { state: "complete" } });
                        if (this.canRetry(result)) {
                            if (result !== null && result.body !== undefined) {
                                if (result.body.success !== undefined && result.body.success === true) {
                                    winston.info(`sendJobComplete: successfully sent for jobId = ${jobId}`);
                                    resolve(result);
                                }
                                else {
                                    winston.warn(`sendJobComplete: did not receive a successful result for jobId = ${jobId}`);
                                    setImmediate(sendRequest);
                                }
                            }
                            else {
                                winston.warn(`sendJobComplete: did not receive a successful object for jobId = ${jobId}`);
                                setImmediate(sendRequest);
                            }
                        }
                        else
                            reject(result);
                    }
                    catch (error) {
                        winston.warn(`sendJobComplete}: received error for jobId = ${jobId}: ${error}`);
                        setImmediate(sendRequest);
                    }
                });
                // Send the initial request and if required retry until successful
                sendRequest();
            }));
        });
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // jobCanceling event response handlers
    ///////////////////////////////////////////////////////////////////////////////////
    // Send a response to the jobCanceling event. When sent, it indicates the client is done with all images and does not intend to respond to any images for this job
    cancelJob(job, retryCount) {
        return __awaiter(this, void 0, void 0, function* () {
            // Check if there are any outstanding images in the image completing list. If there are, it means, we have images currently being printed, so wait until all do get printed
            let continueProcessing = this.isImageCompletingListEmpty(job.id);
            if (continueProcessing || retryCount <= 0) {
                winston.info(`cancelJob: received for ${job.id}`);
                this.cleanupJob(job.id);
                try {
                    const result = yield this.sendJobCancel(job.id);
                    winston.info(`cancelJob: successful for jobId = ${job.id}`);
                    // Inform the Data Path Session to cleanup its memory for any images that were transferred but for which imageCanceling was sent
                    if (this.m_data_path_session.ClientsConnected()) {
                        const message = { type: "cleanup", jobId: job.id };
                        this.m_data_path_session.SendMessage(JSON.stringify(message));
                    }
                }
                catch (error) {
                    winston.error(`cancelJob: received error for jobId = ${job.id}: ${error}`);
                }
            }
            else {
                retryCount--;
                // In the case where the client needs to wait because images are in the pipeline, then try again in a bit
                const waitTimer = 1;
                winston.info(`cancelJob: waiting for images to be printed for jobId = ${job.id}, retry count is ${retryCount}, retrying in ${waitTimer} seconds...`);
                setTimeout(() => this.cancelJob(job, retryCount), waitTimer * 1000);
            }
        });
    }
    // Sends job "cancel" for the jobCanceling event received
    sendJobCancel(jobId) {
        return __awaiter(this, void 0, void 0, function* () {
            winston.info(`sendJobCancel: received for jobId = ${jobId}`);
            return new Promise((resolve, reject) => __awaiter(this, void 0, void 0, function* () {
                const sendRequest = () => __awaiter(this, void 0, void 0, function* () {
                    try {
                        const result = yield this.m_fiery_path_session.Put(`/jobs/${jobId}/status`, { status: { state: "cancel" } });
                        if (this.canRetry(result)) {
                            if (result !== null && result.body !== undefined) {
                                if (result.body.success !== undefined && result.body.success === true) {
                                    winston.info(`sendJobCancel: successfully sent for jobId = ${jobId}`);
                                    resolve(result);
                                }
                                else {
                                    winston.warn(`sendJobCancel: did not receive a successful result for jobId = ${jobId}`);
                                    setImmediate(sendRequest);
                                }
                            }
                            else {
                                winston.warn(`sendJobCancel: did not receive a successful object for jobId = ${jobId}`);
                                setImmediate(sendRequest);
                            }
                        }
                        else
                            reject(result);
                    }
                    catch (error) {
                        winston.warn(`sendJobCancel}: received error for jobId = ${jobId}: ${error}`);
                        setImmediate(sendRequest);
                    }
                });
                // Send the initial request and if required retry until successful
                sendRequest();
            }));
        });
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // Print Helpers
    ///////////////////////////////////////////////////////////////////////////////////
    // Cleans up the job from any lists and maps
    cleanupJob(jobId) {
        winston.info(`cleanupJob: received for jobId = ${jobId}`);
        // Remove all images from the image lists for this jobId except if the image is currently being sent.In that case, it will be removed once the response is received
        const job = this.m_jobs_priority_map.get(jobId);
        if (job) {
            this.m_image_starting_normal_list = _.reject(this.m_image_starting_normal_list, (image) => image.jobId === jobId && !image.sending);
            this.m_image_completing_normal_list = _.reject(this.m_image_completing_normal_list, (image) => image.jobId === jobId && !image.sending);
            this.m_image_completing_pending_normal_list = _.reject(this.m_image_completing_pending_normal_list, (image) => image.jobId === jobId);
            this.m_jobs_priority_map.delete(jobId);
        }
        winston.info(`JOBS_MAP = ${this.m_jobs_priority_map.size}, NORMAL_STARTING_LIST = ${this.m_image_starting_normal_list.length}, NORMAL_COMPLETING_LIST = ${this.m_image_completing_normal_list.length}, NORMAL_COMPLETING_PENDING_LIST = ${this.m_image_completing_pending_normal_list.length}`);
    }
    // Cleans up the image completing pending list for a canceled image
    cleanupimageCompletingPending(jobId, imageId) {
        // If an imageCanceling event is received, we need to cleanup the image completing pending list
        // Normally, this will get cleaned up when a corresponding imageCompleting event is received. But in this case,
        // we will not get that event and hence need to do this cleanup separately
        this.m_image_completing_pending_normal_list = _.reject(this.m_image_completing_pending_normal_list, (image) => image.jobId === jobId && image.imageId === imageId);
    }
    // Determines if we should resend a message
    canRetry(result) {
        if (result !== null && result.status !== undefined) {
            if (result.status.statusCode === 404 || result.status.statusCode === 422 || result.status.statusCode === 500)
                return false;
        }
        return true;
    }
    // Checks if the given list has any image for the jobId specified
    listContainsJobId(list, jobId) {
        return _.some(list, (image) => image.jobId === jobId);
    }
    // Checks if the given list has any image for the jobId and imageId specified
    listContainsImageId(list, jobId, imageId) {
        return _.some(list, (image) => image.jobId === jobId && image.imageId === imageId);
    }
    // Determines if all the completing list is empty
    isImageCompletingListEmpty(jobId) {
        let empty = true;
        const job = this.m_jobs_priority_map.get(jobId);
        if (job) {
            if (this.listContainsJobId(this.m_image_completing_normal_list, jobId))
                empty = false;
        }
        return empty;
    }
    // Determines if all the image lists are empty
    areImageListsEmpty(jobId) {
        let empty = true;
        const job = this.m_jobs_priority_map.get(jobId);
        if (job) {
            if (this.listContainsJobId(this.m_image_starting_normal_list, jobId) || this.listContainsJobId(this.m_image_completing_normal_list, jobId) || this.listContainsJobId(this.m_image_completing_pending_normal_list, jobId))
                empty = false;
        }
        return empty;
    }
}
///////////////////////////////////////////////////////////////////////////////////
// Class that provides the Print API Interface
///////////////////////////////////////////////////////////////////////////////////
class PrinterIfc {
    constructor(commandlineParams) {
        // Create an instance of FieryPathIfc
        this.m_fiery_path_ifc = new FieryPathIfc();
        this.m_credentials = { username: commandlineParams['username'], password: commandlineParams['password'], accessrights: { a1: commandlineParams['accessrights'] } };
        const tls_authorization = false; // Set to true or false as needed
        if (!tls_authorization)
            process.env.NODE_TLS_REJECT_UNAUTHORIZED = "0";
        else
            winston.info("TLS Authorization is enabled, trusted certificate on server is required");
        // Initialize FieryPathIfc using the credentials passed through command-line
        if (this.m_fiery_path_ifc.m_fiery_path_session.Initialize(this.m_fiery_path_ifc, commandlineParams['ip'], this.m_credentials)) {
            winston.info("FPS: session module initialized");
            // Initiate the login process
            this.m_fiery_path_ifc.m_fiery_path_session.Login();
        }
        else
            winston.error("FPS: invalid or missing parameters specified");
        ///////////////////////////////////////////////////////////////////////////////////
        // Fiery Path Events
        ///////////////////////////////////////////////////////////////////////////////////
        // This event will be received once the login process is completed
        this.m_fiery_path_ifc.on("FieryPath_Session_Connected", () => {
            winston.debug("FieryPath_Session_Connected: received");
            this.m_fiery_path_ifc.CheckServerStatus();
        });
        // Event recevied that indicates the start of a job
        this.m_fiery_path_ifc.on("jobStarting", (message) => __awaiter(this, void 0, void 0, function* () {
            winston.info("");
            winston.info(`********************** New Job received (JobId = ${message.params.job.id}) **********************`);
            winston.info("jobStarting: received " + JSON.stringify(message));
            const job = message.params.job;
            // Cleanup any leftover values
            this.m_fiery_path_ifc.cleanupJob(job.id);
            // See if jobMode attribute is available and set based on that
            let jobMode = false;
            if (job.jobMode !== undefined)
                jobMode = job.jobMode;
            // Add to the jobs priority map
            const imageToInksMap = new Map();
            const jobObj = { name: job.name, started: false, canceling: false, inksMap: imageToInksMap, jobNum: this.m_fiery_path_ifc.m_unique_job_number++, jobMode: jobMode };
            this.m_fiery_path_ifc.m_jobs_priority_map.set(job.id, jobObj);
            // Respond to the message and send a "start" for it
            yield this.m_fiery_path_ifc.sendJobStart(job.id, {});
            jobObj.started = true;
            // Initiate the process of sending image "starts" if there are any in the list
            this.m_fiery_path_ifc.processImageStart();
        }));
        // Events received for every new image in the job that should be processed. The printer should save any meta data it needs and respond immediately
        this.m_fiery_path_ifc.on("imageStarting", (message) => {
            winston.info(`imageStarting: received ${JSON.stringify(message)}`);
            const job = message.params.job;
            // Save the inks for this image. This will be required later when a "print" message is sent to DataPathSession client
            const jobObj = this.m_fiery_path_ifc.m_jobs_priority_map.get(job.id);
            if (jobObj)
                jobObj.inksMap.set(job.image.id, job.image.inks);
            else
                winston.error(`Unable to set ink for image ${job.image.id} for job ${job.id}`);
            // This is for future enhancements, settings should be empty
            const imageSettings = {};
            // Provide an offset if required. This will be used both in the response to image "start"
            let offset = 0;
            // Add any other image properties for this image that needs to be sent to the Data Path Session clients. Currently, only one property is being sent, but this can be extended to send any other properties as needed. 
            const imageProperties = { front: job.image.front };
            // Image object to be pushed into the image_starting list
            const image = { jobId: job.id, imageId: job.image.id, offset: offset, settings: imageSettings, sending: false, properties: imageProperties };
            // Add this image to the starting list, it will be processed as soon as possible
            this.m_fiery_path_ifc.addToImageStartingList(image);
        });
        // Events received when the image data has been transferred to the printer. At this point, this image can be printed since the printer is guaranteed to have data for it
        // In general, the printer should initiate printing for an image only once imageCompleting has been received, not otherwise
        this.m_fiery_path_ifc.on("imageCompleting", (message) => {
            winston.info(`imageCompleting: received ${JSON.stringify(message)}`);
            const job = message.params.job;
            const jobObj = this.m_fiery_path_ifc.m_jobs_priority_map.get(job.id);
            if (jobObj) {
                // If the job is in the process of being canceled, we can ignore this imageCompleting request and not print the image
                if (!jobObj.canceling) {
                    // Image object to be pushed into the image_completing list, the actual complete should be sent only once the image has been printed/ejected/stacked.
                    // Once all images have been printed, we need send a response to jobCompleting event. In this case, the last attribute is used to decide when it will be sent
                    // Provide an offset if required. This will be used when sending the "print" message to DataPathSession client(s)
                    let offset = 0;
                    const image = { jobId: job.id, imageId: job.image.id, jobName: jobObj.name, printed: false, sending: false, last: job.image.last, settings: job.image, jobMode: jobObj.jobMode, jobNum: jobObj.jobNum, imageNum: this.m_fiery_path_ifc.m_unique_image_number++, copyNum: job.image.copy, offset: offset };
                    this.m_fiery_path_ifc.addToImageCompletingList(image);
                }
                else
                    winston.info(`imageCompleting: ignoring request since job is being canceled ${JSON.stringify(message)}`);
            }
        });
        // Event received that indicates that all images for this job have been sent to the printer and this job has no more images 
        this.m_fiery_path_ifc.on("jobCompleting", (message) => {
            winston.info(`jobCompleting: received ${JSON.stringify(message)}`);
            const job = message.params.job;
            // This event is an indication that the printer will not receive any more images for this job. Eventually, the printer needs to be send a "complete" for this event
            // The printer must send this "complete" only when all images sent to the printer have been printed/stacked/ejected and not otherwise
            // As mentioned in the imageCompleting event above, the "last" attribute is being used in the simulator to decide when that response will be sent
        });
        // Event received that indicates that the job is being canceled 
        this.m_fiery_path_ifc.on("jobCanceling", (message) => {
            winston.info(`jobCanceling: received ${JSON.stringify(message)}`);
            const job = message.params.job;
            // Mark the job as canceled
            const jobObj = this.m_fiery_path_ifc.m_jobs_priority_map.get(job.id);
            if (jobObj)
                jobObj.canceling = true;
            // The printer should print all the images it wants to before sending a response to this event
            this.m_fiery_path_ifc.cancelJob(job, 60);
        });
        // Event received that indicates that an image data transfer was canceled because the job was canceled 
        this.m_fiery_path_ifc.on("imageCanceling", (message) => {
            winston.info(`imageCanceling: received ${JSON.stringify(message)}`);
            const job = message.params.job;
            // Remove any pending images from the pending list since in this case, we will not get an imageCompleting for this image which would otherwise have cleaned up this list
            this.m_fiery_path_ifc.cleanupimageCompletingPending(job.id, job.image.id);
        });
        // Event received that indicates that the DFE has been suspended
        this.m_fiery_path_ifc.on("dfeSuspended", (message) => {
            winston.info("dfeSuspended: received");
            // Once the DFE is suspended, the printer will not receive any new imageStarting events. However, it will continue to receive imageCompleting events for
            // images that are already being processed. New imageStarting events will be sent only once dfeResumed event is received
        });
        // Event received that indicates that the DFE has been resumed
        this.m_fiery_path_ifc.on("dfeResumed", (message) => {
            winston.info("dfeResumed: received");
            // New imageStarting events will be now be sent since the DFE has resumed
        });
        // Event received that indicates that the DFE has disconnected
        this.m_fiery_path_ifc.on("dfeDisconnected", (message) => {
            winston.info("dfeDisconnected: received");
            // The printer should reinitiating the login sequence at this point. However, this is not being done in the simulator since
            // a websocket disconnect will be detected in session.ts and the login sequence will be reinitiated from there
        });
        // Event received that indicates the DFE has received a cancel request initiated by the printer and is aborting the job(s)
        this.m_fiery_path_ifc.on("dfeAborting", (message) => {
            winston.info("dfeAborting: received");
            // This is for informational purposes only, no particular action is required from the printer
        });
        // Event received that indicates the DFE has started the recovery process
        this.m_fiery_path_ifc.on("jobRecovering", (message) => {
            winston.info("jobRecovering: received");
            // This means that the client had requested job recovery and the DFE has started that process
            // Client can initiate a "recover/resune" sequence as an example when there is a jam. In that case, the client should also send an "abort" to the data path session
            // The client should first remove any leftover job(s) and recreate any internal data structures for this job as needed
        });
        ///////////////////////////////////////////////////////////////////////////////////
        // Fiery Path Optional Events.
        // Check the Fiery Path Addendum document for more details on these events if available.
        // Most printers will not need to implement handlers for these events, but they are available for printers that need them. The details for these events 
        // will be provided by Fiery if they are required to be implemented by the printer
        ///////////////////////////////////////////////////////////////////////////////////
        // Event that allows the printer to specify the order of sheets in each subset for products that require that. This event will be received for each new subset that is detected
        this.m_fiery_path_ifc.on("subsetPreparing", (message) => {
            winston.info("subsetPreparing: received");
            // Some products need to specify the order of sheets in each subset. To allow that, the client will receive a subsetPreparing event for
            // each new subset that is detected. The client can specify the order of sheets that it needs when it responds to each subsetPreparing event
            // Check the Fiery Path Addendum document for more details on how to respond to this event and specify the order of sheets in each subset
        });
        // Event that allows the printer to perform constrain checking before the job starts
        this.m_fiery_path_ifc.on("jobPreparing", (message) => {
            winston.info("jobPreparing: received");
            // Some products need to validate a job’s properties against the current configuration/state of the printer before allowing 
            // the job to print. To support that, the client will receive a jobPreparing event
            // Check the Fiery Path Addendum document for more details on how to respond to this event
        });
        // Event recevied that indicates that the job is being updated with new properties when a force print is initiated
        this.m_fiery_path_ifc.on("jobUpdating", (message) => {
            winston.info("jobUpdating: received");
            // This event is an indication that the job properties are being updated when a force print is initiated.
            // Check the Fiery Path Addendum document for more details on this event
        });
        // IPDS specific event that indicates that the DFE is starting to discard the images in a job. This event will be received only for IPDS jobs
        this.m_fiery_path_ifc.on("dfeDiscardStarting", (message) => {
            winston.info("dfeDiscardStarting: received");
            // Check the Fiery Path Addendum document for more details on this event
        });
        // IPDS specific event that indicates that the DFE will resume sending the images in a job. This event will be received only for IPDS jobs
        this.m_fiery_path_ifc.on("dfeDiscardEnding", (message) => {
            winston.info("dfeDiscardEnding: received");
            // Check the Fiery Path Addendum document for more details on this event
        });
        // Details for this event will be provided by Fiery if is required to be implemented by the printer
        this.m_fiery_path_ifc.on("dfeDPMessage", (message) => {
            winston.info("dfeDPMessage: received");
            // Check the Fiery Path Addendum document for more details on this event
        });
        // Details for this event will be provided by Fiery if is required to be implemented by the printer
        this.m_fiery_path_ifc.on("dfeEVMessage", (message) => {
            winston.info("dfeEVMessage: received");
            // Check the Fiery Path Addendum document for more details on this event
        });
        // Details for this event will be provided by Fiery if is required to be implemented by the printer
        this.m_fiery_path_ifc.on("dfeOVMessage", (message) => {
            winston.info("dfeOVMessage: received");
            // Check the Fiery Path Addendum document for more details on this event
        });
        // Details for this event will be provided by Fiery if is required to be implemented by the printer
        this.m_fiery_path_ifc.on("dfePIMessage", (message) => {
            winston.info("dfePIMessage: received");
            // Check the Fiery Path Addendum document for more details on this event
        });
        ///////////////////////////////////////////////////////////////////////////////////
        // Data Path Events
        ///////////////////////////////////////////////////////////////////////////////////
        // This event will be received once the login process is completed
        this.m_fiery_path_ifc.on("dataPathImagePrinted", (message) => {
            winston.info(`dataPathImagePrinted: received ${JSON.stringify(message)}`);
            // We have received the indication from DataPathSession that an image has printed
            this.m_fiery_path_ifc.ImagePrintedReceivedFromDataPath(message.jobId, message.imageId);
        });
    }
}
exports.PrinterIfc = PrinterIfc;
//# sourceMappingURL=printer.js.map
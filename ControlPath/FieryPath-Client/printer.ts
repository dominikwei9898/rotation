/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/

import * as path from 'path';
import * as _ from 'underscore';
import * as util from 'util';
import * as fs from 'fs';
import * as winston from 'winston';

import { FieryPathSession } from './session';
import { EventEmitter } from 'events';
import { Credentials } from './session';
import { CommandLineParams } from './app';
import { DataPathSession } from './datapath';

///////////////////////////////////////////////////////////////////////////////////
// Class that implements the Fiery Path Printer API's using FieryPathSession
///////////////////////////////////////////////////////////////////////////////////

class FieryPathIfc extends EventEmitter
{
    public m_jobs_priority_map: Map<string, any> = new Map();
    private m_image_starting_normal_list: any[] = [];
    private m_image_starting_rush_list: any[] = [];
    private m_image_completing_normal_list: any[] = [];
    private m_image_completing_rush_list: any[] = [];
    private m_image_completing_pending_normal_list: any[] = [];
    private m_image_completing_pending_rush_list: any[] = [];
    private m_input_slot_list: any[] = [];
    private m_ready_status_set_once: boolean = false;
    private m_defaultInkList = ["cyan", "magenta", "yellow", "black"];
    private m_data_path_session: DataPathSession;

    public m_fiery_path_session: FieryPathSession;

    constructor()
    {
        super();

        // Use FieryPathSession to send REST API's and receive websocket events
        this.m_fiery_path_session = new FieryPathSession();

        // Initialize and start the DataPathSession. This is to let Data Path know when to print images after an imageCompleting has been received
        this.m_data_path_session = new DataPathSession();
        if (this.m_data_path_session.Initialize(this))
            this.m_data_path_session.Start();
        else
            winston.info("Unable to initialize Data Path Session");
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Callbacks required for FieryPathSession
    ///////////////////////////////////////////////////////////////////////////////////

    public InfoCallback(str: string, context: any)
    {
        winston.info(str);
    }

    public WarnCallback(str: string, context: any)
    {
        winston.warn(str);
    }

    public ErrorCallback(str: string, context: any)
    {
        winston.error(str);
    }

    public DebugCallback(str: string, context: any)
    {
        winston.debug(str);
    }

    public LoginCallback(context: any)
    {
        // The login process is successfully completed, initiate the next steps
        context.emit("FieryPath_Session_Connected");
    }

    public EventCallback(method: string, message: any, context: any)
    {
        // We have recevied an event from the server
        context.emit(method, message);
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Server State
    ///////////////////////////////////////////////////////////////////////////////////

    // Check if the server is ready/connected to determine if subsequent commands can be sent
    public async CheckServerStatus()
    {
        try
        {
            const result = await this.m_fiery_path_session.Get("/system/dfe/status", {});
            if (result != null && result.state != null && (result.state.toLowerCase() === "connected" || result.state.toLowerCase() === "ready"))
            {
                winston.info("CheckServerStatus: Server is initialized");

                // Server is ready to accept commands, the first action should be set the "initializing" state for the printer
                this.setPrinterInitializingStatus();
            }
            else
                setTimeout(() => this.CheckServerStatus(), 100);
        }
        catch (err)
        {
            winston.error("CheckServerStatus: error - " + err);
            setTimeout(() => this.CheckServerStatus(), 100);
        }
    }

    // Set printer status as initializing to let the server know it is in the process of connecting/reconnecting
    private async setPrinterInitializingStatus()
    {
        try
        {
            const result = await this.m_fiery_path_session.Put("/system/printer/status", {"action": "initializing"});

            // Send the configuration
            this.sendConfiguration();
        }
        catch (err)
        {
            winston.error("setPrinterInitializingStatus: error - " + err);
            setTimeout(() => this.CheckServerStatus(), 100);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Configuration
    ///////////////////////////////////////////////////////////////////////////////////

    private async sendConfiguration()
    {
        winston.info("sendConfiguration: received");

        try
        {
            const result = await this.sendDeviceConfiguration();
            if (result)
            {
                if (result.status && result.status.statusCode && result.status.statusCode === 201)
                {
                    // Send all other configurations
                    await this.sendInkConfiguration();
                    await this.sendLocalizationConfiguration();

                    // Perform any printer specific initialization. Once all initialization has completed, set the printer status to ready
                    this.setPrinterReadyStatus();
                }
                else
                    winston.error('sendConfiguration: not sending any other configurations, statusCode is - ' + result.status.statusCode + ', body is - ' + result.body);
            }
            else
                winston.error('sendConfiguration: not sending any other configurations');
        }
        catch (err)
        {
            winston.error('sendConfiguration: not sending any other configurations - ' + err);
        }
    }

    private async sendDeviceConfiguration()
    {
        winston.info('sendDeviceConfiguration: sending device specific information');

        const engine_info = { EngineInfo: [ { key: 'Model', value: 'MyModel' }, { key: 'Manufacturer', value: 'MyCompany' }, { key: 'Serial Number', value: 'abc123' } ] };
        const configurationObj = { configuration: { engine: engine_info } };

        try
        {
            const result = await this.m_fiery_path_session.Post('/system/printer/oem/configuration', configurationObj);
            winston.info('sendDeviceConfiguration: device specific information sent');
            return result;
        }
        catch (err)
        {
            winston.error('sendDeviceConfiguration: sending device specific information - ' + err);
            throw err;
        }
    }

    private async sendInk(inkObj: any)
    {
        winston.info('sendInk: sending ink - ' + inkObj.ink.type);

        try
        {
            const res = await this.m_fiery_path_session.Post('/system/printer/inks', inkObj);
            if (res)
            {
                if (res.status && res.status.statusCode && res.status.statusCode === 201)
                    await this.m_fiery_path_session.Put('/system/printer/inks/' + res.body.id + '/status', { ink: { level: 100 } });
                else
                {
                    // Retry only if the server cannot accept inks at this time
                    if (res.status && res.status.statusCode && res.status.statusCode === 503)
                    {
                        await new Promise((resolve) => setTimeout(resolve, 2000));
                        await this.sendInk(inkObj);
                    }
                    else
                        winston.info('sendInk: sending inks - not retrying since status code is not 503');
                }
            }
            else
            {
                await new Promise((resolve) => setImmediate(resolve));
                await this.sendInk(inkObj);
            }
        }
        catch (err)
        {
            winston.error('sendInk: sending inks error - ' + err);
            await new Promise((resolve) => setImmediate(resolve));
            await this.sendInk(inkObj);
        }

        winston.info('sendInk: sending inks success for - ' + inkObj.ink.type);
    }

    private async sendInkConfiguration()
    {
        winston.info('sendInkConfiguration: sending inks');

        try
        {
            // First delete any current inks if set
            await this.m_fiery_path_session.Delete('/system/printer/inks', {});
            const inkTypeList = _.clone(this.m_defaultInkList);
            const inkTypeListLength = inkTypeList.length;
            winston.info('sendInkConfiguration: inkTypeList is ' + JSON.stringify(inkTypeList));

            const dropSizeArray = [1.1, 2.2, 3.3];
            let number = 1;

            for (let s = 0; s < inkTypeListLength; s++)
            {
                const inkType = inkTypeList.shift();
                const inkObj = { ink: {type: inkType, capacity: 100, number: number++, dropsize: dropSizeArray} };
                await this.sendInk(inkObj);
            }
        }
        catch (err)
        {
            winston.error('sendInkConfiguration: deleting inks error - ' + err);
        }
    }

    private async sendLocalizationConfiguration()
    {
        winston.info('sendLocalizationConfiguration: sending localization');

        const localizationObj = {
            localization: [
                {
                    'This is string one':
                    {
                        fr_FR: "C'est la première chaîne",
                        ja_JP: "これは文字列1です"
                    }
                },
                {
                    'This is string two':
                    {
                        fr_FR: "C'est la chaîne deux",
                        ja_JP: "これは文字列2です"
                    }
                }
            ]
        };

        try
        {
            await this.m_fiery_path_session.Post('/system/printer/localization', localizationObj);
            winston.info('sendLocalizationConfiguration: sending localization success');
        }
        catch (err)
        {
            winston.error('sendLocalizationConfiguration: sending localization error - ' + err);
        }
    }

    async setPrinterReadyStatus()
    {
        try
        {
            // Set printer status to ready
            winston.info("setPrinterReadyStatus: setting printer status to ready");
            const result = await this.m_fiery_path_session.Put("/system/printer/status/", { "action": "ready" });
            winston.info("setPrinterReadyStatus: setting printer status to ready success");
        }
        catch (err)
        {
            winston.error("setPrinterReadyStatus: setting printer status to ready error - " + err);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // jobStarting event response handler
    ///////////////////////////////////////////////////////////////////////////////////

    // Sends job "start" for the jobStarting event received
    public async sendJobStart(jobId: string, settings: any)
    {
        winston.info(`sendJobStart: received for jobId = ${jobId}`);

        return new Promise(async (resolve, reject) =>
        {
            const sendRequest = async () =>
            {
                try
                {
                    const result = await this.m_fiery_path_session.Put(`/jobs/${jobId}/status`, { status: { settings, state: "start" } });
                    if (this.canRetry(result))
                    {
                        if (result !== null && result.body !== undefined)
                        {
                            if (result.body.success !== undefined && result.body.success === true)
                            {
                                winston.info(`sendJobStart: successfully sent for jobId = ${jobId}`);
                                resolve(result);
                            }
                            else
                            {
                                winston.warn(`sendJobStart: did not receive a successful result for jobId = ${jobId}`);
                                setImmediate(sendRequest);
                            }
                        }
                        else
                        {
                            winston.warn(`sendJobStart: did not receive a successful object for jobId = ${jobId}`);
                            setImmediate(sendRequest);
                        }
                    }
                    else
                        reject(result)
                }
                catch (error)
                {
                    winston.warn(`sendJobStart}: received error for jobId = ${jobId}: ${error}`);
                    setImmediate(sendRequest); 
                }
            };

            // Send the initial request and if required retry until successful
            sendRequest();
        });
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // imageStarting event response handlers
    ///////////////////////////////////////////////////////////////////////////////////

    // Sends image "start" for an "imageStarting" event received. The client should initiate this as quickly as possible. Any delay in sending this message can result in performance issues
    private async sendImageStart(image: any)
    {
        const imageToSend = { id: image.imageId, offset: image.imageOffset };
        winston.info(`sendImageStart: request sent for jobId = ${image.jobId}, imageId = ${image.imageId}`);
        try
        {
            // Send the image "start" using the Put method
            const result = await this.m_fiery_path_session.Put(`/jobs/${image.jobId}/status`, { status: { image: imageToSend, settings: image.settings, state: "start" } });
            winston.info(`sendImageStart: response received for jobId = ${image.jobId}, imageId = ${image.imageId}`);

            // If ths response indicates we should not retry sending the message in case of a failure, ignore it.
            if (this.canRetry(result))
            {
                if (result !== null && result.body !== undefined)
                {
                    if (result.body.success !== undefined && result.body.success === true)
                    {
                        // We have successfully sent the image "start"
                        let imageRemoved = null;
                        if (image.rush)
                        {
                            imageRemoved = this.m_image_starting_rush_list.shift();

                            // Add this to the pending list if it is not already part of the completing list.This can only happen if we receive a imageCompleting before we get a response to image start ###
                            if (!this.listContainsImageId(this.m_image_completing_rush_list, imageRemoved.jobId, imageRemoved.imageId))
                                this.m_image_completing_pending_rush_list.push({ jobId: imageRemoved.jobId, imageId: imageRemoved.imageId });
                            else
                                winston.warn(`sendImageStart ignoring adding to image completing rush pending list since imageCompleting was already received for jobId ${imageRemoved.jobId} imageId ${imageRemoved.imageId}`);
                        }
                        else
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
                    else
                    {
                        // We need to resend the message
                        winston.warn(`sendImageStart did not receive a successful result for jobId = ${image.jobId}, imageId = ${image.imageId}`);
                        image.sending = false;
                        this.processImageStart();
                    }
                }
                else
                {
                    // We need to resend the message
                    winston.warn(`sendImageStart did not receive result object for jobId = ${image.jobId}, imageId = ${image.imageId}`);
                    image.sending = false;
                    this.processImageStart();
                }
            }
            else
            {
                // No need to resend the message, remove it from our list
                if (image.rush)
                    this.m_image_starting_rush_list.shift();
                else
                    this.m_image_starting_normal_list.shift();
            }
        }
        catch (error)
        {
            // We need to resend the message
            winston.error(`sendImageStart received error for jobId = ${image.jobId}, imageId = ${image.imageId}: ${error}`);
            image.sending = false;
            this.processImageStart();
        }
    }

    // Process the next image for which a "start" needs to be sent
    public processImageStart()
    {
        let image = null;
        if (this.m_image_starting_rush_list.length !== 0)
            image = this.m_image_starting_rush_list[0];
        else if (this.m_image_starting_normal_list.length !== 0)
            image = this.m_image_starting_normal_list[0];

        if (image !== null)
        {
            const job = this.m_jobs_priority_map.get(image.jobId);
            if (job !== null && job.started)
            {
                if (!image.sending)
                {
                    image.sending = true;
                    this.sendImageStart(image);
                }
            }
        }
    }

    // Add the image received with imageStarting to the internal list
    public addToImageStartingList(image: any)
    {
        const job = this.m_jobs_priority_map.get(image.jobId);
        if (job !== null)
        {
            // Add the appropriate list (rush or normal). Keeping a rush list allows responding to rush jobs quickly
            if (job.rush)
            {
                image.rush = true;
                this.m_image_starting_rush_list.push(image);
            }
            else
            {
                image.rush = false;
                this.m_image_starting_normal_list.push(image);
            }

            if (job.started)
            {
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
    private async sendImageComplete(image: any)
    {
        winston.info(`sendImageComplete: request sent for jobId = ${image.jobId}, imageId = ${image.imageId}`);
        try
        {
            // Send the image "complete" using the Put method
            const result = await this.m_fiery_path_session.Put(`/jobs/${image.jobId}/status`, { status: { image: { id: image.imageId }, state: "complete" } });
            winston.info(`sendImageComplete: response received for jobId = ${image.jobId}, imageId = ${image.imageId}`);

            // If ths response indicates we should not retry sending the message in case of a failure, ignore it.
            if (this.canRetry(result))
            {
                if (result !== null && result.body !== undefined)
                {
                    if (result.body.success !== undefined && result.body.success === true)
                    {
                        // We have successfully sent the image "complete"
                        let imageRemoved = null;
                        if (image.rush)
                            imageRemoved = this.m_image_completing_rush_list.shift();
                        else
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
                    else
                    {
                        // We need to resend the message
                        winston.warn(`sendImageComplete did not receive a successful result for jobId = ${image.jobId}, imageId = ${image.imageId}`);
                        image.sending = false;
                        this.processImageComplete();
                    }
                }
                else
                {
                    // We need to resend the message
                    winston.warn(`sendImageComplete did not receive result object for jobId = ${image.jobId}, imageId = ${image.imageId}`);
                    image.sending = false;
                    this.processImageComplete();
                }
            }
            else
            {
                // No need to resend the message, remove it from our list
                if (image.rush)
                    this.m_image_completing_rush_list.shift();
                else
                    this.m_image_completing_normal_list.shift();
            }
        }
        catch (error)
        {
            // We need to resend the message
            winston.error(`sendImageComplete received error for jobId = ${image.jobId}, imageId = ${image.imageId}: ${error}`);
            image.sending = false;
            this.processImageComplete();
        }
    }

    // Process the next image for which a "complete" needs to be sent. This means the next image "complete" should be sent once that image has been printed/stacked/ejected
    // and not before that. This is also the point to let Data Path know to print images(s). The "complete" should be sent only once a response is recevied from Data Path
    // that the particular image has been printed/stacked/ejected
    private processImageComplete()
    {
        let image = null;
        if (this.m_image_completing_rush_list.length !== 0)
            image = this.m_image_completing_rush_list[0];
        else if (this.m_image_completing_normal_list.length !== 0)
            image = this.m_image_completing_normal_list[0];

        if (image !== null)
        {
            const job = this.m_jobs_priority_map.get(image.jobId);
            if (job !== null && job.started)
            {
                if (!image.sending && image.printed)
                {
                    image.sending = true;
                    this.sendImageComplete(image);

                    // Cleanup the inks entry from the map
                    job.inksMap.delete(image.imageId);
                }
            }
        }
    }

    // Send the print request to DataPathSession client(s) to print/ink the image. The client(s) will send a "printed" response once the image has been sent to electronics and printed/stacked/ejected
    private sendImagePrintToDataPath(image: any, inks: string)
    {
        const inksArray: string[] = inks.split(';').map(item => item.toLowerCase());
        const message = { type: "print", jobId: image.jobId, imageId: image.imageId, jobName: image.jobName, inks: inksArray };
        this.m_data_path_session.SendMessage(JSON.stringify(message));
    }

    // Callback when DataPathSession client(s) indicate that the image has been printed/stacked/ejected
    public ImagePrintedReceivedFromDataPath(jobId: string, imageId: string)
    {
        // Update the "printed" status for this image and trigger processing it
        const job = this.m_jobs_priority_map.get(jobId);
        if (job !== null)
        {
            if (job.rush)
            {
                const imageObj = _.find(this.m_image_completing_rush_list, (item) => item.jobId === jobId && item.imageId === imageId);
                if (imageObj)
                    imageObj.printed = true;
            }
            else
            {
                const imageObj = _.find(this.m_image_completing_normal_list, (item) => item.jobId === jobId && item.imageId === imageId);
                if (imageObj)
                    imageObj.printed = true;
            }

            // Trigger sending image complete
            this.processImageComplete();
        }
        else
            winston.error("Unable to update printed status for " + imageId + ", job " + jobId + " is not available");
    }

    // Add the image received with imageCompleting to the internal list
    public addToImageCompletingList(image: any)
    {
        const job = this.m_jobs_priority_map.get(image.jobId);
        if (job !== null)
        {
            // We should have client(s) connected for DataPathSession. However,if we don't have any, then just mark the image "printed" as true
            image.printed = !this.m_data_path_session.ClientsConnected();

            // Add the appropriate list (rush or normal). Keeping a rush list allows responding to rush jobs quickly
            if (job.rush)
            {
                image.rush = true;
                this.m_image_completing_rush_list.push(image);
                this.m_image_completing_pending_rush_list = _.reject(this.m_image_completing_pending_rush_list, (listImage) => image.jobId === listImage.jobId && image.imageId === listImage.imageId);
            }
            else
            {
                image.rush = false;
                this.m_image_completing_normal_list.push(image);
                this.m_image_completing_pending_normal_list = _.reject(this.m_image_completing_pending_normal_list, (listImage) => image.jobId === listImage.jobId && image.imageId === listImage.imageId);
            }

            if (job.started)
            {
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

    private async completeJob(jobId: string)
    {
        this.cleanupJob(jobId);

        try
        {
            const result = await this.sendJobComplete(jobId);
            winston.info(`completeJob: successful for jobId = ${jobId}`); 
        }
        catch (error)
        {
            winston.error(`completeJob: received error for jobId = ${jobId}: ${error}`); 
        }
    }

    // Sends job "complete" for the jobCompleting event received
    private async sendJobComplete(jobId: string)
    {
        winston.info(`sendJobComplete: received for jobId = ${jobId}`);

        return new Promise(async (resolve, reject) =>
        {
            const sendRequest = async () =>
            {
                try
                {
                    const result = await this.m_fiery_path_session.Put(`/jobs/${jobId}/status`, { status: { state: "complete" } });
                    if (this.canRetry(result))
                    {
                        if (result !== null && result.body !== undefined)
                        {
                            if (result.body.success !== undefined && result.body.success === true)
                            {
                                winston.info(`sendJobComplete: successfully sent for jobId = ${jobId}`);
                                resolve(result);
                            }
                            else
                            {
                                winston.warn(`sendJobComplete: did not receive a successful result for jobId = ${jobId}`);
                                setImmediate(sendRequest);
                            }
                        }
                        else
                        {
                            winston.warn(`sendJobComplete: did not receive a successful object for jobId = ${jobId}`);
                            setImmediate(sendRequest);
                        }
                    }
                    else
                        reject(result)
                }
                catch (error)
                {
                    winston.warn(`sendJobComplete}: received error for jobId = ${jobId}: ${error}`);
                    setImmediate(sendRequest);
                }
            };

            // Send the initial request and if required retry until successful
            sendRequest();
        });
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // jobCanceling event response handlers
    ///////////////////////////////////////////////////////////////////////////////////

    // Send a response to the jobCanceling event. When sent, it indicates the client is done with all images and does not intend to respond to any images for this job
    public async cancelJob(job: any, retryCount: number)
    {
        // Check if there are any outstanding images in the image completing list. If there are, it means, we have images currently being printed, so wait until all do get printed
        let continueProcessing = this.isImageCompletingListEmpty(job.id);
        if (continueProcessing || retryCount <=0)
        {
            winston.info(`cancelJob: received for ${job.id}`);
            this.cleanupJob(job.id);

            try
            {
                const result = await this.sendJobCancel(job.id);
                winston.info(`cancelJob: successful for jobId = ${job.id}`);

                // Inform the Data Path Session to cleanup its memory for any images that were transferred but for which imageCanceling was sent
                if (this.m_data_path_session.ClientsConnected())
                {
                    const message = { type: "cancel", jobId: job.id };
                    this.m_data_path_session.SendMessage(JSON.stringify(message));
                }
            }
            catch (error)
            {
                winston.error(`cancelJob: received error for jobId = ${job.id}: ${error}`);
            }
        }
        else
        {
            retryCount--;

            // In the case where the client needs to wait because images are in the pipeline, then try again in a bit
            const waitTimer = 1;
            winston.info(`cancelJob: waiting for images to be printed for jobId = ${job.id}, retry count is ${retryCount}, retrying in ${waitTimer} seconds...`);
            setTimeout(() => this.cancelJob(job, retryCount), waitTimer * 1000);
        }
    }

    // Sends job "cancel" for the jobCanceling event received
    private async sendJobCancel(jobId: string)
    {
        winston.info(`sendJobCancel: received for jobId = ${jobId}`);

        return new Promise(async (resolve, reject) =>
        {
            const sendRequest = async () =>
            {
                try
                {
                    const result = await this.m_fiery_path_session.Put(`/jobs/${jobId}/status`, { status: { state: "cancel" } });
                    if (this.canRetry(result))
                    {
                        if (result !== null && result.body !== undefined)
                        {
                            if (result.body.success !== undefined && result.body.success === true)
                            {
                                winston.info(`sendJobCancel: successfully sent for jobId = ${jobId}`);
                                resolve(result);
                            }
                            else
                            {
                                winston.warn(`sendJobCancel: did not receive a successful result for jobId = ${jobId}`);
                                setImmediate(sendRequest);
                            }
                        }
                        else
                        {
                            winston.warn(`sendJobCancel: did not receive a successful object for jobId = ${jobId}`);
                            setImmediate(sendRequest);
                        }
                    }
                    else
                        reject(result)
                }
                catch (error)
                {
                    winston.warn(`sendJobCancel}: received error for jobId = ${jobId}: ${error}`);
                    setImmediate(sendRequest);
                }
            };

            // Send the initial request and if required retry until successful
            sendRequest();
        });
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Print Helpers
    ///////////////////////////////////////////////////////////////////////////////////

    // Cleans up the job from any lists and maps
    public cleanupJob(jobId: string): void
    {
        winston.info(`cleanupJob: received for jobId = ${jobId}`);

        // Remove all images from the image lists for this jobId except if the image is currently being sent.In that case, it will be removed once the response is received
        const job = this.m_jobs_priority_map.get(jobId);
        if (job)
        {
            if (job.rush)
            {
                this.m_image_starting_rush_list = _.reject(this.m_image_starting_rush_list, (image) => image.jobId === jobId && !image.sending);
                this.m_image_completing_rush_list = _.reject(this.m_image_completing_rush_list, (image) => image.jobId === jobId && !image.sending);
                this.m_image_completing_pending_rush_list = _.reject(this.m_image_completing_pending_rush_list, (image) => image.jobId === jobId);
            }
            else
            {
                this.m_image_starting_normal_list = _.reject(this.m_image_starting_normal_list, (image) => image.jobId === jobId && !image.sending);
                this.m_image_completing_normal_list = _.reject(this.m_image_completing_normal_list, (image) => image.jobId === jobId && !image.sending);
                this.m_image_completing_pending_normal_list = _.reject(this.m_image_completing_pending_normal_list, (image) => image.jobId === jobId);
            }

            this.m_jobs_priority_map.delete(jobId);
        }

        winston.info(`JOBS_MAP = ${this.m_jobs_priority_map.size}, NORMAL_STARTING_LIST = ${this.m_image_starting_normal_list.length}, NORMAL_COMPLETING_LIST = ${this.m_image_completing_normal_list.length}, RUSH_STARTING_LIST = ${this.m_image_starting_rush_list.length}, RUSH_COMPLETING_LIST = ${this.m_image_completing_rush_list.length}, NORMAL_COMPLETING_PENDING_LIST = ${this.m_image_completing_pending_normal_list.length}, RUSH_COMPLETING_PENDING_LIST = ${this.m_image_completing_pending_rush_list.length}`);
    }

    // Cleans up the image completing pending list for a canceled image
    public cleanupimageCompletingPending(jobId: string, imageId: string): void
    {
        // If an imageCanceling event is received, we need to cleanup the image completing pending list
        // Normally, this will get cleaned up when a corresponding imageCompleting event is received. But in this case,
        // we will not get that event and hence need to do this cleanup separately
        this.m_image_completing_pending_rush_list = _.reject(this.m_image_completing_pending_rush_list, (image) => image.jobId === jobId && image.imageId === imageId);
        this.m_image_completing_pending_normal_list = _.reject(this.m_image_completing_pending_normal_list, (image) => image.jobId === jobId && image.imageId === imageId);
    }

    // Determines if we should resend a message
    private canRetry(result: any): boolean
    {
        if (result !== null && result.status !== undefined)
        {
            if (result.status.statusCode === 404 || result.status.statusCode === 422 || result.status.statusCode === 500)
                return false;
        }
        return true;
    }

    // Checks if the given list has any image for the jobId specified
    private listContainsJobId(list: any[], jobId: string): boolean
    {
        return _.some(list, (image) => image.jobId === jobId);
    }

    // Checks if the given list has any image for the jobId and imageId specified
    private listContainsImageId(list: any[], jobId: string, imageId: string): boolean
    {
        return _.some(list, (image) => image.jobId === jobId && image.imageId === imageId);
    }

    // Determines if all the completing list is empty
    private isImageCompletingListEmpty(jobId: string): boolean
    {
        let empty = true;

        const job = this.m_jobs_priority_map.get(jobId);
        if (job)
        {
            if (job.rush)
            {
                if (this.listContainsJobId(this.m_image_completing_rush_list, jobId))
                    empty = false;
            }
            else
            {
                if (this.listContainsJobId(this.m_image_completing_normal_list, jobId))
                    empty = false;
            }
        }

        return empty;
    }

    // Determines if all the image lists are empty
    private areImageListsEmpty(jobId: string): boolean
    {
        let empty = true;

        const job = this.m_jobs_priority_map.get(jobId);
        if (job)
        {
            if (job.rush)
            {
                if (this.listContainsJobId(this.m_image_starting_rush_list, jobId) || this.listContainsJobId(this.m_image_completing_rush_list, jobId) || this.listContainsJobId(this.m_image_completing_pending_rush_list, jobId))
                    empty = false;
            }
            else
            {
                if (this.listContainsJobId(this.m_image_starting_normal_list, jobId) || this.listContainsJobId(this.m_image_completing_normal_list, jobId) || this.listContainsJobId(this.m_image_completing_pending_normal_list, jobId))
                    empty = false;
            }
        }

        return empty;
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Class that provides the Print API Interface
///////////////////////////////////////////////////////////////////////////////////

export class PrinterIfc
{
    private m_fiery_path_ifc: FieryPathIfc;
    private m_credentials: Credentials; 

    constructor(commandlineParams: CommandLineParams)
    {
        // Create an instance of FieryPathIfc
        this.m_fiery_path_ifc = new FieryPathIfc();
        this.m_credentials = { username: commandlineParams['username'] as string, password: commandlineParams['password'] as string, accessrights: { a1: commandlineParams['accessrights'] as string } };

        const tls_authorization: boolean = false; // Set to true or false as needed
        if (!tls_authorization)
            process.env.NODE_TLS_REJECT_UNAUTHORIZED = "0";
        else
            winston.info("TLS Authorization is enabled, trusted certificate on server is required");

        // Initialize FieryPathIfc using the credentials passed through command-line
        if (this.m_fiery_path_ifc.m_fiery_path_session.Initialize(this.m_fiery_path_ifc, commandlineParams['ip'] as string, this.m_credentials))
        {
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
        this.m_fiery_path_ifc.on("FieryPath_Session_Connected", () =>
        {
            winston.debug("FieryPath_Session_Connected: received")
            this.m_fiery_path_ifc.CheckServerStatus()
        });

        // Event recevied that indicates the start of a job
        this.m_fiery_path_ifc.on("jobStarting", async (message) =>
        {
            winston.info("");
            winston.info(`********************** New Job received (JobId = ${message.params.job.id}) **********************`);
            winston.info("jobStarting: received " + JSON.stringify(message));
            const job = message.params.job;
            
            // Cleanup any leftover values
            this.m_fiery_path_ifc.cleanupJob(job.id);

            // Add to the jobs priority map
            const imageToInksMap: Map<string, string> = new Map();
            const jobObj = { name: job.name, rush: job.rush, started: false, canceling: false, inksMap: imageToInksMap };
            this.m_fiery_path_ifc.m_jobs_priority_map.set(job.id, jobObj);

            // Respond to the message and send a "start" for it
            await this.m_fiery_path_ifc.sendJobStart(job.id, {});
            jobObj.started = true;

            // Initiate the process of sending image "starts" if there are any in the list
            this.m_fiery_path_ifc.processImageStart();
        });

        // Events received for every new image in the job that should be processed. The printer should save any meta data it needs and respond immediately
        this.m_fiery_path_ifc.on("imageStarting", (message) =>
        {
            winston.info(`imageStarting: received ${JSON.stringify(message)}`);
            const job = message.params.job;

            // Save the inks for this image. This will be required later when a "print" message is sent to DataPathSession client
            const jobObj = this.m_fiery_path_ifc.m_jobs_priority_map.get(job.id);
            if (jobObj)
                jobObj.inksMap.set(job.image.id, job.image.inks);
            else
                winston.error(`Unable to set ink for image ${job.image.id} for job ${job.id}`);

            // This is for future enhancements, settings should be empty
            const imageSettings: any = {};

            // Provide an offset if required
            let offset = 0;

            // Image object to be pushed into the image_starting list
            const image = {jobId: job.id, imageId: job.image.id, offset: offset, settings: imageSettings, rush: false, sending: false};

            // Add this image to the starting list, it will be processed as soon as possible
            this.m_fiery_path_ifc.addToImageStartingList(image);
        });

        // Events received when the image data has been transferred to the printer. At this point, this image can be printed since the printer is guaranteed to have data for it
        // In general, the printer should initiate printing for an image only once imageCompleting has been received, not otherwise
        this.m_fiery_path_ifc.on("imageCompleting", (message) =>
        {
            winston.info(`imageCompleting: received ${JSON.stringify(message)}`);
            const job = message.params.job;

            const jobObj = this.m_fiery_path_ifc.m_jobs_priority_map.get(job.id);
            if (jobObj)
            {
                // If the job is in the process of being canceled, we can ignore this imageCompleting request and not print the image
                if (!jobObj.canceling)
                {
                    // Image object to be pushed into the image_completing list, the actual complete should be sent only once the image has been printed/ejected/stacked.
                    // Once all images have been printed, we need send a response to jobCompleting event. In this case, the last attribute is used to decide when it will be sent
                    const image = { jobId: job.id, imageId: job.image.id, jobName: jobObj.name, rush: false, printed: false, sending: false, last: job.image.last };
                    this.m_fiery_path_ifc.addToImageCompletingList(image);
                }
                else
                    winston.info(`imageCompleting: ignoring request since job is being canceled ${JSON.stringify(message)}`);
            }
        });

        // Event received that indicates that all images for this job have been sent to the printer and this job has no more images 
        this.m_fiery_path_ifc.on("jobCompleting", (message) =>
        {
            winston.info(`jobCompleting: received ${JSON.stringify(message)}`);
            const job = message.params.job;

            // This event is an indication that the printer will not receive any more images for this job. Eventually, the printer needs to be send a "complete" for this event
            // The printer must send this "complete" only when all images sent to the printer have been printed/stacked/ejected and not otherwise
            // As mentioned in the imageCompleting event above, the "last" attribute is being used in the simulator to decide when that response will be sent
            
        });

        // Event received that indicates that the job is being canceled 
        this.m_fiery_path_ifc.on("jobCanceling", (message) =>
        {
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
        this.m_fiery_path_ifc.on("imageCanceling", (message) =>
        {
            winston.info(`imageCanceling: received ${JSON.stringify(message)}`);
            const job = message.params.job;

            // Remove any pending images from the pending list since in this case, we will not get an imageCompleting for this image which would otherwise have cleaned up this list
            this.m_fiery_path_ifc.cleanupimageCompletingPending(job.id, job.image.id);
        });

        // Event received that indicates that the DFE has been suspended
        this.m_fiery_path_ifc.on("dfeSuspended", (message) =>
        {
            winston.info("dfeSuspended: received");

            // Once the DFE is suspended, the printer will not receive any new imageStarting events. However, it will continue to receive imageCompleting events for
            // images that are already being processed. New imageStarting events will be sent only once dfeResumed event is received

        });

        // Event received that indicates that the DFE has been resumed
        this.m_fiery_path_ifc.on("dfeResumed", (message) =>
        {
            winston.info("dfeResumed: received");
            
            // New imageStarting events will be now be sent since the DFE has resumed

        });

        // Event received that indicates that the DFE has disconnected
        this.m_fiery_path_ifc.on("dfeDisconnected", (message) =>
        {
            winston.info("dfeDisconnected: received");

            // The printer should reinitiating the login sequence at this point. However, this is not being done in the simulator since
            // a websocket disconnect will be detected in session.ts and the login sequence will be reinitiated from there

        });

        // Event received that indicates the DFE has received a cancel request initiated by the printer and is aborting the job(s)
        this.m_fiery_path_ifc.on("dfeAborting", (message) =>
        {
            winston.info("dfeAborting: received");

            // This is for informational purposes only, no particular action is required from the printer

        });

        // Event received that indicates the DFE has started the recovery process
        this.m_fiery_path_ifc.on("jobRecovering", (message) =>
        {
            winston.info("jobRecovering: received");

            // This is for informational purposes only, no particular action is required from the printer

        });

        ///////////////////////////////////////////////////////////////////////////////////
        // Data Path Events
        ///////////////////////////////////////////////////////////////////////////////////

        // This event will be received once the login process is completed
        this.m_fiery_path_ifc.on("dataPathImagePrinted", (message) =>
        {
            winston.info(`dataPathImagePrinted: received ${JSON.stringify(message)}`);

            // We have received the indication from DataPathSession that an image has printed
            this.m_fiery_path_ifc.ImagePrintedReceivedFromDataPath(message.jobId, message.imageId);
        });
    }
}


/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/hadoop/hadoop_subsystem/ShuffleService.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/workflow/failure_causes/NetworkError.h"
#include "../HadoopComputeServiceMessage.h"
#include "wrench/simulation/Simulation.h"

WRENCH_LOG_CATEGORY(shuffle_service, "Log category for Shuffle Actor");

namespace wrench {

/**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param job: the job to execute
     * @param compute_resources: a set of hostnames
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    ShuffleService::ShuffleService(const std::string &hostname, MRJob *job,
                                   const std::set<std::string> &compute_resources,
                                   std::map<std::string, std::string> property_list,
                                   std::map<std::string, double> messagepayload_list
    ) : Service(hostname, "shuffle_service",
                "shuffle_service"), job(job) {
        this->compute_resources = compute_resources;
        this->setProperties(this->default_property_values, std::move(property_list));
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));
    }

    /**
     * @brief Stop the compute service - must be called by the stop()
     *        method of derived classes
     */
    void ShuffleService::stop() {
        Service::stop();
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int ShuffleService::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);
        this->state = Service::UP;

        WRENCH_INFO("New ShuffleService starting (%s) on %ld hosts",
                    this->mailbox_name.c_str(), this->compute_resources.size());

        /** Main loop **/
        while (this->processNextMessage()) {
        }

        WRENCH_INFO("ShuffleService on host %s terminating cleanly!", S4U_Simulation::getHostName().c_str());
        return 0;
    }

    /**
     * @brief Wait for and react to any incoming message
     *
     * @return false if the daemon should terminate, true otherwise
     *
     * @throw std::runtime_error
     */
    bool ShuffleService::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &error) { WRENCH_INFO(
                    "Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_INFO("ShuffleService::ShuffleService() Got a [%s] message", message->getName().c_str());
        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                HadoopComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;
        } else if (auto msg = std::dynamic_pointer_cast<NotifyShuffleServiceToFetchMapperOutputMessage>(message)) {
            try {
                S4U_Mailbox::putMessage(msg->mapper_mailbox,
                                        new RequestMapperMaterializedOutputMessage(this->mailbox_name,
                                                                                   this->getMessagePayloadValue(
                                                                                           MRJobExecutorMessagePayload::MAP_SIDE_SHUFFLE_REQUEST_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) { WRENCH_INFO(
                        "Network error... Failing");
                return false;
            }
            return true;
        } else if (auto msg = std::dynamic_pointer_cast<SendMaterializedOutputMessage>(message)) {
            mapper_outputs.push_back(msg->materialized_bytes);

            if (mapper_outputs.size() == this->job->getNumMappers()) {
                // We have received output files from all of the mapper services.
                std::vector<double> reducer_output_vec(this->job->getNumReducers());
                while (reducer_output_vec.size() < this->job->getNumReducers()) {
                    // This is a hack that just adds values together until we reach `merge_factor`
                    double temp = mapper_outputs.back();
                    mapper_outputs.pop_back();
                    reducer_output_vec.push_back(temp + mapper_outputs.back());
                    mapper_outputs.pop_back();
                }

                // TODO: Figure out the cost in flops of the Shuffle phase merge factor algorithm
                Simulation::compute(1.0);

                for (int i = 0; i < reducer_output_vec.size(); i++) {
                    S4U_Mailbox::putMessage(this->job->getReducerMailboxes()[i],
                                            new TransferOutputFromMapperToReducerMessage(reducer_output_vec[i],
                                                                                         this->getMessagePayloadValue(
                                                                                                 MRJobExecutorMessagePayload::MAP_OUTPUT_MATERIALIZED_BYTES_PAYLOAD)));
                }
                return false;
            }
            return true;
        } else {
            throw std::runtime_error(
                    "MRJobExecutor::processNextMessage(): Received an unexpected [" + message->getName() +
                    "] message!");
        }
    }
}

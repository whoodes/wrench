/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/hadoop/hadoop_subsystem/ReducerService.h"
#include "wrench/services/compute/hadoop/HadoopComputeService.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/workflow/failure_causes/NetworkError.h"
#include "../HadoopComputeServiceMessage.h"

WRENCH_LOG_CATEGORY(reducer_service, "Log category for Reducer Actor");

namespace wrench {
    ReducerService::ReducerService(const std::string &hostname,
                                   MRJob *job, const std::set<std::string> compute_resources,
                                   std::map<std::string, std::string> property_list,
                                   std::map<std::string, double> messagepayload_list
    ) : Service(hostname, "reducer_service",
                "reducer_service"), job(job) {
        this->compute_resources = compute_resources;
        this->setProperties(this->default_property_values, std::move(property_list));
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));
    }

    void ReducerService::stop() {
        Service::stop();
    }

    /** Main loop */
    int ReducerService::main() {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
        while (this->processNextMessage()) {
        }

        WRENCH_INFO("ReducerService on host %s terminating cleanly!", S4U_Simulation::getHostName().c_str());
        return 0;
    }

    /**
     * @brief Wait for and react to any incoming message
     *
     * @return false if the daemon should terminate, true otherwise
     *
     * @throw std::runtime_error
     */
    bool ReducerService::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &error) { WRENCH_INFO(
                    "Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_INFO("ReducerService::ReducerService() Got a [%s] message", message->getName().c_str());
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
        } else if (auto msg = std::dynamic_pointer_cast<TransferOutputFromMapperToReducerMessage>(message)) {
            // TODO: Handle this message.
            return false;
        } else {
            throw std::runtime_error(
                    "ReducerService::processNextMessage(): Received an unexpected [" + message->getName() +
                    "] message!");
        }
    }
}
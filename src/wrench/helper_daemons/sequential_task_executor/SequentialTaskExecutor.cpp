/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::SequentialTaskExecutor class implements a simple
 *  sequential task executor abstraction.
 */

#include <simgrid_S4U_util/S4U_Simulation.h>
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "exception/WRENCHException.h"
#include "SequentialTaskExecutor.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sequential_task_executor, "Log category for Sequential Task Executor");

namespace wrench {

		/**
	 * @brief Constructor, which starts the daemon for the service on a host
	 *
	 * @param hostname is the name of the host
	 */
		SequentialTaskExecutor::SequentialTaskExecutor(std::string hostname, std::string callback_mailbox) :
						S4U_DaemonWithMailbox("sequential_task_executor", "sequential_task_executor") {

				this->hostname = hostname;
			this->callback_mailbox = callback_mailbox;

			// Start my daemon on the host
			this->start(this->hostname);
		}

		/**
		 * @brief Terminate the sequential task executor
		 */
		void SequentialTaskExecutor::stop() {
			// Send a termination message to the daemon's mailbox
			S4U_Mailbox::put(this->mailbox_name, new StopDaemonMessage());
		}

		/**
	 	 * @brief Kills the sequential task executor
	   */
		void SequentialTaskExecutor::kill() {
			this->kill_actor();
		}

		/**
		 * @brief Have the sequential job executor execute a standard job
		 *
		 * @param job is a pointer to the job
		 *
		 * @return 0 on success
		 */
		int SequentialTaskExecutor::runTask(WorkflowTask *task) {
			// Send a "run a task" message to the daemon's mailbox
			S4U_Mailbox::put(this->mailbox_name, new RunTaskMessage(task));
			return 0;
		};


		/**
	 * @brief Main method of the sequential task executor daemon
	 *
	 * @return 0 on termination
	 */
		int SequentialTaskExecutor::main() {

			XBT_INFO("New Sequential Task Executor starting (%s) ", this->mailbox_name.c_str());

			bool keep_going = true;
			while (keep_going) {

				// Wait for a message
				std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);

				switch (message->type) {

					case SimulationMessage::STOP_DAEMON: {
						keep_going = false;
						break;
					}

					case SimulationMessage::RUN_TASK: {
						std::unique_ptr<RunTaskMessage> m(static_cast<RunTaskMessage *>(message.release()));

						// Run the task
						XBT_INFO("Executing task %s (%lf flops)", m->task->getId().c_str(), m->task->getFlops());
						m->task->setRunning();
						S4U_Simulation::compute(m->task->flops);

						// Set the task completion time and state
						m->task->end_date = S4U_Simulation::getClock();
						m->task->setCompleted();

						// Send the callback
						XBT_INFO("Notifying mailbox %s that task %s has finished",
										 this->callback_mailbox.c_str(),
										 m->task->id.c_str());
						S4U_Mailbox::dput(this->callback_mailbox,
															new TaskDoneMessage(m->task, this));

						break;
					}

					default: {
						throw WRENCHException("Unknown message type");
					}
				}
			}

			XBT_INFO("Sequential Task Executor Daemon on host %s terminated!", S4U_Simulation::getHostName().c_str());
			return 0;
		}

}
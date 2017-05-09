/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <csignal>

#include "compute_services/multicore_job_executor/MulticoreJobExecutor.h"
#include "logging/TerminalOutput.h"
#include "simulation/Simulation.h"
#include "SimulationTimestamp.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simulation, "Log category for Simulation");

namespace wrench {

    /* Exception handler to catch SIGBART signals from SimGrid (which should
     * probably throw exceptions at some point)
     */
    void signal_handler(int signal) {
      if (signal == SIGABRT) {
        std::cerr << "[ ABORTING ]" << std::endl;
        std::_Exit(EXIT_FAILURE);
      } else {
        std::cerr << "Unexpected signal " << signal << " received\n";
      }
    }

    /**
     * @brief  Constructor
     *
     */
    Simulation::Simulation() {

      // Customize the logging format
      // xbt_log_control_set("root.fmt:[%d][%h:%t(%i)]%e%m%n");
      xbt_log_control_set("root.fmt:[%d][%h:%t]%e%m%n");

      // Setup the SIGABRT handler
      auto previous_handler = std::signal(SIGABRT, signal_handler);
      if (previous_handler == SIG_ERR) {
        std::cerr << "SIGABRT handler setup failed... uncaught exceptions will lead to unclean terminations\n";
      }

      // Create the S4U simulation wrapper
      this->s4u_simulation = std::unique_ptr<S4U_Simulation>(new S4U_Simulation());

    }

    /**
     * @brief Destructor
     */
    Simulation::~Simulation() {
      // this->s4u_simulation->shutdown();
    }

    /**
     * @brief Initialize the simulation, which parses out WRENCH-specific and SimGrid-specific
     * command-line arguments, if any
     *
     * @param argc: main()'s argument count
     * @param argv: main()'s argument list
     *
     * @throw std::invalid_argument
     */
    void Simulation::init(int *argc, char **argv) {
      if (*argc < 1) {
        throw std::invalid_argument("Invalid argc argument (must be >= 1)");
      }
      if ((argv == nullptr) || (*argv == nullptr)) {
        throw std::invalid_argument("Invalid argument argv (nullptr)");
      }

      int i;
      int skip = 0;
      for (i = 1; i < *argc; i++) {
        if (!strncmp(argv[i], "--wrench-no-color", strlen("--wrench-no-color"))) {
          TerminalOutput::disableColor();
          skip++;
        }
        argv[i] = argv[i + skip];
      }
      *argc = i - skip;

      this->s4u_simulation->initialize(argc, argv);
    }

    /**
     * @brief Append a SimulationEvent to the event trace
     * @param event
     */
    template <class T> void Simulation::newTimestamp(SimulationTimestamp<T> *event) {
      this->output.addTimestamp(event);
    }

    /**
     * @brief Instantiate a simulated platform
     *
     * @param filename: the path to a SimGrid XML platform description file
     *
     * @throw std::runtime_error
     */
    void Simulation::instantiatePlatform(std::string filename) {
      if (!this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation is not initialized");
      }
      static bool already_setup = false;
      if (already_setup) {
        throw std::runtime_error("Platform already setup");
      }
      this->s4u_simulation->setupPlatform(filename);
      already_setup = true;
    }

    /**
     * @brief Retrieve the list of names of all the hosts in the platform
     *
     * @return a set of hostnames
     *
     * @throw std::runtime_error
     */
    std::set<std::string> Simulation::getHostnameList() {
      return this->s4u_simulation->getAllHostnames();
    }

    /**
		 * @brief Launch the simulation
		 *
		 */
    void Simulation::launch() {
      if (!this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation is not initialized");
      }
      this->s4u_simulation->runSimulation();
    }

    /**
     * @brief Adds a MulticoreJobExecutor to the simulation
     *
     * @param executor: a unique pointer to a MulticoreJobExecutor object, the ownership of which is
     *        then transferred to WRENCH
     *
     * @throw std::invalid_argument
     */
    void Simulation::add(std::unique_ptr<MulticoreJobExecutor> executor) {
      if (!this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation is not initialized");
      }

      executor->setSimulation(this);
      // Add a unique ptr to the list of Compute Services
      running_compute_services.push_back(std::move(executor));
      return;
    }

    /**
     * @brief Set a WMS for the simulation
     *
     * @param wms: a unique pointer to a WMS instantiation
     */
    void Simulation::setWMS(std::unique_ptr<WMS> wms) {
      this->wms = std::move(wms);
    }

    /**
     * @brief Obtain the list of compute services
     *
     * @return a vector of compute services
     */
    std::set<ComputeService *> Simulation::getComputeServices() {
      std::set<ComputeService *> set = {};
      for (auto it = this->running_compute_services.begin(); it != this->running_compute_services.end(); it++) {
        set.insert((*it).get());
      }
      return set;
    }

    /**
     * @brief Shutdown all running compute services on the platform
     */
    void Simulation::shutdownAllComputeServices() {

      for (int i = 0; i < this->running_compute_services.size(); i++) {
        this->running_compute_services[i]->stop();
      }
    }


    /**
     * @brief Create an unregistered executor (i.e., that the Simulation instance will not know anything about)
     *
     * @param hostname: the hostname in the simulated platform
     * @param supports_standard_jobs: true if the executor supports StandardJob submissions, false otherwise
     * @param support_pilot_jobs: true if the executor supports PilotJob submissions, false otherwise
     * @param plist: a property (<string,string>) list
     * @param num_cores: the number of cores
     * @param ttl: the time-to-live of the executor
     * @param suffix: a suffix to be appended to the process name (useful for debugging)
     *
     * @throw std::invalid_argument
     */
    MulticoreJobExecutor *Simulation::createUnregisteredMulticoreJobExecutor(std::string hostname,
                                                                             bool supports_standard_jobs,
                                                                             bool supports_pilot_jobs,
                                                                             std::map<MulticoreJobExecutor::Property, std::string> plist,
                                                                             unsigned int num_cores,
                                                                             double ttl,
                                                                             PilotJob *pj,
                                                                             std::string suffix) {

      // Create the compute service
      MulticoreJobExecutor *executor;
      try {
        executor = new MulticoreJobExecutor(hostname, plist, num_cores, ttl, pj, suffix);
        executor->setSupportStandardJobs(supports_standard_jobs);
        executor->setSupportPilotJobs(supports_pilot_jobs);
      } catch (std::invalid_argument e) {
        throw e;
      }
      return executor;
    }


    /**
     * @brief Remove a compute service from the list of known compute services
     *
     * @param cs: a pointer to the compute service
     */
    void Simulation::mark_compute_service_as_terminated(ComputeService *compute_service) {
      for (int i = 0; i < this->running_compute_services.size(); i++) {
        if (this->running_compute_services[i].get() == compute_service) {
          this->terminated_compute_services.push_back(std::move(this->running_compute_services[i]));
          this->running_compute_services.erase(this->running_compute_services.begin() + i);
          return;
        }
      }
      // If we didn't find the service, this means it was a hidden service that was
      // used as a building block for another higher-level service, which is fine
      return;
    }

};
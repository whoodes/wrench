/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_RANDOMSCHEDULER_H
#define WRENCH_RANDOMSCHEDULER_H

#include "wrench/wms/scheduler/Scheduler.h"

namespace wrench {


    /**
     * @brief A random Scheduler
     */
    class RandomScheduler : public Scheduler {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/
    public:
        void scheduleTasks(JobManager *job_manager,
                           std::map<std::string, std::vector<WorkflowTask *>> ready_tasks,
                           const std::set<ComputeService *> &compute_services);

    /***********************/
    /** \endcond           */
    /***********************/
    };

};

#endif //WRENCH_RANDOMSCHEDULER_H
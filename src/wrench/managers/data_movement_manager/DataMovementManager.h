/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_DATAMOVEMENTMANAGER_H
#define WRENCH_DATAMOVEMENTMANAGER_H


#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>

namespace wrench {

    class Workflow;
    class WorkflowFile;
    class StorageService;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A helper daemon (co-located with a WMS) that handles data movement operations
     */
    class DataMovementManager : public S4U_DaemonWithMailbox {

    public:

        DataMovementManager(Workflow *workflow);

        ~DataMovementManager();

        void stop();

        void kill();

        void submitFileCopy(WorkflowFile *file, StorageService *src, StorageService *dst);

    private:


        int main();

        // Relevant workflow
        Workflow *workflow;

        bool processNextMessage();

    };

    /***********************/
    /** \endcond            */
    /***********************/


};


#endif //WRENCH_DATAMOVEMENTMANAGER_H
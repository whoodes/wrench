/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_STORAGESERVICE_H
#define WRENCH_STORAGESERVICE_H


#include <string>
#include <set>

#include "wrench/services/Service.h"
#include "wrench/workflow/execution_events/FailureCause.h"
#include "wrench/services/file_registry/FileRegistryService.h"
#include <wrench/workflow/job/StandardJob.h>

namespace wrench {

    class Simulation;

    class WorkflowFile;

    class FailureCause;

    /**
     * @brief The storage service base class
     */
    class StorageService : public Service {

    public:


        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/

        void stop() override;

        virtual std::map<std::string, double> getFreeSpace();

        virtual std::map<std::string, double> getTotalSpace();

        std::set<std::string> getMountPoints();

        virtual bool lookupFile(WorkflowFile *file);

        virtual bool lookupFile(WorkflowFile *file, std::string dst_mountpoint);

        virtual void deleteFile(WorkflowFile *file, std::shared_ptr<FileRegistryService>file_registry_service=nullptr);

        virtual void deleteFile(WorkflowFile *file, std::string dst_mountpoint, std::shared_ptr<FileRegistryService>file_registry_service=nullptr);

        virtual void readFile(WorkflowFile *file);
        virtual void readFile(WorkflowFile *file, std::string src_mountpoint);

        virtual void writeFile(WorkflowFile *file);
        virtual void writeFile(WorkflowFile *file, std::string dst_mountpoint);

        virtual void downloadFile(WorkflowFile *file, std::string dst_mountpoint, unsigned long buffer_size);
        virtual void downloadFile(WorkflowFile *file, std::string src_mountpoint, std::string dst_mountpoint, unsigned long buffer_size);


        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        virtual void copyFile(WorkflowFile *file, std::shared_ptr<StorageService> src, std::string src_mountpoint, std::string dst_mountpoint);
        virtual void copyFile(WorkflowFile *file, std::shared_ptr<StorageService> src);

        virtual void initiateFileCopy(std::string answer_mailbox,
                                      WorkflowFile *file,
                                      std::shared_ptr<StorageService> src,
                                      std::string src_mountpoint,
                                      std::string dst_mountpoint);


        virtual void deleteFile(WorkflowFile *file, WorkflowJob* job, std::shared_ptr<FileRegistryService> file_registry_service=nullptr);
        virtual bool lookupFile(WorkflowFile *file, WorkflowJob*);
        virtual void readFile(WorkflowFile *file, WorkflowJob* job);
        virtual void writeFile(WorkflowFile *file, WorkflowJob* job);
        virtual void copyFile(WorkflowFile *file, std::shared_ptr<StorageService> src, WorkflowJob* src_job, WorkflowJob* dst_job);
        static void readFiles(std::set<WorkflowFile *> files,
                              std::map<WorkflowFile *, std::pair<std::shared_ptr<StorageService>, std::string>> file_locations,
                              std::shared_ptr<StorageService> default_storage_service,
                              std::set<WorkflowFile*>& files_in_scratch,
                              WorkflowJob* job = nullptr);

        static void writeFiles(std::set<WorkflowFile *> files,
                               std::map<WorkflowFile *, std::pair<std::shared_ptr<StorageService>, std::string>> file_locations,
                               std::shared_ptr<StorageService> default_storage_service,
                               std::set<WorkflowFile*>& files_in_scratch,
                               WorkflowJob* job = nullptr);




        StorageService(const std::string &hostname,
                       const std::set<std::string> mount_points,
                       const std::string &service_name,
                       const std::string &data_mailbox_name_prefix);


    protected:

        friend class Simulation;
        friend class FileRegistryService;
        friend class FileTransferThread;


        void stageFile(WorkflowFile *);
        void stageFile(WorkflowFile *, std::string mountpoint);

        void removeFileFromStorage(WorkflowFile *, std::string mountpoint);

        /** The mount points (each one corresponds to a  disk) **/
        std::set<std::string> mount_points;

        /** @brief The map of mount points and the set of files stored on those mount points inside the storage service */
        std::map<std::string, std::set<WorkflowFile *>> stored_files;

        /** @brief The map of mount points and their total capacities*/
        std::map<std::string, double> capacities;

        /** @brief The map of mount points and their total occupied capacities */
        /** @brief The  service's occupied space */
        std::map<std::string, double> occupied_space;

        /** @brief The service's buffer size */
        unsigned long buffer_size;


        /***********************/
        /** \endcond          **/
        /***********************/

    private:


        enum FileOperation {
            READ,
            WRITE,
        };

        static void writeOrReadFiles(FileOperation action, std::set<WorkflowFile *> files,
                                     std::map<WorkflowFile *, std::pair<std::shared_ptr<StorageService>, std::string>> file_locations,
                                     std::shared_ptr<StorageService> default_storage_service,
                                     std::set<WorkflowFile*>& files_in_scratch,
                                     WorkflowJob* job);


    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_STORAGESERVICE_H

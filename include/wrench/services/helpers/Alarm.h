/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */



#ifndef WRENCH_ALARM_H
#define WRENCH_ALARM_H


#include <string>
#include <memory>
#include <wrench/services/Service.h>
#include "AlarmServiceProperty.h"

namespace wrench {

    class Alarm: public Service {

    public:
        Alarm(double date, std::string hostname, std::string reply_mailbox_name,
              WorkflowJob* job, std::string suffix);

    private:
        //private parameters
        std::map<std::string, std::string> default_property_values =
                {{AlarmServiceProperty::ALARM_TIMEOUT_MESSAGE_PAYLOAD,          "1024"}
                };

        double date;
        std::string reply_mailbox_name;
        WorkflowJob* job;

        int main() override;

    };

};


#endif //WRENCH_ALARM_H
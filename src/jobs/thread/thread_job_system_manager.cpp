////////////////////////////////////////////////////////////////////////////////
//         Distributed under the Boost Software License, Version 1.0.         //
//            (See accompanying file LICENSE or copy at                       //
//                 https://www.boost.org/LICENSE_1_0.txt)                     //
////////////////////////////////////////////////////////////////////////////////

#include "jobs/thread/thread_job_system_manager.h"

#include <memory>
#include <vector>

#include "core/error_handling.h"
#include "jobs/job.h"
#include "jobs/job_system_manager.h"
#include "jobs/thread/thread_job_system.h"

namespace iris
{

JobSystem *ThreadJobSystemManager::create_job_system()
{
    ensure(job_system_ == nullptr, "job system already created");

    job_system_ = std::make_unique<ThreadJobSystem>();
    return job_system_.get();
}

void ThreadJobSystemManager::add(const std::vector<Job> &jobs)
{
    job_system_->add_jobs(jobs);
}

void ThreadJobSystemManager::wait(const std::vector<Job> &jobs)
{
    job_system_->wait_for_jobs(jobs);
}

}

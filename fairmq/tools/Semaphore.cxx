/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Semaphore.h"

namespace fair::mq::tools
{

Semaphore::Semaphore()
    : Semaphore(0)
{}

Semaphore::Semaphore(std::size_t initial_count)
    : fCount(initial_count)
{}

auto Semaphore::Wait() -> void
{
    std::unique_lock<std::mutex> lk(fMutex);
    if (fCount > 0) {
        --fCount;
    } else {
        fCv.wait(lk, [this] { return fCount > 0; });
        --fCount;
    }
}

auto Semaphore::Signal() -> void
{
    std::unique_lock<std::mutex> lk(fMutex);
    ++fCount;
    lk.unlock();
    fCv.notify_one();
}

auto Semaphore::GetCount() const -> std::size_t
{
    std::unique_lock<std::mutex> lk(fMutex);
    return fCount;
}

SharedSemaphore::SharedSemaphore()
    : fSemaphore(std::make_shared<Semaphore>())
{}

SharedSemaphore::SharedSemaphore(std::size_t initial_count)
    : fSemaphore(std::make_shared<Semaphore>(initial_count))
{}

auto SharedSemaphore::Wait() -> void
{
    fSemaphore->Wait();
}

auto SharedSemaphore::Signal() -> void
{
    fSemaphore->Signal();
}

auto SharedSemaphore::GetCount() const -> std::size_t
{
    return fSemaphore->GetCount();
}

} // namespace fair::mq::tools

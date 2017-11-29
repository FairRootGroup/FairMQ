/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * ShmChunk.h
 *
 * @since 2016-04-08
 * @author A. Rybalchenko
 */

#ifndef SHMCHUNK_H_
#define SHMCHUNK_H_

#include <thread>
#include <chrono>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp>

#include "FairMQLogger.h"

namespace bipc = boost::interprocess;

class SegmentManager
{
  public:
    static SegmentManager& Instance()
    {
        static SegmentManager man;
        return man;
    }

    void InitializeSegment(const std::string& op, const std::string& name, const size_t size = 0)
    {
        if (!fSegment)
        {
            try
            {
                if (op == "open_or_create")
                {
                    fSegment = new bipc::managed_shared_memory(bipc::open_or_create, name.c_str(), size);
                }
                else if (op == "create_only")
                {
                    fSegment = new bipc::managed_shared_memory(bipc::create_only, name.c_str(), size);
                }
                else if (op == "open_only")
                {
                    int numTries = 0;
                    bool success = false;

                    do
                    {
                        try
                        {
                            fSegment = new bipc::managed_shared_memory(bipc::open_only, name.c_str());
                            success = true;
                        }
                        catch (bipc::interprocess_exception& ie)
                        {
                            if (++numTries == 5)
                            {
                                LOG(ERROR) << "Could not open shared memory after " << numTries << " attempts, exiting!";
                                exit(EXIT_FAILURE);
                            }
                            else
                            {
                                LOG(DEBUG) << "Could not open shared memory segment on try " << numTries << ". Retrying in 1 second...";
                                LOG(DEBUG) << ie.what();

                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                            }
                        }
                    }
                    while (!success);
                }
                else
                {
                    LOG(ERROR) << "Unknown operation when initializing shared memory segment: " << op;
                }
            }
            catch (std::exception& e)
            {
                LOG(ERROR) << "Exception during shared memory segment initialization: " << e.what() << ", application will now exit";
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            LOG(INFO) << "Segment already initialized";
        }
    }

    bipc::managed_shared_memory* Segment() const
    {
        if (fSegment)
        {
            return fSegment;
        }
        else
        {
            LOG(ERROR) << "Segment not initialized";
            exit(EXIT_FAILURE);
        }
    }

  private:
    SegmentManager()
        : fSegment(nullptr)
    {}

    bipc::managed_shared_memory* fSegment;
};

struct alignas(16) ExMetaHeader
{
    uint64_t fSize;
    bipc::managed_shared_memory::handle_t fHandle;
};

// class ShmChunk
// {
//   public:
//     ShmChunk()
//         : fHandle()
//         , fSize(0)
//     {
//     }

//     ShmChunk(const size_t size)
//         : fHandle()
//         , fSize(size)
//     {
//         void* ptr = SegmentManager::Instance().Segment()->allocate(size);
//         fHandle = SegmentManager::Instance().Segment()->get_handle_from_address(ptr);
//     }

//     ~ShmChunk()
//     {
//         SegmentManager::Instance().Segment()->deallocate(SegmentManager::Instance().Segment()->get_address_from_handle(fHandle));
//     }

//     bipc::managed_shared_memory::handle_t GetHandle() const
//     {
//         return fHandle;
//     }

//     void* GetData() const
//     {
//         return SegmentManager::Instance().Segment()->get_address_from_handle(fHandle);
//     }

//     size_t GetSize() const
//     {
//         return fSize;
//     }

//   private:
//     bipc::managed_shared_memory::handle_t fHandle;
//     size_t fSize;
// };

// typedef bipc::managed_shared_ptr<ShmChunk, bipc::managed_shared_memory>::type ShPtrType;

// struct ShPtrOwner
// {
//     ShPtrOwner(const ShPtrType& other)
//         : fPtr(other)
//         {}

//     ShPtrOwner(const ShPtrOwner& other)
//         : fPtr(other.fPtr)
//         {}

//     ShPtrType fPtr;
// };

#endif /* SHMCHUNK_H_ */

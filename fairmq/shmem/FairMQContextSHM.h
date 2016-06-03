/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQCONTEXTSHM_H_
#define FAIRMQCONTEXTSHM_H_

class FairMQContextSHM
{
  public:
    /// Constructor
    FairMQContextSHM(int numIoThreads);
    FairMQContextSHM(const FairMQContextSHM&) = delete;
    FairMQContextSHM operator=(const FairMQContextSHM&) = delete;

    virtual ~FairMQContextSHM();
    void* GetContext();
    void Close();

  private:
    void* fContext;
};

#endif /* FAIRMQCONTEXTSHM_H_ */

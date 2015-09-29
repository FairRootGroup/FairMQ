/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQContext.h
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQCONTEXTZMQ_H_
#define FAIRMQCONTEXTZMQ_H_

class FairMQContextZMQ
{
  public:
    /// Constructor
    FairMQContextZMQ(int numIoThreads);

    virtual ~FairMQContextZMQ();
    void* GetContext();
    void Close();

  private:
    void* fContext;

    /// Copy Constructor
    FairMQContextZMQ(const FairMQContextZMQ&);
    FairMQContextZMQ operator=(const FairMQContextZMQ&);
};

#endif /* FAIRMQCONTEXTZMQ_H_ */

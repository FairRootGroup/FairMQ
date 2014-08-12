/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQPoller.h
 *
 * @since 2014-01-23
 * @author A. Rybalchenko
 */

#ifndef FAIRMQPOLLER_H_
#define FAIRMQPOLLER_H_

class FairMQPoller
{
  public:
    virtual void Poll(int timeout) = 0;
    virtual bool CheckInput(int index) = 0;
    virtual bool CheckOutput(int index) = 0;

    virtual ~FairMQPoller() {};
};

#endif /* FAIRMQPOLLER_H_ */
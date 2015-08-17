/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQConfigurable.h
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQCONFIGURABLE_H_
#define FAIRMQCONFIGURABLE_H_

#include <string>

class FairMQConfigurable
{
  public:
    enum
    {
        Last = 1
    };
    FairMQConfigurable();
    virtual ~FairMQConfigurable();

    virtual void SetProperty(const int key, const std::string& value);
    virtual std::string GetProperty(const int key, const std::string& default_ = "");
    virtual void SetProperty(const int key, const int value);
    virtual int GetProperty(const int key, const int default_ = 0);
};

#endif /* FAIRMQCONFIGURABLE_H_ */

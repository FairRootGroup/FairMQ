/*
 * FairMQConfigurable.h
 *
 *  Created on: Oct 25, 2012
 *      Author: dklein
 */

#ifndef FAIRMQCONFIGURABLE_H_
#define FAIRMQCONFIGURABLE_H_

#include "Rtypes.h"
#include "TString.h"


class FairMQConfigurable
{
  public:
    enum {
      Last = 1
    };
    FairMQConfigurable();
    virtual void SetProperty(Int_t key, TString value, Int_t slot = 0);
    virtual TString GetProperty(Int_t key, TString default_ = "", Int_t slot = 0);
    virtual void SetProperty(Int_t key, Int_t value, Int_t slot = 0);
    virtual Int_t GetProperty(Int_t key, Int_t default_ = 0, Int_t slot = 0);
    virtual ~FairMQConfigurable();
};

#endif /* FAIRMQCONFIGURABLE_H_ */

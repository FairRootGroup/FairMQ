/**
 * FairMQConfigurable.h
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
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
    virtual void SetProperty(const Int_t& key, const TString& value, const Int_t& slot = 0);
    virtual TString GetProperty(const Int_t& key, const TString& default_ = "", const Int_t& slot = 0);
    virtual void SetProperty(const Int_t& key, const Int_t& value, const Int_t& slot = 0);
    virtual Int_t GetProperty(const Int_t& key, const Int_t& default_ = 0, const Int_t& slot = 0);
    virtual ~FairMQConfigurable();
};

#endif /* FAIRMQCONFIGURABLE_H_ */

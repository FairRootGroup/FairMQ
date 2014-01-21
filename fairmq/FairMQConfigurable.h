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
    enum {
      Last = 1
    };
    FairMQConfigurable();
    virtual void SetProperty(const int& key, const std::string& value, const int& slot = 0);
    virtual std::string GetProperty(const int& key, const std::string& default_ = "", const int& slot = 0);
    virtual void SetProperty(const int& key, const int& value, const int& slot = 0);
    virtual int GetProperty(const int& key, const int& default_ = 0, const int& slot = 0);
    virtual ~FairMQConfigurable();
};

#endif /* FAIRMQCONFIGURABLE_H_ */

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

    virtual ~FairMQPoller() {};

};

#endif /* FAIRMQPOLLER_H_ */
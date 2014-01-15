/**
 * FairMQMessage.h
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQMESSAGE_H_
#define FAIRMQMESSAGE_H_

#include <cstddef> // for size_t


class FairMQMessage
{
  public:
    virtual void Rebuild() = 0;
    virtual void Rebuild(size_t size) = 0;
    virtual void Rebuild(void* data, size_t size) = 0;

    virtual void* GetMessage() = 0;
    virtual void* GetData() = 0;
    virtual size_t GetSize() = 0;
    virtual void SetMessage(void* data, size_t size) = 0;

    virtual void CloseMessage() = 0;
    virtual void Copy(FairMQMessage* msg) = 0;

    virtual ~FairMQMessage() {};
};

#endif /* FAIRMQMESSAGE_H_ */

/**
 * FairMQMessageNN.h
 *
 * @since 2013-12-05
 * @author: A. Rybalchenko
 */

#ifndef FAIRMQMESSAGENN_H_
#define FAIRMQMESSAGENN_H_

#include <cstddef>

#include "FairMQMessage.h"


class FairMQMessageNN : public FairMQMessage
{
  public:
    FairMQMessageNN();
    FairMQMessageNN(size_t size);
    FairMQMessageNN(void* data, size_t size);

    virtual void Rebuild();
    virtual void Rebuild(size_t size);
    virtual void Rebuild(void* data, size_t site);

    virtual void* GetMessage();
    virtual void* GetData();
    virtual size_t GetSize();

    virtual void Copy(FairMQMessage* msg);

    void SetMessage(void* data, size_t size);
    void Clear();

    virtual ~FairMQMessageNN();

  private:
    void* fMessage;
    size_t fSize;
};

#endif /* FAIRMQMESSAGENN_H_ */

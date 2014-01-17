/**
 * FairMQMessage.h
 *
 * @since 2012-12-05
 * @author: D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQMESSAGE_H_
#define FAIRMQMESSAGE_H_

#include <cstddef>

#include <zmq.h>


class FairMQMessage
{
  public:
    FairMQMessage();
    FairMQMessage(size_t size);
    FairMQMessage(void* data, size_t size);

    void Rebuild();
    void Rebuild(size_t size);
    void Rebuild(void* data, size_t site);

    zmq_msg_t* GetMessage();
    void* GetData();
    size_t GetSize();

    void Copy(FairMQMessage* msg);

    static void CleanUp(void* data, void* hint);

    virtual ~FairMQMessage();

  private:
    zmq_msg_t fMessage;
};

#endif /* FAIRMQMESSAGE_H_ */

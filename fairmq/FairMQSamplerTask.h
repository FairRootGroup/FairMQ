/**
 * FairMQSamplerTask.h
 *
 * @since 2012-11-22
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSAMPLERTASK_H_
#define FAIRMQSAMPLERTASK_H_

#include "FairTask.h"
#include <vector>
#include "TClonesArray.h"
#include <string>
#include "FairMQMessage.h"
#include "FairMQTransportFactory.h"
#include "TString.h"


class FairMQSamplerTask: public FairTask
{
  public:
    FairMQSamplerTask();
    FairMQSamplerTask(const Text_t* name, int iVerbose=1);
    virtual ~FairMQSamplerTask();
    virtual InitStatus Init();
    virtual void Exec(Option_t* opt) = 0;
    void SetBranch(TString branch);
    FairMQMessage* GetOutput();
    void SetTransport(FairMQTransportFactory* factory);

  protected:
    TClonesArray* fInput;
    TString fBranch;
    FairMQMessage* fOutput;
    FairMQTransportFactory* fTransportFactory;
};

#endif /* FAIRMQSAMPLERTASK_H_ */

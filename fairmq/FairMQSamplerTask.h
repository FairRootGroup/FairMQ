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
#include "FairMQMessageZMQ.h"
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
  protected:
    TClonesArray* fInput;
    TString fBranch;
    FairMQMessage* fOutput;
};

#endif /* FAIRMQSAMPLERTASK_H_ */

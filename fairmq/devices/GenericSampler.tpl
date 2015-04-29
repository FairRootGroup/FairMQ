/* 
 * File:   GenericSampler.tpl
 * Author: winckler
 *
 * Created on November 24, 2014, 3:59 PM
 */

template <typename SamplerPolicy, typename OutputPolicy>
GenericSampler<SamplerPolicy,OutputPolicy>::GenericSampler()
  : fNumEvents(0)
  , fEventRate(1)
  , fEventCounter(0)
  , fContinuous(false)
  , fInputFile()
  , fParFile()
  , fBranch()
{
}

template <typename SamplerPolicy, typename OutputPolicy>
GenericSampler<SamplerPolicy,OutputPolicy>::~GenericSampler()
{
}

template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::SetTransport(FairMQTransportFactory* factory)
{
    FairMQDevice::SetTransport(factory);
    // OutputPolicy::SetTransport(factory);
}

template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::InitTask()
{
    SamplerPolicy::InitSampler();
    fNumEvents = SamplerPolicy::GetNumberOfEvent();
}

template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::Run()
{
    // boost::thread resetEventCounter(boost::bind(&GenericSampler::ResetEventCounter, this));

    int sentMsgs = 0;

    boost::timer::auto_cpu_timer timer;

    LOG(INFO) << "Number of events to process: " << fNumEvents;

    do {
        for (int64_t eventNr = 0; eventNr < fNumEvents; ++eventNr)
        {
            //fSamplerTask->SetEventIndex(eventNr);
            FairMQMessage* msg = fTransportFactory->CreateMessage();
            OutputPolicy::SetMessage(msg);
            fChannels["data-out"].at(0).Send(OutputPolicy::SerializeMsg(SamplerPolicy::GetDataBranch(eventNr)));
            ++sentMsgs;

            if (msg)
            {
                msg->CloseMessage();
            }

            // Optional event rate limiting
            // --fEventCounter;
            // while (fEventCounter == 0) {
            //   boost::this_thread::sleep(boost::posix_time::milliseconds(1));
            // }

            if (GetCurrentState() != RUNNING)
            {
              break;
            }
        }
    }
    while ( GetCurrentState() == RUNNING && fContinuous );

    boost::timer::cpu_times const elapsed_time(timer.elapsed());
    LOG(INFO) << "Sent everything in:\n" << boost::timer::format(elapsed_time, 2);
    LOG(INFO) << "Sent " << sentMsgs << " messages!";
}

/*
template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::SendPart()
{
    fChannels["data-out"].at(0).Send(OutputPolicy::GetMessage(), "snd-more");
    OutputPolicy::CloseMessage();
}
*/

template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::SetContinuous(bool flag)
{
    fContinuous = flag;
}

template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::ResetEventCounter()
{
    while (GetCurrentState() == RUNNING)
    {
        try
        {
          fEventCounter = fEventRate / 100;
          boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        }
        catch (boost::thread_interrupted &) 
        {
            LOG(DEBUG) << "resetEventCounter interrupted";
            break;
        }
    }
    LOG(DEBUG) << ">>>>>>> stopping resetEventCounter <<<<<<<";
}

template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::SetProperty(const int key, const std::string& value)
{
    switch (key)
    {
        case InputFile:
            fInputFile = value;
            break;
        case ParFile:
            fParFile = value;
            break;
        case Branch:
            fBranch = value;
            break;
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

template <typename SamplerPolicy, typename OutputPolicy>
std::string GenericSampler<SamplerPolicy,OutputPolicy>::GetProperty(const int key, const std::string& default_/*= ""*/)
{
    switch (key)
    {
        case InputFile:
            return fInputFile;
        case ParFile:
            return fParFile;
        case Branch:
            return fBranch;
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::SetProperty(const int key, const int value)
{
    switch (key)
    {
        case EventRate:
            fEventRate = value;
            break;
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

template <typename SamplerPolicy, typename OutputPolicy>
int GenericSampler<SamplerPolicy,OutputPolicy>::GetProperty(const int key, const int default_/*= 0*/)
{
    switch (key)
    {
        case EventRate:
            return fEventRate;
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

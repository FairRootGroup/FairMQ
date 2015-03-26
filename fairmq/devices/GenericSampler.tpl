/* 
 * File:   GenericSampler.tpl
 * Author: winckler
 *
 * Created on November 24, 2014, 3:59 PM
 */

template <typename SamplerPolicy, typename OutputPolicy>
GenericSampler<SamplerPolicy,OutputPolicy>::GenericSampler() :
  fNumEvents(0),
  fEventRate(1),
  fEventCounter(0),
  fContinuous(false)
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
    //OutputPolicy::SetTransport(factory);
}

template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::Init()
{
  FairMQDevice::Init();
  SamplerPolicy::InitSampler();
  fNumEvents=SamplerPolicy::GetDataBunchNumber();
}

template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::Run()
{
  LOG(INFO) << ">>>>>>> Run <<<<<<<";

  boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));
  // boost::thread resetEventCounter(boost::bind(&GenericSampler::ResetEventCounter, this));
  // boost::thread commandListener(boost::bind(&GenericSampler::ListenToCommands, this));

  int sentMsgs = 0;

  boost::timer::auto_cpu_timer timer;

  LOG(INFO) << "Number of events to process: " << fNumEvents;

//  while ( fState == RUNNING ) {

    do 
    {
        for ( unsigned long eventNr = 0 ; eventNr < fNumEvents; ++eventNr ) 
        {
            //fSamplerTask->SetEventIndex(eventNr);
            FairMQMessage* msg = fTransportFactory->CreateMessage();
            OutputPolicy::SetMessage(msg);
            fPayloadOutputs->at(0)->Send(OutputPolicy::SerializeMsg(
                                            SamplerPolicy::GetDataBranch(eventNr)));
            ++sentMsgs;

            if(msg)
                msg->CloseMessage();
            
            // Optional event rate limiting
            // --fEventCounter;
            // while (fEventCounter == 0) {
            //   boost::this_thread::sleep(boost::posix_time::milliseconds(1));
            // }

            if( fState != RUNNING ) { break; }
        }
    } 
    while ( fState == RUNNING && fContinuous );

//  }

  boost::timer::cpu_times const elapsed_time(timer.elapsed());
  LOG(INFO) << "Sent everything in:\n" << boost::timer::format(elapsed_time, 2);
  LOG(INFO) << "Sent " << sentMsgs << " messages!";

  try {
    rateLogger.interrupt();
    rateLogger.join();
    // resetEventCounter.interrupt();
    // resetEventCounter.join();
    // commandListener.interrupt();
    // commandListener.join();
  }
  catch (boost::thread_resource_error &e) {
    LOG(ERROR) << e.what();
  }

  FairMQDevice::Shutdown();

  // notify parent thread about end of processing.
  boost::lock_guard<boost::mutex> lock(fRunningMutex);
  fRunningFinished = true;
  fRunningCondition.notify_one();
}

/*
template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::SendPart()
{
  fPayloadOutputs->at(0)->Send(OutputPolicy::GetMessage(), "snd-more");
  OutputPolicy::CloseMessage();
}
*/

template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::ResetEventCounter()
{
  while (true) 
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
void GenericSampler<SamplerPolicy,OutputPolicy>::ListenToCommands()
{
  LOG(INFO) << ">>>>>>> ListenToCommands <<<<<<<";

  int received = 0;

  while (true) {
    try {
      FairMQMessage *msg = fTransportFactory->CreateMessage();

      received = fPayloadInputs->at(0)->Receive(msg);

      if (received > 0) {
        // command handling goes here.
        LOG(INFO) << "> received command <";
        received = 0;
      }

      delete msg;

      boost::this_thread::interruption_point();
    }
    catch (boost::thread_interrupted &) {
      LOG(DEBUG) << "commandListener interrupted";
      break;
    }
  }
  LOG(DEBUG) << ">>>>>>> stopping commandListener <<<<<<<";
}

template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::SetProperty(const int key, const std::string& value, const int slot/*= 0*/)
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
      FairMQDevice::SetProperty(key, value, slot);
      break;
  }
}

template <typename SamplerPolicy, typename OutputPolicy>
std::string GenericSampler<SamplerPolicy,OutputPolicy>::GetProperty(const int key, const std::string& default_/*= ""*/, const int slot/*= 0*/)
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
      return FairMQDevice::GetProperty(key, default_, slot);
  }
}

template <typename SamplerPolicy, typename OutputPolicy>
void GenericSampler<SamplerPolicy,OutputPolicy>::SetProperty(const int key, const int value, const int slot/*= 0*/)
{
  switch (key)
  {
    case EventRate:
      fEventRate = value;
      break;
    default:
      FairMQDevice::SetProperty(key, value, slot);
      break;
  }
}

template <typename SamplerPolicy, typename OutputPolicy>
int GenericSampler<SamplerPolicy,OutputPolicy>::GetProperty(const int key, const int default_/*= 0*/, const int slot/*= 0*/)
{
  switch (key)
  {
    case EventRate:
      return fEventRate;
    default:
      return FairMQDevice::GetProperty(key, default_, slot);
  }
}

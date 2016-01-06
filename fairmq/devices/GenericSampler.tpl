/* 
 * File:   GenericSampler.tpl
 * Author: winckler
 *
 * Created on November 24, 2014, 3:59 PM
 */

template <typename T, typename U, typename K, typename L>
base_GenericSampler<T,U,K,L>::base_GenericSampler()
  : fOutChanName("data-out")
  , fNumEvents(0)
  , fCurrentIdx(0)
  , fEventRate(1)
  , fEventCounter(0)
  , fContinuous(false)
  , fTaskList()
{
}

template <typename T, typename U, typename K, typename L>
base_GenericSampler<T,U,K,L>::~base_GenericSampler()
{
}

template <typename T, typename U, typename K, typename L>
void base_GenericSampler<T,U,K,L>::SetTransport(FairMQTransportFactory* factory)
{
    FairMQDevice::SetTransport(factory);
}

template <typename T, typename U, typename K, typename L>
void base_GenericSampler<T,U,K,L>::InitTask()
{
    BindingSendPart();
    BindingGetSocketNumber();
    BindingGetCurrentIndex();

    source_type::InitSource();
    fNumEvents = source_type::GetNumberOfEvent();
}

template <typename T, typename U, typename K, typename L>
void base_GenericSampler<T,U,K,L>::Run()
{
    // boost::thread resetEventCounter(boost::bind(&GenericSampler::ResetEventCounter, this));

    int sentMsgs = 0;

    boost::timer::auto_cpu_timer timer;

    LOG(INFO) << "Number of events to process: " << fNumEvents;

    do
    {
        for (fCurrentIdx = 0; fCurrentIdx < fNumEvents; fCurrentIdx++)
        {
            for (auto& p : fChannels.at(fOutChanName))
            {
                std::unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());
                serialization_type::SetMessage(msg.get());
                source_type::SetIndex(fCurrentIdx);
                ExecuteTasks();
                p.Send(serialization_type::SerializeMsg(source_type::GetOutData()));
                sentMsgs++;

                // Optional event rate limiting
                // --fEventCounter;
                // while (fEventCounter == 0) {
                //   boost::this_thread::sleep(boost::posix_time::milliseconds(1));
                // }

                if (!CheckCurrentState(RUNNING))
                {
                    break;
                }
            }

            if (!CheckCurrentState(RUNNING))
            {
                break;
            }
        }
    }
    while (CheckCurrentState(RUNNING) && fContinuous);

    boost::timer::cpu_times const elapsed_time(timer.elapsed());
    LOG(INFO) << "Sent everything in:\n" << boost::timer::format(elapsed_time, 2);
    LOG(INFO) << "Sent " << sentMsgs << " messages!";
}


template <typename T, typename U, typename K, typename L>
int base_GenericSampler<T,U,K,L>::GetSocketNumber() const
{
    return fChannels.at(fOutChanName).size();
}

template <typename T, typename U, typename K, typename L>
int base_GenericSampler<T,U,K,L>::GetCurrentIndex() const
{
    return fCurrentIdx;
}

template <typename T, typename U, typename K, typename L>
void base_GenericSampler<T,U,K,L>::SetContinuous(bool flag)
{
    fContinuous = flag;
}

template <typename T, typename U, typename K, typename L>
void base_GenericSampler<T,U,K,L>::ResetEventCounter()
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

template <typename T, typename U, typename K, typename L>
void base_GenericSampler<T,U,K,L>::SetProperty(const int key, const int value)
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

template <typename T, typename U, typename K, typename L>
int base_GenericSampler<T,U,K,L>::GetProperty(const int key, const int default_/*= 0*/)
{
    switch (key)
    {
        case EventRate:
            return fEventRate;
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

template <typename T, typename U, typename K, typename L>
void base_GenericSampler<T,U,K,L>::SetProperty(const int key, const std::string& value)
{
    switch (key)
    {
        case OutChannelName:
            fOutChanName = value;
            break;
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

template <typename T, typename U, typename K, typename L>
std::string base_GenericSampler<T,U,K,L>::GetProperty(const int key, const std::string& default_)
{
    switch (key)
    {
        case OutChannelName:
            return fOutChanName;
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

template<typename T, typename U>
using GenericSampler = base_GenericSampler<T,U,int,std::function<void()>>;
typedef std::map<int, std::function<void()>> SamplerTasksMap;

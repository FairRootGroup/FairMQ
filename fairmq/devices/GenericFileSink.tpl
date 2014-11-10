/* 
 * File:   GenericFileSink.tpl
 * Author: winckler
 *
 * Created on October 7, 2014, 7:21 PM
 */

template <typename InputPolicy, typename OutputPolicy>
GenericFileSink<InputPolicy, OutputPolicy>::GenericFileSink() : 
    InputPolicy(),
    OutputPolicy()
{
}

template <typename InputPolicy, typename OutputPolicy>
GenericFileSink<InputPolicy, OutputPolicy>::~GenericFileSink()
{
}

template <typename InputPolicy, typename OutputPolicy>
void GenericFileSink<InputPolicy, OutputPolicy>::SetTransport(FairMQTransportFactory* transport)
{
    FairMQDevice::SetTransport(transport);
    //InputPolicy::SetTransport(transport);
}


template <typename InputPolicy, typename OutputPolicy>
void GenericFileSink<InputPolicy, OutputPolicy>::Init()
{
    FairMQDevice::Init();
    InitOutputFile();
    //InputPolicy::Init();
    //OutputPolicy::Init();
}

template <typename InputPolicy, typename OutputPolicy>
void GenericFileSink<InputPolicy, OutputPolicy>::InitOutputFile()
{
    OutputPolicy::InitOutFile();
}

template <typename InputPolicy, typename OutputPolicy>
void GenericFileSink<InputPolicy, OutputPolicy>::Run()
{
    MQLOG(INFO) << ">>>>>>> Run <<<<<<<";

    boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

    int received = 0;
    int receivedMsg = 0;

    while (fState == RUNNING)
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();
        received = fPayloadInputs->at(0)->Receive(msg);
        if(received>0)
        {
            AddToFile(message(msg));
            receivedMsg++;
        }
        delete msg;
    }

    MQLOG(INFO) << "Received " << receivedMsg << " messages!";
    try 
    {
        rateLogger.interrupt();
        rateLogger.join();
    } 
    catch(boost::thread_resource_error& e) 
    {
        MQLOG(ERROR) << e.what();
    }

    FairMQDevice::Shutdown();

    // notify parent thread about end of processing.
    boost::lock_guard<boost::mutex> lock(fRunningMutex);
    fRunningFinished = true;
    fRunningCondition.notify_one();
}

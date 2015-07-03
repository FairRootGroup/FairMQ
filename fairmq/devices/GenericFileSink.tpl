/* 
 * File:   GenericFileSink.tpl
 * Author: winckler
 *
 * Created on October 7, 2014, 7:21 PM
 */

template <typename InputPolicy, typename OutputPolicy>
GenericFileSink<InputPolicy, OutputPolicy>::GenericFileSink()
    : InputPolicy()
    , OutputPolicy()
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
    // InputPolicy::SetTransport(transport);
}


template <typename InputPolicy, typename OutputPolicy>
void GenericFileSink<InputPolicy, OutputPolicy>::InitTask()
{
    InitOutputFile();
    // InputPolicy::Init();
    // OutputPolicy::Init();
}

template <typename InputPolicy, typename OutputPolicy>
void GenericFileSink<InputPolicy, OutputPolicy>::InitOutputFile()
{
    OutputPolicy::InitOutFile();
}

template <typename InputPolicy, typename OutputPolicy>
void GenericFileSink<InputPolicy, OutputPolicy>::Run()
{
    int receivedMsg = 0;

    while (CheckCurrentState(RUNNING))
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();
        if (fChannels.at("data-in").at(0).Receive(msg) > 0)
        {
            OutputPolicy::AddToFile(InputPolicy::DeSerializeMsg(msg));
            receivedMsg++;
        }
        delete msg;
    }

    MQLOG(INFO) << "Received " << receivedMsg << " messages!";
}

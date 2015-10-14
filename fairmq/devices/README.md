

## Introduction
The Tutorial 7 show how to use three generic devices, namely a generic sampler, processor and filesink.
These three devices are policy based designed classes. These devices all inherit from FairMQDevice and from specific template parameters, namely :
* The sampler inherit from 
 * a sampler policy (e.g a file reader) 
 * an output policy (serialize)
* The processor inherit from 
 * an input policy (deserialize) 
 * an output policy (serialize)
 * a task policy (process data)
* The filesink inherit from 
 * an input policy (deserialize)
 * a storage policy.

Some policy examples (c.f. e.g. BoostSerializer.h) are available in FairRoot/Base/MQ. In this design the user only need to implement the policies, which are, depending on the devices, the serialization/deserialization tasks, the process tasks, the source and the sink implementations.
The message queue implementation are provided by the host class, and do not need to be provided by the users.




## Generic devices
The generic devices are part of the FairMQ library, and can be found in the FairRoot/fairmq/devices directory. 
These generic devices are called host classes and publicly inherit from template parameters called policies. 

### Generic Sampler
In the example of the GenericSampler.h, the beginning of the header look like :

``` C++
// in GenericSampler.h
template <typename T, typename U, typename K, typename L>
class base_GenericSampler : public FairMQDevice, public T, public U
{
    typedef T source_type;
    typedef U serialization_type;
    // etc...
};
```
The host template class base_GenericSampler has four template parameters, i.e. T, U, K, L. The parameter T and U correspond to the source policy and serialization policy, respectively. 

The template parameters K and L are used to define the key and function type of a task container (std::map<K,L>). For the moment, only L=std::function<void()> is supported by the base_GenericSampler host class. The task map mechanism is an experimental feature and might be replaced with boost::signal2. The users do not need to use directly the base_GenericSampler, and define the K and L parameter types. Instead an alias template, GenericSampler, that take two template parameters (source and serialization policies) is defined at the end of the implementation file (GenericSampler.tpl).

``` C++
// in GenericSampler.tpl
// base_GenericSampler implementation ...

// alias template for the policies. K and L types have default values.
template<typename T, typename U>
using GenericSampler = base_GenericSampler<T,U,int,std::function<void()>>;
```

#### Sampler policies

The policies must have at least a couple of methods that will be called by the host class. For example all policies must have default constructor. In addition some functions are required and other are enabled only if the function signatures exist in the policy classes.
##### Input policy (Source)

``` C++
				source_type::InitSource(); 							// must be there to compile
int64_t 		source_type::GetNumberOfEvent(); 					// must be there to compile

				source_type::SetIndex(int64_t eventIdx);			// must be there to compile
CONTAINER_TYPE  source_type::GetOutData(); 							// must be there to compile

				source_type::SetFileProperties(Args&... args); 		// if called by the host, then must be there to compile

 void 			source_type::BindSendPart(std::function<void(int)> callback);		// enabled if exists
 void 			source_type::BindGetSocketNumber(std::function<int()> callback); 	// enabled if exists
 void 			source_type::BindGetCurrentIndex(std::function<int()> callback);	// enabled if exists
```

The function members above that have no returned type means that the returned types are not used and can be anything. 
The CONTAINER_TYPE above must correspond to the input parameter of the serialization_type::SerializeMsg(CONTAINER_TYPE container) function (see below).

##### Output policy (Serialization)

``` C++
		serialization_type::SerializeMsg(CONTAINER_TYPE container);		// must be there to compile
		serialization_type::SetMessage(FairMQMessage* msg); 			// must be there to compile
```


Examples of host class instantiation can be found in FairRoot/example/Tutorial7/run directory. 
Before the instanciation we need to form the sampler type with the proper policies.
For example in FairRoot/example/Tutorial7/run/runSamplerBoost.cxx we form the sampler type as follow:

``` C++
typedef MyDigi                                         		TDigi;				// simple digi class of tutorial7
typedef SimpleTreeReader<TClonesArray>                 		TSamplerPolicy;		// simple root file reader with a TClonesArray container.
typedef BoostSerializer<TDigi>                         		TSerializePolicy; 	// boost serializer for the digi class.
typedef GenericSampler<TSamplerPolicy,TSerializePolicy> 	TSampler; 			// the sampler type.

int main(int argc, char** argv)
{
	// ...
	TSampler sampler;
	// ...
}

```

Once the Sampler type is defined we can instantiate the sampler device as shown above and start the configuration and the state machine. 

Host classes inherit publicly from the policies (called enriched policy), which allow policy function members to be accessible outside the host. Enriched policies offer additional functionnalities to the user without affecting the abstract host class and in a typesafe manner.
When using multiple policy inheritance, function members may have the same signatures. Also, some function name should be reserved. To resolve ambiguities we can wrap some policy members, necessary for the host functionalities. For example in the sampler :

``` C++
void SetTransport(FairMQTransportFactory* factory)
{
    FairMQDevice::SetTransport(factory);
}
```
or 

``` C++
template <typename... Args>
void SetFileProperties(Args&... args)
{
    source_type::SetFileProperties(args...);
}
```

### Generic Processor
The function members required by the processor policies are :



# Readout example

This examples shows two possible topologies (out of many) for a node connected to a detector readout (followed by a processing node).

## Setup without new data generation

```
|------------------------------- Readout Node ---------------------------|     |- Processing Node -|
| Readout            -->            Builder          -->          Sender | --> |     Receiver      |
| [# shared memory segment (unused in this topology) ##################] | ofi |                   |
| [# shmem unmanaged region (readout writes here, others read) ########] |     |                   |
|------------------------------------------------------------------------|     |-------------------|
```

The devices one the Readout Node communicate via shared memory transport. Readout device writes into shared memory unmanaged region. The data is then forwarded through Builder to Sender process, which sends it out via OFI transport.

## Setup with generating new data on the Readout node

```
|------------------------------- Readout Node ---------------------------|     |- Processing Node -|
| Readout     -->     Builder      -->      Processor     -->     Sender | --> |     Receiver      |
| [# shared memory segment (used between Proccessor and Sender) #######] | ofi |                   |
| [# shmem unmanaged region (readout writes here, builder & proc read) ] |     |                   |
|------------------------------------------------------------------------|     |-------------------|
```

In this topology one more device is added - Processor. It examines the arriving data and creates new data in shared memory. This data is not part of the unmanaged region, but lives in the general shared memory segment (unused in the previous setup). This new data is then forwarded to Sender and the Readout device is notified that the corresponding data piece in the unmanaged region is no longer used.

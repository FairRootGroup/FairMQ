{
  FairRuntimeDb *rtdb = FairRuntimeDb::instance();

  Bool_t kParameterMerged = kTRUE;
  FairParRootFileIo* parOut = new FairParRootFileIo(kParameterMerged);
  parOut->open("mqexample7_param.root");
  rtdb->setFirstInput(parOut);
  //rtdb->saveOutput();
  //rtdb->print();

  FairMQExample7ParOne *par = rtdb->getContainer("FairMQExample7ParOne");

  for(Int_t i = 0; i < 100; i++)
  {
    rtdb->initContainers(2000+i);

    par->print();
  }
}

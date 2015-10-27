{
  FairRuntimeDb *rtdb = FairRuntimeDb::instance();

  Bool_t kParameterMerged = kTRUE;
  FairParRootFileIo* parOut = new FairParRootFileIo(kParameterMerged);
  parOut->open("mqexample7_param.root");
  rtdb->setOutput(parOut);
  //rtdb->saveOutput();
  //rtdb->print();

  FairMQExample7ParOne *par = rtdb->getContainer("FairMQExample7ParOne");

  for(Int_t i = 0; i < 100; i++)
  {
    rtdb->addRun(2000 + i);

    par->SetValue(1983 + i);
    par->setChanged();

    rtdb->saveOutput();
  }

  rtdb->print();
}

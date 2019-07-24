/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/States.h>
#include <iostream>

int main(int /*argc*/, char ** /*argv*/)
{
  std::cout << fair::mq::State::Idle << std::endl;
   
  return 0;
}

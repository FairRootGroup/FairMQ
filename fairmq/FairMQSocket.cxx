/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSocket.cxx
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#include "FairMQSocket.h"

bool FairMQSocket::Attach(const std::string& config, bool serverish)
{
  if (config.empty())
    return false;
  if (config.size()<2)
    return false;

  const char* endpoints = config.c_str();

  //  We hold each individual endpoint here
  char endpoint [256];
  while (*endpoints) {
    const char *delimiter = strchr (endpoints, ',');
    if (!delimiter)
      delimiter = endpoints + strlen (endpoints);
    if (delimiter - endpoints > 255)
      return false;
    memcpy (endpoint, endpoints, delimiter - endpoints);
    endpoint [delimiter - endpoints] = 0;

    bool rc;
    if (endpoint [0] == '@') {
      rc = Bind(endpoint + 1);
    }
    else if (endpoint [0] == '>' || endpoint [0] == '-' || endpoint [0] == '+' ) {
      Connect(endpoint + 1);
    }
    else if (serverish) {
      rc = Bind(endpoint);
    }
    else {
      Connect(endpoint);
    }

    if (!rc) {
      return false;
    }

    if (*delimiter == 0) {
      break;
    }

    endpoints = delimiter + 1;
  }

  return true;
}

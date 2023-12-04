// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFpLib/FFpExport/FFpBatchExport.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"
#include "FiDeviceFunctions/FiRPC3File.H"


bool FFpRPC3Data::readDataFromFile(const std::string& rpcFile)
{
  // Open input RPC-file and read number of repeats, etc.
  FiRPC3File rpc(rpcFile.c_str());
  if (!rpc.open(FiDeviceFunctionBase::Read_Only))
  {
    ListUI <<"===> Failed to get parameters from RPC-file "<< rpcFile <<"\n";
    return false;
  }

  ListUI <<"\n===> Reading number of repeats, etc. from "<< rpcFile <<"\n";
  repeats  = rpc.getRepeats();
  averages = rpc.getAverages();
  framePts = rpc.getFramePoints();
  groupPts = rpc.getGroupPoints();
  rpc.close();

  ListUI <<"     Repeats = "<< repeats <<"\n";
  ListUI <<"     Averages = "<< averages <<"\n";
  ListUI <<"     Frame size = "<< framePts <<"\n";
  ListUI <<"     Group size = "<< groupPts <<"\n";
  return true;
}
